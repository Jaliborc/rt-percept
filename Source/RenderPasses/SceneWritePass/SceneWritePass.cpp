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

#include "SceneWritePass.h"

 // Don't remove this. it's required for hot-reload to function properly
extern "C" __declspec(dllexport) const char* getProjDir()
{
    return PROJECT_DIR;
}

extern "C" __declspec(dllexport) void getPasses(Falcor::RenderPassLibrary & lib)
{
    lib.registerClass("SceneWritePass", "Renders a depth buffer. Abuses it to output entire scene with stream output.", SceneWritePass::create);
}

const char* SceneWritePass::kDesc = "Creates a depth-buffer using the scene's active camera";

namespace
{
    const std::string kProgramFile = "RenderPasses/DepthPass/DepthPass.ps.slang"; // No real shader needed, minimum will do
    const std::string kDepth = "depth";
    const std::string kDepthFormat = "depthFormat";
    const std::string kOutFile = "outFile";
}

void SceneWritePass::parseDictionary(const Dictionary& dict)
{
    for (const auto& [key, value] : dict)
    {
        if (key == kDepthFormat) setDepthBufferFormat(value);
        else if (key == kOutFile) setWriteFilePath(value);
        else logWarning("Unknown field '" + key + "' in a SceneWritePass dictionary");
    }
}

Dictionary SceneWritePass::getScriptingDictionary()
{
    Dictionary d;
    d[kDepthFormat] = mDepthFormat;
    d[kOutFile] = mOutFilePath;
    return d;
}

SceneWritePass::SharedPtr SceneWritePass::create(RenderContext* pRenderContext, const Dictionary& dict)
{
    return SharedPtr(new SceneWritePass(dict));
}

SceneWritePass::SceneWritePass(const Dictionary& dict)
{
    Program::Desc desc;
    desc.addShaderLibrary(kProgramFile).psEntry("main");
    GraphicsProgram::SharedPtr pProgram = GraphicsProgram::create(desc);
    mpState = GraphicsState::create();
    mpState->setProgram(pProgram);
    mpFbo = Fbo::create();

    parseDictionary(dict);

    // Dump buffer
    mDumpGeomSize = 1'000'000'000;
    mDumpGeomBuffer = Buffer::create(mDumpGeomSize, Falcor::ResourceBindFlags::StreamOutput);
    long long int zero = 0;
    mDumpGeomCounter = Buffer::create(sizeof(long long int), Falcor::ResourceBindFlags::StreamOutput, Falcor::Buffer::CpuAccess::None, &zero);

    std::vector < D3D12_SO_DECLARATION_ENTRY> decls(1);
    decls[0].Stream = 0;
    decls[0].OutputSlot = 0;
    decls[0].SemanticName = "POSW";
    decls[0].SemanticIndex = 0;
    decls[0].StartComponent = 0;
    decls[0].ComponentCount = 3;

    std::vector<UINT> strides = { 3 * sizeof(float) };
    StreamOutputState::SharedPtr pSOState = StreamOutputState::create(StreamOutputState::Desc());
    pSOState->setRasterizedStream(0);
    pSOState->setDeclarations(decls);
    pSOState->setStrides(strides);
    mpState->setStreamOutputState(pSOState);
}

RenderPassReflection SceneWritePass::reflect(const CompileData& compileData)
{
    RenderPassReflection reflector;
    reflector.addOutput(kDepth, "Depth-buffer").bindFlags(Resource::BindFlags::DepthStencil).format(mDepthFormat).texture2D(0, 0, 0);
    return reflector;
}

void SceneWritePass::setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene)
{
    mpScene = pScene;
    if (mpScene) mpState->getProgram()->addDefines(mpScene->getSceneDefines());
    mpVars = GraphicsVars::create(mpState->getProgram()->getReflector());
}

