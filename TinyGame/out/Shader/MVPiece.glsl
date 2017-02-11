struct GlobalParam
{
	mat4 matView;
	mat4 matWorld;
	mat4 matWorldInv;
	mat4 matViewInv;
	vec3 viewPos;
};

uniform GlobalParam gParam = GlobalParam( 
   mat4( 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 ) ,
   mat4( 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 ) ,
   mat4( 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 ) ,
   mat4( 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 ) ,
   vec3( 0,0,0 ) );

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

void mainVS()
{
	mat4 matWorld = gParam.matViewInv * gl_ModelViewMatrix;
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

void mainFS() 
{
	vec3 N = normalize( normal );
	gl_FragColor = vec4( outColor , 1.0 );
	//gl_FragColor = vec4( 0.5 * N + 0.5 , 1.0 );
}

#endif //PIXEL_SHADER
