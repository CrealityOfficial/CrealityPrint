#version 150 core

in vec3 vertexPosition;
in float vertexFlag;

flat out vec4 passColor;
out vec3 passVert;
out float flag;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;
uniform ivec2 vertexBase;

void main() 
{
	int ids[2];
	ids[0] = int(gl_VertexID / 3.0) + int(vertexBase.x / 3);
	ids[1] = int(vertexBase.x / 3);
	
	int _faceId = ids[vertexBase.y];
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
	gl_Position = projectionMatrix * viewMatrix * modelMatrix * position;
	flag			= vertexFlag;
}
