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

#define LIMIT2 (LIMIT / 2.13)

cbuffer constants {
  uint2 resolution;
  uint area;
  uint2 stride;
}

StructuredBuffer<float> prediction;
Texture2D<float4> reproject;
RWTexture2D<uint> rate;

float luma(uint2 uv) {
  float3 color = reproject[uv].rgb;
  return 0.299*color.r + 0.587*color.g + 0.114*color.b;
}

[numthreads(32, 32, 1)]
void main(uint3 tile : SV_DispatchThreadID)
{
    uint2 uv = tile.xy;
    if (any(uv > resolution)) return;

    uint2 topleft = uv * stride;
    float2 jnd = (0.0, 0.0);

    if (USE_REPROJECTION && reproject[topleft].a > 0.0) {
      float sum = 0;
      for (uint u = 0; u<stride.x; u++) {
        for (uint v = 0; v<stride.y; v++) {
          uint2 pixel = topleft + uint2(u,v);
          float c = luma(pixel + uint2(0,1));
          float b = luma(pixel + uint2(1,0));
          float a = luma(pixel);

          float dx = abs(a - b);
          float dy = abs(a - c);

          jnd = max(jnd, float2(dx,dy));
          sum += a + b + c;
        }
      }

      jnd = jnd / (sum / (stride * stride * 3.0) + 0.00000001);
    } else {
      int k = uv.y * resolution.x + uv.x;
      jnd = (float2(prediction[area + k], prediction[k]) + 0.06) / 0.7;
    }

    if (jnd.x < LIMIT2 && jnd.y < LIMIT) {
      if (jnd.y < LIMIT2) {
         rate[uv] = VRS_4x4;
      } else {
        rate[uv] = VRS_4x2;
      }
    } else if (jnd.x < LIMIT) {
      if (jnd.y < LIMIT2) {
        rate[uv] = VRS_2x4;
      } else if (jnd.y < LIMIT) {
        rate[uv] = VRS_2x2;
      } else {
        rate[uv] = VRS_2x1;
      }
    } else if (jnd.y < LIMIT) {
      rate[uv] = VRS_1x2;
    } else {
      rate[uv] = VRS_1x1;
    }
}
