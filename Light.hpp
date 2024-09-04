#pragma once
#include "Vector3f.h"
#include "Object3D.hpp"

using namespace std;

enum light_type { DIRECTIONAL, POINT };

class Light
{
    public:
        Light()
        {}

        virtual ~Light()
        {}

        virtual void getIllumination(
            const Vector3f& p, 
            Vector3f& dir, 
            Vector3f& color,
            float& distanceToLight) const = 0;
};

class DirectionalLight : public Light
{
    public:
        DirectionalLight(const Vector3f& d, const Vector3f& c)
        {
            direction = d.normalized();
            color = c;
        }

        ~DirectionalLight()
        {}

        virtual void getIllumination(
            const Vector3f& p, 
            Vector3f& dir2Light, 
            Vector3f& col, 
            float& distanceToLight) const
        {
            dir2Light = -direction;
            col = color;
            distanceToLight = FLT_MAX;
        }

        light_type getType()
        {
            return DIRECTIONAL;
        }

    private:

        DirectionalLight(); // don't use

        Vector3f direction;
        Vector3f color;
};

class PointLight : public Light
{
    public:
        PointLight(const Vector3f& p, const Vector3f& c, float fall)
        {
            position = p;
            color = c;
            falloff = fall;
        }

        ~PointLight()
        {}

        virtual void getIllumination(
            const Vector3f& p,
            Vector3f& dir2Light,
            Vector3f& col, 
            float& distanceToLight) const
        {
            dir2Light = (position - p);
            distanceToLight = dir2Light.length();
            dir2Light = dir2Light / distanceToLight;
            col = color / (1 + falloff * distanceToLight * distanceToLight);
        }

        light_type getLight()
        {
            return POINT;
        }

    private:

        PointLight(); // don't use
        float falloff;
        Vector3f position;
        Vector3f color;
};
