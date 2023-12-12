/* moon.vert */

attribute vec3 vertexPosition;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

varying float radius;

void main() 
{
	mat4 modelview_matrix = viewMatrix * modelMatrix;
	vec4 worldPosition = modelview_matrix * vec4(vertexPosition, 1.0);
    gl_Position = projectionMatrix *  worldPosition;
	
	radius = length(vertexPosition);
}
