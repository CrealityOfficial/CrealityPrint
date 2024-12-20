#version 150 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VS_OUT {
vec3 vert;
vec3 color;
} gs_in[];

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec3 viewDirection;
out vec3 normal;
out vec3 gnormal;
out vec3 worldPosition;
out vec3 barycentric;
out vec3 gcolor;

void combindVertex(int index, mat4 mvMatrix)
{
    gcolor = gs_in[index].color;

    vec4 pos = vec4(gs_in[index].vert, 1.0);
    worldPosition = vec3(modelMatrix * pos);
    viewDirection = normalize(vec3(-(mvMatrix * pos)));
    gl_Position = projectionMatrix * mvMatrix * pos;

    EmitVertex();
}


void main() {
    
    mat4 modelViewMatrix = viewMatrix * modelMatrix;

    vec3 a = gs_in[1].vert - gs_in[0].vert;
    vec3 b = gs_in[2].vert - gs_in[1].vert;
    vec3 tmpCross = cross(a, b);
    normal = normalize(mat3(modelViewMatrix) * tmpCross);
    gnormal = normalize(mat3(modelMatrix) * tmpCross);
    
    barycentric = vec3(1.0, 0.0, 0.0);
    combindVertex(0, modelViewMatrix);
    
    barycentric = vec3(0.0, 1.0, 0.0);
    combindVertex(1, modelViewMatrix);

    barycentric = vec3(0.0, 0.0, 1.0);
    combindVertex(2, modelViewMatrix);

    EndPrimitive();
}
