#pragma once
#include <cmath>

#include "Object3d.hpp"
#include "Vecmath.h"
#include "Box.hpp"
#include <gsl/gsl_math.h>	//M_PI

class Sphere : public Object3D
{
	Vector2f getCoord(const Vector3f& normal, const Vector3f& toViewPoint)
	{
		//theta = vertical angle, phi = horizontal angle
		float theta;
		float phi;

		//compute x axis from view point
		Vector3f X = Vector3f::cross(Vector3f(0, 0, 1), toViewPoint).normalized();
		Vector3f Y = Vector3f::cross(Vector3f(0, 0, 1), X).normalized();

		//compute point on unit circle
		Vector3f P = Vector3f(normal.x(), normal.y(), 0).normalized();

		theta = asin(normal.z());

		float dotX = Vector3f::dot(X, P); 
		dotX = (dotX < -1) ? -1 : dotX;
		dotX = (dotX > 1) ? 1 : dotX;
		float dotY = Vector3f::dot(Y, P);
		dotY = (dotY < -1) ? -1 : dotY;
		dotY = (dotY > 1) ? 1 : dotY;

		phi = acos(dotX);
		if (dotY < 0)
			phi = 2 * M_PI - phi;

		//compute bias
		float phi_bias = acos(X.x());
		if (X.y() < 0)
			phi_bias = 2 * M_PI - phi_bias;

		phi = phi + phi_bias;
		if (phi > 2 * M_PI)
			phi -= 2 * M_PI;

		//calculate longitude and latitude
		float longitude = phi / (2 * M_PI);
		float latitude = (theta / M_PI_2 + 1) / 2.0;

		return Vector2f(longitude, latitude);
	}

public:
	Sphere()
	{
		// unit ball at the center
		center = Vector3f(0, 0, 0);
		radius = 1;
	}

	Sphere(const Vector3f& c, float r, Material* material) : Object3D(material)
	{
		center = c;
		radius = r;
	}

	~Sphere() override = default;

	bool intersect(const Ray& r, Hit& h, float tmin) override
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
				h.set(t, material, normal);

				if (material->hasValidTexture())
				{
					h.setTexCoord(getCoord(normal, -r.getDirection()));
				}
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
				h.set(t1, material, normal);
				changed = true;

				if (material->hasValidTexture())
				{
					h.setTexCoord(getCoord(normal, -r.getDirection()));
				}
			}
			if (t2 > tmin && t2 < h.getT())
			{
				Vector3f normal = r.pointAtParameter(t2) - center;
				normal.normalize();
				h.set(t2, material, normal);
				changed = true;

				if (material->hasValidTexture())
				{
					h.setTexCoord(getCoord(normal, -r.getDirection()));
				}
			}
			return changed;
		}
	}

	object_type getType() override
	{
		return SPHERE;
	}

protected:
	Vector3f center;
	float radius;
};