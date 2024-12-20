#version 150 core

in vec3 vertexPosition;
in float vertexFlag;
in float originFlag;

out VS_OUT {
    vec3 vert;
    float flag;
    float hFlag;
} vs_out;

void main( void )
{
    vs_out.vert   = vertexPosition;
    vs_out.flag   = vertexFlag;
    vs_out.hFlag  = originFlag;
}
