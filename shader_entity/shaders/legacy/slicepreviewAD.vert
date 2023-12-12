#version 150 core

in vec3 vertexPosition;
in vec3 vertexNormal;
in vec2 vertexFlag;
in vec4 vertexDrawFlag;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec3 normal;
flat out vec2 flag;
flat out vec4 drawFlag;
out vec3 viewDirection;

void main( void )
{	
	flag = vertexFlag;
	drawFlag = vertexDrawFlag;

	mat4 usematrix = modelMatrix;

	if((int(drawFlag.z) == 1 ||int(drawFlag.z) == 7) && int(drawFlag.w) == 1)
	{
		mat4 translate = mat4(
			1.0, 0.0, 0.0, 0.0,
			0.0, 1.0, 0.0, 0.0,
			0.0, 0.0, 1.0, 0.0,
			-35.0, 0.0, 0.0, 1.0
		);
		usematrix = modelMatrix * translate;
	}
	mat4 modelview_matrix = viewMatrix * usematrix;
	vec4 world_position = modelview_matrix * vec4(vertexPosition, 1.0);
    gl_Position = projectionMatrix *  world_position;
	
	
	viewDirection  = normalize(vec3(-world_position));
    normal          = mat3(modelview_matrix) * vertexNormal;
}
