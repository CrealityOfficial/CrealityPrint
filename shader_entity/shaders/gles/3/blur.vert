#version 300 es
/* blur.vert */
precision mediump float;
in vec3 vertexPosition;
in vec2 vertexTexCoord;
out vec2 texCoord0;
void main()
{
   gl_Position = vec4(vertexPosition, 1.0);
	texCoord0 = vertexTexCoord;
}
