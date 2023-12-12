#version 150 core

in vec3 vertexPosition;
in vec3 vertexNormal;

out vec3 normal;
out vec3 viewDirection;
out vec3 lightDir;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform vec3 lightDirection = vec3(0.0, 1.0, 0.8);

void main() 
{
   vec3 view_position = vec3(viewMatrix * modelMatrix * vec4(vertexPosition, 1.0));
   viewDirection  = normalize(view_position);
   normal = mat3(viewMatrix * modelMatrix) * vertexNormal;
   // normal = -viewDirection;
   lightDir = mat3(viewMatrix) * lightDirection;
   gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(vertexPosition, 1.0);
}
