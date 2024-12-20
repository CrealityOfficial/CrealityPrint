#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VS_OUT {
vec3 vert;
} gs_in[];

out vec3 passVert;
out vec3 passNormal;

// uniform mat4 projectionMatrix;
// uniform mat4 viewMatrix;
uniform mat4 modelMatrix;
uniform mat4 modelViewProjection;


void combindVertex(int index)
{
    vec4 pos = vec4(gs_in[index].vert, 1.0);
    passVert = vec3(modelMatrix * pos);
    gl_Position = modelViewProjection * pos;
	gl_PrimitiveID = gl_PrimitiveIDIn;
    EmitVertex();
}

void main() 
{
    vec3 a = gs_in[1].vert - gs_in[0].vert;
    vec3 b = gs_in[2].vert - gs_in[1].vert;
    vec3 tmpCross = cross(a, b);
    passNormal = normalize(mat3(modelMatrix) * tmpCross);
    
    combindVertex(0);
    
    combindVertex(1);

    combindVertex(2);

    EndPrimitive();
}
