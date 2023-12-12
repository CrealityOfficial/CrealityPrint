/* color.vert */

attribute vec3 vertexPosition;
attribute vec4 vertexColor;

uniform mat4 modelViewProjection;
varying vec4 fcolor;

void main() 
{
   gl_Position = modelViewProjection * vec4(vertexPosition, 1.0);
   fcolor = vertexColor;
}
