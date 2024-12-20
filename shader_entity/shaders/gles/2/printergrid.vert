/* printergrid.vert */

attribute vec3 vertexPosition;
attribute vec2 vertexFlag;
uniform mat4 modelViewProjection;

varying vec2 flag;
void main() 
{
   gl_Position = modelViewProjection * vec4(vertexPosition, 1.0);
   flag = vertexFlag;
}
