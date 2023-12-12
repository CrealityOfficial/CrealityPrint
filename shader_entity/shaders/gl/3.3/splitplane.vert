#version 150 core

in vec3 vertexPosition;

out vec4 clip; 
out vec3 worldPosition;

uniform mat4 modelViewProjection;
uniform mat4 modelMatrix;

void main() 
{
   worldPosition = (modelMatrix * vec4(vertexPosition, 1.0)).xyz;
   clip = modelViewProjection * vec4(vertexPosition, 1.0);
   gl_Position = clip;

}
