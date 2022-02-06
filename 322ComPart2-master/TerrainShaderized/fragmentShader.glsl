#version 420 core

#define TERRAIN 0
#define WATER 1
#define SKYBOX 2
#define CLOUDS 3

in vec3 normalExport;
in vec2 texCoordsExport;
in float yValue;
in float transp;

in vec3 SkytexCoordsExport;

out vec4 colorsExport;

uniform sampler2D grassTex;
uniform sampler2D sandTex;
uniform sampler2D stoneTex;
uniform sampler2D snowTex;
uniform sampler2D waterTex;
uniform sampler2D skyTex;

uniform samplerCube skyboxTexture;

struct Light
{
	vec4 ambCols;
	vec4 difCols;
	vec4 specCols;
	vec4 coords;
};

uniform Light light0;
uniform vec4 globAmb;

struct Material
{
	vec4 ambRefl;
	vec4 difRefl;
	vec4 specRefl;
	vec4 emitCols;
	float Shininess;
};

float maxRange=25.0f;
float upRange=10.0f;
float downRange=-5.0f;

float blendFactor=0.7f;
float blendRange=3.0f;

uniform Material terrainFandB;
uniform int objectId;

vec3 normal, lightDirection;
vec4 fAndBDif;

void main(void)
{

	vec4 grassTexColor = texture(grassTex, texCoordsExport);
	vec4 sandTexColor = texture(sandTex, texCoordsExport);
	vec4 stoneTexColor = texture(stoneTex, texCoordsExport);
	vec4 snowTexColor = texture(snowTex, texCoordsExport);
	vec4 waterTexColor = texture(waterTex, texCoordsExport);
	vec4 skyTexColor = texture(skyTex , texCoordsExport);
	vec4 texColor;

	//terrain
	if(objectId==TERRAIN)
	{
		if(yValue < downRange)
		{texColor = sandTexColor;}
		
		if(yValue > downRange - blendRange && yValue < downRange + blendRange)
		{texColor = mix(sandTexColor, grassTexColor, blendFactor);}
		
		if(yValue > downRange && yValue < upRange)
		{texColor = grassTexColor;}
		
		if(yValue > upRange - blendRange && yValue < upRange + blendRange)
		{texColor = mix( grassTexColor, stoneTexColor, blendFactor);}

		if(yValue > upRange && yValue < maxRange)
		{texColor=stoneTexColor;}

		if(yValue > maxRange - blendRange && yValue < maxRange + blendRange)
		{texColor = mix(stoneTexColor, snowTexColor, blendFactor);}

		if(yValue > maxRange)
		{texColor = snowTexColor;}

		texColor.a=10.0f;

		normal=normalize(normalExport);

		lightDirection=normalize(vec3(light0.coords));

	    fAndBDif=max(dot(normal, lightDirection), 0.0f) * (light0.difCols * terrainFandB.difRefl);

		colorsExport=vec4(vec3(min(fAndBDif, vec4(1.0))),1.0) * texColor * fAndBDif;
	}

	//water
	if(objectId==WATER)
	{
		texColor=waterTexColor;

		if(yValue>-6.9)
		{
				texColor=vec4(255,255,255,1.0);
		}

		texColor.a=0.6f;

		normal=normalize(normalExport);

		lightDirection=normalize(vec3(light0.coords));

	    fAndBDif=max(dot(normal, lightDirection), 0.0f) * (light0.difCols * terrainFandB.difRefl);

		colorsExport=vec4(vec3(min(fAndBDif, vec4(1.0))),1.0) * texColor * fAndBDif;;
	}

	//clouds
	if(objectId==CLOUDS)
	{
			texColor=vec4(255,255,255,0.0);
		
		if(transp>0.0f)
		{
			texColor.a=0.1f;
		}
		else
		{
			texColor.a=0.6f;
		}

		normal=normalize(normalExport);

		lightDirection=normalize(vec3(light0.coords));

	    fAndBDif=max(dot(normal, lightDirection), 0.0f) * (light0.difCols * terrainFandB.difRefl);

		colorsExport=vec4(vec3(min(fAndBDif, vec4(1.0))),1.0) * texColor * fAndBDif;

		if(transp>0.0f)
			colorsExport.a = 0.0f;

	}

	//skybox
	if(objectId==SKYBOX)
	{
		texColor=texture(skyboxTexture, SkytexCoordsExport);

		colorsExport= texColor;
	}
}