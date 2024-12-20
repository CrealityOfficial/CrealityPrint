#version 330 core

in vec3 vertexPosition;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 model_matrix;

void main() 
{
	gl_Position = projectionMatrix * viewMatrix * model_matrix * vec4(vertexPosition, 1.0);
}
