#include <iostream>
#include <fstream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#include <GL/glew.h>
#include <GL/freeglut.h>
//#include <GL/glext.h>
#pragma comment(lib, "glew32.lib") 

#include "getbmp.h"
#include "FastNoiseLite.h"
#include <time.h>

//structs
#include "Structs.h"

//namespaces
using namespace std;
using namespace glm;

// Size of the terrain
//the size needs to be 2^n+1
const int MAP_SIZE = 513;

const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;

static const vec4 globAmb = vec4(0.2, 0.2, 0.2, 1.0);
static mat3 normalMat = mat3(1.0);

//what this?
static mat4 projMat = mat4(1.0);

glm::vec3 CamPos(0.0, 0.0, -5.0);

//A bunch of other things
//Identity matrix
static const Matrix4x4 IDENTITY_MATRIX4x4 =
{
	{
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 1.0
	}
};
// Front and back material properties.
static const Material terrainFandB =
{
	vec4(1.0, 1.0, 1.0, 1.0),
	vec4(1.0, 1.0, 1.0, 1.0),
	vec4(1.0, 1.0, 1.0, 1.0),
	vec4(0.0, 0.0, 0.0, 1.0),
	50.0f
};
//Lights
static const Light light0 =
{
	vec4(0.0, 0.0, 0.0, 1.0),
	vec4(1.0, 1.0, 1.0, 1.0),
	vec4(1.0, 1.0, 1.0, 1.0),
	vec4(1.0, 1.0, 0.0, 0.0)
};

static enum buffer { TERRAIN_VERTICES, WATER_VERTICES, SKYBOX_VERTICES, CLOUDS_VERTICES };
static enum object { TERRAIN, WATER, SKYBOX, CLOUDS };

// Globals
static Vertex terrainVertices[MAP_SIZE * MAP_SIZE] = {};
static Vertex waterVertices[MAP_SIZE * MAP_SIZE] = {};
static Vertex cloudVertices[MAP_SIZE * MAP_SIZE] = {};

//clouds
//array for basic noise map
float map32[32 * 32];
//cloud map to hold cloud
float map256[256 * 256];

const int numStripsRequired = MAP_SIZE - 1;
const int verticesPerStrip = 2 * MAP_SIZE;

unsigned int terrainIndexData[numStripsRequired][verticesPerStrip];
unsigned int waterIndexData[numStripsRequired][verticesPerStrip];
unsigned int cloudIndexData[numStripsRequired][verticesPerStrip];

//skybox datsa
unsigned int skyboxVAO, skyboxVBO;

static unsigned int
programId,
vertexShaderId,
fragmentShaderId,
modelViewMatLoc,
waveFlowLoc,
objectLoc,
projMatLoc,
buffer[4],
vao[4];

static unsigned int skyvao, skyvbo, skybuffer;

//water settings
float waveFlow = 0;

//texture mapping
//storage for the bmp data
static BitMapFile* image[6];

static BitMapFile* skyBox[6];

//create an array of texture ids
static unsigned int texture[7], grassTexLoc, sandTexLoc, stoneTexLoc, snowTexLoc, waterTexLoc, skyTexLoc, cubeMapTexLoc;

float height;

void SetSize()
{
	cout << "Please set the desired height." << endl;
	cout << "Please note that the higher this number is the taller the terrain will be, also keep in mind that this generator has been optimised for 60 and under and those values will yield the best results" << endl;

	cin >> height;
}

// Function to read text file, used to read shader files
char* readTextFile(char* aTextFile)
{
	FILE* filePointer = fopen(aTextFile, "rb");
	char* content = NULL;
	long numVal = 0;

	fseek(filePointer, 0L, SEEK_END);
	numVal = ftell(filePointer);
	fseek(filePointer, 0L, SEEK_SET);
	content = (char*)malloc((numVal + 1) * sizeof(char));
	fread(content, 1, numVal, filePointer);
	content[numVal] = '\0';
	fclose(filePointer);
	return content;
}

void shaderCompileTest(GLuint shader)
{
	GLint result = GL_FALSE;
	int logLength;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
	std::vector<GLchar> vertShaderError((logLength > 1) ? logLength : 1);
	glGetShaderInfoLog(shader, logLength, NULL, &vertShaderError[0]);
	std::cout << &vertShaderError[0] << std::endl;
}

//declaring the terrain array
float terrain[MAP_SIZE][MAP_SIZE] = {};

//declaring the water array
float water[MAP_SIZE][MAP_SIZE] = {};

//declaring the cloud array
float cloud[MAP_SIZE][MAP_SIZE] = {};


//Fast noise
FastNoiseLite noise;

