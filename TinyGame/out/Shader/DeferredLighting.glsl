#include "DeferredLightingCommon.glsl"
#include "LightingCommon.glsl"
#include "ShadowCommon.glsl"
#include "MaterialCommon.glsl"
#include "ScreenVertexShader.glsl"
#include "ViewParam.glsl"
#include "LightParam.glsl"

uniform mat4 ProjectShadowMatrix[8];

#ifndef DEFERRED_LIGHT_TYPE
#define DEFERRED_LIGHT_TYPE LIGHTTYPE_POINT
#endif

#ifdef PIXEL_SHADER

in VSOutput vsOutput;

void LightingPassPS()
{
	vec2 ScreenUVs = vsOutput.UVs;
	GBufferData GBuffer = GetScreenGBuffer(ScreenUVs);

	if( GBuffer.shadingModel == SHADINGMODEL_UNLIT )
	{
		discard;
		//gl_FragColor = float4( 0 , 0 , 0 , 1 );
	}

#if 0
	float3 worldPos = ScreenUVToWorldPos(ScreenUVs);
	float NDCz = tex2D(FrameDepthTexture, ScreenUVs).r;
	//gl_FragColor = float4(NDCz, NDCz, NDCz, 1);
	worldPos = StoreWorldPos(GBuffer.depth , ScreenUVs);
	float dz = GBuffer.depth - tex2D(FrameDepthTexture, ScreenUVs).r;
	float depth = tex2D(FrameDepthTexture, ScreenUVs).r;
	float4 svPos = float4(0, 0, GBuffer.depth, 1);
	svPos = View.clipToWorld * svPos;
	gl_FragColor = float4( worldPos , 1 );
	gl_FragColor = float4(NDCz, 0 , 0,0);
	gl_FragColor = float4(GBuffer.depth, GBuffer.depth, GBuffer.depth, 1);
	return;
#endif

	gl_FragColor = float4( GBuffer.worldPos , 1 );
	//return;
	float3 viewOffset = View.worldPos - GBuffer.worldPos;
	
	float attenuation = 1;
	float3 shadow = 1;
	float3 L = float3(0,0,1);
	float3 outColor = 0;
	if( GLight.type == LIGHTTYPE_DIRECTIONAL )
//#if DEFERRED_LIGHT_TYPE == LIGHTTYPE_DIRECTIONAL
	{
		L = -GLight.dir;
		float4 shadowPos = ProjectShadowMatrix[0] * float4(GBuffer.worldPos, 1);
		shadow = CalcDirectionalLightShadow(GBuffer.worldPos, ProjectShadowMatrix);
		//shadow = 1;
	}
//#else
	else
	{
		float3 lightVector = GLight.worldPosAndRadius.xyz - GBuffer.worldPos;
		float dist = length(lightVector);

		L = lightVector / dist;

		attenuation = CalcRaidalAttenuation(dist, 1.0 / GLight.worldPosAndRadius.w);
		if( GLight.type == LIGHTTYPE_POINT )
//#if DEFERRED_LIGHT_TYPE == LIGHTTYPE_POINT
		{
			shadow = CalcPointLightShadow(GBuffer.worldPos , ProjectShadowMatrix , -lightVector);
		}
//#else
		else
		{
			float cosTheta = dot(-L, GLight.dir);
			//cosTheta = -0.5;
			if ( cosTheta > GLight.spotParam.y )
			{
				shadow = CalcSpotLightShadow(GBuffer.worldPos, ProjectShadowMatrix[0]);

			}

			float fallOff = saturate((cosTheta - GLight.spotParam.y) / (GLight.spotParam.x - GLight.spotParam.y));
			attenuation *= fallOff;
		}
//#endif
		//attenuation = 1;
		//shadow = 1;
	}
//#endif

	//float  s = 1;
	//GBuffer.shadingModel = 1;
	if ( GBuffer.shadingModel == SHADINGMODEL_DEFAULT_LIT )
	{
		float c = 1;
		float3 N = GBuffer.normal;
		float3 V = normalize(viewOffset);

		float3 roughness = float3(GBuffer.roughness, GBuffer.roughness, 1);

		roughness = max(roughness, 0.04);
		//float3 shading = phongShading(data.baseColor.rgb, data.baseColor.rgb, N, L, V, 20);
		float3 shading = StandardShading(GBuffer.diffuseColor, GBuffer.specularColor, roughness, float3(1), L, V, N);
		
		outColor = attenuation * shadow * shading * GLight.color;
		//outColor = 2 * shadow * shading;
	}
	//gl_FragColor = float4( float2( ProjectShadowMatrix[0] * float4(GBuffer.worldPos, 1) ) , 0 , 1 );
	gl_FragColor = float4(outColor, 1);
	//gl_FragColor = float4(GLight.spotParam.x);
	//gl_FragColor = float4(shadow, 1);
	
}

#endif //PIXEL_SHADER
