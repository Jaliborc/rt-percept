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

#pragma once
#include "Falcor.h"
#include "FalcorExperimental.h"

using namespace Falcor;

class SceneWritePass : public RenderPass
{
public:
    using SharedPtr = std::shared_ptr<SceneWritePass>;

    static const char* kDesc;

    /** Create a new object
    */
    static SharedPtr create(RenderContext* pRenderContext = nullptr, const Dictionary& dict = {});

    virtual RenderPassReflection reflect(const CompileData& compileData) override;
    virtual void execute(RenderContext* pContext, const RenderData& renderData) override;
    virtual void setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene) override;
    virtual void renderUI(Gui::Widgets& widget) override;
    virtual Dictionary getScriptingDictionary() override;
    virtual std::string getDesc() override { return kDesc; }

    SceneWritePass& setWriteFilePath(std::string path);
    SceneWritePass& setDepthBufferFormat(ResourceFormat format);
    SceneWritePass& setDepthStencilState(const DepthStencilState::SharedPtr& pDsState);
    SceneWritePass& setRasterizerState(const RasterizerState::SharedPtr& pRsState);

private:

    static const char* outFileName;

    SceneWritePass(const Dictionary& dict);
    void parseDictionary(const Dictionary& dict);

    Fbo::SharedPtr mpFbo;
    GraphicsState::SharedPtr mpState;
    GraphicsVars::SharedPtr mpVars;
    RasterizerState::SharedPtr mpRsState;
    ResourceFormat mDepthFormat = ResourceFormat::D32Float;
    std::string mOutFilePath = "out.ply";
    Scene::SharedPtr mpScene;

    //For writing geometry
    Buffer::SharedPtr   mDumpGeomBuffer;
    Buffer::SharedPtr   mDumpGeomCounter;
    int mDumpGeomSize;
};
