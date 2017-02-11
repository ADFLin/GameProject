#include "Common.glsl"
#include "LightingCommon.glsl"

#define USE_POINT_LIGHT 1

uniform vec3 lightPos;
uniform vec3 lightColor = vec3( 1 , 1 , 1 );
uniform mat4 matLightView;

uniform float SMSize = 512;
uniform float shadowBias = 0.5;
uniform float shadowFactor = 1;


struct GlobalParam
{
	mat4 matWorld;
	vec3 viewPos;
};

uniform GlobalParam gParam = GlobalParam( 
	mat4( 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 ) ,
	vec3( 0,0,0 ) );

uniform vec2 depthParam;
#if USE_POINT_LIGHT
uniform samplerCube texSM;
#else
uniform sampler2D texSM;
#endif

varying vec3 viewOffset;
varying vec3 lightViewOffset;
varying vec3 lightOffset;
varying vec3 normal;
varying vec3 color;

#ifdef VERTEX_SHADER

void mainVS() 
{
	vec3 pos = vec3( gParam.matWorld * gl_Vertex );
	normal = mat3( gParam.matWorld ) * gl_Normal;
	lightViewOffset = vec3( matLightView * vec4( pos , 1 ) );
	lightOffset = lightPos - pos;
	viewOffset = gParam.viewPos - pos;
	color = gl_Color.rgb;
	gl_Position = ftransform();
}

#endif //VERTEX_SHADER

#ifdef PIXEL_SHADER

//layout(depth_unchanged) 
out float gl_FragDepth;

float calcShadowFactor( in vec3 viewOffset )
{

	const float factor = 0.01;
#if USE_POINT_LIGHT
	float dist = length( viewOffset );
	vec3 lightViewDir = viewOffset / dist;
	float testOffset = shadowFactor * dist - shadowBias;
	float depth = ( testOffset - depthParam.x ) / ( depthParam.y - depthParam.x ) ; 
	if ( depth > 1.0 )
		return factor;

	if ( texture( texSM , lightViewDir ).r  < depth )
		return factor;

	return 1.0;

#else
	vec3 lightViewDir = normalize( viewOffset );
	float testOffset = shadowFactor * -viewOffset.z - shadowBias;

	float depth = shadowFactor * ( testOffset - depthParam.x ) / ( depthParam.y - depthParam.x ) - shadowBias;
	if ( depth > 1.0 )
		return factor;

	vec2 t = 0.5 - 0.5 * ( lightViewDir.xy ) / lightViewDir.z;

	float c = 1;
	if ( dot( lightViewDir , vec3(0,0,-1) ) > 0.707 )
	{
		vec2 dx = vec2( 1.0 / SMSize , 0 );
		vec2 dy = vec2( 0 , 1.0 / SMSize );
		float count;
		count += ( texture2D( texSM , t ).r < depth ) ? 1.0 : 0.0;
		count += ( texture2D( texSM , t + dx ).r < depth ) ? 1.0 : 0.0;
		count += ( texture2D( texSM , t - dx ).r < depth ) ? 1.0 : 0.0;
		count += ( texture2D( texSM , t + dy ).r < depth ) ? 1.0 : 0.0;
		count += ( texture2D( texSM , t - dy ).r < depth ) ? 1.0 : 0.0;
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


void mainFS() 
{

	float s = calcShadowFactor( lightViewOffset );

	vec3 N = normalize( normal );
	float dist = length( lightOffset );
	vec3 L = lightOffset / dist;
	vec3 V = normalize( viewOffset );

	float3 shading = phongShading(lightColor, lightColor, N, L, V, 20.0);
	gl_FragColor = float4( (s) * ( 1 / ( 0.05 * dist * dist + 1 ) ) * shading * color , 1 );
	//gl_FragColor = vec4( c *( 0.5 * N + 0.5 ) / 3  , 1 );
	//gl_FragColor = vec4( color.rgb ,1 );
}

#endif //PIXEL_SHADER