//FUNCTIONS FOR THE RANDOM NOISE GENERATION
//generate random noise
//float noise(int x, int y, int random)
//{
//	int n = x + y * 57 + random * 131;
//	n = (n << 13) ^ n;
//	return(1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) * 0.000000000931322574615478515625f);
//}
//
////set noise to map
//void setNoise(float* map)
//{
//	//temporary array
//	float temp[34][34];
//
//	int random = rand() % 5000;
//
//	//inserting values in temporary array
//	for (int y = 1; y < 33; y++)
//	{
//		for (int x = 1; x < 33; x++)
//		{
//			temp[x][y] = 128.0f + noise(x, y, random) * 128.0f;
//		}
//	}
//
//	//make the cloud seamless
//	for (int x = 1; x < 33; x++)
//	{
//		temp[0][x] = temp[32][x];
//		temp[33][x] = temp[1][x];
//		temp[x][0] = temp[x][32];
//		temp[x][33] = temp[x][1];
//	}
//	temp[0][0] = temp[32][32];
//	temp[33][33] = temp[1][1];
//	temp[0][33] = temp[32][1];
//	temp[33][0] = temp[1][32];
//
//	//making the noise smooth
//	for (int y = 1; y < 33; y++)
//	{
//		for (int x = 1; x < 33; x++)
//		{
//			float center = temp[x][y] / 4.0f;
//			float sides = (temp[x + 1][y] + temp[x - 1][y] + temp[x][y + 1] + temp[x][y - 1]) / 8.0f;
//			float corners = (temp[x + 1][y + 1] + temp[x + 1][y - 1] + temp[x - 1][y + 1] + temp[x - 1][y - 1]) / 16.0f;
//
//			map32[((x - 1) * 32) + (y - 1)] = center + sides + corners;
//		}
//	}
//}
//
////make noise less blocky
//float interpolate(float x, float y, float* map)
//{
//	int Xint = (int)x;
//	int Yint = (int)y;
//
//	float Xfrac = x - Xint;
//	float Yfrac = y - Yint;
//
//	int X0 = Xint % 32;
//	int Y0 = Yint % 32;
//	int X1 = (Xint + 1) % 32;
//	int Y1 = (Yint + 1) % 32;
//
//	float bot = map[X0 * 32 + Y0] + Xfrac * (map[X1 * 32 + Y0] - map[X0 * 32 + Y0]);
//	float top = map[X0 * 32 + Y1] + Xfrac * (map[X1 * 32 + Y1] - map[X0 * 32 + Y1]);
//
//	return (bot + Yfrac * (top - bot));
//}
//
////overlap octaves
//void overlapOctaves(float* map32, float* map256)
//{
//	for (int x = 0; x < 256 * 256; x++)
//	{
//		map256[x] = 0;
//	}
//
//	for (int octave = 0; octave < 4; octave++)
//	{
//		for (int x = 0; x < 256; x++)
//		{
//			for (int y = 0; y < 256; y++)
//			{
//				float scale = 1 / pow(2, 3 - octave);
//				float noise = interpolate(x * scale, y * scale, map32);
//
//				map256[(y * 256) + x] += noise / pow(2, octave);
//			}
//		}
//	}
//}
//
////filtering noise with exponential function
//void expFilter(float* map)
//{
//	float cover = 20.0f;
//	float sharpness = 0.95f;
//
//	for (int x = 0; x < 256 * 256; x++)
//	{
//		float c = map[x] - (255.0f - cover);
//		if (c < 0)
//		{
//			c = 0;
//		}
//
//		map[x] = 255.0f - ((float)(pow(sharpness, c)) * 255.0f);
//	}
//}
//
////initialise map
//void init()
//{
//	setNoise(map32);
//}
//
////loop function
//void loopForever()
//{
//	overlapOctaves(map32, map256);
//	expFilter(map256);
//}


//FUNCTIONS TO CREATE THE VERTICES

//add data to vertex array
void addDataToVertexArray(static Vertex Vertices[MAP_SIZE * MAP_SIZE], float arr[MAP_SIZE][MAP_SIZE])
{
	// Intialise vertex array
	int i = 0;

	for (int z = 0; z < MAP_SIZE; z++)
	{
		for (int x = 0; x < MAP_SIZE; x++)
		{
			Vertices[i].coords.x = (float)x;
			Vertices[i].coords.y = arr[x][z];
			Vertices[i].coords.z = (float)z;
			Vertices[i].coords.w = 1.0;

			Vertices[i].normals.x = 0.0;
			Vertices[i].normals.y = 0.0;
			Vertices[i].normals.z = 0.0;
			i++;
		}
	}
}

