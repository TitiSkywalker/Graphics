//bounding volume hierarchy for triangle mesh intersection
#pragma once
#include <vector>
#include <iostream>

#include "Box.hpp"

using namespace std;

struct Trig;
class Mesh;

class BVHNode
{
	friend class BVH;

	//bounding box
	Box box;

	BVHNode* back;
	BVHNode* front;

	//store all triangle indices (real triangles stored in Mesh)
	int* triangles = NULL;

	int size = 0;

	bool isLeaf = false;
	bool splitDim = 0;

	BVHNode(BVHNode* b, BVHNode* f) : back(b), front(f)
	{}

	~BVHNode()
	{}
};

class BVH
{
	BVHNode* root;
	BVHNode* NIL;

	int* allIndices;	//a large array for in-place computation

	Box* allBoxes;		//bounding boxes for all triangles, temporary

	void free(BVHNode* parent);

	bool intersectNode(BVHNode* node, const Ray& ray);

	void buildNode(BVHNode* node, int* indices, int numTriangles, const Mesh& mesh);

	void splitTriangles(float* mids, int* indices, int numTriangles);

	void computeAllBoundingBoxes(int numTriangles, const Mesh& mesh);

	Box computeBoundingBox(int* indices, int numTriangles, const Mesh& mesh);

	Box computeBoundingBox(const Trig& triangle, const Mesh& mesh);

public:
	BVH()
	{
		arg = NULL;
		termFunc = NULL;
		allIndices = NULL;
		allBoxes = NULL;

		NIL = new BVHNode(NULL, NULL);
		root = new BVHNode(NIL, NIL);
	}

	~BVH()
	{
		free(root);
		delete NIL;

		if (allIndices != NULL)
			delete[] allIndices;
	}

	void build(const Mesh& mesh);

	void intersect(const Ray& ray);

	//arg[0] = pointer to a "Mesh" object
	//arg[1] = a boolean flag(hit or not) turned into void* 
	void** arg;

	//this is "intersectCall" in Mesh.cpp
	//use this to detect intersection between triangle and ray, 
	//because data are stored in "Mesh" object
	void (*termFunc) (int idx, void** arg);
};
