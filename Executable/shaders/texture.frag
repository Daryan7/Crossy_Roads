#version 330

uniform sampler2D tex;
uniform vec3 lightDir;

in vec2 texCoordFrag;
in vec3 fragNormal;
out vec4 outColor;

void main() {
	vec3 n = normalize(fragNormal);
	vec4 texColor = texture(tex, texCoordFrag);
	outColor =  texColor;
}
