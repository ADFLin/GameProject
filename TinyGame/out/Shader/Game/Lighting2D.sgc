#include "Common.glsl"

uniform float2 LightLocation;
uniform float3 LightColor;
uniform float screenHeight;
uniform float3 LightAttenuation;

#if PIXEL_SHADER

void LightingPS() 
{
	float distance = length(LightLocation - gl_FragCoord.xy);
	float attenuation = 1.0 /( dot( LightAttenuation , float3( 1 , distance , distance*distance) ) );
	float4 color = float4( attenuation * LightColor, 1);
	gl_FragColor = color;
}

#endif