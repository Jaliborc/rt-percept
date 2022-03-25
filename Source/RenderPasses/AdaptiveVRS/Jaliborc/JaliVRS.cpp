/***************************************************************************
 # Copyright (c) 2022, Joao Cardoso and Bernhard Kerbl. All rights reserved.
 #
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted provided that the following conditions
 # are met:
 #  * Redistributions of source code must retain the above copyright
 #    notice, this list of conditions and the following disclaimer.
 #  * Redistributions in binary form must reproduce the above copyright
 #    notice, this list of conditions and the following disclaimer in the
 #    documentation and/or other materials provided with the distribution.
 #  * Names of contributors may not be used to endorse or promote products
 #    derived from this software without specific prior written permission.
 #
 # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS "AS IS" AND ANY
 # EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 # IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 # PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 # CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 # EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 # PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 # PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 # OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 # (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 # OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **************************************************************************/

#include "JaliVRS.h"
#include <NvOnnxParser.h>
#include <filesystem>
#include <cuda.h>

class Log : public ILogger {
public:
    void log(Severity severity, const char* msg) override {
        if ((severity == Severity::kERROR) || (severity == Severity::kINTERNAL_ERROR))
            std::cout << "\033[1;32m----------[TENSOR RT]----------033[0m\n" << msg << "\n\033[1;32m-----------------------------033[0m\n";
    }
} Log;

int size(DataType t)
{
    switch (t) {
        case DataType::kINT32:
        case DataType::kFLOAT: return 32;
        case DataType::kHALF:  return 16;
        case DataType::kINT8:  return 8;
    }
    return 0;
}

int length(Dims shape)
{
    int length = 1;
    for (int i = 0; i < shape.nbDims; i++)
        length *= shape.d[i];

    return length;
}

void resetCuda()
{
    cuInit(0);
    CUcontext context;
    cuCtxGetCurrent(&context);
    CUdevice device;
    cuCtxGetDevice(&device);
    cuCtxCreate(&context, 0, device);
    cuCtxSetCurrent(context);
}

JaliVRS::JaliVRS()
{
    resetCuda(); // because magic

    Shader::DefineList defines;
    defines.add("VRS_1x1", std::to_string(D3D12_SHADING_RATE_1X1));
    defines.add("VRS_1x2", std::to_string(D3D12_SHADING_RATE_1X2));
    defines.add("VRS_2x1", std::to_string(D3D12_SHADING_RATE_2X1));
    defines.add("VRS_2x2", std::to_string(D3D12_SHADING_RATE_2X2));
    defines.add("VRS_2x4", std::to_string(D3D12_SHADING_RATE_2X4));
    defines.add("VRS_4x2", std::to_string(D3D12_SHADING_RATE_4X2));
    defines.add("VRS_4x4", std::to_string(D3D12_SHADING_RATE_4X4));
    defines.add("USE_REPROJECTION", std::to_string(reproject));
    defines.add("LIMIT", std::to_string(limit));

    pickRate = ComputePass::create("RenderPasses/AdaptiveVRS/Jaliborc/PickRate.slang", "main", defines);
    copyInput = ComputePass::create("RenderPasses/AdaptiveVRS/Jaliborc/CopyInput.slang", "main");

    auto onnxPath = "../RenderPasses/AdaptiveVRS/Jaliborc/Model.onnx";
    if (!std::filesystem::exists(onnxPath))
        throw std::runtime_error("ONNX file not found");

    RT<IBuilder> builder {createInferBuilder(Log)};
    RT<IBuilderConfig> config {builder->createBuilderConfig()};
    RT<INetworkDefinition> network {builder->createNetworkV2(1U << static_cast<uint32_t>(NetworkDefinitionCreationFlag::kEXPLICIT_BATCH))};

    RT<nvonnxparser::IParser> parser {nvonnxparser::createParser(*network, Log)};
    if (!parser->parseFromFile(onnxPath, static_cast<int>(ILogger::Severity::kINFO)))
        throw std::runtime_error("Could not parse ONNX file");

    config->setMaxWorkspaceSize((1 << 30));
    config->setFlag(BuilderFlag::kFP16);
    config->setFlag(BuilderFlag::kTF32);
    builder->setMaxBatchSize(1);

    engine.reset(builder->buildEngineWithConfig(*network, *config));
    if (!engine.get())
        throw std::runtime_error("Error creating TensorRT engine");

    if (engine->getNbBindings() != 2)
        throw std::runtime_error("Wrong number of input and output buffers in TensorRT network");

    for (int i = 0; i < engine->getNbBindings(); i++) {
        std::string name = engine->getBindingName(i);

        auto shape = engine->getBindingDimensions(i);
        auto bits = size(engine->getBindingDataType(i));
        auto buffer = Buffer::createStructured(bits, length(shape), Resource::BindFlags::Shared | Resource::BindFlags::UnorderedAccess, Buffer::CpuAccess::None, nullptr, false);
        buffer->setName(name);

        if (name.compare("input") == 0) {
            copyInput["constants"]["resolution"] = resIn = int2(shape.d[3], shape.d[2]);
            copyInput["constants"]["area"] = resIn.x * resIn.y;
            copyInput["buffer"] = buffer;
        }
        else if (name.compare("metric") == 0) {
            pickRate["constants"]["resolution"] = resOut = int2(shape.d[3], shape.d[2]);
            pickRate["constants"]["area"] = resOut.x * resOut.y;
            pickRate["prediction"] = buffer;
        }
        else {
            throw std::runtime_error("Unexpected name for buffer in TensorRT network");
        }

        buffers.push_back(buffer->getCUDADeviceAddress());
    }

    pickRate["constants"]["stride"] = resIn / resOut;
    execution.reset(engine->createExecutionContext());
}

void JaliVRS::setScene(RenderContext* context, const Scene::SharedPtr& scene)
{
    if (scene)
        this->scene = scene;
}

RenderPassReflection JaliVRS::reflect(const CompileData& data)
{
    RenderPassReflection reflector;
    reflector.addOutput("rate", "Rate").bindFlags(ResourceBindFlags::UnorderedAccess).format(ResourceFormat::R8Uint).texture2D(resOut.x, resOut.y);
    reflector.addInput("reproject", "Reproject");
    reflector.addInput("specular", "Specular");
    reflector.addInput("diffuse", "Diffuse");
    reflector.addInput("normals", "Normals");
    reflector.addInput("shadows", "Shadows");
    return reflector;
}

void JaliVRS::execute(RenderContext* context, const RenderData& data)
{
    if (scene) {
      copyInput["constants"]["world2View"] = scene->getCamera()->getViewMatrix();
      copyInput["specular"] = data["specular"]->asTexture();
      copyInput["diffuse"] = data["diffuse"]->asTexture();
      copyInput["normals"] = data["normals"]->asTexture();
      copyInput["shadows"] = data["shadows"]->asTexture();
      copyInput->execute(context, resIn.x, resIn.y);

      execution->executeV2(buffers.data());

      pickRate["rate"] = data["rate"]->asTexture();
      pickRate["reproject"] = data["reproject"]->asTexture();
      pickRate->execute(context, resOut.x, resOut.y);
    }
}

void JaliVRS::renderUI(Gui::Widgets& widget)
{
    widget.slider("Max Perceptual Error", limit, 0.0f, 2.0f);
    pickRate->addDefine("LIMIT", std::to_string(limit));

    widget.checkbox("Use Reprojection", reproject);
    widget.text("Disable to pretend screen is all unseen.");
    pickRate->addDefine("USE_REPROJECTION", std::to_string(reproject));
}
