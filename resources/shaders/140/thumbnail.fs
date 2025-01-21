#version 140

uniform bool ban_light;
uniform vec4 uniform_color;
uniform float emission_factor;

uniform vec4 specificColorLayers[256];	// vec4(color, z), 最多显示256个分段
uniform int layersCount;	

// x = tainted, y = specular;
in vec2 intensity;
//varying float drop;
in vec4 world_pos;

void main()
{
    if (world_pos.z < 0.0)
        discard;

	vec4 _color;
	if (layersCount > 0) {
		vec3 tempColor = uniform_color.rgb;
		for (int i = 0; i < layersCount; ++i) 
		{
			vec4 layerInfo = specificColorLayers[i];
			if (world_pos.z >= layerInfo.w)
				tempColor = vec3(layerInfo.rgb);
			else 
				break;
		}
        _color = vec4(vec3(intensity.y) + tempColor * (intensity.x + emission_factor), 1.0);
	} else {
        if(ban_light) {
            _color = uniform_color;
        } else {
            _color = vec4(vec3(intensity.y) + uniform_color.rgb * (intensity.x + emission_factor), uniform_color.a);
        }
	}

    gl_FragColor = _color;
}
