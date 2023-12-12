#version 330 core

in vec3 vertexPosition;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

void main() 
{
	vec4 position = vec4(vertexPosition, 1.0);

	gl_Position = projectionMatrix * viewMatrix * modelMatrix * position;
}
