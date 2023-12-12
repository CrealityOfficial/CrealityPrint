#version 150 core

in vec3 vertexPosition;
// in vec3 vertexNormal;

// in vec3 worldPos;
in vec2 stepsFlag;

out vec2 flags;

// uniform mat4 modelViewProjection;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main() 
{
   flags = stepsFlag;
   gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(vertexPosition, 1.0);
}
