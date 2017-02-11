#include "DeferredLightingCommon.glsl"


#ifdef BASS_PASS

struct GlobalParam
{
	mat4 matVP;
	mat4 matWorld;
	mat4 matWorldInv;
	mat4 matView;
	mat4 matViewInv;
	float3 viewPos;
};

uniform GlobalParam gParam;

struct VSOutput
{
	float4 worldPos;
	float4 normal;
	float4 baseColor;
};

#ifdef VERTEX_SHADER

out VSOutput vsOutput;
void BassPassVS()
{
	gl_Position = ftransform();
	vsOutput.worldPos = gParam.matWorld * gl_Vertex;
	vsOutput.normal = float4( gl_Normal , 0 ) * gParam.matWorldInv;
	vsOutput.baseColor = gl_Color;
}

#endif //VERTEX_SHADER

#ifdef PIXEL_SHADER

in VSOutput vsOutput;

void BasePassFS()
{
	float4 GBufferA, GBufferB, GBufferC;

	GBufferData data;
	data.worldPos = vsOutput.worldPos.xyz;
	data.normal = normalize(vsOutput.normal.xyz);
	data.baseColor = vsOutput.baseColor;
	encodeGBuffer(data, GBufferA, GBufferB, GBufferC);
	gl_FragData[0] = GBufferA;
	gl_FragData[1] = GBufferB;
	gl_FragData[2] = GBufferC;
	gl_FragData[3] = float4(gl_FragCoord.zzz, 1);
}


#endif //PIXEL_SHADER

#else

#include "LightingCommon.glsl"
#include "ShadowCommon.glsl"


struct VSOutput
{
	vec2 UVs;
};

#ifdef VERTEX_SHADER

out VSOutput vsOutput;

void LightingPassVS()
{
	gl_Position  = gl_Vertex;
	vsOutput.UVs = gl_MultiTexCoord0.xy;
}

#endif //VERTEX_SHADER

#ifdef PIXEL_SHADER

uniform float3 lightPos;
uniform float3 lightColor;
uniform float3 viewPos;

uniform sampler2D GBufferTextureA;
uniform sampler2D GBufferTextureB;
uniform sampler2D GBufferTextureC;

in VSOutput vsOutput;

void LightingPassFS()
{
	vec2 UVs = vsOutput.UVs;
	GBufferData data = decodeGBuffer( 
		texture2D(GBufferTextureA, UVs), 
		texture2D(GBufferTextureB, UVs), 
		texture2D(GBufferTextureC, UVs));

	float c = 1;
	float3 lightOffset = lightPos - data.worldPos;
	float3 viewOffset = viewPos - data.worldPos;

	float3 N = data.normal;
	float dist = length(lightOffset);
	float3 L = lightOffset / dist;
	float3 V = normalize(viewOffset);

	float  s = calcPointLightShadow(lightOffset);
	//float  s = 1;
	float3 shading = phongShading(lightColor, lightColor, N, L, V, 20);

	gl_FragColor = float4( s * (1 / (0.05 * dist * dist + 1)) * shading * data.baseColor.rgb , 1);
}

#endif //PIXEL_SHADER

#endif //BASS_PASS
