#version 150 core

in vec3 vertexPosition;
in vec3 vertexNormal;
in vec2 vertexTexcoord;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec3 viewDirection;
out vec3 normal;
out vec2 texcoord;

void main( void )
{
	mat4 modelview_matrix = viewMatrix * modelMatrix;
	vec4 worldPosition = modelview_matrix * vec4(vertexPosition, 1.0);
    gl_Position = projectionMatrix *  worldPosition;
	
	viewDirection  = normalize(vec3(-worldPosition));
    normal          = mat3(modelview_matrix) * vertexNormal;
	texcoord = vertexTexcoord;
}
