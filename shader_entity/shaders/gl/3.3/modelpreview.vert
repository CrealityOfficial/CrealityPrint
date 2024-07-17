#version 330 core

in vec3 vertexPosition;
in vec3 vertexColor;
uniform vec2 offset;

out VS_OUT {
    vec3 vert;
    vec3 color;
} vs_out;

void main( void )
{
    vs_out.vert    = vertexPosition + vec3(offset, 0.0);
    vs_out.color   = vertexColor;
}
