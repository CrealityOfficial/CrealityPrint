/* bottomface.vert */

attribute vec3 vertexPosition;
attribute vec3 vertexNormal;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

varying vec2 texcoord;
varying vec3 normal;

void main() 
{
   gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(vertexPosition, 1.0);
   
   mat3 normalMatrix = mat3(viewMatrix);
   normal          = normalMatrix * vertexNormal;
}
