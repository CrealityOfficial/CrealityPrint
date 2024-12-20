#version 150 core

in vec3 vertexPosition;

uniform mat4 model_matrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main( void )
{
    vec4 tworldPosition = model_matrix * vec4(vertexPosition, 1.0);
	tworldPosition.z = -0.01;
    gl_Position = projectionMatrix * viewMatrix *  tworldPosition;
}
