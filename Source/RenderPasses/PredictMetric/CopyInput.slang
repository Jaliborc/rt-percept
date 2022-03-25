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

cbuffer constants {
  float4x4 world2View;
  uint2 resolution;
  uint area;
}

Texture2D<float4> reproject;
Texture2D<float3> diffuse;
Texture2D<float3> normals;
RWStructuredBuffer<float> buffer;

[numthreads(32, 32, 1)]
void main(uint3 pixel : SV_DispatchThreadID)
{
    uint2 uv = pixel.xy;
    if (any(uv > resolution)) return;
    int k = uv.y * resolution.x + uv.x;

    #if USE_REPROJECTION
      buffer[k] = reproject[uv].r;
      buffer[area + k] = reproject[uv].a;
    #else
      buffer[k] = 0;
      buffer[area + k] = 0;
    #endif

    buffer[area * 2 + k] = diffuse[uv].r;
    buffer[area * 3 + k] = mul(float4(normalize(normals[uv]), 0.0f), world2View).b;
}
