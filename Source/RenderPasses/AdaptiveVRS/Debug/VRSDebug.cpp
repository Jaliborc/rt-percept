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

#include "VRSDebug.h"

VRSDebug::VRSDebug()
{
    D3D12_FEATURE_DATA_D3D12_OPTIONS6 hardware;
    gpDevice->getApiHandle()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &hardware, sizeof(hardware));
    tileSize = hardware.ShadingRateImageTileSize;

    Shader::DefineList defines;
    defines.add("VRS_TILE", std::to_string(tileSize));
    defines.add("VRS_1x1", std::to_string(D3D12_SHADING_RATE_1X1));
    defines.add("VRS_1x2", std::to_string(D3D12_SHADING_RATE_1X2));
    defines.add("VRS_2x1", std::to_string(D3D12_SHADING_RATE_2X1));
    defines.add("VRS_2x2", std::to_string(D3D12_SHADING_RATE_2X2));
    defines.add("VRS_2x4", std::to_string(D3D12_SHADING_RATE_2X4));
    defines.add("VRS_4x2", std::to_string(D3D12_SHADING_RATE_4X2));
    defines.add("VRS_4x4", std::to_string(D3D12_SHADING_RATE_4X4));

    shader = ComputePass::create("RenderPasses/AdaptiveVRS/Debug/VRSDebug.slang", "main", defines);
}

RenderPassReflection VRSDebug::reflect(const CompileData& data)
{
    auto fbo = gpFramework->getTargetFbo();
    resolution = uint2(fbo->getWidth(), fbo->getHeight());
    shader["constant"]["resolution"] = resolution;

    RenderPassReflection reflector;
    reflector.addInput("rendering", "Rendering").texture2D(resolution.x, resolution.y);
    reflector.addInput("rate", "Rate").format(ResourceFormat::R8Uint).texture2D(resolution.x / tileSize, resolution.y / tileSize);
    reflector.addOutput("color", "Color").bindFlags(ResourceBindFlags::UnorderedAccess).format(ResourceFormat::RGBA32Float);
    return reflector;
}

void VRSDebug::execute(RenderContext* context, const RenderData& data)
{
    shader["rate"] = data["rate"]->asTexture();
    shader["color"] = data["color"]->asTexture();
    shader["rendering"] = data["rendering"]->asTexture();
    shader->execute(context, resolution.x, resolution.y);
}
