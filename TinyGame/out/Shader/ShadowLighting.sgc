#include "Common.glsl"
#include "LightingCommon.glsl"
#include "ShadowCommon.glsl"
#include "ViewParam.glsl"
#include "LightParam.glsl"

#define USE_POINT_LIGHT 1

uniform mat4 matLightView;

uniform float SMSize = 512;

struct PrimitiveParameters
{
	mat4 localToWorld;
};

uniform PrimitiveParameters Primitive = PrimitiveParameters( 
	mat4( 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 ) );

#if USE_POINT_LIGHT

#else
uniform sampler2D ShadowTextureCube;
#endif

varying vec3 viewOffset;
varying vec3 lightViewOffset;
varying vec3 lightOffset;
varying vec3 normal;
varying vec3 color;

#if VERTEX_SHADER

void MainVS() 
{
	vec3 pos = vec3( Primitive.localToWorld * gl_Vertex );
	normal = mat3( Primitive.localToWorld ) * gl_Normal;
	lightViewOffset = vec3( matLightView * vec4( pos , 1 ) );
	lightOffset = GLight.worldPosAndRadius.xyz - pos;
	viewOffset = View.worldPos - pos;
	color = gl_Color.rgb;
	gl_Position = ftransform();
}

#endif //VERTEX_SHADER

#if PIXEL_SHADER

//layout(depth_unchanged) 
out float gl_FragDepth;

float calcShadowFactor( in vec3 viewOffset )
{
	const float factor = 0.01;
#if USE_POINT_LIGHT
	return CalcPointLightShadow(viewOffset);
#else
	vec3 lightViewDir = normalize( viewOffset );
	float testOffset = ShadowFactor * -viewOffset.z - ShadowBias;

	float depth = ShadowFactor * ( testOffset - DepthParam.x ) / ( DepthParam.y - DepthParam.x ) - ShadowBias;
	if ( depth > 1.0 )
		return factor;

	vec2 t = 0.5 - 0.5 * ( lightViewDir.xy ) / lightViewDir.z;

	float c = 1;
	if ( dot( lightViewDir , vec3(0,0,-1) ) > 0.707 )
	{
		vec2 dx = vec2( 1.0 / SMSize , 0 );
		vec2 dy = vec2( 0 , 1.0 / SMSize );
		float count;
		count += ( texture2D( ShadowTextureCube , t ).r < depth ) ? 1.0 : 0.0;
		count += ( texture2D( ShadowTextureCube , t + dx ).r < depth ) ? 1.0 : 0.0;
		count += ( texture2D( ShadowTextureCube , t - dx ).r < depth ) ? 1.0 : 0.0;
		count += ( texture2D( ShadowTextureCube , t + dy ).r < depth ) ? 1.0 : 0.0;
		count += ( texture2D( ShadowTextureCube , t - dy ).r < depth ) ? 1.0 : 0.0;
		//count += ( texture2D( texSM , t + dx - dy ).r < depth ) ? 1.0 : 0.0;
		//count += ( texture2D( texSM , t + dx + dy ).r < depth ) ? 1.0 : 0.0;
		//count += ( texture2D( texSM , t - dx - dy ).r < depth ) ? 1.0 : 0.0;
		//count += ( texture2D( texSM , t - dx + dy ).r < depth ) ? 1.0 : 0.0;

		c = 1 - ( 1 - factor ) * ( count ) / 5;	
	}
	else
	{
		c = factor;
	}
	return c;
#endif
}


void MainPS() 
{

	float s = calcShadowFactor( lightViewOffset );
	//s = 1;
	vec3 N = normalize( normal );
	float dist = length( lightOffset );
	vec3 L = lightOffset / dist;
	vec3 V = normalize( viewOffset );

	float3 shading = PhongShading(GLight.color, GLight.color, N, L, V, 20.0);
	gl_FragColor = float4( (s) * ( 1 / ( dist * dist + 1 ) ) * shading * color , 1 );
	//gl_FragColor = vec4( c *( 0.5 * N + 0.5 ) / 3  , 1 );
	//gl_FragColor = vec4( color.rgb ,1 );
}

#endif //PIXEL_SHADER