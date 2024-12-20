#version 150 core

out vec4 fragColor;
flat in vec2 flag;
uniform float visible;
uniform vec4 xshowcolor;
uniform vec4 yshowcolor;
uniform vec4 linecolor;

void main() 
{
	if(int(flag.x) == 0)
	{
		fragColor = yshowcolor;
	}
	else if(int(flag.y) == 0)
	{
		fragColor = xshowcolor;
	}
	else
	{
		if((((int(flag.x/10.0))%4 > 0) || ((int(flag.y/10.0))%4 > 0)) && (visible == 0.0))
			discard;
		
		fragColor = linecolor;
	}
	

/*
	if (visible == 0.0)
	{
		discard;
	}
	else
	{
		if(((int(flag.x/10.0))%4 > 0) || ((int(flag.y/10.0))%4 > 0))
			//fragColor = showcolor;
			discard;
		else	
			fragColor = linecolor;
	}
	*/
}
