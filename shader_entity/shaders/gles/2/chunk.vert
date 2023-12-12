/* chunk.vert */

attribute vec3 vertexPosition;
attribute vec3 vertexNormal;
attribute float vertexFlag;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

varying float flag;

void main( void )
{
	mat4 modelViewMatrix = viewMatrix * modelMatrix;
	vec4 tworldPosition = modelViewMatrix * vec4(vertexPosition, 1.0);
    gl_Position = projectionMatrix *  tworldPosition;
	
	flag			= vertexFlag;
}
