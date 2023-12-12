#version 330

layout (triangles) in;
layout (triangle_strip,max_vertices = 3) out;

in VS_OUT {
vec3 vert;
} gs_in[];

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec3 viewDirection;
out vec3 vertexnormal;
out vec3 worldPosition;
out vec3 gnormal;

void main()
{
mat4 modelViewMatrix = viewMatrix * modelMatrix;

worldPosition = vec3(modelMatrix * vec4(gs_in[0].vert, 1.0));
vec4 tworldPosition0 = modelViewMatrix * vec4(gs_in[0].vert, 1.0);
viewDirection = normalize(vec3(-tworldPosition0));
vec3 a = gs_in[1].vert - gs_in[0].vert;
vec3 b = gs_in[2].vert - gs_in[1].vert;
vec3 tmpCross = cross(a, b);
vertexnormal = normalize(mat3(viewMatrix) * mat3(modelMatrix) * tmpCross);
gnormal = normalize(mat3(modelMatrix) * tmpCross);

gl_Position = projectionMatrix * tworldPosition0;
EmitVertex();

worldPosition = vec3(modelMatrix * vec4(gs_in[1].vert, 1.0));
vec4 tworldPosition1 = modelViewMatrix * vec4(gs_in[1].vert, 1.0);
viewDirection = normalize(vec3(-tworldPosition1));
gl_Position = projectionMatrix * tworldPosition1;
EmitVertex();

worldPosition = vec3(modelMatrix * vec4(gs_in[2].vert, 1.0));
vec4 tworldPosition2 = modelViewMatrix * vec4(gs_in[2].vert, 1.0);
viewDirection = normalize(vec3(-tworldPosition2));
gl_Position = projectionMatrix * tworldPosition2;
EmitVertex();

EndPrimitive();
}