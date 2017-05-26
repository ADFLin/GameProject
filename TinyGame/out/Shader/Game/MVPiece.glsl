#include "ViewParam.glsl"

struct PrimitiveParameters
{
	mat4 localToWorld;
	mat4 worldToLocal;
};

uniform PrimitiveParameters Primitive = PrimitiveParameters( 
   mat4( 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 ) ,
   mat4( 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 ) );

const float3 dirOffset[ 6 ] = float3[](
   float3( 1 , 0 , 0 ) , float3( -1 , 0 , 0 ) ,
   float3( 0 , 1 , 0 ) , float3( 0 , -1 , 0 ) ,
   float3( 0 , 0 , 1 ) , float3( 0 , 0 , -1 ) ); 

uniform float3 color = float3( 0.7 , 0.8 , 0.75 );
uniform float3 lightDir = float3( 0.4 , 0.5 , 0.8 );
uniform float3 localScale = float3( 1 , 1 , 1 );
uniform int  dirX = 0;
uniform int  dirZ = 4;

varying float3 outColor;
varying float3 normal;

#if VERTEX_SHADER

void MainVS()
{
	mat4 matWorld = View.viewToWorld * gl_ModelViewMatrix;
	float3 axisX = dirOffset[ dirX ];
	float3 axisZ = dirOffset[ dirZ ];
	float3 axisY = cross( axisZ , axisX );

	mat4 matDir = mat4( 
		float4( localScale.x * axisX , 0 ) , 
		float4( localScale.y * axisY , 0 ) , 
		float4( localScale.z * axisZ , 0 ) , 
		float4( 0,0,0,1) );

	normal = matWorld * matDir * float4( gl_Normal , 0 );
	outColor = abs( dot( normal , normalize(lightDir) ) ) * gl_Color.rgb;
	gl_Position = gl_ModelViewProjectionMatrix * matDir * gl_Vertex;
}

#endif //VERTEX_SHADER

#if PIXEL_SHADER

void MainPS() 
{
	float3 N = normalize( normal );
	gl_FragColor = float4( outColor , 1.0 );
	//gl_FragColor = vec4( 0.5 * N + 0.5 , 1.0 );
}

#endif //PIXEL_SHADER
