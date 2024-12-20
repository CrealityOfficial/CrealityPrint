#version 300 es
/* pickFace_pwd.vert */
precision mediump float;

in vec3 vertexPosition;
in vec3 vertexNormal;

out vec3 passVert;
out vec3 passNormal;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

void main() 
{
	vec4 position = vec4(vertexPosition, 1.0);
	
	passVert = (modelMatrix * position).xyz;
	passNormal = mat3(modelMatrix) * vertexNormal;
	gl_Position = projectionMatrix * viewMatrix * modelMatrix * position;

}
