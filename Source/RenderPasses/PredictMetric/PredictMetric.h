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
#include "NvInfer.h"

using namespace Falcor;
using namespace nvinfer1;

class PredictMetric : public RenderPass
{
private:
    struct Destroy {
        template<class T>
        void operator()(T* self) const { if (self) self->destroy(); }
    };
    template<class T>
    using RT = std::unique_ptr<T, Destroy>;

    bool reprojection = true;
    uint32_t metric = 0;
    uint2 resIn, resOut;
    ComputePass::SharedPtr copyIn, copyOut;
    Scene::SharedPtr scene;
    RT<ICudaEngine> engine;
    RT<IExecutionContext> execution;
    std::vector<void*> buffers;

    PredictMetric();
    void loadEngine();

public:
    using SharedPtr = std::shared_ptr<PredictMetric>;
    static SharedPtr create(RenderContext* context = nullptr, const Dictionary& dict = {}) { return SharedPtr(new PredictMetric); };
    static const char* desc;

    virtual std::string getDesc() override { return desc; }
    virtual RenderPassReflection reflect(const CompileData& data) override;
    virtual void compile(RenderContext* context, const CompileData& data) override {};
    virtual void setScene(RenderContext* context, const Scene::SharedPtr& scene) override;
    virtual void execute(RenderContext* context, const RenderData& data) override;
    virtual void renderUI(Gui::Widgets& widget) override;
};
