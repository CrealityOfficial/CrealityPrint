#version 150 core

in vec3 vertexPosition;
uniform vec4 layoutRect;
uniform int mirrorType;

void main()
{
	vec2 project = (vertexPosition.xy - layoutRect.xy) / layoutRect.zw * 2.0 - 1.0;
	
	if(0 == mirrorType)  //normal type
		gl_Position = vec4(project.x, -project.y, 0.0, 1.0);
	else if(1 == mirrorType) //mirror_x
		gl_Position = vec4(project.x, project.y, 0.0, 1.0);
	else if(2 == mirrorType) //mirror_y
		gl_Position = vec4(-project.x, -project.y, 0.0, 1.0);
}
