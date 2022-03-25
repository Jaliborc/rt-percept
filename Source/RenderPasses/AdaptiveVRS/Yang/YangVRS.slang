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
Texture2D<float3> input;
RWTexture2D<uint> rate;

float luma(uint2 uv) {
  float3 color = input[uv];
  return 0.299*color.r + 0.587*color.g + 0.114*color.b;
}

[numthreads(32, 32, 1)]
void main(uint3 tile : SV_DispatchThreadID)
{
    uint2 uv = tile.xy;
    if (any(uv > resolution)) return;

    uint2 start = uv * VRS_TILE;
    float2 diff = (0.0, 0.0);
    float sum = 0;

    for (uint u = 0; u<VRS_TILE; u++) {
      for (uint v = 0; v<VRS_TILE; v++) {
        uint2 pixel = start + uint2(u,v);
        float c = luma(pixel + uint2(0,1));
        float b = luma(pixel + uint2(1,0));
        float a = luma(pixel);

        float dx = abs(a - b);
        float dy = abs(a - c);

        diff = max(diff, float2(dx,dy));
        sum += a + b + c;
      }
    }

    float average = sum / (VRS_TILE * VRS_TILE * 3.0);
    float limit = LIMIT * (average + LUMINANCE);
    float limit2 = limit / 2.13;

    if (diff.x < limit2 && diff.y < limit) {
      if (diff.y < limit2) {
         rate[uv] = VRS_4x4;
      } else {
        rate[uv] = VRS_4x2;
      }
    } else if (diff.x < limit) {
      if (diff.y < limit2) {
        rate[uv] = VRS_2x4;
      } else if (diff.y < limit) {
        rate[uv] = VRS_2x2;
      } else {
        rate[uv] = VRS_2x1;
      }
    } else if (diff.y < limit) {
      rate[uv] = VRS_1x2;
    } else {
      rate[uv] = VRS_1x1;
    }
}
