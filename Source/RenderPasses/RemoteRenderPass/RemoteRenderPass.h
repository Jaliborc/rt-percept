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
#include <winsock.h>

using namespace Falcor;

class WSOrg
{
public:
    int port = 4242;
    sockaddr_in sockaddr;
    SOCKET sockfd;
    SOCKET connection = 0;
    WSOrg();
    ~WSOrg();
};

class RemoteRenderPass : public RenderPass
{
public:
    using SharedPtr = std::shared_ptr<RemoteRenderPass>;
    static SharedPtr create(RenderContext* context = nullptr, const Dictionary& dict = {});
    static const char* desc;

    virtual Dictionary getScriptingDictionary() override;
    virtual RenderPassReflection reflect(const CompileData& data) override;
    virtual void execute(RenderContext* context, const RenderData& data) override;
    virtual void setScene(RenderContext* context, const Scene::SharedPtr& scene) override;
    virtual void renderUI(Gui::Widgets& widget) override;
    virtual std::string getDesc() override { return desc; }

    void setFilter(uint32_t filter);
    void setScale(float scale) { this->scale = scale; }
    float getScale() { return scale; }
    uint32_t getFilter() { return (uint32_t)filter; }

private:
    WSOrg wsorg;
    QueryHeap::SharedPtr occlusionHeap;
    Buffer::SharedPtr occlusionBuffer;
    GpuFence::SharedPtr copyFence;

    bool capturing = false;
    int viewPointsToDo = 0;
    int donePoints = 0;
    float threshold;
    std::vector<char> passed;

    RemoteRenderPass();
    void loadImage();
    void setTexture(const Texture::SharedPtr& texture);

    float scale = 1;
    bool srgb = true;
    Sampler::Filter filter = Sampler::Filter::Linear;
    Texture::SharedPtr texture;
    std::string textureName;

    Scene::SharedPtr cubeScene;
    GraphicsProgram::SharedPtr program;
    GraphicsVars::SharedPtr vars;
    GraphicsState::SharedPtr state;
    Fbo::SharedPtr framebuffer;
    Scene::SharedPtr scene;
};
