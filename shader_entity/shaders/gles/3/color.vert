#version 300 es
/* color.vert */
precision mediump float;

in vec3 vertexPosition;
in vec4 vertexColor;

uniform mat4 modelViewProjection;
out vec4 fcolor;

void main() 
{
   gl_Position = modelViewProjection * vec4(vertexPosition, 1.0);
   fcolor = vertexColor;
}
