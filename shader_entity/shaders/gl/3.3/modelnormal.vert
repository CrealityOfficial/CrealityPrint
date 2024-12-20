#version 330 core

in vec3 vertexPosition;
in float vertexFlag;
uniform vec3 offset;

out VS_OUT {
    vec3 vert;
    float flag;
} vs_out;

void main( void )
{
    vs_out.vert    = vertexPosition + offset;
    vs_out.flag   = vertexFlag;
}
