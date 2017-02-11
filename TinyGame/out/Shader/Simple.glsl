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

uniform vec3 color = vec3( 0.7 , 0.8 , 0.75 );
uniform vec3 lightDir = vec3( 0.4 , 0.5 , 0.8 );

varying vec3 outColor;
varying vec3 normal;

#ifdef VERTEX_SHADER

void mainVS()
{
	mat4 matWorld = gParam.matViewInv * gl_ModelViewMatrix;
	normal = normalize( transpose( inverse( mat3( matWorld ) ) ) * gl_Normal );
	outColor = abs( dot( normal , normalize(lightDir) ) ) * gl_Color.rgb;
	gl_Position = ftransform();
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
