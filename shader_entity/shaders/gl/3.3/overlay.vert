#version 150 core

in vec3 vertexPosition;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main() 
{
   vec4 pos = projectionMatrix * viewMatrix * modelMatrix * vec4(vertexPosition, 1.0);
   pos.z = -pos.w;
   gl_Position = pos;
}
