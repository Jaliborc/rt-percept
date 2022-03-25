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
#include "RenderGraph/RenderPassHelpers.h"

using namespace Falcor;

class CapturePass : public RenderPass
{
public:
    struct ImageFormat { Bitmap::FileFormat id; std::string extension;  bool hdr = false; };
    enum class ViewpointGeneration { FromFile, FromGameplay };
    using SharedPtr = std::shared_ptr<CapturePass>;

    static SharedPtr create(RenderContext* context = nullptr, const Dictionary& dict = {});
    static const char* desc;

    virtual RenderPassReflection reflect(const CompileData& data) override;
    virtual void compile(RenderContext* context, const CompileData& data) override {}
    virtual void setScene(RenderContext* context, const Scene::SharedPtr& scene) override;
    virtual void execute(RenderContext* context, const RenderData& data) override;
    virtual void renderUI(Gui::Widgets& widget) override;
    virtual std::string getDesc() override { return desc; }

private:
    std::string dumpDir = ".";
    ImageFormat dumpFormat = { Bitmap::FileFormat::PngFile, "png" };
    ViewpointGeneration viewpointMethod = ViewpointGeneration::FromFile;
    size_t captureInterval = 1000;
    size_t framesWait = 4;

    Scene::SharedPtr scene;
    std::queue<glm::mat4x4> viewpoints;
    bool capturing = false;
    int numDumped = 0;
    float delay = 0;

    void loadViewpoints();
    void nextViewpoint();
    void dumpReproject(RenderContext* context, const RenderData& data);
    void dumpFrame(RenderContext* context, const RenderData& data);
    void dumpFile(RenderContext* context, const RenderData& data, ChannelDesc channel);
};
