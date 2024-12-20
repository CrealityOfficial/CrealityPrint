#version 150 core
out vec4 fragment_color;

in vec3 normal;
flat in vec2 step;
flat in float visualType;
in vec3 viewDirection;

uniform vec4 color = vec4(0.8, 0.8, 0.8, 1.0);

uniform vec3 light_direction = vec3(0.0, 0.0, 1.0);
uniform vec4 front_ambient = vec4(0.8, 0.8, 0.8, 1.0);
uniform vec4 front_diffuse = vec4(0.6, 0.6, 0.6, 1.0);
uniform vec4 back_ambient = vec4(0.3, 0.3, 0.3, 1.0);
uniform vec4 back_diffuse = vec4(0.3, 0.3, 0.3, 1.0);
uniform vec4 specular = vec4(0.5, 0.5, 0.5, 1.0);

uniform int showType = 0;
uniform int animation = 0;

uniform vec4 clipValue;
uniform vec2 layershow;
uniform int layerstartflag_show;

uniform float speedcolorSize;
uniform vec4 typecolors[18];
uniform vec4 speedcolors[13];
uniform vec4 nozzlecolors[6];
uniform int typecolorsshow[18];

uniform float specularPower = 32.0;

vec4 directLight(vec3 light_dir, vec3 fnormal, vec4 diffuse_color, vec4 core_color)
{
	float NdotL 		  = max(dot(fnormal, light_dir), 0.0);
	diffuse_color         = diffuse_color * core_color;
	vec4 total_diffuse    = NdotL * diffuse_color;
	
	vec3 freflection      = reflect(-light_dir, fnormal);
	vec3 fViewDirection   = normalize(viewDirection);
	float RdotV           = max(0.0, dot(freflection, fViewDirection)); 
	vec4 specularColor    = specular * pow( RdotV, specularPower) * core_color;
	
	return total_diffuse + specularColor;
}

void main( void )
{
	if(step.x < clipValue.x || step.x > clipValue.y)
		discard;
		
	if(step.x == clipValue.y && (step.y < clipValue.z || step.y > clipValue.w))
		discard;
		
	if(step.x < layershow.x || step.x > layershow.y)
		discard;
	
	if(showType == 1 && typecolorsshow[int(visualType)] == 0)
		discard;
	
	vec4 core_color = vec4(0.5, 0.5, 0.5, 1.0);
	vec3 lightDir = normalize(light_direction);
	
	if(showType == 0)
	{
		float speedFlag = visualType * speedcolorSize;
		int stype = int(speedFlag);
		vec4 lastColor = speedcolors[stype];
		vec4 nextColor = speedcolors[stype + 1];
		core_color = lastColor + (nextColor - lastColor) * (speedFlag - stype);
	}
	else if(showType == 1)
	{
		core_color = typecolors[int(visualType)];
	}
	else if(showType == 2)
		core_color = nozzlecolors[int(visualType)];
	
	vec3 fnormal 		  =	normalize(normal);
	vec4 ambient_color 	  = front_ambient;
	vec4 diffuse_color    = front_diffuse;
	
	ambient_color 		  = ambient_color * core_color;
	
	vec4 light_color1     = directLight(lightDir, fnormal, diffuse_color, core_color);
	
	core_color = ambient_color + light_color1;
	core_color.a = color.a;
	
	if(animation > 0)
	{
		if(step.x == clipValue.y)
		{
			core_color += vec4(0.3, 0.3, 0.3, 0.0);
		}
		else
		{
			core_color -= vec4(0.3, 0.3, 0.3, 0.0);
		}
	}
	
	fragment_color = core_color;
}
