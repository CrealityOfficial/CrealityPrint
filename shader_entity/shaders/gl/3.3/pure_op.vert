#version 150 core

in vec3 vertexPosition;

uniform mat4 model_matrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main() 
{
   gl_Position = projectionMatrix * viewMatrix * model_matrix * vec4(vertexPosition, 1.0);
}
