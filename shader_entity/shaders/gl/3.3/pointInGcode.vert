#version 150 core

in vec3 vertexPosition;

in vec2 stepsFlag;

out vec2 flags;

// uniform mat4 modelViewProjection;
uniform mat4 model_matrix;
uniform mat4 viewProjectionMatrix;

uniform vec3 eyePosition;

uniform float layerHeight;
uniform float lineWidth;

void main() 
{
   flags = stepsFlag;
   
   vec4 worldPos =  model_matrix * vec4(vertexPosition, 1.0);
   vec3 dir = normalize(eyePosition - worldPos.xyz);

   float d = abs( dot(dir, vec3(0.0, 0.0, 1.0)) );
   float k = mix(lineWidth, layerHeight, d) * 0.5 ;

   worldPos += vec4(dir, 0.0) * k * (1.1);

   gl_Position = viewProjectionMatrix * worldPos;
   
}
