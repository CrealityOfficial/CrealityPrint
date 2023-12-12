#version 150 core

in vec3 vertexPosition;
in vec3 vertexNormal;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

noperspective out vec3 viewDirection;
noperspective out vec3 normal;

void main( void )
{
	mat4 modelViewMatrix = viewMatrix * modelMatrix;
	vec4 tworldPosition = modelViewMatrix * vec4(vertexPosition, 1.0);
    gl_Position = projectionMatrix *  tworldPosition;
	
	viewDirection  = normalize(vec3(-tworldPosition));
	mat3 normalMatrix = mat3(modelViewMatrix);
	
    normal          = normalMatrix * vertexNormal;
}
