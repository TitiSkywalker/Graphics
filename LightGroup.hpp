#pragma once
#include <iostream>
#include <vector>

#include "LightObject.hpp"
#include "Ray.hpp"
#include "Hit.hpp"

using namespace std;

class LightGroup : public LightObject
{
	std::vector<LightObject*> light_objects;

public:
	LightGroup()
	{}

	~LightGroup() override
	{
		for (auto i : light_objects)
		{
			delete (i);
		}
	}

	virtual bool intersect(const Ray& r, Hit& h, float tmin) override
	{
		bool hit = false;
		for (auto i : light_objects)
		{
			if (i->intersect(r, h, tmin))
			{
				hit = true;
			}
		}
		return hit;
	}

	virtual void getIllumination(const Vector3f& p, Vector3f& dir, Vector3f& col, float& distance) override
	{
		cout << "Warning: you should not call LightGroup::getIllumination" << endl;
		return;
	}

	void addLightObject(LightObject* obj)
	{
		light_objects.push_back(obj);
	}

	int getLightGroupSize()
	{
		return light_objects.size();
	}

	LightObject* getLightObject(int index)
	{
		return light_objects[index];
	}
};