void SceneWritePass::execute(RenderContext* pContext, const RenderData& renderData)
{
    if (mpScene)
    {
        const auto& pDepth = renderData[kDepth]->asTexture();
        mpFbo->attachDepthStencilTarget(pDepth);

        mpState->setFbo(mpFbo);
        pContext->clearDsv(pDepth->getDSV().get(), 1, 0);

        // Set up stream out
        unsigned long long* numWritten = (unsigned long long*)mDumpGeomCounter->map(Falcor::Buffer::MapType::Read);
        std::cout << *numWritten << std::endl;
        mDumpGeomCounter->unmap();

        pContext->resourceBarrier(mDumpGeomBuffer.get(), Falcor::Resource::State::StreamOut);
        pContext->resourceBarrier(mDumpGeomCounter.get(), Falcor::Resource::State::StreamOut);

        D3D12_STREAM_OUTPUT_BUFFER_VIEW view;
        view.BufferFilledSizeLocation = mDumpGeomCounter->getGpuAddress();
        view.BufferLocation = mDumpGeomBuffer->getGpuAddress();
        view.SizeInBytes = mDumpGeomSize;
        pContext->getLowLevelData()->getCommandList()->SOSetTargets(0, 1, &view);

        mpScene->rasterize(pContext, mpState.get(), mpVars.get(), mpRsState ? Scene::RenderFlags::UserRasterizerState : Scene::RenderFlags::None);

        // Wait for finish, write captured data
        gpDevice->flushAndSync();

        numWritten = (unsigned long long*)mDumpGeomCounter->map(Falcor::Buffer::MapType::Read);
        std::cout << *numWritten << std::endl;
        mDumpGeomBuffer->unmap();

        std::vector<float> dumped(*numWritten / sizeof(float));
        float* raw = (float*)mDumpGeomBuffer->map(Falcor::Buffer::MapType::Read);
        std::copy(raw, raw + dumped.size(), dumped.data());

        struct uchar3 {
            unsigned char x, y, z;
        };

        auto f = [](const float3& a, const float3& b) {return a.x == b.x ? (a.y == b.y ? a.z < b.z : a.y < b.y) : a.x < b.x; };
        std::map < float3, int, decltype(f)> seen(f);
        std::vector<float3> verts;
        {
            std::ofstream outfile(mOutFilePath, std::ios_base::out | std::ios_base::binary);
            outfile << "ply\n" << "format binary_little_endian 1.0\n" << "element vertex " << dumped.size() / 3 << "\n";
            outfile << ""
                "property float x\n"
                "property float y\n"
                "property float z\n";
            outfile << "element face " << dumped.size() / 9 << "\n";
            outfile << "property list uchar int vertex_indices\n";
            outfile << "end_header\n";

            for (long long unsigned int i = 0; i < dumped.size() / 3; i += 3)
            {
                for (long long unsigned int j = 0; j < 3; j++)
                {
                    long long unsigned int base = (i + j) * 3;
                    float3 xyz = { dumped[base + 0] , -dumped[base + 2], dumped[base + 1] };
                    outfile.write((const char*)&xyz, sizeof(xyz));
                }
            }
            for (long long unsigned int i = 0; i < dumped.size() / 3; i += 3)
            {
                unsigned char lead = 3;
                outfile.write((const char*)&lead, sizeof(lead));
                uint3 ids = { i, i + 1, i + 2 };
                outfile.write((const char*)&ids, sizeof(ids));
            }

            std::cout << "******************************************************************" << std::endl;
            std::cout << "\tWrote scene geometry to file \"" << mOutFilePath << "\", ending" << std::endl;
            std::cout << "\tNote: y = -z, z = y to facilitate editing in Blender" << std::endl;
            std::cout << "\tBlender tool must flip them back when it's done :)" << std::endl;
            std::cout << "******************************************************************" << std::endl;
            exit(0);
        }
    }
}

SceneWritePass& SceneWritePass::setWriteFilePath(std::string path)
{
    mOutFilePath = path;
    return *this;
}

SceneWritePass& SceneWritePass::setDepthBufferFormat(ResourceFormat format)
{
    if (isDepthStencilFormat(format) == false)
    {
        logWarning("SceneWritePass buffer format must be a depth-stencil format");
    }
    else
    {
        mDepthFormat = format;
        mPassChangedCB();
    }
    return *this;
}

SceneWritePass& SceneWritePass::setDepthStencilState(const DepthStencilState::SharedPtr& pDsState)
{
    mpState->setDepthStencilState(pDsState);
    return *this;
}

SceneWritePass& SceneWritePass::setRasterizerState(const RasterizerState::SharedPtr& pRsState)
{
    mpRsState = pRsState;
    mpState->setRasterizerState(mpRsState);
    return *this;
}

static const Gui::DropdownList kDepthFormats =
{
    { (uint32_t)ResourceFormat::D16Unorm, "D16Unorm"},
    { (uint32_t)ResourceFormat::D32Float, "D32Float" },
    { (uint32_t)ResourceFormat::D24UnormS8, "D24UnormS8" },
    { (uint32_t)ResourceFormat::D32FloatS8X24, "D32FloatS8X24" },
};

void SceneWritePass::renderUI(Gui::Widgets& widget)
{
    uint32_t depthFormat = (uint32_t)mDepthFormat;

    if (widget.dropdown("Buffer Format", kDepthFormats, depthFormat)) setDepthBufferFormat(ResourceFormat(depthFormat));
}
