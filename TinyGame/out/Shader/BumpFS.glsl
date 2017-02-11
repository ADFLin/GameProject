uniform sampler2D texBase;
uniform sampler2D texNormal;

varying vec3 lightOffset;
varying vec3 viewOffset;

void main()
{
	vec2 st = gl_TexCoord[0].st;

	vec3 N = texture2D( texNormal , st ).xyz * 2.0 - 1.0;
	vec3 baseColor = texture2D( texBase , st ).rgb;

	vec3 color = vec3(0,0,0);
	vec3 L = normalize( lightOffset );

	float diff = max(0.0, dot(N, L));
	float spec = 0.0;

	if(diff != 0.0)
	{
		vec3 E = normalize( viewOffset );	
		vec3 H = normalize(L + E);
		spec = pow( max( dot(N,H), 0.0 ) , 10 );
	}
	color += ( diff + spec ) * vec3( 0.6 , 0.6 , 0.6 );
	gl_FragColor = vec4( 0.5 * L + 0.5 , 1.0 );
	//gl_FragColor = texture2D( tex , gl_TexCoord[0].xy ) ;
	gl_FragColor = vec4( color , 1.0 );
	gl_FragColor = vec4( color * baseColor , 1.0 );
}