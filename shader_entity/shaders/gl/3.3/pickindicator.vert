#version 330 core

in vec3 vertexPosition;
in vec3 vertexNormal;
in vec2 vertexTexCoord;
in float facesIndex;

flat out vec4 passColor;

uniform mat4 projection_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;
uniform mat4 viewport_matrix;
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
					
	gl_Position = viewport_matrix * projection_matrix * view_matrix * model_matrix * position;

}
