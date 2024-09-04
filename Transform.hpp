//transform is imposed to another object
#pragma once

#include "vecmath.h"
#include "object3d.hpp"
#include <iostream>

using namespace std;

static Vector3f transformPoint(const Matrix4f& mat, const Vector3f& point)
{
    return (mat * Vector4f(point, 1)).xyz();
}

static Vector3f transformDirection(const Matrix4f& mat, const Vector3f& dir)
{
    return (mat * Vector4f(dir, 0)).xyz();
}

class Transform : public Object3D
{
    public:
        Transform() = delete;

        Transform(const Matrix4f& m, Object3D* obj) : o(obj)
        {
            transform = m.inverse();
        }

        ~Transform()
        {
            delete o;
        }

        virtual bool intersect(const Ray& r, Hit& h, float tmin)
        {
            Vector3f trSource = transformPoint(transform, r.getOrigin());
            Vector3f trDirection = transformDirection(transform, r.getDirection());
            float len = trDirection.length();
            trDirection.normalize();

            Ray tr(trSource, trDirection);
            Hit h0;
            bool inter = o->intersect(tr, h0, tmin);

            if (inter)
            {
                float t0 = h0.getT() / len;
                if (t0 < h.getT())
                {
                    h.set(t0, h0.getMaterial(), transformDirection(transform.transposed(), h0.getNormal()).normalized());
                    if (h0.hasTex)
                        h.setTexCoord(h0.texCoord);
                }
            }
            return inter;
        }

        object_type getType() override
        {
            return TRANSFORM;
        }

    protected:
        Object3D* o; //un-transformed object
        Matrix4f transform;
};