//build the index data
void buildIndexData(unsigned int IndexData[numStripsRequired][verticesPerStrip])
{
	// Now build the index data 
	int i = 0;
	for (int z = 0; z < MAP_SIZE - 1; z++)
	{
		i = z * MAP_SIZE;
		for (int x = 0; x < MAP_SIZE * 2; x += 2)
		{
			IndexData[z][x] = i;
			i++;
		}
		for (int x = 1; x < MAP_SIZE * 2 + 1; x += 2)
		{
			IndexData[z][x] = i;
			i++;
		}
	}
}

//compute normal vectors for each vertices
void computeNormalVec(static Vertex Vertices[MAP_SIZE * MAP_SIZE], unsigned int IndexData[numStripsRequired][verticesPerStrip])
{
	int index1, index2, index3;
	float dot_value;
	vec3 Pt1, Pt2, Pt3, ttVec, edgeVec1, edgeVec2, norVec, upvec;
	upvec.x = 0.0; upvec.y = 1.0; upvec.z = 0.0;
	for (int z = 0; z < MAP_SIZE - 1; z++)
	{
		for (int x = 0; x < (MAP_SIZE * 2 - 2); x++)
		{
			index1 = IndexData[z][x];
			index2 = IndexData[z][x + 1];
			index3 = IndexData[z][x + 2];

			Pt1.x = Vertices[index1].coords.x;
			Pt1.y = Vertices[index1].coords.y;
			Pt1.z = Vertices[index1].coords.z;

			Pt2.x = Vertices[index2].coords.x;
			Pt2.y = Vertices[index2].coords.y;
			Pt2.z = Vertices[index2].coords.z;

			Pt3.x = Vertices[index3].coords.x;
			Pt3.y = Vertices[index3].coords.y;
			Pt3.z = Vertices[index3].coords.z;

			edgeVec1 = Pt2 - Pt1;
			edgeVec2 = Pt3 - Pt1;
			if (x % 2 == 1)
			{
				ttVec = cross(edgeVec2, edgeVec1);
			}
			else
			{
				ttVec = cross(edgeVec1, edgeVec2);
			}
			//norVec = normalize(ttVec);
			dot_value = dot(ttVec, upvec);
			if (dot_value < 0.0000001)
			{
				norVec = -ttVec;
			}
			else
			{
				norVec = ttVec;
			}

			Vertices[index1].normals = norVec + Vertices[index1].normals;
			Vertices[index2].normals = norVec + Vertices[index2].normals;
			Vertices[index3].normals = norVec + Vertices[index3].normals;

			//terrainVertices[index1].normals = norVec;
			//terrainVertices[index2].normals = norVec;
			//terrainVertices[index3].normals = norVec;
		}
	}

	///smooth the normal vectors 
	//vector<vec3> ttVecArr;
	int total;
	total = MAP_SIZE * MAP_SIZE;
	for (int i = 0; i < (total - 1); i++)
	{
		//for terrain
		ttVec = Vertices[i].normals;
		norVec = normalize(ttVec);
		Vertices[i].normals = norVec;
	}
}

//generate texture coordonates
void computeTextureCoords(static Vertex Vertices[MAP_SIZE * MAP_SIZE])
{
	//calculating the scale
	float fTextureS = float(MAP_SIZE) * 0.1f;
	float fTextureT = float(MAP_SIZE) * 0.1f;

	int i = 0;

	//we loop through all the vertices in the array
	for (int y = 0; y < MAP_SIZE; y++)
	{
		for (int x = 0; x < MAP_SIZE; x++)
		{
			//we normalize the x,y coordonates
			float fScaleC = float(x) / float(MAP_SIZE - 1);
			float fScaleR = float(y) / float(MAP_SIZE - 1);

			//we multiply the tex coords by scale and then assign them to the terrain texcoords
			Vertices[i].texcoords = vec2(fTextureS * fScaleC, fTextureT * fScaleR);

			i++;
		}
	}
}




//FUNCTIONS TO CREATE THE OBJ

