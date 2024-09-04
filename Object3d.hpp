#pragma once
#include "Ray.hpp"
#include "Hit.hpp"
#include "Material.hpp"

enum object_type { TRIANGLE, SPHERE, GROUP, MESH, PLANE, TRANSFORM, VELOCITY, OBJECT };

class Object3D
{
	public:
		Object3D()
		{
			material = NULL;
		}
		virtual ~Object3D() {}

		Object3D(Material* material) {
			this->material = material;
		}

		virtual bool intersect(const Ray& r, Hit& h, float tmin) = 0;

		virtual object_type getType()
		{
			return OBJECT;
		}

	protected:
		Material* material;
};