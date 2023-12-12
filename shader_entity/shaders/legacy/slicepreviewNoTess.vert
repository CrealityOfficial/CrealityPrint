#version 150 core

in vec4 vertexEndPosition;
in vec3 vertexBeginPosition;
in vec4 vertexFlag;
in vec4 vertexDrawFlag;
in vec4 vertexSmoothFlag;

out vec3 worldPos_vs_out;
out vec3 viewDirection;
flat out vec4 vertexFlag_vs_out;
flat out vec4 vertexDrawFlag_vs_out;
out vec4 vertexSmoothFlag_vs_out;
out float vertexFlag_Step;

flat out float segmentLength_vs_out;
flat out vec3 segmentMidPos_vs_out;
flat out vec3 segmentNDir_vs_out;

flat out float VOPNCosValue_vs_out;
flat out vec3 planeNormalNDir_vs_out;
flat out vec3 worldViewNDir_vs_out;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform vec3 eyePosition = vec3(0.0, 0.0, 1.0);


vec3 calActualPos(vec3 positionBegin, vec3 positionEnd, int inIndex, float lineWidthR, float thicknessR)
{
	float r = 0.1;

	segmentMidPos_vs_out = (positionBegin + positionEnd) / 2.0;
	segmentLength_vs_out = distance(positionEnd, positionBegin);
	segmentNDir_vs_out = normalize(positionEnd - positionBegin);


	float halfHeight = segmentLength_vs_out / 2;

	vec3 patchNDirection = segmentNDir_vs_out;
	vec3 viewNDirection = normalize(eyePosition - segmentMidPos_vs_out);

	// vec3 upDirection = vec3(0.0, 0.0, 1.0);
	
	// if (dot(viewNDirection, upDirection) < 0)
	// 	upDirection = -upDirection;
	
	// // int patchDirFlag = 1;
	// // if (dot(viewNDirection, patchNDirection) < 0)
	// // 	patchDirFlag = -1;

	// // vec3 xoyDirection = normalize(cross(patchNDirection, upDirection));
	// // vec3 radiusUpperDirection = normalize(xoyDirection * lineWidthR + upDirection * thicknessR);
	// // float cosValueRUpper2Up = dot(radiusUpperDirection, upDirection);
	// // vec3 radiusLowerDirection = normalize(xoyDirection * lineWidthR - upDirection * thicknessR);
	// // float cosValueRLower2Up = dot(radiusLowerDirection, upDirection);
	// // vec3 lengthUpperDirection = normalize(patchDirFlag * patchNDirection * halfHeight + upDirection * thicknessR);
	// // float cosValueLUpper2Up = dot(lengthUpperDirection, patchDirFlag * patchNDirection);
	// // vec3 lengthLowerDirection = normalize(patchDirFlag * patchNDirection * halfHeight - upDirection * thicknessR);
	// // float cosValueLLower2Up = dot(lengthLowerDirection, patchDirFlag * patchNDirection);

	// vec3 radiusNDirection;
	// float pvCosValue = abs(dot(patchNDirection, viewNDirection));
	// if (pvCosValue >= 1.0)
	// 	radiusNDirection = normalize(cross(patchNDirection, vec3(0.0, 0.0, 1.0)));
	// else
	// 	radiusNDirection = normalize(cross(patchNDirection, viewNDirection));

	// float cosValueR2Up = dot(radiusNDirection, upDirection);

	// // if (cosValueR2Up > cosValueRUpper2Up)
	// // 	radiusNDirection = radiusUpperDirection;
	// // else if (cosValueR2Up < cosValueRLower2Up)
	// // 	radiusNDirection = radiusLowerDirection;


	// vec3 ropUpNDirection = normalize(cross(radiusNDirection, patchNDirection));

	// float rotateLimit = pvCosValue * 1.5;
	// vec3 lengthUpperDirection = normalize(patchNDirection * halfHeight + ropUpNDirection * thicknessR * rotateLimit);
	// float cosValueLUpper2Up = dot(lengthUpperDirection, ropUpNDirection);
	// vec3 lengthLowerDirection = normalize(patchNDirection * halfHeight - ropUpNDirection * thicknessR * rotateLimit);
	// float cosValueLLower2Up = dot(lengthLowerDirection, ropUpNDirection);

	// vec3 lengthDirection = normalize(cross(viewNDirection, radiusNDirection));
	// float cosValueL2Up = dot(lengthDirection, ropUpNDirection);

	// if (cosValueL2Up > cosValueLUpper2Up)
	// {	
	// 	lengthDirection = lengthUpperDirection;
	// 	cosValueL2Up = cosValueLUpper2Up;
	// }
	// else if (cosValueL2Up < cosValueLLower2Up)
	// {
	// 	lengthDirection = lengthLowerDirection;
	// 	cosValueL2Up = cosValueLLower2Up;
	// }

	// planeNormalNDir_vs_out = normalize(cross(radiusNDirection, lengthDirection));
	// worldViewNDir_vs_out = viewNDirection;
	// VOPNCosValue_vs_out = dot(worldViewNDir_vs_out, planeNormalNDir_vs_out);
		

	// radiusNDirection = radiusNDirection * inIndex;
	// // xoyDirection = xoyDirection * inIndex;
	// // lengthDirection = lengthDirection * (vertexIndex * 2 - 1);

	// // vec3 extendDirection = radiusNDirection;

	// // float cosa = dot(radiusNDirection, xoyDirection);
	// // if (abs(cosa) > thicknessR * inversesqrt(thicknessR * thicknessR + lineWidthR * lineWidthR))
	// // {	
	// // 	r = lineWidthR;
	// // 	extendDirection = xoyDirection;
	// // }
	// // else
	// // {
	// // 	r = lineWidthR * thicknessR * inversesqrt(lineWidthR * lineWidthR * (1 - cosa * cosa) + thicknessR * thicknessR * cosa * cosa);
	// // }

	// float sinValueR2Horizon = cosValueR2Up;
	// // float halfLength = sqrt(thicknessR * thicknessR + halfHeight * halfHeight);
	// float halfLength = halfHeight * inversesqrt(1 - cosValueL2Up * cosValueL2Up);
	// // r = max(thicknessR * 2 * segmentLength_vs_out / (2 * halfLength), lineWidthR);
	// r = thicknessR * lineWidthR * inversesqrt(sinValueR2Horizon * sinValueR2Horizon * lineWidthR * lineWidthR + (1 - sinValueR2Horizon * sinValueR2Horizon) * thicknessR * thicknessR);
	// r = max(lineWidthR * sqrt(1 - cosValueR2Up * cosValueR2Up), r) * 1.25;


	vec3 upDirection = (2 * step(0, dot(viewNDirection, vec3(0.0, 0.0, 1.0))) - 1) * vec3(0.0, 0.0, 1.0);

	vec3 xoyDirection = normalize(cross(patchNDirection, upDirection));
	vec3 radiusUpperDirection = normalize(xoyDirection * lineWidthR + upDirection * thicknessR);
	float cosValueRUpper2Up = dot(radiusUpperDirection, upDirection);
	vec3 radiusLowerDirection = normalize(xoyDirection * lineWidthR - upDirection * thicknessR);
	float cosValueRLower2Up = dot(radiusLowerDirection, upDirection);
	// vec3 lengthUpperDirection = normalize(patchDirFlag * patchNDirection * halfHeight + upDirection * thicknessR);
	// float cosValueLUpper2Up = dot(lengthUpperDirection, patchDirFlag * patchNDirection);
	// vec3 lengthLowerDirection = normalize(patchDirFlag * patchNDirection * halfHeight - upDirection * thicknessR);
	// float cosValueLLower2Up = dot(lengthLowerDirection, patchDirFlag * patchNDirection);

	vec3 radiusNDirection;
	float pvCosValue = abs(dot(patchNDirection, viewNDirection));
	float pvCosValueFlag = step(1.0, pvCosValue);
	vec3 tempNDirection = (1 - pvCosValueFlag) * viewNDirection + pvCosValueFlag * vec3(0.0, 0.0, 1.0);
	
	radiusNDirection = normalize(cross(patchNDirection, tempNDirection));

	float cosValueR2Up = dot(radiusNDirection, upDirection);

	float cosValueUpperFlag = step(cosValueRUpper2Up, cosValueR2Up);
	float cosValueLowerFlag = step(cosValueRLower2Up, cosValueR2Up);
	radiusNDirection = cosValueUpperFlag * radiusUpperDirection + (1 - cosValueLowerFlag) * radiusLowerDirection + (1 - cosValueUpperFlag) * cosValueLowerFlag * radiusNDirection;

	cosValueR2Up = abs(dot(radiusNDirection, upDirection));

	vec3 ropUpNDirection = normalize(cross(radiusNDirection, patchNDirection));

	float patchDirFlag = 2 * step(0, dot(viewNDirection, ropUpNDirection)) - 1;

	// float rotateLimit = pvCosValue;
	// vec3 lengthUpperDirection = normalize(patchDirFlag * patchNDirection * halfHeight + ropUpNDirection * thicknessR * rotateLimit);
	// float cosValueLUpper2Up = dot(lengthUpperDirection, ropUpNDirection);
	// vec3 lengthLowerDirection = normalize(patchDirFlag * patchNDirection * halfHeight - ropUpNDirection * thicknessR * rotateLimit);
	// float cosValueLLower2Up = dot(lengthLowerDirection, ropUpNDirection);

	// vec3 lengthDirection = normalize(cross(viewNDirection, radiusNDirection));
	// float cosValueL2Up = dot(lengthDirection, ropUpNDirection);

	// if (cosValueL2Up > cosValueLUpper2Up)
	// {	
	// 	lengthDirection = lengthUpperDirection;
	// 	cosValueL2Up = cosValueLUpper2Up;
	// }
	// else if (cosValueL2Up < cosValueLLower2Up)
	// {
	// 	lengthDirection = lengthLowerDirection;
	// 	cosValueL2Up = cosValueLLower2Up;
	// }
	vec3 lengthDirection = patchNDirection;
	
	// vec3 normalDirection = normalize(cross(radiusNDirection, patchNDirection));

	// planeNormalNDir_cs_out = normalize(cross(radiusNDirection, lengthDirection));
	planeNormalNDir_vs_out = ropUpNDirection;
	worldViewNDir_vs_out = viewNDirection;
	VOPNCosValue_vs_out = dot(worldViewNDir_vs_out, planeNormalNDir_vs_out);

	radiusNDirection = radiusNDirection * inIndex;
	// lengthDirection = lengthDirection * (1 - vertexIndex * 2);

	float sinValueR2Horizon = cosValueR2Up;

	float halfLength = sqrt(lineWidthR * lineWidthR + halfHeight * halfHeight) * 0.99;	

	r = lineWidthR * inversesqrt(1 - sinValueR2Horizon * sinValueR2Horizon);

	// r = sqrt(lineWidthR * lineWidthR + thicknessR * thicknessR);


	vec3 actualPos = segmentMidPos_vs_out + halfLength * lengthDirection + r * radiusNDirection;
	return actualPos;
}

void main( void )
{	
	vertexFlag_vs_out = vertexFlag;
	vertexDrawFlag_vs_out = vertexDrawFlag;
	vertexSmoothFlag_vs_out = vertexSmoothFlag;
	vertexFlag_Step = vertexFlag.y;

	vec3 vertexEndWorldPos = vec3(modelMatrix * vec4(vertexEndPosition.xyz, 1.0));
	vec3 vertexBeginWorldPos = vec3(modelMatrix * vec4(vertexBeginPosition, 1.0));

	float lineWidthR = vertexFlag_vs_out.z / 2;
	float thicknessR = vertexFlag_vs_out.w / 2;
	worldPos_vs_out = calActualPos(vertexBeginWorldPos, vertexEndWorldPos, int(vertexEndPosition.w), lineWidthR, thicknessR);
	vec4 viewPos = viewMatrix * vec4(worldPos_vs_out, 1.0);
	viewDirection  = normalize(-(viewPos.xyz));
	gl_Position = projectionMatrix * viewMatrix * vec4(worldPos_vs_out, 1.0);
}
