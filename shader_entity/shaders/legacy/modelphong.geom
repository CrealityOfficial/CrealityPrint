#version 150 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in vec3 viewDirectionVS[3];
in vec3 normalVS[3];
in vec3 gnormalVS[3];
in vec3 worldPositionVS[3];
in vec3 colorVS[3];

out vec3 viewDirection;
out vec3 normal;
out vec3 gnormal;
out vec3 worldPosition;
out vec3 barycentric;
out vec3 gcolor;

void combindVertex(int index)
{
    gl_Position = gl_in[index].gl_Position;

    viewDirection = viewDirectionVS[index];
    normal = normalVS[index];
    gnormal = gnormalVS[index];
	gcolor =  colorVS[index];
    worldPosition = worldPositionVS[index];

    EmitVertex();
}


void main() {
    
    barycentric = vec3(1.0, 0.0, 0.0);
    combindVertex(0);
    
    barycentric = vec3(0.0, 1.0, 0.0);
    combindVertex(1);

    barycentric = vec3(0.0, 0.0, 1.0);
    combindVertex(2);

    EndPrimitive();
}
