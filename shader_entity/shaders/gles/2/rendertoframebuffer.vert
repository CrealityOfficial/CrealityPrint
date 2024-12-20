#version 150 core

in vec3 vertexPosition;
uniform vec4 layoutRect;

void main()
{
	vec2 project = (vertexPosition.xy - layoutRect.xy) / layoutRect.zw * 2.0 - 1.0;
	gl_Position = vec4(project.x, -project.y, 0.0, 1.0);
}
