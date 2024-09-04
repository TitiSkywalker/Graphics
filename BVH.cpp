#include <iostream>
#include <vector>
#include <algorithm>

#include "BVH.hpp"
#include "Mesh.hpp"

using namespace std;

//number of triangles in the leaf node
constexpr int PACK = 50;

//compute the bounding box for a triangle given it's Trig representation 
Box BVH::computeBoundingBox(const Trig& triangle, const Mesh& mesh)
{
	//extract vertices
	Vector3f a = mesh.v[triangle[0]];
	Vector3f b = mesh.v[triangle[1]];
	Vector3f c = mesh.v[triangle[2]];

	Vector3f mins(
		min(min(a[0], b[0]), min(a[0], c[0])),
		min(min(a[1], b[1]), min(a[1], c[1])),
		min(min(a[2], b[2]), min(a[2], c[2])));
	Vector3f maxs(
		max(max(a[0], b[0]), max(a[0], c[0])),
		max(max(a[1], b[1]), max(a[1], c[1])),
		max(max(a[2], b[2]), max(a[2], c[2])));

	return Box(mins, maxs);
}

//compute the bounding box for a group of triangles
Box BVH::computeBoundingBox(int* indices, int numTriangles, const Mesh& mesh)
{
	Box& box0 = allBoxes[indices[0]];
	float minx = box0.lower[0];
	float miny = box0.lower[1];
	float minz = box0.lower[2];
	float maxx = box0.upper[0];
	float maxy = box0.upper[1];
	float maxz = box0.upper[2];

	for (int i = 1; i < numTriangles; i++)
	{
		Box& boxi = allBoxes[indices[i]];
		minx = min(minx, boxi.lower[0]);
		miny = min(miny, boxi.lower[1]);
		minz = min(minz, boxi.lower[2]);

		maxx = max(maxx, boxi.upper[0]);
		maxy = max(maxy, boxi.upper[1]);
		maxz = max(maxz, boxi.upper[2]);
	}

	return Box(Vector3f(minx, miny, minz), Vector3f(maxx, maxy, maxz));
}

//compute bounding boxes for all triangles
void BVH::computeAllBoundingBoxes(int numTriangles, const Mesh& mesh)
{
	const vector<Trig>& trigs = mesh.t;

	for (int i = 0; i < numTriangles; i++)
		allBoxes[i] = computeBoundingBox(trigs[i], mesh);
}

//split triangles into 2 groups, given the middle  point of each triangle
void BVH::splitTriangles(float* mids, int* indices, int numTriangles)
{
	//sort the indices and split is automatically done
	vector<pair<float, int>> midpoints(numTriangles);
	for (int i = 0; i < numTriangles; i++)
		midpoints[i] = { mids[i], indices[i] };
	
	sort(midpoints.begin(), midpoints.end());

	for (int i=0; i<numTriangles; i++)
		indices[i] = midpoints[i].second;
}

void BVH::buildNode(BVHNode* node, int* indices, int numTriangles, const Mesh& mesh)
{
	node->box = computeBoundingBox(indices, numTriangles, mesh);
	node->size = numTriangles;
	
	if (numTriangles <= PACK)
	{
		node->isLeaf = true;
		node->triangles = indices;
		return;
	}

	//find longest dimension to split
	Vector3f dist = node->box.getSize();
	int splitDim = 0;
	if (dist[1] > dist[splitDim])
		splitDim = 1;
	if (dist[2] > dist[splitDim])
		splitDim = 2;

	node->splitDim = splitDim;
	
	//middle point in split dimension
	float* midpoints = new float[numTriangles];

	for (int i=0; i<numTriangles; i++)
	{
		Box box = allBoxes[indices[i]];

		midpoints[i] = (box.lower[splitDim] + box.upper[splitDim]) / 2.0f;
	}

	//sort triangles so that I can split them
	splitTriangles(midpoints, indices, numTriangles);

	delete[] midpoints;

	int backSize = numTriangles / 2;
	int frontSize = numTriangles - backSize;

	BVHNode* backNode = new BVHNode(NIL, NIL);
	node->back = backNode;
	buildNode(backNode, indices, backSize, mesh);

	BVHNode* frontNode = new BVHNode(NIL, NIL);
	node->front = frontNode;
	buildNode(frontNode, indices + backSize, frontSize, mesh);
}

void BVH::build(const Mesh& mesh)
{
	int numTriangles = mesh.t.size();

	allIndices = new int[numTriangles];
	for (int i = 0; i < numTriangles; i++)
		allIndices[i] = i;

	allBoxes = new Box[numTriangles];
	computeAllBoundingBoxes(numTriangles, mesh);

	buildNode(root, allIndices, numTriangles, mesh);

	delete[] allBoxes;
	allBoxes = NULL;
}

void BVH::intersect(const Ray& ray)
{
	intersectNode(root, ray);
}

void BVH::free(BVHNode* parent)
{
	if (parent->front != NIL)
		free(parent->front);
	if (parent->back != NIL)
		free(parent->back);

	delete parent;
}

bool BVH::intersectNode(BVHNode* node, const Ray& ray)
{
	bool hasHit;
	float tstart;
	float tend;

	tie(hasHit, tstart, tend) = node->box.intersect(ray);

	if (!hasHit)
		return false;

	if (node->isLeaf)
	{
		bool hasHit = false;
		int numTriangles = node->size;
		int* targetIndices = node->triangles;
		for (int i = 0; i < numTriangles; i++)
		{
			termFunc(targetIndices[i], arg);
			hasHit |= (bool)arg[1];
		}
		return hasHit;
	}

	hasHit = false;
	hasHit |= intersectNode(node->front, ray);
	hasHit |= intersectNode(node->back, ray);
	return hasHit;
}