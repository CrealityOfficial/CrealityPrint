/* modelwireframe.geom */

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in vec3 viewDirectionVS[3];
in vec3 normalVS[3];
in vec3 gnormalVS[3];
in vec3 worldPositionVS[3];
in vec3 worldWaterVS[3];

varying vec3 viewDirection;
varying vec3 normal;
varying vec3 gnormal;
varying vec3 worldPosition;
varying vec3 worldWater;
varying vec3 barycentric;


void combindVertex(int index)
{
    gl_Position = gl_in[index].gl_Position;

    viewDirection = viewDirectionVS[index];
    normal = normalVS[index];
    gnormal = gnormalVS[index];
    worldPosition = worldPositionVS[index];
    worldWater = worldWaterVS[index];

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
