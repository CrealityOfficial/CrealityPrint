/* modeldlp.vert */

attribute vec4 vertexPosition;
attribute vec3 vertexNormal;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

varying vec3 viewDirection;
varying vec3 normal;
varying vec3 gnormal;
varying vec3 worldPosition;
varying vec3  worldWater;

void main( void )
{
    mat4 modelViewMatrix = viewMatrix * modelMatrix;
    vec4 tworldPosition = modelViewMatrix * vec4(vertexPosition.xyz, 1.0);
    gl_Position = projectionMatrix *  tworldPosition;

    viewDirection  = normalize(vec3(-tworldPosition));
    mat3 normalMatrix = mat3(modelViewMatrix);

    normal          = normalMatrix * vertexNormal;
    gnormal        = mat3(modelMatrix) * vertexNormal;
		
    worldPosition   = vec3(modelMatrix * vec4(vertexPosition.xyz, 1.0));
}