//generating the data
void terrainGen()
{
	float h1, h2, h3, h4, aver, h;
	///Generate random numbers ///
	// get different srand values //seed
	int seed;
	cout << "please input terrain seed" << endl;
	cin >> seed;

	srand(seed);
	h1 = (rand() % 10) / 5.0 - 1.0;
	h2 = (rand() % 10) / 5.0 - 1.0;
	h3 = (rand() % 10) / 5.0 - 1.0;
	h4 = (rand() % 10) / 5.0 - 1.0;

	// Initialise terrain - set values in the height map to 0

	for (int x = 0; x < MAP_SIZE; x++)
	{
		for (int z = 0; z < MAP_SIZE; z++)
		{
			terrain[x][z] = 0;
		}
	}

	// Diamond Square Algorithm
	terrain[0][0] = h1 * height;
	terrain[MAP_SIZE - 1][0] = h2 * height;
	terrain[MAP_SIZE - 1][MAP_SIZE - 1] = h3 * height;
	terrain[0][MAP_SIZE - 1] = h4 * height;
	int step_size, tt, H, count;
	float rand_max;
	tt = MAP_SIZE;
	step_size = tt - 1;
	H = 1;
	rand_max = 1.0;

	while (step_size > 1)
	{
		for (int x = 0; x < MAP_SIZE - 1; x += step_size)
			for (int y = 0; y < MAP_SIZE - 1; y += step_size)
			{
				//diamond_step(x, y, stepsize)
				h1 = terrain[x][y];
				h2 = terrain[x + step_size][y];
				h3 = terrain[x][y + step_size];
				h4 = terrain[x + step_size][y + step_size];
				aver = (h1 + h2 + h3 + h4) / 4.0;
				h = (rand() % 10) / 5.0 - 1.0;
				aver = aver + h * height * rand_max;
				terrain[x + step_size / 2][y + step_size / 2] = aver;
			}

		for (int x = 0; x < MAP_SIZE - 1; x += step_size)
			for (int y = 0; y < MAP_SIZE - 1; y += step_size)
			{
				//square_step(x, y)
				count = 0;
				h1 = terrain[x][y];  count++;
				h2 = terrain[x][y + step_size];  count++; //below
				if ((x - step_size / 2) >= 0) { h3 = terrain[x - step_size / 2][y + step_size / 2]; count++; }
				else { h3 = 0.0; }
				if ((x + step_size / 2) < MAP_SIZE) { h4 = terrain[x + step_size / 2][y + step_size / 2]; count++; }
				else { h4 = 0.0; }
				aver = (h1 + h2 + h3 + h4) / (float)count;
				h = (rand() % 10) / 5.0 - 1.0;
				aver = aver + h * height * rand_max;
				terrain[x][y + step_size / 2] = aver;

				//second one
				count = 0;
				h1 = terrain[x][y];  count++;
				h2 = terrain[x + step_size][y];  count++; //below
				if ((y - step_size / 2) >= 0) { h3 = terrain[x + step_size / 2][y - step_size / 2]; count++; }
				else { h3 = 0.0; }
				if ((y + step_size / 2) < MAP_SIZE) { h4 = terrain[x + step_size / 2][y + step_size / 2]; count++; }
				else { h4 = 0.0; }
				aver = (h1 + h2 + h3 + h4) / (float)count;
				h = (rand() % 10) / 5.0 - 1.0;
				aver = aver + h * height * rand_max;
				terrain[x + step_size / 2][y] = aver;

				//third one
				count = 0;
				h1 = terrain[x + step_size][y];  count++;
				h2 = terrain[x + step_size][y + step_size];  count++; //below
				h3 = terrain[x + step_size / 2][y + step_size / 2]; count++;
				if ((x + 3 * step_size / 2) < MAP_SIZE) { h4 = terrain[x + 3 * step_size / 2][y + step_size / 2]; count++; }
				else { h4 = 0.0; }
				aver = (h1 + h2 + h3 + h4) / (float)count;
				h = (rand() % 10) / 5.0 - 1.0;
				aver = aver + h * height * rand_max;
				terrain[x + step_size][y + step_size / 2] = aver;

				//fourth one
				count = 0;
				h1 = terrain[x][y + step_size];  count++;
				h2 = terrain[x + step_size][y + step_size];  count++; //below
				h3 = terrain[x + step_size / 2][y + step_size / 2]; count++;
				if ((y + 3 * step_size / 2) < MAP_SIZE) { h4 = terrain[x + step_size / 2][y + 3 * step_size / 2]; count++; }
				else { h4 = 0.0; }
				aver = (h1 + h2 + h3 + h4) / (float)count;
				h = (rand() % 10) / 5.0 - 1.0;
				aver = aver + h * height * rand_max;
				terrain[x + step_size / 2][y + step_size] = aver;
			}

		rand_max = rand_max * pow(2, -H);
		step_size = step_size / 2;
	}

	addDataToVertexArray(terrainVertices,terrain);
	buildIndexData(terrainIndexData);

	computeNormalVec(terrainVertices, terrainIndexData);

	computeTextureCoords(terrainVertices);
}

