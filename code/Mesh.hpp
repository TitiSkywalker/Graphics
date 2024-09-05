#pragma once
#include <vector>
#include <iostream>

#include "Object3D.hpp"
#include "Triangle.hpp"
#include "Vector2f.h"
#include "Vector3f.h"
#include "BVH.hpp"

using namespace std;

//basic triangle element
//by default counterclockwise winding is front face
struct Trig 
{
	Trig() 
	{
		x[0] = 0; x[1] = 0; x[2] = 0; 
		texORnormID[0] = 0; texORnormID[1] = 0; texORnormID[2] = 0;
	}
	int& operator[](int i) { return x[i]; }
	int operator[](int i) const { return x[i]; }

	//index of a 3D vertex stored in "v"
	int x[3];

	//index for "texCoord" or "n"
	int texORnormID[3];

	//how to calculate normal of a triangle:
	//	if SMOOTH: then x[i] can be used to access "n" 
	//	else: this Trig's index can be used to access "n"
};

class Mesh :public Object3D 
{
	friend class BVH;

	//need to store it
	const Ray* ray;
	Hit* hit;
	float tmin;

	//if have enough vertices, smooth it
	bool smooth;
	bool autoNormal;
	bool hasTexture;

	void computeNorm();

	//BVH will not calculate intersection by itself.
	//instead, it lets "Mesh" to calculate a specific triangle for it.
	BVH hierarchy;

public:
	Mesh(const char* filename, Material* m);

	virtual bool intersect(const Ray& r, Hit& h, float t);
	virtual bool intersectTrig(int idx);

	object_type getType() override
	{
		return MESH;
	}

	//all 3D vertices
	std::vector<Vector3f>v;

	//all triangles
	std::vector<Trig>t;

	//all normals
	std::vector<Vector3f>n;

	//all texture coordinates
	std::vector<Vector2f>texCoord;
};