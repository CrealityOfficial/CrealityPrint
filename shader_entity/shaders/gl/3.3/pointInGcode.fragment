#version 150 core

in vec2 flags;

out vec4 fragColor;

uniform vec4 color;
uniform vec4 darkColor = vec4(0.161, 0.161, 0.161, 1.0);

uniform vec4 clipValue = vec4(0.0, 200.0, 0.0, 300.0);
uniform vec2 layershow = vec2(-1.0, 9999999.0);

uniform int animation = 0;

void main() 
{
	if (flags.x < clipValue.x || flags.x > clipValue.y)
		discard;
		
	if (flags.x == clipValue.y && (flags.y < clipValue.z || flags.y > clipValue.w))
		discard;

	if (flags.x < layershow.x || flags.x > layershow.y)
		discard;

	fragColor = (animation == 1 && flags.x != clipValue.y) ? darkColor : color;
}
