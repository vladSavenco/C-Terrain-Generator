#pragma once

#include <glm/glm.hpp>

using namespace glm;

struct Vertex
{
	vec4 coords;
	vec3 normals;
	vec2 texcoords;

	float transparent;
};

struct Matrix4x4
{
	float entries[16];
};

struct Material
{
	vec4 ambRefl;
	vec4 difRefl;
	vec4 specRefl;
	vec4 emitCols;
	float shininess;
};

struct Light
{
	vec4 ambCols;
	vec4 difCols;
	vec4 specCols;
	vec4 coords;
};