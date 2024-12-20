#version 150 core

in vec3 vertexPosition;
uniform vec4 layoutRect;
uniform vec2 u_dir;

void main()
{
	vec2 project = (vertexPosition.xy - layoutRect.xy) / layoutRect.zw * 2.0 - 1.0;
	gl_Position = vec4(u_dir.x * project.x, u_dir.y * project.y, 0.0, 1.0);
}
