#pragma once

#include "Object3d.hpp"
#include "Ray.hpp"
#include "Hit.hpp"
#include <iostream>
#include <vector>

class Group : public Object3D
{
	std::vector<Object3D*> objects;

	public:
		Group()
		{}

		~Group() override
		{
			for (auto i : objects)
			{
				delete (i);
			}
		}

		bool intersect(const Ray& r, Hit& h, float tmin) override
		{
			bool hit = false;
			for (auto obj:objects)
			{
				if (obj->intersect(r, h, tmin))
				{
					hit = true;
				}
			}
			return hit;
		}

		void addObject(Object3D* obj)
		{
			objects.push_back(obj);
		}

		int getGroupSize()
		{
			return objects.size();
		}
};