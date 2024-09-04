#pragma once
#include <cassert>
#include <iostream>

#include "Vector3f.h"

class Ray
{
    public:
        Ray() = delete;
        Ray(const Vector3f& orig, const Vector3f& dir)
        {
            origin = orig;
            direction = dir;
        }

        Ray(const Ray& r)
        {
            origin = r.origin;
            direction = r.direction;
        }

        const Vector3f& getOrigin() const
        {
            return origin;
        }

        const Vector3f& getDirection() const
        {
            return direction;
        }

        Vector3f pointAtParameter(float t) const
        {
            return origin + direction * t;
        }

    private:

        Vector3f origin;
        Vector3f direction;

};