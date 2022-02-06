#version 420 core

#define TERRAIN 0
#define WATER 1
#define SKYBOX 2
#define CLOUDS 3

layout(location=0) in vec4 terrainCoords;
layout(location=1) in vec3 terrainNormals;
layout(location=2) in vec2 terrainTexCoords;

layout(location=3) in vec3 SkyCoords;

layout(location=4) in float transparent;

struct Material
{
	vec4 ambRefl;
	vec4 difRefl;
	vec4 specRefl;
	vec4 emitCols;
	float shininess;
};

uniform vec4 globAmb;
uniform mat4 projMat;
uniform mat4 modelViewMat;
uniform mat3 normalMat;

uniform float waveFlow;
uniform int objectId;
vec4 finalCoords;

out vec3 normalExport;
out vec2 texCoordsExport;
out float yValue;

out vec3 SkytexCoordsExport;

out float transp;

void main(void)
{
	//water
	if(objectId==WATER)
	{
		finalCoords=terrainCoords;

		finalCoords.y = -8;

		finalCoords.y += 0.4f * (sin(finalCoords.x + waveFlow) + cos(finalCoords.z + waveFlow))+0.5f;

		normalExport=terrainNormals;

		yValue=finalCoords.y;

	}

	//terrain
	if(objectId==TERRAIN)
	{
		finalCoords=terrainCoords;

		normalExport=terrainNormals;

	    yValue=finalCoords.y;
	}

	//clouds
	if(objectId==CLOUDS)
	{
		finalCoords=terrainCoords;

		normalExport=terrainNormals;

		finalCoords.y=300;

		if(transparent<0)
		{
			finalCoords.y=finalCoords.y-1.0f;

			finalCoords.y+=transparent*5;

			finalCoords.y -= 0.4f * (sin(finalCoords.x + waveFlow) + cos(finalCoords.z + waveFlow))+0.5f;
		}

	    yValue=finalCoords.y;
		transp=transparent;
	}

	//skybox
	if (objectId == SKYBOX)
    {
        SkytexCoordsExport = vec3(-terrainCoords.x, -terrainCoords.y, -terrainCoords.z);
		finalCoords=terrainCoords;

    }

   
   normalExport=normalize(normalMat * normalExport);

   texCoordsExport=terrainTexCoords;

   gl_Position=projMat * modelViewMat * finalCoords;
}