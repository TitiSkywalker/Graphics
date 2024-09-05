#pragma once
#include <float.h>
#include <cmath>
#include <random>

#include "Ray.hpp"
#include "Vecmath.h"

using namespace std;

class Camera 
{
    public:
        Camera(const Vector3f& center, const Vector3f& direction, const Vector3f& up)
        {
            this->center = center;
            this->direction = direction.normalized();
            this->up = up.normalized();
            this->horizontal = Vector3f::cross(this->direction, up).normalized();

            width = 0;
            height = 0;
        }

        // Generate rays for each screen-space coordinate
        virtual Ray generateRay(int x, int y) = 0;
        virtual Ray generateJittoredRay(int x, int y) = 0;

        virtual ~Camera() = default;

        void setCenter(const Vector3f& pos)
        {
            this->center = pos;
        }
        Vector3f getCenter() const
        {
            return this->center;
        }

        void setRotation(const Matrix3f& mat)
        {
            this->horizontal = mat.getCol(0);
            this->up = -mat.getCol(1);
            this->direction = mat.getCol(2);
        }

        Matrix3f getRotation() const
        {
            return Matrix3f(this->horizontal, -this->up, this->direction);
        }

        virtual float getTMin() const = 0;

        void setSize(int imgW, int imgH)
        {
            width = imgW;
            height = imgH;
        }

    protected:
        // Extrinsic parameters
        Vector3f center;
        Vector3f direction;
        Vector3f up;
        Vector3f horizontal;

        int width, height;
};

class PerspectiveCamera : public Camera
{
    float perspect_angle;

    public:
        PerspectiveCamera(
            const Vector3f& center,
            const Vector3f& direction,
            const Vector3f& up,
            float angle) :
            Camera(center, direction, up), perspect_angle(angle)
        {}

        Ray generateRay(int x, int y) override
        {
            float fx = height / (2 * tan(perspect_angle / 2.0));
            float fy = fx;

            Vector3f view =
                ((x - width / 2.0) / fx) * horizontal +
                ((y - height / 2.0) / fy) * up +
                direction;
            view.normalize();

            return Ray(center, view);
        }

        Ray generateJittoredRay(int x, int y) override
        {
            static random_device rd;
            static mt19937 gen(rd());
            uniform_real_distribution<> dis(-0.5, 0.5);

            float jittor1 = dis(gen);
            float jittor2 = dis(gen);

            float fx = height / (2 * tan(perspect_angle / 2.0));
            float fy = fx;

            Vector3f view =
                ((x + jittor1 - width / 2.0) / fx) * horizontal +
                ((y + jittor2 - height / 2.0) / fy) * up +
                direction;
            view.normalize();

            return Ray(center, view);
        }

        virtual float getTMin() const 
        {
            return 0.0f;
        }
};

//achieves depth of field effect
class DOFCamera : public Camera
{
    float focalLength;
    float aperture;

    float perspectAngle;

    Vector3f generatePrimaryRay(int x, int y)
    {
        float fx = height / (2 * tan(perspectAngle / 2.0));
        float fy = fx;

        Vector3f result =
            ((x - width / 2.0) / fx) * horizontal +
            ((y - height / 2.0) / fy) * up +
            direction;
        return result.normalized();
    }

    Vector3f generateJittoredPrimaryRay(int x, int y)
    {
        static random_device rd;
        static mt19937 gen(rd());
        uniform_real_distribution<> dis(-0.5, 0.5);

        float jittor1 = dis(gen);
        float jittor2 = dis(gen);

        float fx = height / (2 * tan(perspectAngle / 2.0));
        float fy = fx;

        Vector3f result =
            ((x + jittor1 - width / 2.0) / fx) * horizontal +
            ((y + jittor2 - height / 2.0) / fy) * up +
            direction;
        return result.normalized();
    }

    Vector3f sampleAperture()
    {
        static random_device rd;
        static mt19937 gen(rd());
        uniform_real_distribution<> dis(-1, 1);

        float x = dis(gen);
        float y = dis(gen);

        return (x * horizontal + y * up).normalized() * aperture;
    }

public:
    DOFCamera(
        const Vector3f& center,
        const Vector3f& direction,
        const Vector3f& up,
        float angle,
        float focal,
        float aper):
        Camera(center, direction, up), perspectAngle(angle), focalLength(focal), aperture(aper)
    {}

    Ray generateRay(int x, int y) override
    {
        //generate primary ray
        Vector3f primary = generatePrimaryRay(x, y);

        //calculate convergence point
        Vector3f C = center + primary * focalLength;

        //calculate new center and direction
        Vector3f newCenter = center + sampleAperture();
        Vector3f newDir = (C - newCenter).normalized();

        return Ray(newCenter, newDir);
    }

    Ray generateJittoredRay(int x, int y) override
    {
        //generate primary ray
        Vector3f primary = generateJittoredPrimaryRay(x, y);

        //calculate convergence point
        Vector3f C = center + primary * focalLength;

        //calculate new center and direction
        Vector3f newCenter = center + sampleAperture();
        Vector3f newDir = (C - newCenter).normalized();

        return Ray(newCenter, newDir);
    }

    virtual float getTMin() const
    {
        return 0.0f;
    }
};