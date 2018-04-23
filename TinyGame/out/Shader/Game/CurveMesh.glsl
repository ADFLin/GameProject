#include "Common.glsl"

#include "ViewParam.glsl"

#ifndef USE_OIT
#define USE_OIT 0
#endif

#if USE_OIT
#include "OITCommon.glsl"
#endif

struct OutputVSParam
{
	float3 pos;
	float4 color;
	float3 normal;
};

#if VERTEX_SHADER

layout(location = ATTRIBUTE_POSITION) in float3 InPos;
layout(location = ATTRIBUTE_COLOR) in float4 InColor;
layout(location = ATTRIBUTE_NORMAL) in float3 InNormal;

out OutputVSParam OutputVS;
void MainVS()
{
	OutputVS.pos = InPos;
	OutputVS.normal = InNormal;
	OutputVS.color = InColor;
	gl_Position = View.worldToClip * float4(InPos, 1.0);
}

#endif //VERTEX_SHADER


#if PIXEL_SHADER

in OutputVSParam OutputVS;
layout(location = 0) out float4 OutColor;

#if USE_OIT
layout(early_fragment_tests) in;
#endif

void MainPS()
{
	float3 normal = normalize(OutputVS.normal);
	float3 color = dot(normal, float3(0, 0, 1)) * OutputVS.color.rgb;
	color = OutputVS.normal;
#if USE_OIT
	float pixelDepth = -(View.worldToView * float4(OutputVS.pos, 1)).z;
	OITProcessPS(float4(color , 1), pixelDepth);
	OutColor = float4(0, 0, 0, 1);


#else
	OutColor = float4(dot(normal, float3(0, 0, 1)) * OutputVS.color.rgb, OutputVS.color.a);

	//OutColor = float4((0.5 * normal + 0.5), 1);
	//OutColor = float4( normal , 1);

#endif

}

#endif //PIXEL_SHADER