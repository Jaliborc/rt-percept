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

#include "TemporalReproject.h"

const char* TemporalReproject::desc = "Temporal reprojection of previous render using screen-space motion.";
extern "C" __declspec(dllexport) void getPasses(Falcor::RenderPassLibrary& lib)
{
    lib.registerClass("TemporalReproject", "Temporal reprojection of previous render using screen-space motion.", TemporalReproject::create);
}

extern "C" __declspec(dllexport) const char* getProjDir()
{
    return PROJECT_DIR;
}

TemporalReproject::TemporalReproject()
{
    framebuffer = Fbo::create();
    shader = FullScreenPass::create("RenderPasses/TemporalReproject/TemporalReproject.slang");
    shader["sampler"] = Sampler::create(Sampler::Desc().setAddressingMode(Sampler::AddressMode::Border, Sampler::AddressMode::Border, Sampler::AddressMode::Border));
}

RenderPassReflection TemporalReproject::reflect(const CompileData& data)
{
    RenderPassReflection reflector;
    reflector.addInput("motion", "Screen-space motion vectors.");
    reflector.addOutput("target", "Data to be reprojected to next frame.");
    reflector.addOutput("dst", "Temporal reprojected texture.");
    return reflector;
}

void TemporalReproject::execute(RenderContext* context, const RenderData& data)
{
    framebuffer->attachColorTarget(data["dst"]->asTexture(), 0);
    shader["motion"] = data["motion"]->asTexture();
    shader["source"] = data["target"]->asTexture();
    shader->execute(context, framebuffer);
}
