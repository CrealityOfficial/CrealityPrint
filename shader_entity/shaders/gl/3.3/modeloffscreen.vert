#version 330 core

in vec3 vertexPosition;
in float vertexFlag;
uniform vec3 offset1;
uniform vec3 offset2;

out VS_OUT {
    vec3 vert;
    float flag;
} vs_out;

void main( void )
{
    vs_out.vert    = vertexPosition + offset1 + offset2;
    vs_out.flag   = vertexFlag;
}
