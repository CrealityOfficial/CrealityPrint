#version 330 core

in vec3 vertexPosition;
in vec2 vertexTexCoord;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec2 texcoord;

void main() 
{
   texcoord = vertexTexCoord;
   gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(vertexPosition, 1.0);
}
