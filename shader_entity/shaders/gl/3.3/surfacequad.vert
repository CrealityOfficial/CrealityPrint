#version 150 core

in vec3 vertexPosition;

void main() 
{
   gl_Position = vec4(vertexPosition.xy, -0.999, 1.0);
}
