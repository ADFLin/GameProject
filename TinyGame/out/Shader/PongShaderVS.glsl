varying vec3  vNormal;
varying vec3  vPosition;

void main() 
{
	gl_Position = ftransform();
	vPosition = vec3( gl_ModelViewMatrix * gl_Vertex );
	vNormal   = normalize( gl_NormalMatrix * gl_Normal );
}

void mainPS()
{
	vec3 N = normalize( vNormal );
	vec3 L = normalize( gl_LightSource[i].position.xyz - vPosition );
	vec3 R = normalize(-reflect(L,N));

	vec3 dirEye = normalize( -vPosition );

	vec4 dif = max( dot( N , L ) , 0 );

}