#pragma once
#include <cmath>

#include "Object3d.hpp"

class Triangle : public Object3D
{
	friend class Mesh;
public:
	Triangle() = delete;

	Triangle(
		const Vector3f& a,
		const Vector3f& b,
		const Vector3f& c,
		Material* m) : Object3D(m)
	{
		vertices[0] = a;
		vertices[1] = b;
		vertices[2] = c;
		//normal vector already computed in Mesh::intersectTrig
		hasTex = false;
	}

	virtual bool intersect(const Ray& ray, Hit& hit, float tmin)
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
				//interpolate normal vector
				Vector3f normal = alpha * normals[0] +
					beta * normals[1] +
					gamma * normals[2];
				normal.normalize();

				hit.set(t, material, normal);
				//interpolate texture coordinate
				if (hasTex)
				{
					Vector2f texCoord = alpha * texCoords[0] +
						beta * texCoords[1] +
						gamma * texCoords[2];
					hit.setTexCoord(texCoord);
				}
				return true;
			}
			else
			{
				return false;
			}
		}
	}

	object_type getType() override
	{
		return TRIANGLE;
	}

protected:
	Vector3f vertices[3];
	Vector3f normals[3];
	Vector2f texCoords[3];
	bool hasTex;
};