//generating the water data
void waterGen()
{
	for (int x = 0; x < MAP_SIZE; x++)
	{
		for (int y = 0; y < MAP_SIZE; y++)
		{
			water[x][y] = 0;
		}
	}

	addDataToVertexArray(waterVertices,water);
	buildIndexData(waterIndexData);

	computeNormalVec(waterVertices, waterIndexData);

	computeTextureCoords(waterVertices);
}

void cloudGen()
{
	for (int x = 0; x < MAP_SIZE; x++)
	{
		for (int y = 0; y < MAP_SIZE; y++)
		{
			cloud[x][y] = 0;
		}
	}

	addDataToVertexArray(cloudVertices, cloud);
	buildIndexData(cloudIndexData);

	computeNormalVec(cloudVertices, cloudIndexData);

	computeTextureCoords(cloudVertices);

	//add noise data

	srand(time(NULL));

	float skySeed = (rand() % 10) / 5.0 - 1.0;

	noise.SetSeed(skySeed);

	//noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);

	int i = 0;

	for (int z = 0; z < MAP_SIZE; z++)
	{
		for (int x = 0; x < MAP_SIZE; x++)
		{
			float nos = noise.GetNoise(cloudVertices[i].coords.x, cloudVertices[i].coords.y, cloudVertices[i].coords.z);

			cloudVertices[i].transparent = nos;

			i++;
		}
	}
}

//generating the skybox
void InitialiseSkybox(unsigned int VAO,unsigned int VBO)
{
	float skyboxVertices[] =
    {
        -1500.0f,  1500.0f, -1500.0f,1.0f,
        -1500.0f, -1500.0f, -1500.0f,1.0f,
         1500.0f, -1500.0f, -1500.0f,1.0f,
         1500.0f, -1500.0f, -1500.0f,1.0f,
         1500.0f,  1500.0f, -1500.0f,1.0f,
        -1500.0f,  1500.0f, -1500.0f,1.0f,
		 	  		  	 
        -1500.0f, -1500.0f,  1500.0f,1.0f,
        -1500.0f, -1500.0f, -1500.0f,1.0f,
        -1500.0f,  1500.0f, -1500.0f,1.0f,
        -1500.0f,  1500.0f, -1500.0f,1.0f,
        -1500.0f,  1500.0f,  1500.0f,1.0f,
        -1500.0f, -1500.0f,  1500.0f,1.0f,
		  	   	   		 
         1500.0f, -1500.0f, -1500.0f,1.0f,
         1500.0f, -1500.0f,  1500.0f,1.0f,
         1500.0f,  1500.0f,  1500.0f,1.0f,
         1500.0f,  1500.0f,  1500.0f,1.0f,
         1500.0f,  1500.0f, -1500.0f,1.0f,
         1500.0f, -1500.0f, -1500.0f,1.0f,
		 	  		 	 
        -1500.0f, -1500.0f,  1500.0f,1.0f,
        -1500.0f,  1500.0f,  1500.0f,1.0f,
         1500.0f,  1500.0f,  1500.0f,1.0f,
         1500.0f,  1500.0f,  1500.0f,1.0f,
         1500.0f, -1500.0f,  1500.0f,1.0f,
        -1500.0f, -1500.0f,  1500.0f,1.0f,
		 	  	   		 
        -1500.0f,  1500.0f, -1500.0f,1.0f,
         1500.0f,  1500.0f, -1500.0f,1.0f,
         1500.0f,  1500.0f,  1500.0f,1.0f,
         1500.0f,  1500.0f,  1500.0f,1.0f,
        -1500.0f,  1500.0f,  1500.0f,1.0f,
        -1500.0f,  1500.0f, -1500.0f,1.0f,
		 	  		  	 
        -1500.0f, -1500.0f, -1500.0f,1.0f,
        -1500.0f, -1500.0f,  1500.0f,1.0f,
         1500.0f, -1500.0f, -1500.0f,1.0f,
         1500.0f, -1500.0f, -1500.0f,1.0f,
        -1500.0f, -1500.0f,  1500.0f,1.0f,
         1500.0f, -1500.0f,  1500.0f,1.0f

    };

    skyboxVAO = VAO;
    skyboxVBO = VBO;

	glGenVertexArrays(1, &skyvao);
	glGenBuffers(1, &skyvbo);

    glBindVertexArray(skyvao);
    glBindBuffer(GL_ARRAY_BUFFER, skyvbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

}

void InitialiseCubeMap(unsigned int textureID)
{
	skyBox[0] = getbmp("textures/Skybox/right.bmp");
	skyBox[1] = getbmp("textures/Skybox/left.bmp");
	skyBox[2] = getbmp("textures/Skybox/bottom.bmp");
	skyBox[3] = getbmp("textures/Skybox/top.bmp");
	skyBox[4] = getbmp("textures/Skybox/back.bmp");
	skyBox[5] = getbmp("textures/Skybox/front.bmp");


	glGenTextures(1, &texture[textureID]);

	glActiveTexture(GL_TEXTURE0 + textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture[textureID]);

	for (unsigned int i = 0; i < 6; i++)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, skyBox[i]->sizeX, skyBox[i]->sizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, skyBox[i]->data);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glGenerateMipmap(GL_TEXTURE_2D);

	unsigned int skyTexLoc = glGetUniformLocation(programId, "skyboxTexture");
	glUniform1i(skyTexLoc, textureID);
}

