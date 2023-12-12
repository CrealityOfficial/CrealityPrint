#version 150 core

in vec3 vertexPosition;
in vec2 vertexFlag;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

flat out vec2 flag;

void main( void )
{	
	mat4 modelview_matrix = viewMatrix * modelMatrix;
	vec4 world_position = modelview_matrix * vec4(vertexPosition, 1.0);
    gl_Position = projectionMatrix *  world_position;
	flag = vertexFlag;
}
