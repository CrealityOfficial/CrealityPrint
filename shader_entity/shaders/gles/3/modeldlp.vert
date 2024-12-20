#version 300 es
/* modeldlp.vert */
precision mediump float;

in vec3 vertexPosition;

out VS_OUT {
vec3 vert;
} vs_out;

void main( void )
{
	vs_out.vert = vertexPosition;
}