void drawCubeMap()
{
	glDepthFunc(GL_LEQUAL);

	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);
}

//generating the textures from the bmp files
void createTexture()
{
	//create a texture for grass
	
	//Load the texture
	image[0] = getbmp("./textures/grass.bmp");
	image[1] = getbmp("./textures/sand.bmp");
	image[2] = getbmp("./textures/stone.bmp");
	image[3] = getbmp("./textures/snow.bmp");
	image[4] = getbmp("./textures/water.bmp");
	image[5] = getbmp("./textures/sky.bmp");

	//create texture id
	glGenTextures(6, texture);

	//create a texture for grass
	//bind texture immage
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (image[0])
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image[0]->sizeX, image[0]->sizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, image[0]->data);
	}
	else
		cout << "faild to generate texture" << endl;
	grassTexLoc = glGetUniformLocation(programId, "grassTex");
	glUniform1i(grassTexLoc, 0);

	//create a texture for sand
	//bind texture immage
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (image[1])
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image[1]->sizeX, image[1]->sizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, image[1]->data);
	}
	else
		cout << "faild to generate texture" << endl;
	sandTexLoc = glGetUniformLocation(programId, "sandTex");
	glUniform1i(sandTexLoc, 1);

	//create a texture for stone
	//bind texture immage
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, texture[2]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (image[2])
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image[2]->sizeX, image[2]->sizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, image[2]->data);
	}
	else
		cout << "faild to generate texture" << endl;
	stoneTexLoc = glGetUniformLocation(programId, "stoneTex");
	glUniform1i(stoneTexLoc, 2);

	//create a texture for snow
	//bind texture immage
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, texture[3]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (image[3])
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image[3]->sizeX, image[3]->sizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, image[3]->data);
	}
	else
		cout << "faild to generate texture" << endl;
	snowTexLoc = glGetUniformLocation(programId, "snowTex");
	glUniform1i(snowTexLoc, 3);

	//create a texture for water
	//bind texture immage
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, texture[4]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (image[4])
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image[4]->sizeX, image[4]->sizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, image[4]->data);
	}
	else
		cout << "faild to generate texture" << endl;
	waterTexLoc = glGetUniformLocation(programId, "waterTex");
	glUniform1i(waterTexLoc, 4);

	//create a texture for water
	//bind texture immage
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, texture[5]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (image[5])
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image[5]->sizeX, image[5]->sizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, image[5]->data);
	}
	else
		cout << "faild to generate texture" << endl;
	skyTexLoc = glGetUniformLocation(programId, "skyTex");
	glUniform1i(skyTexLoc, 5);

	//create a texture for clouds
	//loopForever();
	//char texture[256][256][3];
	//
	//for (int i = 0; i < 256; i++)
	//{
	//	for (int j = 0; j < 256; j++)
	//	{
	//		float color = map256[i * 256 + j];
	//		texture[i][j][0] = color;
	//		texture[i][j][1] = color;
	//		texture[i][j][2] = color;
	//	}
	//}
	//
	////texture
	//unsigned int ID;
	//glGenTextures(1, &ID);
	//glBindTexture(GL_TEXTURE_2D, ID);
	//
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//
	//gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, 256, 256, GL_RGB, GL_UNSIGNED_BYTE, texture);
	//
	//skyTexLoc = glGetUniformLocation(programId, "skyTex");
	//glUniform1i(skyTexLoc, 6);

}

//vertex shader
void createVertexShader()
{
	// Create shader program executable - read, compile and link shaders
	char* vertexShader = readTextFile("vertexShader.glsl");
	vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShaderId, 1, (const char**)&vertexShader, NULL);
	glCompileShader(vertexShaderId);
	// Test for vertex shader compilation errors
	//std::cout << "VERTEX::" << std::endl;
	shaderCompileTest(vertexShaderId);
}

