#include "Common.glsl"

struct VSOutput
{
	float2 UVs;
};

#if VERTEX_SHADER

out VSOutput vsOutput;

void CopyTextureVS()
{
	gl_Position = gl_Vertex;
	vsOutput.UVs = gl_MultiTexCoord0.xy;
}

#endif //VERTEX_SHADER

#if PIXEL_SHADER

in VSOutput vsOutput;

uniform sampler2D CopyTexture;
void CopyTexturePS()
{
	float2 UVs = vsOutput.UVs;
	gl_FragColor = texture2D(CopyTexture, UVs);
}


uniform float4 ColorMask;
void CopyTextureMaskPS()
{
	float2 UVs = vsOutput.UVs;
	float c = dot(texture2D(CopyTexture, UVs), ColorMask);
	gl_FragColor = float4( c , c , c , 1 );
}


uniform float2 ColorBais;
void CopyTextureBaisPS()
{
	float2 UVs = vsOutput.UVs;
	float3 color = ColorBais.x * texture2D(CopyTexture, UVs).rgb + ColorBais.y;
	//color = texture2D(CopyTexture, UVs).rgb;
	gl_FragColor = float4(color, 1);
}

uniform float2 ValueFactor;

const float3 Color[] =
{
	float3(0,0,0),
	float3(1,0,0),
	float3(0,1,0),
	float3(0,0,1),
	float3(1,1,0),
	float3(0,1,1),
	float3(1,0,1),
	float3(1,0.5,0.5),
	float3(0.5,1,0.5),
	float3(0.5,0.5,1),
};

void MappingTextureColorPS()
{
	vec2 UVs = vsOutput.UVs;
	float c = dot(texture2D(CopyTexture, UVs), ColorMask);
	int idx = int( round(ValueFactor.x * c + ValueFactor.y) );
	if ( idx < 0 || idx > 9 )
		gl_FragColor = float4(1, 1, 1, 1);
	gl_FragColor = float4( Color[idx] , 1 );
	//gl_FragColor = float4(c, c, c, 1);
}

#endif //PIXEL_SHADER