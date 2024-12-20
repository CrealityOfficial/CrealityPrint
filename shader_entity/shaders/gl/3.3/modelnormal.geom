#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VS_OUT {
vec3 vert;
float flag;
} gs_in[];

uniform mat4 model_matrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec3 normal;
out vec3 gnormal;
out vec3 worldPosition;
out float colorFlag;

void combindVertex(int index, mat4 mvMatrix)
{
    colorFlag = gs_in[index].flag;

    vec4 pos = vec4(gs_in[index].vert, 1.0);
    worldPosition = vec3(model_matrix * pos);
    gl_Position = projectionMatrix * mvMatrix * pos;

    EmitVertex();
}


void main() 
{
    mat4 modelViewMatrix = viewMatrix * model_matrix;
	
	vec3 a = gs_in[1].vert - gs_in[0].vert;
	vec3 b = gs_in[2].vert - gs_in[1].vert;
	vec3 tmpCross = cross(a,b);
	
	mat4 normalMatrix = transpose(inverse(modelViewMatrix));
	normal = normalize(mat3(normalMatrix) * tmpCross);
	
	mat4 ggnmatrix = transpose(inverse(model_matrix));
	gnormal = normalize(mat3(ggnmatrix) * tmpCross);

    combindVertex(0, modelViewMatrix);
    
    combindVertex(1, modelViewMatrix);

    combindVertex(2, modelViewMatrix);

    EndPrimitive();
	
}