//fragment shader
void createFragmentShader()
{
	char* fragmentShader = readTextFile("fragmentShader.glsl");
	fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShaderId, 1, (const char**)&fragmentShader, NULL);
	glCompileShader(fragmentShaderId);
	// Test for vertex shader compilation errors
	//std::cout << "FRAGMENT::" << std::endl;
	shaderCompileTest(fragmentShaderId);
}

// Obtain modelview matrix uniform location and set value.
mat4 modelViewMat = mat4(1.0);

// Initialization routine.
void setup(void)
{
	//Enable blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	SetSize();

	//generating terrain data
	terrainGen();
	//genertaing water data
	waterGen();
	//generating cloud data
	cloudGen();

	glClearColor(0.0, 0.0, 0.0, 1.0);

	//Vertex & Fragment shaders
	createVertexShader();
	createFragmentShader();
	
	//program setup
	programId = glCreateProgram();
	glAttachShader(programId, vertexShaderId);
	glAttachShader(programId, fragmentShaderId);
	glLinkProgram(programId);
	glUseProgram(programId);

	////Add for workshop 7
	glUniform4fv(glGetUniformLocation(programId, "terrainFandB.ambRefl"), 1,
		&terrainFandB.ambRefl[0]);
	glUniform4fv(glGetUniformLocation(programId, "terrainFandB.difRefl"), 1,
		&terrainFandB.difRefl[0]);
	glUniform4fv(glGetUniformLocation(programId, "terrainFandB.specRefl"), 1,
		&terrainFandB.specRefl[0]);
	glUniform4fv(glGetUniformLocation(programId, "terrainFandB.emitCols"), 1,
		&terrainFandB.emitCols[0]);
	glUniform1f(glGetUniformLocation(programId, "terrainFandB.shininess"),
		terrainFandB.shininess);

	glUniform4fv(glGetUniformLocation(programId, "globAmb"), 1, &globAmb[0]);

	glUniform4fv(glGetUniformLocation(programId, "light0.ambCols"), 1,
		&light0.ambCols[0]);
	glUniform4fv(glGetUniformLocation(programId, "light0.difCols"), 1,
		&light0.difCols[0]);
	glUniform4fv(glGetUniformLocation(programId, "light0.specCols"), 1,
		&light0.specCols[0]);
	glUniform4fv(glGetUniformLocation(programId, "light0.coords"), 1,
		&light0.coords[0]);

	createTexture();

	// Create vertex array object (VAO) and vertex buffer object (VBO) and associate data with vertex shader.
	glGenVertexArrays(4, vao);
	glGenBuffers(4, buffer);

	//terrain
	glBindVertexArray(vao[TERRAIN]);
	glBindBuffer(GL_ARRAY_BUFFER, buffer[TERRAIN_VERTICES]);

	glBufferData(GL_ARRAY_BUFFER, sizeof(terrainVertices), terrainVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(terrainVertices[0]), 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(terrainVertices[0]), (GLvoid*)sizeof(terrainVertices[0].normals));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(terrainVertices[0]), (GLvoid*)(sizeof(terrainVertices[0].coords) + sizeof(terrainVertices[0].normals)));
	glEnableVertexAttribArray(2);

	//water
	glBindVertexArray(vao[WATER]);
	glBindBuffer(GL_ARRAY_BUFFER, buffer[WATER_VERTICES]);
	
	glBufferData(GL_ARRAY_BUFFER, sizeof(waterVertices), waterVertices, GL_STATIC_DRAW);
	
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(waterVertices[0]), 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(waterVertices[0]), (GLvoid*)sizeof(waterVertices[0].normals));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(waterVertices[0]), (GLvoid*)(sizeof(waterVertices[0].coords) + sizeof(waterVertices[0].normals)));
	glEnableVertexAttribArray(2);

	//skybox
	InitialiseSkybox(vao[SKYBOX], buffer[SKYBOX_VERTICES]);
	InitialiseCubeMap(6);

	//cloud
	glBindVertexArray(vao[CLOUDS]);
	glBindBuffer(GL_ARRAY_BUFFER, buffer[CLOUDS_VERTICES]);

	glBufferData(GL_ARRAY_BUFFER, sizeof(cloudVertices), cloudVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(cloudVertices[0]), 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(cloudVertices[0]), (GLvoid*)sizeof(cloudVertices[0].coords));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(cloudVertices[0]), (GLvoid*)(sizeof(cloudVertices[0].coords) + sizeof(cloudVertices[0].normals)));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(cloudVertices[0]), (GLvoid*)(sizeof(cloudVertices[0].coords) + sizeof(cloudVertices[0].normals) + sizeof(cloudVertices[0].texcoords)));
	glEnableVertexAttribArray(4);

	objectLoc = glGetUniformLocation(programId, "objectId");

	// Obtain projection matrix uniform location and set value.
	projMatLoc = glGetUniformLocation(programId, "projMat");
	projMat = perspective(radians(90.0), (double)SCREEN_WIDTH / (double)SCREEN_HEIGHT, 0.1, 10000.0);
	glUniformMatrix4fv(projMatLoc, 1, GL_FALSE, value_ptr(projMat));

	// Move terrain into view - glm::translate replaces glTranslatef
	modelViewMatLoc = glGetUniformLocation(programId, "modelViewMat");
	glUniformMatrix4fv(modelViewMatLoc, 1, GL_FALSE, value_ptr(modelViewMat));


	normalMat = transpose(inverse(mat3(modelViewMat)));
	glUniformMatrix3fv(glGetUniformLocation(programId, "normalMat"), 1, GL_FALSE, value_ptr(normalMat));
}

