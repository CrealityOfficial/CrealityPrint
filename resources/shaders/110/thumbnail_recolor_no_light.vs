#version 110

attribute vec2 Position;

void main()
{
    gl_Position = vec4(Position.xy, 0.0, 1.0);
}
