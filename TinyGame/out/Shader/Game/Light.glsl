uniform vec2 LightLocation;
uniform vec3 LightColor;
uniform float screenHeight;
uniform vec3 LightAttenuation;

void main() 
{
	float distance = length(LightLocation - gl_FragCoord.xy);
	float attenuation = 1.0 /( dot( LightAttenuation , vec3( 1 , distance , distance*distance) ) );
	vec4 color = vec4( attenuation * LightColor, 1);
	gl_FragColor = color;
}