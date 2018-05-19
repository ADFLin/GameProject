#ifndef SHADOW_DEPTH_ONE_PASS
#define SHADOW_DEPTH_ONE_PASS 0
#endif

#if SHADOW_DEPTH_ONE_PASS
#define VERTEX_FACTORY_USE_GEOMETRY_SHADER 1
#endif

#include "Common.glsl"
#include "MaterialProcess.glsl"
#include "VertexProcess.glsl"
#include "ShadowCommon.glsl"



#ifndef SHADOW_LIGHT_TYPE
#define SHADOW_LIGHT_TYPE LIGHTTYPE_POINT
#endif
uniform mat4 DepthShadowMatrix;

#if VERTEX_SHADER

out float Depth;

out VertexFactoryOutputVS VFOutputVS;
void MainVS()
{
	VertexFactoryIntermediatesVS intermediates = GetVertexFactoryIntermediatesVS();

	MaterialInputVS materialInput = InitMaterialInputVS();
	MaterialParametersVS materialParameters = GetMaterialParameterVS(intermediates);

	CalcMaterialInputVS(materialInput, materialParameters);

	gl_Position = CalcVertexFactoryOutputVS(VFOutputVS, intermediates, materialInput, materialParameters);
	float4 outPosition = DepthShadowMatrix * float4(VFOutputVS.worldPos , 1 );

#if 0
	float4 viewOffset = View.worldToView * float4(VFOutputVS.worldPos, 1);
#	if SHADOW_LIGHT_TYPE == LIGHTTYPE_POINT
	Depth = (length(viewOffset.xyz) - DepthParam.x) / (DepthParam.y - DepthParam.x);
#	else
	Depth = (-viewOffset.z - DepthParam.x) / (DepthParam.y - DepthParam.x);
#	endif
#else
#	if USE_PERSPECTIVE_DEPTH
	Depth = outPosition.z / outPosition.w;
#	else
	Depth = outPosition.z * ShadowParam.y;
#	endif
#endif

}
#endif //VERTEX_SHADER

#if SHADOW_DEPTH_ONE_PASS

#if GEOMETRY_SHADER
void MainGS()
{
	for( int face = 0; face < 6; ++face )
	{




	}
}

#endif //GEOMETRY_SHADER

#endif //SHADOW_DEPTH_ONE_PASS

#if PIXEL_SHADER

#if SHADOW_DEPTH_ONE_PASS
#define VFInputPS VFOutputGS
#else
#define VFInputPS VFOutputVS
#endif  //SHADOW_DEPTH_ONE_PASS


in VertexFactoryIutputPS VFInputPS;

in float Depth;
void MainPS()
{

	MaterialInputPS materialInput = InitMaterialInputPS();
	MaterialParametersPS materialParameters = GetMaterialParameterPS(VFInputPS);

	CalcMaterialParameters(materialInput, materialParameters);

	float outDepth = Depth;

#if MATERIAL_USE_DEPTH_OFFSET
	float3 svPosition = materialParameters.clipPos;
	if( materialInput.depthOffset != 0 )
	{
		svPosition.z += materialInput.depthOffset;

		float4 worldPos = View.clipToWorld * float4( svPosition , 1 );
		float4 outPosition = DepthShadowMatrix * float4( worldPos.xyz / worldPos.w , 1);
#if USE_PERSPECTIVE_DEPTH
		outDepth = outPosition.z / outPosition.w;
#else
		outDepth = outPosition.z * ShadowParam.y;
#endif
	}
	WritePixelDepth(svPosition.z);

#endif
	//depth = 0.5;
	gl_FragColor = vec4(outDepth, outDepth, outDepth, 1.0);
	//gl_FragColor = float4(shadowPos.xy / shadowPos.w, shadowPos.z / shadowPos.w, 0);
	//gl_FragColor *= float4(materialParameters.worldNormal, 1);
	//gl_FragColor = depth;
}
#endif //PIXEL_SHADER