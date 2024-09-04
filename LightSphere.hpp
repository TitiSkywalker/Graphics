#pragma once
#include <random>
#include <tuple>
#include <iostream>

#include "LightObject.hpp"

using namespace std;

class LightSphere : public LightObject
{
	Vector3f center;
	float radius;

	//this is a complicated sampling function, I'm not using it
	void getComplicatedIllumination(const Vector3f& p, Vector3f& dir, Vector3f& col, float& distance)
	{
		//randomly sample a position (must be visible from view point)
		static random_device rd;
		static mt19937 gen(rd());
		uniform_real_distribution<> dis(0, 1);

		//solve visible area geometrically
		Vector3f center2point = (p - center);
		float pointDistance = center2point.length();
		center2point = center2point / pointDistance;

		//theta is the maximum angle between sample direction and point direction
		float cosTheta = radius / pointDistance;
		float sinTheta = sqrt(1 - cosTheta * cosTheta);

		//cross a random vector with point direction to get a random perpendicular direction
		Vector3f randDir(dis(gen), dis(gen), dis(gen));
		randDir.normalize();
		randDir = Vector3f::cross(center2point, randDir);

		//now can get a visible random position
		Vector3f sample = center + cosTheta * center2point + sinTheta * randDir;
		dir = sample - p;
		distance = (sample - p).length();
		dir = dir / distance;
		col = color / (1 + falloff * distance * distance);
	}

public:
	LightSphere(const Vector3f& ctr, const Vector3f& clr, const float r, const float falloff, int id) : LightObject(clr, falloff, id)
	{
		center = ctr;
		radius = r;
	}

	virtual bool intersect(const Ray& r, Hit& h, float tmin) override
	{
		//a*t^2+2b*t+c=0
		float a = Vector3f::dot(r.getDirection(), r.getDirection());
		float b = Vector3f::dot(r.getOrigin() - center, r.getDirection());
		float c = Vector3f::dot(r.getOrigin() - center, r.getOrigin() - center) - radius * radius;

		//delta=4b^2-4ac, quarter_delta=b^2-4ac
		float quarter_delta = b * b - a * c;
		if (quarter_delta < 0)
		{
			//no intersection
			return false;
		}
		else if (quarter_delta < 1e-20)
		{
			//roughly 1 intersection
			float t = -b / a;

			if (t > tmin && t < h.getT())
			{
				Vector3f normal = r.pointAtParameter(t) - center;
				normal.normalize();
				h.setLightObject(t, (LightObject*)this);
				return true;
			}
			return false;
		}
		else
		{
			//2 intersections
			float sqrt_delta = sqrt(quarter_delta);
			float t1 = (-b - sqrt_delta) / a;
			float t2 = (-b + sqrt_delta) / a;
			bool changed = false;
			if (t1 > tmin && t1 < h.getT())
			{
				Vector3f normal = r.pointAtParameter(t1) - center;
				normal.normalize();
				h.setLightObject(t1, (LightObject*)this);
				changed = true;
			}
			if (t2 > tmin && t2 < h.getT())
			{
				Vector3f normal = r.pointAtParameter(t2) - center;
				normal.normalize();
				h.setLightObject(t2, (LightObject*)this);
				changed = true;
			}
			return changed;
		}
	}

	virtual void getIllumination(const Vector3f& p, Vector3f& dir, Vector3f& col, float& distance) override
	{
		//randomly sample a position inside sphere
		static random_device rd;
		static mt19937 gen(rd());
		uniform_real_distribution<> dis(0, 1);

		Vector3f randDir(dis(gen), dis(gen), dis(gen));
		randDir.normalize();

		Vector3f sample = center + (radius * dis(gen)) * randDir;
		dir = sample - p;
		distance = (sample - p).length();
		dir = dir / distance;
		col = color / (1 + falloff * distance * distance);
	}
};