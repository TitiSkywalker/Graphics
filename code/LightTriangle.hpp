#pragma once
#include <random>

#include "LightObject.hpp"

using namespace std;

class LightTriangle : public LightObject
{
public:
	LightTriangle(const Vector3f& a, const Vector3f& b, const Vector3f& c, const Vector3f& color, const float falloff, int id) : 
		LightObject(color, falloff, id)
	{
		vertices[0] = a;
		vertices[1] = b;
		vertices[2] = c;
	}

	virtual bool intersect(const Ray& ray, Hit& hit, float tmin) override
	{
		Vector3f a = vertices[0];
		Vector3f b = vertices[1];
		Vector3f c = vertices[2];

		Vector3f D1 = a - b;
		Vector3f D2 = a - c;
		Vector3f D3 = ray.getDirection();

		Matrix3f M(D1, D2, D3, true);

		bool issingular;
		Matrix3f MI = M.inverse(&issingular);
		if (issingular)
		{
			return false;
		}
		else
		{
			//solve for barycentric coordinates
			Vector3f B = a - ray.getOrigin();
			Vector3f x = MI * B;

			float beta = x[0];
			float gamma = x[1];
			float alpha = 1.0 - beta - gamma;
			float t = x[2];

			if ((alpha >= 0) && (beta >= 0) && (gamma >= 0) && (t > tmin) && (t < hit.getT()))
			{
				hit.setLightObject(t, (LightObject*)this);

				return true;
			}
			else
			{
				return false;
			}
		}
	}

	virtual void getIllumination(const Vector3f& p, Vector3f& dir, Vector3f& col, float& distance) override
	{
		static random_device rd;
		static mt19937 gen(rd());
		uniform_real_distribution<> dis(0, 1);

		//sample in a uniform square
		float rand1 = dis(gen);
		float rand2 = dis(gen);

		if (rand1 + rand2 > 1.0f)
		{
			float tmp = rand1;
			rand1 = 1.0f - rand2;
			rand2 = 1.0f - tmp;
		}

		const Vector3f& a = vertices[0];
		const Vector3f& b = vertices[1];
		const Vector3f& c = vertices[2];

		Vector3f sample = a + rand1 * (b - a) + rand2 * (c - a);
		dir = sample - p;
		distance = (sample - p).length();
		dir = dir / distance;
		col = color / (1 + falloff * distance * distance);
	}

protected:
	Vector3f vertices[3];
};