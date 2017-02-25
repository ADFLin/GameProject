#include "DeferredShadingCommon.glsl"
#include "MaterialProcess.glsl"
#include "ViewParam.glsl"
#include "VertexFactory.glsl"

#ifdef VERTEX_SHADER

out VertexFactoryOutputVS VFOutputVS;
void BassPassVS()
{
	VertexFactoryIntermediatesVS intermediates = GetVertexFactoryIntermediatesVS();

	MaterialInputVS materialInput = InitMaterialInputVS();
	MaterialParametersVS materialParameters = GetMaterialParameterVS( intermediates );

	CalcMaterialInputVS( materialInput , materialParameters );

	gl_Position = CalcVertexFactoryOutputVS(VFOutputVS, intermediates , materialInput , materialParameters );
	//gl_Position = ftransform();
}

#endif //VERTEX_SHADER

#ifdef PIXEL_SHADER

in VertexFactoryIutputPS VFOutputVS;

void BasePassPS()
{
	VertexFactoryIntermediatesPS intermediates = GetVertexFactoryIntermediatesPS( VFOutputVS );

	MaterialInputPS materialInput = InitMaterialInputPS();
	MaterialParametersPS materialParameters = GetMaterialParameterPS( VFOutputVS, intermediates );

	CalcMaterialParameters(materialInput , materialParameters);

	//float4 color = float4( materialParameters.vectexColor * 0.1 , 1 );
	float4 color = 0;
	color.rgb += materialInput.emissiveColor;
	color.a = 1;

	GBufferData GBuffer;
	GBuffer.worldPos = GetMaterialWorldPositionAndCheckDepthOffset(materialInput, materialParameters);
	//GBuffer.normal = normalize(vsOutput.normal.xyz);
	float3 materialNormal = GetMaterialNormal(materialInput , materialParameters);
	materialNormal.y *= intermediates.normalYSign;
	GBuffer.normal = normalize( materialParameters.tangentToWorld * materialNormal );

	//GBuffer.normal = materialParameters.worldNormal;

	GBuffer.baseColor = materialInput.baseColor;
	GBuffer.metallic = materialInput.metallic;
	GBuffer.roughness = materialInput.roughness;
	GBuffer.specular = materialInput.specular;
	GBuffer.shadingModel = materialInput.shadingModel;
	float4 svPos = View.worldToClip * float4(materialParameters.worldPos, 1);
	svPos = materialParameters.screenPos;
	GBuffer.depth = materialParameters.screenPos.z;
	
	float4 GBufferA, GBufferB, GBufferC, GBufferD;
	EncodeGBuffer(GBuffer, GBufferA, GBufferB, GBufferC, GBufferD);


	gl_FragData[0] = color;
#if 1
	gl_FragData[1] = GBufferA;
	gl_FragData[2] = GBufferB;
	gl_FragData[3] = GBufferC;
#else
	gl_FragData[1] = float4( tangentX, 1 );
	gl_FragData[2] = float4( tangentY, 1 );
	gl_FragData[3] = float4( tangentZ, 1 );
#endif
	gl_FragData[4] = GBufferD;

}


#endif //PIXEL_SHADER
