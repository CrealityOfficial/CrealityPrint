/* Support Vertex*/
precision mediump float;

attribute vec4 vertexPosition;
attribute vec3 vertexNormal;
attribute float vertexFlag;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

varying vec3 viewDirection;
varying vec3 normal;
varying vec3 worldPosition;
varying float flag;

void main( void )
{
	mat4 modelViewMatrix = viewMatrix * modelMatrix;
	vec4 tworldPosition = modelViewMatrix * vec4(vertexPosition.xyz, 1.0);
    gl_Position = projectionMatrix *  tworldPosition;
	
	viewDirection  = normalize(vec3(-tworldPosition));
	mat3 normalMatrix = mat3(modelViewMatrix);
	
    normal          = normalMatrix * vertexNormal;
	worldPosition   = vec3(modelMatrix * vec4(vertexPosition.xyz, 1.0));
	flag			= vertexFlag;
	
	if(vertexFlag == 0.0)
	{
		gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
	}
}
