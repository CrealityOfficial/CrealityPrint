#version 150 core

in vec3 vertexPosition;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

uniform float offset;

void main() 
{
    vec4 worldPosition = modelMatrix * vec4(vertexPosition, 1.0);
    vec4 viewPosition = viewMatrix * worldPosition;
    vec4 projectedPosition = projectionMatrix * viewPosition;
    vec3 _offset = normalize(viewPosition.xyz) * offset; // 向镜头偏移
    gl_Position = projectedPosition + vec4(_offset, 0);
}
