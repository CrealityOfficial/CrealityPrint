#version 150 core

in vec3 vertexPosition;
in vec3 vertexNormal;
in vec2 vertexTexcoord;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec2 texcoord;
out vec3 normal;

void main() 
{
   gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(vertexPosition, 1.0);
   texcoord = vertexTexcoord;
   
   mat3 normalMatrix = mat3(viewMatrix);
   normal          = normalMatrix * vertexNormal;
}
