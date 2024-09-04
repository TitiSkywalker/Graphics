//velocity is like a special transform, but in time domain
#pragma once

#include <iostream>
#include <random>

#include "vecmath.h"
#include "Object3d.hpp"

using namespace std;

class Velocity : public Object3D
{
	Object3D* object;
	Vector3f velocity;

public:
	Velocity() = delete;

	Velocity(const Vector3f& v, Object3D* o) : object(o)
	{
		velocity = v;
	}

	~Velocity()
	{
		delete object;
	}

    virtual bool intersect(const Ray& r, Hit& h, float tmin)
    {
		//radomly sample a time between -1 and 1
		static random_device rd;
		static mt19937 gen(rd());
		uniform_real_distribution<> dis(-1, 1);

		float time = dis(gen);
		Vector3f bias = velocity * time;

		//move this object is equal to moving the incoming ray at opposite direction
		Vector3f origin = r.getOrigin() - bias;
		Ray newRay(origin, r.getDirection());

		return object->intersect(newRay, h, tmin);
    }

    object_type getType() override
    {
        return VELOCITY;
    }

};