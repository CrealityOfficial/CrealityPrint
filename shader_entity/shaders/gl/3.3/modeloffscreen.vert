#version 330 core

in vec3 vertexPosition;
in vec3 vertexColor;
uniform vec3 offset1;
uniform vec3 offset2;

out VS_OUT {
    vec3 vert;
    vec3 color;
} vs_out;

void main( void )
{
    vs_out.vert    = vertexPosition + offset1 + offset2;
    vs_out.color   = vertexColor;
}
