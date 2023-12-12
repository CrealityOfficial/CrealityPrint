#version 150 core

in vec3 vertexPosition;
in vec3 vertexColor;

out VS_OUT {
    vec3 vert;
    vec3 color;
} vs_out;

void main( void )
{
    vs_out.vert      = vertexPosition;
    vs_out.color   = vertexColor;
}
