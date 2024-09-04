//axis-aligned bounding box
#pragma once
#include <cmath>
#include <tuple>
#include <cassert>

#include "Object3d.hpp"
#include "vecmath.h"

using namespace std;

class Box
{
public:
	//2 corners of a 3D box
	Vector3f lower;
	Vector3f upper;

	Box() : lower(Vector3f::ZERO), upper(Vector3f::ZERO)
	{}

	Box(const Vector3f& low, const Vector3f& up) : lower(low), upper(up)
	{
		assert(low[0] <= up[0]);
		assert(low[1] <= up[1]);
		assert(low[2] <= up[2]);
	}

	Box(const Box& box) : lower(box.lower), upper(box.upper)
	{}

	//returns (hit, tstart, tend)
	//hit=false: no intersection
	tuple<bool, float, float> intersect(const Ray& ray)
	{
		Vector3f direction = ray.getDirection();
		Vector3f origin = ray.getOrigin();

		float tstart = -1e30;
		float tend = 1e30;
		float t1, t2;
		bool parallel = true;

		//x1 & x2
		if (fabs(direction[0]) > 1e-20)
		{
			parallel = false;
			t1 = (lower[0] - origin[0]) / direction[0];
			t2 = (upper[0] - origin[0]) / direction[0];
			if (t1 > t2)
				swap(t1, t2);
			tstart = max(tstart, t1);
			tend = min(tend, t2);
		}

		//y1 & y2
		if (fabs(direction[1]) > 1e-20)
		{
			parallel = false;
			t1 = (lower[1] - origin[1]) / direction[1];
			t2 = (upper[1] - origin[1]) / direction[1];
			if (t1 > t2)
				swap(t1, t2);
			tstart = max(tstart, t1);
			tend = min(tend, t2);
		}

		//z1 & z2		
		if (fabs(direction[2]) > 1e-20)
		{
			parallel = false;
			t1 = (lower[2] - origin[2]) / direction[2];
			t2 = (upper[2] - origin[2]) / direction[2];
			if (t1 > t2)
				swap(t1, t2);
			tstart = max(tstart, t1);
			tend = min(tend, t2);
		}

		if (parallel || (tend <= tstart))
			return make_tuple(false, 0, 0);
		else
			return make_tuple(true, tstart, tend);
	}

	//size = (width, length, height)
	Vector3f getSize()
	{
		return Vector3f(upper[0] - lower[0], upper[1] - lower[1], upper[2] - lower[2]);
	}

	//median of specified dimension
	float getMid(int dim)
	{
		return (upper[dim] + lower[dim]) / 2.0;
	}

	bool overlaps(const Box& box)
	{
		if (upper[0] <= box.lower[0]) return false;
		if (upper[1] <= box.lower[1]) return false;
		if (upper[2] <= box.lower[2]) return false;

		if (lower[0] >= box.upper[0]) return false;
		if (lower[1] >= box.upper[1]) return false;
		if (lower[2] >= box.upper[2]) return false;

		return true;
	}
};
