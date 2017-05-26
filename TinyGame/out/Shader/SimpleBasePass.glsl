#include "Common.glsl"
#include "ViewParam.glsl"
#include "PrimitiveParam.glsl"
#include "VertexProcess.glsl"
#include "DeferredShadingCommon.glsl"

struct VSOutputParam
{
	float4 color;
	float4 worldPos;
	float4 normal;
};

#if VERTEX_SHADER

out VSOutputParam VSOutput;
void BassPassVS()
{
	VSOutput.color = gl_Color;
	VSOutput.worldPos = Primitive.localToWorld * gl_Vertex;
	VSOutput.normal = float4(gl_Normal, 0) * Primitive.worldToLocal;
	gl_Position = View.worldToClip * VSOutput.worldPos;
}

#endif //VERTEX_SHADER

#if PIXEL_SHADER

in VSOutputParam VSOutput;
void BasePassPS()
{
	GBufferData GBuffer;
	GBuffer.worldPos = VSOutput.worldPos.xyz;
	GBuffer.normal = normalize( VSOutput.normal.xyz );
	GBuffer.baseColor = VSOutput.color.rgb;
	GBuffer.metallic = 0.3;
	GBuffer.roughness = 0.5;
	GBuffer.specular = 0.5;
	GBuffer.shadingModel = SHADINGMODEL_DEFAULT_LIT;
	float4 clipPos = View.worldToClip * float4(GBuffer.worldPos, 1);
	GBuffer.depth = -clipPos.z;

	float4 GBufferA, GBufferB, GBufferC, GBufferD;

	GBufferA = 0;
	GBufferB = 0;
	GBufferC = 0;
	GBufferD = 0;

	GBufferA = float4(GBuffer.worldPos, GBuffer.depth);
	GBufferB = float4(GBuffer.normal, GBuffer.depth);
	GBufferC = float4(GBuffer.baseColor, GBuffer.depth);

	GBufferD.r = GBuffer.metallic;
	GBufferD.g = GBuffer.roughness;
	GBufferD.b = GBuffer.specular;
	GBufferD.a = EncodeShadingModel(GBuffer.shadingModel);

	gl_FragData[0] = float4(0,0,1.0,1.0);
	gl_FragData[1] = GBufferA;
	gl_FragData[2] = GBufferB;
	gl_FragData[3] = GBufferC;
	gl_FragData[4] = GBufferD;
}

#endif //PIXEL_SHADER