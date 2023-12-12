#version 300 es
/* pickindicator.vert */
precision mediump float;

in vec3 vertexPosition;
in vec3 vertexNormal;
in vec2 vertexTexCoord;
in float facesIndex;

flat out vec4 passColor;

out vec3 passVert;
out vec3 passNormal;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;
uniform mat4 viewportMatrix;
uniform ivec2 vertexBase;

void main() 
{

	int _faceId =  int(facesIndex) + int(vertexBase.x / 3);;
	int i3 = _faceId / 0x1000000;
	_faceId = _faceId - i3 * 0x1000000;
	int i2 = _faceId / 0x10000;
	_faceId = _faceId - i2 * 0x10000;
	int i1 = _faceId / 0x100;
	_faceId = _faceId - i1 * 0x100;
	int i0 = _faceId;
	
	vec4 position = vec4(vertexPosition, 1.0);
	passColor = vec4(
					float(i0) / 255.0,
					float(i1) / 255.0,
					float(i2) / 255.0,
					float(i3) / 255.0);
	
	passVert = (modelMatrix * position).xyz;
	passNormal = mat3(modelMatrix) * vertexNormal;
	gl_Position = viewportMatrix * projectionMatrix * viewMatrix * modelMatrix * position;

}
