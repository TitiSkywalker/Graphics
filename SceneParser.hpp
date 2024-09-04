#pragma once
#include "Vecmath.h"
#include "File.hpp"

#include "SceneParser.hpp"
#include "Camera.hpp"
#include "Material.hpp"

#include "Object3D.hpp"
#include "Mesh.hpp"
#include "Group.hpp"
#include "Sphere.hpp"
#include "Plane.hpp"
#include "Triangle.hpp"
#include "Transform.hpp"
#include "Velocity.hpp"

#include "Light.hpp"
#include "LightGroup.hpp"
#include "LightObject.hpp"
#include "LightTriangle.hpp"
#include "LightSphere.hpp"

#define MAX_PARSER_TOKEN_LENGTH 100

class SceneParser
{
public:
    SceneParser(const char* filename);
    ~SceneParser();

    Camera* getCamera() const
    {
        return camera;
    }

    Vector3f getBackgroundColor() const
    {
        return backgroundColor;
    }

    Vector3f getAmbientLight() const
    {
        return ambientLight;
    }

    int getNumLights() const
    {
        return numLights;
    }

    Light* getLight(int i) const
    {
        assert(i >= 0 && i < numLights);
        return lights[i];
    }

    int getNumMaterials() const
    {
        return numMaterials;
    }

    Material* getMaterial(int i) const
    {
        if (i < 0 || i >= numMaterials)
            throw runtime_error("illegal material index");
        return materials[i];
    }

    Group* getGroup() const
    {
        return group;
    }

    int getNumObjects() const
    {
        return numObjects;
    }

    LightGroup* getLightGroup() const
    {
        return lightGroup;
    }

    int getNumLightObjects() const
    {
        return numLightObjects;
    }

    bool checkStatus() const
    {
        return everythingOK;
    }

    string getErrorMessage() const
    {
        return errorMessage;
    }

    bool hasStochasticScene() const
    {
        return stochastic;
    }

    bool hasStochasticCamera() const
    {
        return stochasticCamera;
    }

private:
    void parseFile();

    void parsePerspectiveCamera();
    void parseDOFCamera();

    void parseBackground();

    void parseLights();
    Light* parseDirectionalLight();
    Light* parsePointLight();

    LightGroup* parseLightGroup();
    LightObject* parseLightObject(char token[MAX_PARSER_TOKEN_LENGTH], int id);
    LightTriangle* parseLightTriangle(int id);
    LightSphere* parseLightSphere(int id);

    void parseMaterials();
    Material* parsePhongMaterial();
    Material* parseGlossyMaterial();
    Material* parseAmbientMaterial();
    Material* parseMirror();
    Material* parseGlass();

    Object3D* parseObject(char token[MAX_PARSER_TOKEN_LENGTH]);
    Group* parseGroup();
    Sphere* parseSphere();
    Plane* parsePlane();
    Triangle* parseTriangle();
    Mesh* parseTriangleMesh();
    Transform* parseTransform();
    Velocity* parseVelocity();

    int getToken(char token[MAX_PARSER_TOKEN_LENGTH]);
    void matchToken(char token[MAX_PARSER_TOKEN_LENGTH], const char* target);
    Vector3f readVector3f();
    Vector2f readVector2f();
    float readFloat();
    int readInt();

    FILE* file;
    Camera* camera;

    Vector3f backgroundColor;
    Vector3f ambientLight;

    Light** lights;
    Material** materials;
    Material* currentMaterial;
    Group* group;
    LightGroup* lightGroup;
    
    int numLights;
    int numMaterials;
    int numObjects;
    int numLightObjects;

    string errorMessage;
    bool everythingOK;

    bool stochastic;
    bool stochasticCamera;
};