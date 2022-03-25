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

import Scene.Shading;
import Scene.Lights.LightData;
import Experimental.Scene.Lights.EnvMapLighting;
import Utils.Helpers;
import Utils.Color.ColorHelpers;

cbuffer constants {
	float3 cameraPosition;
	int numLights;
	int fakeMath;
  int fakeLookups;
};

SamplerState gSampler;
Texture2D gPosW;
Texture2D gNormW;
Texture2D gDiffuse;
Texture2D gSpecRough;
Texture2D gEmissive;
Texture2D gVisibility;
EnvMapLighting gEnvMapLighting;
StructuredBuffer<LightData> gLights;
Buffer<int> gFakeBuffer;

struct Output {
	float4 color : SV_TARGET0;
};

float rand(float2 co){
    float v = sin(dot(co, float2(12.9898, 78.233))) * 43758.5453;
    return v - floor(v);
}

Output main(float2 uv : TEXCOORD) : SV_TARGET {
	ShadingData material;
	material.emissive = gEmissive.SampleLevel(gSampler, uv, 0).rgb;
	material.posW = gPosW.SampleLevel(gSampler, uv, 0).xyz;

	if (material.posW.x == 0 && material.posW.y == 0 && material.posW.z == 0)
		discard;

    float a = material.posW.y;
    float b = material.posW.x;
    float c = 0;
    for(int i = 0; i < fakeMath; i++)
        c = c + (a * b);
    if (isnan(c))
        discard;

    int d = 0;
    for (int i = 0; i < fakeLookups; i++)
        d += gFakeBuffer[rand(uv + i * 0.712345826f) * 32 * 1024 * 1024];
    if (d > 0)
        discard;

	float4 diffuseOp = gDiffuse.SampleLevel(gSampler, uv, 0);
	float4 specRough = gSpecRough.SampleLevel(gSampler, uv, 0);

	material.V = normalize(cameraPosition - material.posW);
	material.N = normalize(gNormW.SampleLevel(gSampler, uv, 0).xyz);
	material.NdotV = dot(material.N, material.V);
	material.diffuse = diffuseOp.rgb;
	material.opacity = diffuseOp.a;
	material.specular = specRough.rgb;
	material.linearRoughness = specRough.a;
	material.ggxAlpha = max(0.0064, material.linearRoughness * material.linearRoughness);
	material.uv = uv;

	float4 color = float4(0, 0, 0, 1);
	float shadow = gVisibility.SampleLevel(gSampler, uv, 0).r * material.opacity;

	for (uint l = 0; l < numLights; l++) {
	    float visibility;
	    if (l == 0)
			visibility = shadow;
		else
			visibility = 1;

		color.rgb += evalMaterial(material, gLights[l], visibility).color;
	}

	color.a = material.opacity;
	color.rgb += material.emissive;
	color.rgb += evalMaterial(material, gEnvMapLighting).color;

	Output out;
	out.color = color;
	return out;
}
