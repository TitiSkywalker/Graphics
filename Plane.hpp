#pragma once

#include "object3d.hpp"
#include "vecmath.h"
#include <cmath>

class Plane : public Object3D
{
    public:
        Plane()
        {
            D = 0;
        }

        Plane(const Vector3f& normal, float d, Material* m)
            :Object3D(m)
        {
            N = normal.normalized();
            D = -d;
        }

        ~Plane()
        {}

        bool intersect(const Ray& r, Hit& h, float tmin) override
        {
            float parallel = Vector3f::dot(N, r.getDirection());
            if (parallel == 0)
            {
                return false;
            }
            float t = -(D + Vector3f::dot(N, r.getOrigin())) / parallel;

            if (t > tmin && t < h.getT())
            {
                if (parallel > 0)
                    h.set(t, material, -N);
                else
                    h.set(t, material, N);
                return true;
            }
            else
            {
                return false;
            }
        }

        object_type getType() override
        {
            return PLANE;
        }

    protected:
        Vector3f N;
        float D;
};