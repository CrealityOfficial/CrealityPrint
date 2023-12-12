#version 150 core

in vec3 vertexPosition;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main() 
{
    vec4 worldPosition = modelMatrix * vec4(vertexPosition, 1.0);
    vec4 viewPosition = viewMatrix * worldPosition;
    vec4 projectedPosition = projectionMatrix * viewPosition;
    vec3 offset = normalize(viewPosition.xyz) * 2.0; // 向镜头偏移两个像素点

    gl_Position = projectedPosition + vec4(offset, 0.0);
}
