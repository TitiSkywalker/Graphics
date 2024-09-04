#include <iostream>

#include "Material.hpp"

using namespace std;

Material::Material(const Vector3f& d_color, const Vector3f& s_color, float shine, float refract, float rough) :
	diffuseColor(d_color), specularColor(s_color), shininess(shine), refractionIndex(refract), roughness(rough)
{}

Material::~Material()
{}

void Material::loadTexture(const char* filename)
{
	texture.load(filename);
}

bool Material::hasValidTexture()
{
	return texture.valid();
}

float Material::getRefractionIndex()
{
	return refractionIndex;
}

float Material::getRoughness()
{
	return roughness;
}

Vector3f Material::getDiffuseColor()
{
	return diffuseColor;
}

Vector3f Material::getSpecularColor()
{
	return specularColor;
}

Vector3f Material::Shade(const Ray& ray, const Hit& hit, const Vector3f& dirToLight, const Vector3f& lightColor) 
{
	Vector3f kd;
	if (texture.valid() && hit.hasTex) 
	{
		Vector2f texCoord = hit.texCoord;
		Vector3f texColor = texture(texCoord[0], texCoord[1]); //overloaded operator ()
		kd = texColor;
	}
	else 
	{
		kd = diffuseColor;
	}

	Vector3f n = hit.getNormal(); //already normalized

	Vector3f ks = specularColor;

	Vector3f reflectLight = 2 * (Vector3f::dot(dirToLight, n)) * n - dirToLight;
	float reflect = -Vector3f::dot(reflectLight, ray.getDirection());
	reflect = (reflect < 0) ? 0 : pow(reflect, shininess);

	Vector3f color = Vector3f::clampedDot(dirToLight, n) * Vector3f::pointwiseDot(lightColor, kd) +
		reflect * Vector3f::pointwiseDot(lightColor, ks);

	return color;
}

Vector3f Material::shadeDiffuse(const Ray& ray, const Hit& hit, const Vector3f& dirToLight, const Vector3f& lightColor)
{
	//get local color
	Vector3f kd;
	if (texture.valid() && hit.hasTex)
	{
		Vector2f texCoord = hit.texCoord;
		Vector3f texColor = texture(texCoord[0], texCoord[1]);
		kd = texColor;
	}
	else
	{
		kd = this->diffuseColor;
	}

	Vector3f n = hit.getNormal();

	return Vector3f::clampedDot(dirToLight, n) * Vector3f::pointwiseDot(lightColor, kd);
}

Vector3f Material::shadeSpecular(const Ray& ray, const Hit& hit, const Vector3f& dirToLight, const Vector3f& lightColor)
{
	//get local color
	Vector3f ks = specularColor;

	Vector3f n = hit.getNormal();

	Vector3f reflectLight = 2 * (Vector3f::dot(dirToLight, n)) * n - dirToLight;
	float reflect = -Vector3f::dot(reflectLight, ray.getDirection());
	reflect = (reflect < 0) ? 0 : pow(reflect, shininess);

	return reflect * Vector3f::pointwiseDot(lightColor, ks);
}

Vector3f Material::shadeAmbient(const Ray& ray, const Hit& hit, const Vector3f& lightColor)
{
	//get local color
	Vector3f kd;
	if (texture.valid() && hit.hasTex)
	{
		Vector2f texCoord = hit.texCoord;
		Vector3f texColor = texture(texCoord[0], texCoord[1]);
		kd = texColor;
	}
	else
	{
		kd = this->diffuseColor;
	}

	return Vector3f::pointwiseDot(lightColor, kd);
}
