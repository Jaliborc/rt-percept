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

#include "PredictMetric.h"
#include <NvOnnxParser.h>
#include <filesystem>
#include <cuda.h>

const char* PredictMetric::desc = "Predicts the chosen metric given some gbuffer data.";
extern "C" __declspec(dllexport) const char* getProjDir() { return PROJECT_DIR; }
extern "C" __declspec(dllexport) void getPasses(Falcor::RenderPassLibrary & lib)
{
    lib.registerClass("PredictMetric", PredictMetric::desc, PredictMetric::create);
}

const Gui::DropdownList ExampleMetrics = {
    {0, "Flip"},
    {1, "JNFlip"},
    {2, "JNYang"},
    {3, "LPIPS"},
    {4, "PSNR"},
};

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

PredictMetric::PredictMetric()
{
    copyOut = ComputePass::create("RenderPasses/PredictMetric/CopyOutput.slang", "main");
    copyIn = ComputePass::create("RenderPasses/PredictMetric/CopyInput.slang", "main");
    copyIn->addDefine("USE_REPROJECTION", std::to_string(reprojection));

    // because magic
    cuInit(0);
    CUcontext context;
    cuCtxGetCurrent(&context);
    CUdevice device;
    cuCtxGetDevice(&device);
    cuCtxCreate(&context, 0, device);
    cuCtxSetCurrent(context);

    loadEngine();
}

void PredictMetric::setScene(RenderContext* context, const Scene::SharedPtr& scene)
{
    if (scene)
        this->scene = scene;
}


void PredictMetric::loadEngine()
{
    auto filePath = std::string("../RenderPasses/PredictMetric/Metrics/") + ExampleMetrics[metric].label + std::string(".onnx");
    if (!std::filesystem::exists(filePath))
        throw std::runtime_error("ONNX file not found");

    RT<IBuilder> builder {createInferBuilder(Log)};
    RT<IBuilderConfig> config {builder->createBuilderConfig()};
    RT<INetworkDefinition> network {builder->createNetworkV2(1U << static_cast<uint32_t>(NetworkDefinitionCreationFlag::kEXPLICIT_BATCH))};

    RT<nvonnxparser::IParser> parser {nvonnxparser::createParser(*network, Log)};
    if (!parser->parseFromFile(filePath.c_str(), static_cast<int>(ILogger::Severity::kINFO)))
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

    if (buffers.size() == 0)
        for (int i = 0; i < engine->getNbBindings(); i++) {
            std::string name = engine->getBindingName(i);

            auto shape = engine->getBindingDimensions(i);
            auto bits = size(engine->getBindingDataType(i));
            auto buffer = Buffer::createStructured(bits, length(shape), Resource::BindFlags::Shared | Resource::BindFlags::UnorderedAccess, Buffer::CpuAccess::None, nullptr, false);
            buffer->setName(name);

            if (name.compare("input") == 0) {
                copyIn["constants"]["resolution"] = resIn = int2(shape.d[3], shape.d[2]);
                copyIn["constants"]["area"] = resIn.x * resIn.y;
                copyIn["buffer"] = buffer;
            }
            else if (name.compare("metric") == 0) {
                copyOut["constants"]["resolution"] = resOut = int2(shape.d[3], shape.d[2]);
                copyOut["buffer"] = buffer;
            }
            else {
                throw std::runtime_error("Unexpected name for buffer in TensorRT network");
            }

            buffers.push_back(buffer->getCUDADeviceAddress());
        }

    execution.reset(engine->createExecutionContext());
}

RenderPassReflection PredictMetric::reflect(const CompileData& data)
{
    RenderPassReflection reflector;
    reflector.addOutput("metric", "Metric").bindFlags(ResourceBindFlags::UnorderedAccess).format(ResourceFormat::RGBA32Float).texture2D(resOut.x, resOut.y); // using RGB for visual demo, 1 channel enough
    reflector.addInput("reproject", "Reproject");
    reflector.addInput("diffuse", "Diffuse");
    reflector.addInput("normals", "Normals");
    return reflector;
}

void PredictMetric::execute(RenderContext* context, const RenderData& data)
{
    if (scene) {
      copyIn["constants"]["world2View"] = scene->getCamera()->getViewMatrix();
      copyIn["reproject"] = data["reproject"]->asTexture();
      copyIn["diffuse"] = data["diffuse"]->asTexture();
      copyIn["normals"] = data["normals"]->asTexture();
      copyIn->execute(context, resIn.x, resIn.y);

      execution->executeV2(buffers.data());

      copyOut["metric"] = data["metric"]->asTexture();
      copyOut->execute(context, resOut.x, resOut.y);
    }
}

void PredictMetric::renderUI(Gui::Widgets& widget)
{
    if (widget.dropdown("Metric", ExampleMetrics, metric)) {
        copyOut->addDefine("METRIC_ID", std::to_string(metric));
        loadEngine();
    }

    if (widget.checkbox("Use Reprojection", reprojection)) {
        copyIn->addDefine("USE_REPROJECTION", std::to_string(reprojection));
    }

    widget.text("Disable to pretend screen is all unseen.");;
}
