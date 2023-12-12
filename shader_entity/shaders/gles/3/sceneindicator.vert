#version 300 es
/* sceneindicator.vert */
precision mediump float;

in vec3 vertexPosition;
in vec3 vertexNormal;
in vec2 vertexTexCoord;
in float facesIndex;

out vec2 texCoord;
out float dirIndex;
out vec3 normal;

uniform mat4 modelViewProjection;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 viewportMatrix;

void main() 
{
   texCoord = vertexTexCoord;
   dirIndex = facesIndex;
   
   gl_Position = viewportMatrix * projectionMatrix * viewMatrix * modelMatrix * vec4(vertexPosition, 1.0);

   normal = mat3(viewMatrix * modelMatrix ) * vertexNormal;
   // gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(vertexPosition, 1.0);

}
