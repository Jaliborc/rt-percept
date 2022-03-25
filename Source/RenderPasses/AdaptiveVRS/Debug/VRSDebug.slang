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

cbuffer constant { uint2 resolution; }
Texture2D<uint> rate;
Texture2D<float3> rendering;
RWTexture2D<float3> color;

[numthreads(32, 32, 1)]
void main(uint3 pixel : SV_DispatchThreadID)
{
    uint2 uv = pixel.xy;
    if (any(uv > resolution)) return;

    uint mode = rate[uv / VRS_TILE];
    if (any((uv % VRS_TILE) == uint2(0,0))) {
      color[uv] = rendering[uv] * 0.95 + float3(1,1,1) * 0.05;
    } else {
      uint mode = rate[uv / VRS_TILE];
      if (mode == VRS_1x1) {
        color[uv] = rendering[uv];
      } else {
        float3 overlay;
        if (mode == VRS_4x4)
          overlay = float3(1,0,0);
        else if (mode == VRS_4x2 || mode == VRS_2x4)
          overlay = float3(1,1,0);
        else if (mode == VRS_2x1 || mode == VRS_1x2 || mode == VRS_2x2)
          overlay = float3(0,1,0);

        color[uv] = rendering[uv] * .7 + overlay * .3;
      }
    }
}
