#pragma once

#include "Vector3f.h"
#include "Hit.hpp"

class LightObject
{
	public:
		LightObject()
		{
			color = Vector3f(0, 0, 0);
			falloff = 0;
			ID = -1;
		}

		LightObject(const Vector3f& c, float f = 0.0, int id = -1)
		{
			color = c;
			falloff = f;
			ID = id;
		}

		virtual ~LightObject() 
		{}

		virtual bool intersect(const Ray& r, Hit& h, float tmin) = 0;

		//return a sample point
		virtual void getIllumination(const Vector3f& p, Vector3f& dir, Vector3f& col, float& distance) = 0;

		virtual Vector3f getColor()
		{
			return color;
		}

		virtual int getID()
		{
			return ID;
		}

	protected:
		Vector3f color;
		float falloff;
		int ID;
};