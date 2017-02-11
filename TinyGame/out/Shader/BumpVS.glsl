
uniform vec3 lightPos = vec3( 0 , -10 , 5 );

varying vec3 lightOffset;
varying vec3 viewOffset;

void main()
{
	vec3 tang   = normalize( gl_NormalMatrix * gl_MultiTexCoord1.xyz );
	vec3 normal = normalize( gl_NormalMatrix * gl_Normal );
	vec3 binormal = cross( normal , tang );

	mat3 tbn  = mat3( tang , binormal , normal );
	vec3 pos = vec3( gl_ModelViewMatrix * gl_Vertex );

	vec3 dir = vec3( gl_ModelViewMatrix * vec4( lightPos , 1.0 ) ) -  pos;
	lightOffset = dir * tbn;
	viewOffset   = ( -pos ) * tbn ;

	gl_Position = ftransform();
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;

}