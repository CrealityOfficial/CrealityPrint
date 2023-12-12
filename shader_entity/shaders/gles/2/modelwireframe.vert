/* modelwireframe.vert */

attribute vec3 vertexPosition;
attribute vec3 vertexNormal;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform vec3 water;

varying vec3 viewDirectionVS;
varying vec3 normalVS;
varying vec3 gnormalVS;
varying vec3 worldPositionVS;
varying vec3 worldWaterVS;

void main( void )
{
    mat4 modelViewMatrix = viewMatrix * modelMatrix;
    vec4 tworldPosition = modelViewMatrix * vec4(vertexPosition, 1.0);
    gl_Position = projectionMatrix *  tworldPosition;

    viewDirectionVS  = normalize(vec3(-tworldPosition));
    mat3 normalMatrix = mat3(modelViewMatrix);

    normalVS          = normalMatrix * vertexNormal;
    gnormalVS        = mat3(modelMatrix) * vertexNormal;
		
    worldPositionVS   = vec3(modelMatrix * vec4(vertexPosition, 1.0));
    worldWaterVS = water;
}
