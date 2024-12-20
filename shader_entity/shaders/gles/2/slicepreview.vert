/* slicepreview.vert */

attribute vec3 vertexPosition;
attribute vec3 vertexNormal;
attribute vec2 vertexFlag;
attribute vec4 vertexDrawFlag;
attribute vec4 vertexSmoothFlag;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

varying vec3 normal;
varying vec2 flag;
varying vec4 drawFlag;
varying vec4 smoothFlag;
varying vec3 viewDirection;

void main( void )
{	
	mat4 modelview_matrix = viewMatrix * modelMatrix;
	vec4 world_position = modelview_matrix * vec4(vertexPosition, 1.0);
    gl_Position = projectionMatrix *  world_position;
	flag = vertexFlag;
	drawFlag = vertexDrawFlag;
	smoothFlag = vertexSmoothFlag;
	
	viewDirection  = normalize(vec3(-world_position));
    normal          = mat3(modelview_matrix) * vertexNormal;
}
