#version 150 core

in vec3 vertexPosition;
in vec2 vertexTexcoord;

uniform mat4 modelViewProjection;
out vec2 texcoord;

void main() 
{
   gl_Position = modelViewProjection * vec4(vertexPosition, 1.0);
   texcoord = vertexTexcoord;
}