// Drawing routine.
void drawScene()
{
	//glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//skybox
	glBindVertexArray(skyvao);
	glUniform1i(objectLoc, SKYBOX);
	drawCubeMap();

	//draw terrain
	glBindVertexArray(vao[TERRAIN]);
	glUniform1i(objectLoc, TERRAIN);
	
	for (int i = 0; i < MAP_SIZE - 1; i++)
	{
		glDrawElements(GL_TRIANGLE_STRIP, verticesPerStrip, GL_UNSIGNED_INT, terrainIndexData[i]);
	}

	//draw water
	glBindVertexArray(vao[WATER]);
	glUniform1i(objectLoc, WATER);

	//water settings
	waveFlow += 0.004f;
	waveFlowLoc = glGetUniformLocation(programId, "waveFlow");
	glUniform1f(waveFlowLoc, waveFlow);

	// For each row - draw the triangle strip
	for (int i = 0; i < MAP_SIZE - 1; i++)
	{
		glDrawElements(GL_TRIANGLE_STRIP, verticesPerStrip, GL_UNSIGNED_INT, waterIndexData[i]);
	}

	//clouds
	glBindVertexArray(vao[CLOUDS]);
	glUniform1i(objectLoc, CLOUDS);
	
	// For each row - draw the triangle strip
	for (int i = 0; i < MAP_SIZE - 1; i++)
	{
		glDrawElements(GL_TRIANGLE_STRIP, verticesPerStrip, GL_UNSIGNED_INT, cloudIndexData[i]);
	}

	glUniformMatrix4fv(modelViewMatLoc, 1, GL_FALSE, value_ptr(modelViewMat));

	glFlush();
}

// OpenGL window reshape routine.
void resize(int w, int h)
{
	glViewport(0, 0, w, h);
}

// Keyboard input processing routine.
void KeyInputCallback(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 'w':
	{
		modelViewMat = translate(modelViewMat, vec3(0, 0, 5.0));
	}break;
	case 's':
	{
		modelViewMat = translate(modelViewMat, vec3(0, 0, -5.0));
	}break;
	case 'a':
	{
		modelViewMat = translate(modelViewMat, vec3(5.0, 0, 0));
	}break;
	case 'd':
	{
		modelViewMat = translate(modelViewMat, vec3(-5.0, 0, 0));
	}break;
	case 'e':
	{
		modelViewMat = translate(modelViewMat, vec3(0, 5.0, 0));
	}break;
	case 'q':
	{
		modelViewMat = translate(modelViewMat, vec3(0, -5.0, 0));
	}break;
	case 27:
	{
		exit(0);
	}break;
	}

}

void idle()
{
	glutPostRedisplay();
}

// Main routine
int main(int argc, char* argv[])
{
	glutInit(&argc, argv);

	// Set the version of OpenGL (4.2)
	glutInitContextVersion(4, 2);
	// The core profile excludes all discarded features
	glutInitContextProfile(GLUT_CORE_PROFILE);
	// Forward compatibility excludes features marked for deprecation ensuring compatability with future versions
	glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);

	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
	glutInitWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
	glutInitWindowPosition(1000, 100);
	glutCreateWindow("TerrainGeneration");

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_SMOOTH);

	// Set OpenGL to render in wireframe mode
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glutDisplayFunc(drawScene);

	glutReshapeFunc(resize);

	glutKeyboardFunc(KeyInputCallback);

	//glewExperimental = GL_TRUE;

	glewInit();

	setup();

	modelViewMat = translate(modelViewMat, vec3(-30.0f, -20.0f, -150.0f)); // 5x5 grid

	glutIdleFunc(idle);

	glutMainLoop();
}