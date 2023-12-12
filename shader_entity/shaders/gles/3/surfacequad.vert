#version 300 es
/* surfacequad.vert */
precision mediump float;

in vec3 vertexPosition;

void main() 
{
   gl_Position = vec4(vertexPosition.xy, -0.999, 1.0);
}
