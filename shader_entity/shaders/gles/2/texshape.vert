/* texshape.vert */

attribute vec3 vertexPosition;
attribute vec2 vertexTexcoord;

uniform mat4 modelViewProjection;
varying vec2 texcoord;

void main() 
{
   gl_Position = modelViewProjection * vec4(vertexPosition, 1.0);
   texcoord = vertexTexcoord;
}
