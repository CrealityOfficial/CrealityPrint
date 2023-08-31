#version 150 core
in highp vec3 qt_Vertex;
in highp vec2 texCoord;

uniform mat4 u_modelMatrix;
uniform mat4 u_viewMatrix;
uniform mat4 u_projectMatrix;

out vec2 v_texCoord;
void main(void)
{
    gl_Position = u_projectMatrix * u_viewMatrix * u_modelMatrix * vec4(qt_Vertex, 1.0f);
    v_texCoord = texCoord;
}

