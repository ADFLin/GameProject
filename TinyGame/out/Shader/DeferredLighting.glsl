#include "DeferredShadingCommon.glsl"
#include "LightingCommon.glsl"
#include "ShadowCommon.glsl"
#include "MaterialCommon.glsl"

#include "ViewParam.glsl"
#include "LightParam.glsl"

#ifndef DEFERRED_SHADING_USE_BOUND_SHAPE
#define DEFERRED_SHADING_USE_BOUND_SHAPE 0
#endif

#if DEFERRED_SHADING_USE_BOUND_SHAPE

#ifdef VERTEX_SHADER
//layout(location = ATTRIBUTE0) in float4 BoundVertex;
uniform mat4 BoundTransform;
void LightingPassVS()
{
	//gl_Position = BoundTransform * BoundVertex;
	gl_Position = ftransform();
}
#endif //VERTEX_SHADER

#else

#include "ScreenVertexShader.glsl"

#endif

uniform mat4 ProjectShadowMatrix[8];

#ifndef DEFERRED_LIGHT_TYPE
#define DEFERRED_LIGHT_TYPE LIGHTTYPE_POINT
#endif

#ifdef PIXEL_SHADER

#if DEFERRED_SHADING_USE_BOUND_SHAPE
//layout(early_fragment_tests) in;
#else
in VSOutput vsOutput;
#endif

void ShowBoundPS()
{
	float2 ScreenUVs = (gl_FragCoord.xy + View.viewportPosAndSizeInv.xy) * View.viewportPosAndSizeInv.zw;
	gl_FragColor = ( 1.0 / 100.0 ) * float4(0,1,1,0);
}

void LightingPassPS()
{
#if DEFERRED_SHADING_USE_BOUND_SHAPE
	float2 ScreenUVs = (gl_FragCoord.xy + View.viewportPosAndSizeInv.xy) * View.viewportPosAndSizeInv.zw;
#else
	float2 ScreenUVs = vsOutput.UVs;
#endif

	GBufferData GBuffer = GetGBufferData(ScreenUVs);

	if( GBuffer.shadingModel == SHADINGMODEL_UNLIT )
	{
		//discard;
		gl_FragColor = float4( 0 , 0 , 0 , 1 );
		return;
	}

	float3 worldPos = GBuffer.worldPos;
	//worldPos = StoreWorldPos(ScreenUVs , GBuffer.depth);
	//gl_FragColor = float4(worldPos, 1);
	//gl_FragColor = float4(ScreenUVs, 0, 0);
	//float depth = BufferDepthToSceneDepth(GBuffer.depth) / 800;
	//gl_FragColor = depth;

	float attenuation = 1.0;

	float3 shadow = float3(1.0,1.0,1.0);
	float3 L = float3(0, 0, 1);
	float3 outColor = float3(0.0);
	//if( GLight.type == LIGHTTYPE_DIRECTIONAL )
#if DEFERRED_LIGHT_TYPE == LIGHTTYPE_DIRECTIONAL
	{
		L = -GLight.dir;
		float4 shadowPos = ProjectShadowMatrix[0] * float4(worldPos, 1);
		if( GLight.bCastShadow == 1 )
			shadow = CalcDirectionalLightShadow(worldPos, ProjectShadowMatrix);
	}
#else
	//else
	{
		float3 lightVector = GLight.worldPosAndRadius.xyz - worldPos;
		float dist = length(lightVector);

		L = lightVector / dist;

		attenuation = CalcRaidalAttenuation(dist, 1.0 / GLight.worldPosAndRadius.w);

		//if( GLight.type == LIGHTTYPE_POINT )
#if DEFERRED_LIGHT_TYPE == LIGHTTYPE_POINT
		{
			if( GLight.bCastShadow == 1 )
				shadow = CalcPointLightShadow(GBuffer.worldPos, ProjectShadowMatrix, -lightVector);
		}
#else
		//else
		{

			float cosTheta = dot(-L, GLight.dir);
			//cosTheta = -0.5;
			if( cosTheta > GLight.spotParam.y )
			{
				if( GLight.bCastShadow == 1 )
					shadow = CalcSpotLightShadow(worldPos, ProjectShadowMatrix[0]);
			}

			float fallOff = saturate((cosTheta - GLight.spotParam.y) / (GLight.spotParam.x - GLight.spotParam.y));
			attenuation *= fallOff;
		}
#endif
		//attenuation = 1;
		
	}
#endif //DEFERRED_LIGHT_TYPE == LIGHTTYPE_DIRECTIONAL
	float3 viewOffset = View.worldPos - worldPos;
		//float  s = 1;
		//GBuffer.shadingModel = 1;
	if( GBuffer.shadingModel == SHADINGMODEL_DEFAULT_LIT )
	{
		float c = 1;
		float3 N = GBuffer.normal;
		float3 V = normalize(viewOffset);

		float3 roughness = float3(GBuffer.roughness, GBuffer.roughness, 1);

		roughness = max(roughness, 0.04);
		//float3 shading = phongShading(data.baseColor.rgb, data.baseColor.rgb, N, L, V, 20);
		float3 shading = StandardShading(GBuffer.diffuseColor, GBuffer.specularColor, roughness, float3(1), L, V, N);

		outColor = attenuation  * shading * GLight.color;
		//outColor = shading * attenuation * GLight.color;
		outColor *= shadow;

		//outColor = shadow;
		//outColor = 2 * shadow * shading;
	}

	//outColor *= shadow;
	//gl_FragColor = float4( float2( ProjectShadowMatrix[0] * float4(GBuffer.worldPos, 1) ) , 0 , 1 );
	gl_FragColor = float4(outColor, 1);
	//gl_FragColor = float4(GLight.spotParam.x);
	//gl_FragColor = float4(shadow, 1);

	
}

#endif //PIXEL_SHADER
