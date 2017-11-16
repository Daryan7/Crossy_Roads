#version 330

uniform mat4 projection, model, view;
uniform mat3 normalMatrix;

in vec3 position;
in vec3 normal;
in vec2 texCoord;

out vec3 pos;
out vec3 N;
out vec2 fragTexCoord;

void main() {
	N = mat3(view)*normalMatrix*normal;
	fragTexCoord = texCoord;
	vec4 almostPos = view * model * vec4(position, 1.0);
	pos = almostPos.xyz;
	gl_Position = projection * almostPos;
}

