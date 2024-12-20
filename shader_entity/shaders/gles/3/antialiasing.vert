#version 300 es
/* antialiasing.vert */
precision mediump float;

in vec3 vertexPosition;
in vec2 vertexTexCoord;

out vec2 texCoord;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main() 
{
   //gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(vertexPosition, 1.0);
   gl_Position = vec4(vertexPosition, 1.0);
   texCoord = vertexTexCoord;
}
