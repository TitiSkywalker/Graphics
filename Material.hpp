#pragma once
#include <cassert>

#include "Vecmath.h"
#include "Ray.hpp"
#include "Hit.hpp"
#include "Texture.hpp"

enum material_type { PHONG, GLOSSY, MIRROR, AMBIENT, GLASS };

class Material
{
	public:
		Material(const Vector3f& d_color, const Vector3f& s_color = Vector3f::ZERO, float shine = 0, float refract = 0, float rough = 0.5);

		virtual ~Material();

		virtual Vector3f Shade(const Ray& ray, const Hit& hit, const Vector3f& dirToLight, const Vector3f& lightColor);
		virtual Vector3f shadeDiffuse(const Ray& ray, const Hit& hit, const Vector3f& dirToLight, const Vector3f& lightColor);
		virtual Vector3f shadeSpecular(const Ray& ray, const Hit& hit, const Vector3f& dirToLight, const Vector3f& lightColor);
		virtual Vector3f shadeAmbient(const Ray& ray, const Hit& hit, const Vector3f& lightColor);

		virtual void loadTexture(const char* filename);
		virtual bool hasValidTexture();

		virtual float getRefractionIndex();
		virtual float getRoughness();

		virtual Vector3f getDiffuseColor();
		virtual Vector3f getSpecularColor();

		virtual material_type getType()=0;
	
	protected:
		Vector3f diffuseColor;
		float refractionIndex;
		float shininess;
		float roughness;
		Vector3f specularColor;
		Texture texture;
		material_type type;
};

//simple, classic Phong material
class Phong: public Material
{
	public:
		Phong(const Vector3f& d_color, const Vector3f& s_color = Vector3f::ZERO, float shine = 0, float refract = 0) :
			Material(d_color, s_color, shine, refract)
		{}

		~Phong()
		{}

		material_type getType() override
		{
			return PHONG;
		}
};

//glossy material with a leaf-shape BRDF function
class Glossy: public Material
{
	public:
		Glossy(const Vector3f& d_color, const Vector3f& s_color = Vector3f::ZERO, float shine = 0, float rough=0.5) :
			Material(d_color, s_color, shine, 0, rough)
		{
			if (roughness <= 0)
				roughness = 0.01;
		}

		~Glossy()
		{}

		material_type getType() override
		{
			return GLOSSY;
		}
};

//ambient material with uniform BRDF function
class Ambient : public Material
{
	public:
		Ambient(const Vector3f& d_color, const Vector3f& s_color = Vector3f::ZERO, float shine = 0) :
			Material(d_color, s_color, shine)
		{}

		~Ambient()
		{}

		material_type getType() override
		{
			return AMBIENT;
		}
};

//mirror with perfect reflection
class Mirror: public Material
{
	public:
		Mirror() : Material(Vector3f::ZERO)
		{}

		~Mirror()
		{}

		material_type getType() override
		{
			return MIRROR;
		}
};

//glass with perfect refraction
class Glass : public Material
{
public:
	Glass(float refract) : Material(Vector3f::ZERO)
	{
		refractionIndex = refract;
	}

	~Glass()
	{}

	material_type getType() override
	{
		return GLASS;
	}
};