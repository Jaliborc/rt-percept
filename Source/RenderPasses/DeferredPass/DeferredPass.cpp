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

#include "DeferredPass.h"

const char* DeferredPass::desc = "Deferred lighting pass with optional variable shading rate.";
extern "C" __declspec(dllexport) const char* getProjDir() { return PROJECT_DIR; }
extern "C" __declspec(dllexport) void getPasses(Falcor::RenderPassLibrary & lib)
{
    lib.registerClass("DeferredPass", DeferredPass::desc, DeferredPass::create);
}

const ChannelList GBuffers =
{
    { "depth",            "gDepth",           "depth buffer",                 false, ResourceFormat::D32Float},
    { "posW",             "gPosW",            "world space position",         true},
    { "normW",            "gNormW",           "world space normal",           true},
    { "diffuseOpacity",   "gDiffuseOpacity",  "diffuse color",                true},
    { "specRough",        "gSpecRough",       "specular color",               true},
    { "emissive",         "gEmissive",        "emissive color",               true},
};

const D3D12_SHADING_RATE_COMBINER Combiners[] = {
    D3D12_SHADING_RATE_COMBINER_PASSTHROUGH,
    D3D12_SHADING_RATE_COMBINER_OVERRIDE
};

void DeferredPass::setScene(RenderContext* context, const Scene::SharedPtr& scene)
{
    if (scene) {
        this->scene = scene;
        int numLights = scene->getLightCount();

        pass = FullScreenPass::create("RenderPasses/DeferredPass/DeferredPass.slang", scene->getSceneDefines());
        pass["gSampler"] = Sampler::create(Sampler::Desc().setFilterMode(Sampler::Filter::Point, Sampler::Filter::Point, Sampler::Filter::Point));
        pass["constants"]["numLights"] = numLights;

        fakeBuffer = Buffer::create(sizeof(int) * 32 * 1024 * 1024);
        std::vector<int> fakey(32 * 1024 * 1024, 0);
        fakeBuffer->setBlob(fakey.data(), 0, sizeof(int) * 32 * 1024 * 1024);
        pass["gFakeBuffer"] = fakeBuffer;

        if (numLights) {
            auto lights = Buffer::createStructured(sizeof(LightData), numLights, Resource::BindFlags::ShaderResource, Buffer::CpuAccess::None, nullptr, false);
            lights->setName("lightsBuffer");

            for (int l = 0; l < numLights; l++) {
                auto light = scene->getLight(l);
                if (light->isActive())
                    lights->setElement(l, light->getData());
            }

            pass["gLights"] = lights;
        }

        if (scene->useEnvLight()) {
            auto vars = GraphicsVars::create(pass->getProgram()->getReflector());
            auto envmap = EnvMapLighting::create(context, scene->getEnvMap());
            envmap->setShaderData(vars["gEnvMapLighting"]);
        }
    }
}

RenderPassReflection DeferredPass::reflect(const CompileData& data)
{
    RenderPassReflection reflector;
    addRenderPassInputs(reflector, GBuffers);
    reflector.addInputOutput("output", "Shaded result").flags(RenderPassReflection::Field::Flags::Optional);
    reflector.addInput("visibility", "Visibility for shadowing").flags(RenderPassReflection::Field::Flags::Optional);
    reflector.addInput("vrs", "Variable rate shading image").flags(RenderPassReflection::Field::Flags::Optional);
    return reflector;
}

void DeferredPass::execute(RenderContext* context, const RenderData& data)
{
    if (scene) {
        pass["gPosW"] = data["posW"]->asTexture();
        pass["gNormW"] = data["normW"]->asTexture();
        pass["gDiffuse"] = data["diffuseOpacity"]->asTexture();;
        pass["gSpecRough"] = data["specRough"]->asTexture();
        pass["gEmissive"] = data["emissive"]->asTexture();
        pass["gVisibility"] = data["visibility"]->asTexture();
        pass["constants"]["cameraPosition"] = scene->getCamera()->getPosition();
        pass["constants"]["fakeLookups"] = fakeLookups;
        pass["constants"]["fakeMath"] = fakeMath;

        ID3D12GraphicsCommandList5Ptr directX;
        auto hasVRS = data["vrs"]->asTexture() != nullptr;
        if (hasVRS) {
            d3d_call(context->getLowLevelData()->getCommandList()->QueryInterface(IID_PPV_ARGS(&directX)));
            directX->RSSetShadingRateImage(data["vrs"]->getApiHandle());
            directX->RSSetShadingRate(D3D12_SHADING_RATE_1X1, Combiners);
        }

        framebuffer->attachColorTarget(data["output"]->asTexture(), 0);
        pass->execute(context, framebuffer);

        if (hasVRS) {
            directX->RSSetShadingRate(D3D12_SHADING_RATE_1X1, nullptr);
            directX->RSSetShadingRateImage(nullptr);
        }
    }
}

void DeferredPass::renderUI(Gui::Widgets& widget)
{
    widget.slider("Fake Arithmetic Load", fakeMath, 0, 300000);
    widget.slider("Fake Memory Lookups", fakeLookups, 0, 5000);
}
