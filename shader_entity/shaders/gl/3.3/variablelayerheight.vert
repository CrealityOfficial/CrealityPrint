#version 330 core

in vec3 vertexPosition;
out vec4 worldPosition;

uniform mat4 model_matrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main( void )
{
    worldPosition = model_matrix * vec4(vertexPosition, 1.0);
    gl_Position = projectionMatrix * viewMatrix * worldPosition;
}
