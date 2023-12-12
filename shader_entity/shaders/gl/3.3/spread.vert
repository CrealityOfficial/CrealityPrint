#version 150 core

in vec3 vertexPosition;
in float vertexFlag;

out VS_OUT {
    vec3 vert;
    float flag;
} vs_out;

void main( void )
{
    vs_out.vert   = vertexPosition;
    vs_out.flag   = vertexFlag;
}
