#include "ViewParam.glsl"

struct VertexFactoryParameters
{
	mat4 localToWorld;
	mat4 worldToLocal;
};

uniform VertexFactoryParameters VertexFactoryParams = VertexFactoryParameters( 
   mat4( 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 ) ,
   mat4( 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 ) );

const vec3 dirOffset[ 6 ] = vec3[](
   vec3( 1 , 0 , 0 ) , vec3( -1 , 0 , 0 ) ,
   vec3( 0 , 1 , 0 ) , vec3( 0 , -1 , 0 ) ,
   vec3( 0 , 0 , 1 ) , vec3( 0 , 0 , -1 ) ); 

uniform vec3 color = vec3( 0.7 , 0.8 , 0.75 );
uniform vec3 lightDir = vec3( 0.4 , 0.5 , 0.8 );
uniform vec3 localScale = vec3( 1 , 1 , 1 );
uniform int  dirX = 0;
uniform int  dirZ = 4;

varying vec3 outColor;
varying vec3 normal;

#ifdef VERTEX_SHADER

void MainVS()
{
	mat4 matWorld = View.viewToWorld * gl_ModelViewMatrix;
	vec3 axisX = dirOffset[ dirX ];
	vec3 axisZ = dirOffset[ dirZ ];
	vec3 axisY = cross( axisZ , axisX );

	mat4 matDir = mat4( 
		vec4( localScale.x * axisX , 0 ) , 
		vec4( localScale.y * axisY , 0 ) , 
		vec4( localScale.z * axisZ , 0 ) , 
		vec4( 0,0,0,1) );

	normal = matWorld * matDir * vec4( gl_Normal , 0 );
	outColor = abs( dot( normal , normalize(lightDir) ) ) * gl_Color.rgb;
	gl_Position = gl_ModelViewProjectionMatrix * matDir * gl_Vertex;
}

#endif //VERTEX_SHADER

#ifdef PIXEL_SHADER

void MainPS() 
{
	vec3 N = normalize( normal );
	gl_FragColor = vec4( outColor , 1.0 );
	//gl_FragColor = vec4( 0.5 * N + 0.5 , 1.0 );
}

#endif //PIXEL_SHADER
