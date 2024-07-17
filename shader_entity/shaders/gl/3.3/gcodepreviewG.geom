#version 150 core

layout (points) in;
layout (triangle_strip, max_vertices = 18) out;

in vec3 startVertexVS[1];
in vec3 endVertexVS[1];
in vec3 vNormalVS[1];
flat in vec2 stepVS[1];
flat in float visualTypeVS[1];
flat in vec2 lineWidthLayerHeightVS[1];
flat in vec4 colorVS[1];
flat in vec2 cornerCompensate[1];

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 model_matrix;

out vec3 normal;
flat out vec2 step;
flat out float visualType;
flat out vec4 color;
out vec3 viewDirection;

//uniform float layerHeight = 0.1;
//uniform float lineWidth = 0.4;

void combindVertex(vec3 pos, vec3 norm)
{
	vec4 world_position = viewMatrix * model_matrix * vec4(pos, 1.0);
	gl_Position = projectionMatrix *  world_position;
	normal = mat3(viewMatrix) * norm;
	viewDirection  = normalize(vec3(world_position));
	EmitVertex();
}

void main( void )
{	
	step = stepVS[0];
	visualType = visualTypeVS[0];
	color = colorVS[0];

	vec3 norms[8];
	norms[0] = vec3(0.0, 0.0, 1.0);
	norms[1] = vNormalVS[0];
	norms[2] = vec3(0.0, 0.0, -1.0);
	norms[3] = - vNormalVS[0];
	norms[4] = vec3(0.0, 0.0, 1.0);
	norms[5] = vNormalVS[0];
	norms[6] = vec3(0.0, 0.0, -1.0);
	norms[7] = - vNormalVS[0];
	
	vec3 poses[8];
 	float lineWidth = lineWidthLayerHeightVS[0].x;
	float layerHeight = lineWidthLayerHeightVS[0].y;

	vec3 k = normalize(endVertexVS[0] - startVertexVS[0]) * lineWidth * 0.5;
	vec3 ax = k * cornerCompensate[0].x;
	vec3 bx = k * cornerCompensate[0].y;

	poses[0] = startVertexVS[0] + layerHeight * 0.5 * norms[0];
	poses[1] = startVertexVS[0] + lineWidth   * 0.5 * norms[1] + ax;
	poses[2] = startVertexVS[0] + layerHeight * 0.5 * norms[2];
	poses[3] = startVertexVS[0] + lineWidth   * 0.5 * norms[3] - ax;
	poses[4] = endVertexVS[0]   + layerHeight * 0.5 * norms[4];
	poses[5] = endVertexVS[0]   + lineWidth   * 0.5 * norms[5] - bx;
	poses[6] = endVertexVS[0]   + layerHeight * 0.5 * norms[6];
	poses[7] = endVertexVS[0]   + lineWidth   * 0.5 * norms[7] + bx;
	
	combindVertex(poses[0], norms[0]);
	combindVertex(poses[4], norms[4]);
	combindVertex(poses[1], norms[1]);
	combindVertex(poses[5], norms[5]);
	combindVertex(poses[2], norms[2]);
	combindVertex(poses[6], norms[6]);
	combindVertex(poses[3], norms[3]);
	combindVertex(poses[7], norms[7]);
	combindVertex(poses[0], norms[0]);
	combindVertex(poses[4], norms[4]);
	EndPrimitive();


	combindVertex(poses[0], norms[0]);
	combindVertex(poses[1], norms[1]);
	combindVertex(poses[3], norms[3]);
	combindVertex(poses[2], norms[2]);
	EndPrimitive();


	combindVertex(poses[4], norms[4]);
	combindVertex(poses[7], norms[7]);
	combindVertex(poses[5], norms[5]);
	combindVertex(poses[6], norms[6]);
	EndPrimitive();	

}

