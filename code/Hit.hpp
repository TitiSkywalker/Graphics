//information about hit point
#pragma once

#include "vecmath.h"
#include "ray.hpp"

class Material;
class LightObject;

class Hit
{
public:
	Hit()
	{
		material = nullptr;
		lightObject = nullptr;
		t = 1e38;
		hasTex = false;
		isLight = false;
	}
	Hit(float _t, Material* m, const Vector3f& n)
	{
		t = _t;
		material = m;
		normal = n;
		hasTex = false;
		isLight = false;
		lightObject = nullptr;
	}
	Hit(const Hit& h)
	{
		t = h.t;
		material = h.material;
		normal = h.normal;
		hasTex = h.hasTex;
		isLight = h.isLight;
		lightObject = h.lightObject;
	}

	~Hit() = default;

	float getT() const
	{
		return t;
	}

	Material* getMaterial() const
	{
		return material;
	}

	const Vector3f& getNormal() const
	{
		return normal;
	}

	void set(float _t, Material* m, const Vector3f& n)
	{
		t = _t;
		material = m;
		normal = n;
		isLight = false;
	}

	void setLightObject(float _t, LightObject* object)
	{
		t = _t;
		lightObject = object;
		isLight = true;
	}

	LightObject* getLightObject()
	{
		return lightObject;
	}

	void setTexCoord(const Vector2f& coord)
	{
		texCoord = coord;
		hasTex = true;
	}

	Vector2f texCoord;
	Vector3f normal;

	Material* material;
	LightObject* lightObject;

	float t;
	bool hasTex;
	bool isLight;
};