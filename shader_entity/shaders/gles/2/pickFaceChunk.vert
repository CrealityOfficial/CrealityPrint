/* pickFaceChunk.vert */

attribute vec3 vertexPosition;
attribute float vertexFlag;

varying vec4 passColor;
varying float flag;

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
					
	//vec4 passColors[2];
	//passColors[0] = vec4(1.0, 0.0, 0.0, 1.0);
	//passColors[1] = vec4(0.0, 1.0, 0.0, 1.0);
	//
	//int index = vertexBase.x == 120000000 ? 1 : 0;
	//passColor = passColors[index];
	
	gl_Position = projectionMatrix * viewMatrix * modelMatrix * position;
	flag			= vertexFlag;
}
