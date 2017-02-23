#include "ViewParam.glsl"

struct VertexFactoryParameters
{
	mat4 localToWorld;
	mat4 worldToLocal;
};

uniform VertexFactoryParameters VertexFactoryParams = VertexFactoryParameters( 
   mat4( 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 ) ,
   mat4( 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 ) );

uniform vec3 color = vec3( 0.7 , 0.8 , 0.75 );
uniform vec3 lightDir = vec3( 0.4 , 0.5 , 0.8 );

varying vec3 outColor;
varying vec3 normal;

#ifdef VERTEX_SHADER

void MainVS()
{
	mat4 matWorld = View.viewToWorld * gl_ModelViewMatrix;
	normal = normalize( transpose( inverse( mat3( matWorld ) ) ) * gl_Normal );
	outColor = abs( dot( normal , normalize(lightDir) ) ) * gl_Color.rgb;
	gl_Position = ftransform();
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
