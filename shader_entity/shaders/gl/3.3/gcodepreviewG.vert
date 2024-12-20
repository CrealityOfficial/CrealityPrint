#version 150 core

in vec3 vertexPosition;
in vec3 endVertexPosition;
in vec3 vertexNormal;
in vec2 stepsFlag;
in float visualTypeFlags;
in vec4 lineWidth_layerHeight;


out vec3 startVertexVS;
out vec3 endVertexVS;
out vec3 vNormalVS;
flat out vec2 stepVS;
flat out float visualTypeVS;
flat out vec2 lineWidthLayerHeightVS; 
flat out vec4 colorVS;
flat out vec2 cornerCompensate;

uniform int showType = 0;
uniform int animation = 0;
uniform float speedcolorSize;
uniform vec4 typecolors[53];
uniform vec4 speedcolors[13];
uniform vec4 nozzlecolors[51];
uniform vec4 darkColor = vec4(0.161, 0.161, 0.161, 1.0);
uniform vec4 clipValue;

vec4 typeColor()
{
	return typecolors[int(visualTypeFlags)];
}

vec4 nozzleColor()
{
	return nozzlecolors[int(visualTypeFlags)];
}

vec4 getSpeedColor()
{
	float speedFlag = visualTypeFlags * speedcolorSize;
	int stype = int(speedFlag);
	vec4 lastColor = speedcolors[stype];
	vec4 nextColor = speedcolors[stype + 1];
	return lastColor + (nextColor - lastColor) * (speedFlag - float(stype));
}

void main( void )
{	
	vec4 speedColor = getSpeedColor();
	vec4 colors[10];
	colors[0] = speedColor; //speedColor
	colors[1] = typeColor();
	colors[2] = nozzleColor();
	colors[3] = speedColor; //heightColor
	colors[4] = speedColor; //lineWidthColor
	colors[5] = speedColor; //flowColor
	colors[6] = speedColor; //layerTimeColor
	colors[7] = speedColor; //fanSpeedColor
	colors[8] = speedColor; //temperatureColor
	colors[9] = speedColor; //accColor

	colorVS = colors[showType];
	if (animation == 1 && stepsFlag.x != clipValue.y) 
		colorVS = darkColor;

	startVertexVS = vertexPosition;
	endVertexVS = endVertexPosition;
	vNormalVS = vertexNormal;
	stepVS = stepsFlag;
	visualTypeVS = visualTypeFlags;
	lineWidthLayerHeightVS = lineWidth_layerHeight.xy;
	cornerCompensate = lineWidth_layerHeight.zw;
}
