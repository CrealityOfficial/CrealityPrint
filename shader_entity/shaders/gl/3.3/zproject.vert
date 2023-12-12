#version 150 core

in vec3 vertexPosition;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main( void )
{
    vec4 tworldPosition = modelMatrix * vec4(vertexPosition, 1.0);
	tworldPosition.z = -0.01;
    gl_Position = projectionMatrix * viewMatrix *  tworldPosition;
}
