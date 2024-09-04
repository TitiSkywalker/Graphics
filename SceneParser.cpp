#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES
#include <string>
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <iostream>

#include "SceneParser.hpp"

using namespace std;

#define M_PI 3.14159265358979
#define DegreesToRadians(x) ((M_PI * x) / 180.0f)

SceneParser::SceneParser(const char* filename) 
{
    //initialize some reasonable default values
    file = NULL;

    camera = NULL;
    backgroundColor = Vector3f(0.5, 0.5, 0.5);
    ambientLight = Vector3f(0, 0, 0);
    lights = NULL;
    materials = NULL;
    currentMaterial = NULL;
    group = NULL;
    lightGroup = NULL;

    numLights = 0;
    numMaterials = 0;
    numObjects = 0;
    numLightObjects = 0;

    everythingOK = true;
    errorMessage = "unknown error";

    stochastic = false;
    stochasticCamera = false;

    //parse the file
    try
    {
        if (filename == NULL)
            throw runtime_error("input filename is NULL");

        const char* ext = &filename[strlen(filename) - 6];
        if (strcmp(ext, ".scene") != 0)
            throw runtime_error("wrong file name extension: " + string(ext));

        string filePath = getInputFilePath(filename);
        file = fopen(filePath.c_str(), "r");
        if (file == NULL)
            throw runtime_error("cannot open scene file");

        parseFile();
        fclose(file);
        file = NULL;

        if (camera == NULL)
            throw runtime_error("no camera specified");

        if (group == NULL)
        {
            group = new Group();
        }
        if (lightGroup == NULL)
        {
            lightGroup = new LightGroup();
        }
    }
    catch (const runtime_error& e)
    {
        everythingOK = false;
        errorMessage = e.what();
    }
}

SceneParser::~SceneParser() 
{
    if (group != NULL)
        delete group;
    if (lightGroup != NULL)
        delete lightGroup;
    if (camera != NULL)
        delete camera;
    if (numMaterials > 0)
    {
        for (int i = 0; i < numMaterials; i++)
        {
            delete materials[i];
        }
        delete[] materials;
    }
    if (numLights > 0)
    {
        for (int i = 0; i < numLights; i++)
        {
            delete lights[i];
        }
        delete[] lights;
    }
}
// ====================================================================
// ====================================================================
void SceneParser::parseFile() 
{
    char token[MAX_PARSER_TOKEN_LENGTH];
    while (getToken(token)) 
    {
        if (!strcmp(token, "DOFCamera"))
        {
            stochastic = true;
            stochasticCamera = true;
            parseDOFCamera();
        }
        else if (!strcmp(token, "PerspectiveCamera")) 
        {
            parsePerspectiveCamera();
        }
        else if (!strcmp(token, "Background")) 
        {
            parseBackground();
        }
        else if (!strcmp(token, "Lights")) 
        {
            parseLights();
        }
        else if (!strcmp(token, "Materials")) 
        {
            parseMaterials();
        }
        else if (!strcmp(token, "Group")) 
        {
            group = parseGroup();
        }
        else if (!strcmp(token, "LightGroup"))
        {
            lightGroup = parseLightGroup();
        }
        else
        {
            throw runtime_error("unknown token in parseFile: " + string(token));
        }
    }
}
// ====================================================================
// ====================================================================
void SceneParser::parsePerspectiveCamera() 
{
    char token[MAX_PARSER_TOKEN_LENGTH];
    // read in the camera parameters
    getToken(token); matchToken(token, "{");

    getToken(token); matchToken(token, "center");
    Vector3f center = readVector3f();

    getToken(token); matchToken(token, "direction");
    Vector3f direction = readVector3f();

    getToken(token); matchToken(token, "up");
    Vector3f up = readVector3f();

    getToken(token); matchToken(token, "angle");
    float angle_degrees = readFloat();
    float angle_radians = DegreesToRadians(angle_degrees);

    getToken(token); matchToken(token, "}"); 

    camera = new PerspectiveCamera(center, direction, up, angle_radians);
}

void SceneParser::parseDOFCamera()
{
    char token[MAX_PARSER_TOKEN_LENGTH];
    // read in the camera parameters
    getToken(token); matchToken(token, "{");

    getToken(token); matchToken(token, "center");
    Vector3f center = readVector3f();

    getToken(token); matchToken(token, "direction");
    Vector3f direction = readVector3f();

    getToken(token); matchToken(token, "up");
    Vector3f up = readVector3f();

    getToken(token); matchToken(token, "angle");
    float angle_degrees = readFloat();
    float angle_radians = DegreesToRadians(angle_degrees);

    getToken(token); matchToken(token, "focalLength");
    float focalLength = readFloat();

    getToken(token); matchToken(token, "aperture");
    float aperture = readFloat();

    getToken(token); matchToken(token, "}"); 

    camera = new DOFCamera(center, direction, up, angle_radians, focalLength, aperture);
}

void SceneParser::parseBackground() 
{
    char token[MAX_PARSER_TOKEN_LENGTH];
    // read in the background color
    getToken(token); matchToken(token, "{");
    while (1) 
    {
        getToken(token);
        if (!strcmp(token, "}")) 
        {
            break;
        }
        else if (!strcmp(token, "color")) 
        {
            backgroundColor = readVector3f();
        }
        else if (!strcmp(token, "ambientLight")) 
        {
            ambientLight = readVector3f();
        }
        else 
        {
            throw runtime_error("unknown token in parseBackground: " + string(token));
        }
    }
}
// ====================================================================
// ====================================================================
void SceneParser::parseLights() 
{
    char token[MAX_PARSER_TOKEN_LENGTH];
    getToken(token); matchToken(token, "{");

    // read in the number of objects
    getToken(token); matchToken(token, "numLights");
    numLights = readInt();
    if (numLights <= 0)
    {
        numLights = 0;
        return;
    }
    lights = new Light * [numLights];

    // read in the objects
    int count = 0;
    while (numLights > count) 
    {
        getToken(token);
        if (!strcmp(token, "DirectionalLight")) 
        {
            lights[count] = parseDirectionalLight();
        }
        else if (!strcmp(token, "PointLight"))
        {
            lights[count] = parsePointLight();
        }
        else 
        {
            throw runtime_error("unknown token in parseLight: " + string(token));
        }
        count++;
    }
    getToken(token); matchToken(token, "}");
}

Light* SceneParser::parseDirectionalLight() 
{
    char token[MAX_PARSER_TOKEN_LENGTH];
    getToken(token); matchToken(token, "{");

    getToken(token); matchToken(token, "direction");
    Vector3f direction = readVector3f();
    getToken(token); matchToken(token, "color");
    Vector3f color = readVector3f();

    getToken(token); matchToken(token, "}");
    return new DirectionalLight(direction, color);
}
Light* SceneParser::parsePointLight() 
{
    char token[MAX_PARSER_TOKEN_LENGTH];

    Vector3f position, color;
    float falloff = 0;
    
    getToken(token); matchToken(token, "{");
    while (1) 
    {
        getToken(token);
        if (!strcmp(token, "position")) 
        {
            position = readVector3f();
        }
        else if (!strcmp(token, "color")) 
        {
            color = readVector3f();
        }
        else if (!strcmp(token, "falloff")) 
        {
            falloff = readFloat();
        }
        else 
        {
            matchToken(token, "}");
            break;
        }
    }
    return new PointLight(position, color, falloff);
}
// ====================================================================
// ====================================================================
LightGroup* SceneParser::parseLightGroup()
{
    //parse light group, similar to parseGroup
    char token[MAX_PARSER_TOKEN_LENGTH];
    getToken(token); matchToken(token, "{");

    // read in the number of objects
    getToken(token); matchToken(token, "numObjects");
    numLightObjects = readInt();
    if (numLightObjects <= 0)
    {
        numLightObjects = 0;
    }
    //need random sampling
    stochastic = true;
    LightGroup* answer = new LightGroup();

    // read in the objects
    int count = 0;
    while (numLightObjects > count)
    {
        getToken(token);
        LightObject* lightObject = parseLightObject(token, count);
        if (lightObject == NULL)
            throw runtime_error("a NULL light object is produced in parseLightGroup");
        answer->addLightObject(lightObject);
        count++;
    }
    getToken(token); matchToken(token, "}");

    // return the group
    return answer;
}

LightObject* SceneParser::parseLightObject(char token[MAX_PARSER_TOKEN_LENGTH], int id)
{
    LightObject* answer = NULL;
    if (!strcmp(token, "LightTriangle"))
    {
        answer = (LightObject*)parseLightTriangle(id);
    }
    else if (!strcmp(token, "LightSphere"))
    {
        answer = (LightObject*)parseLightSphere(id);
    }
    else
    {
        throw runtime_error("unknown token in parseLightObject: " + string(token));
    }
    return answer;
}
LightTriangle* SceneParser::parseLightTriangle(int id)
{
    char token[MAX_PARSER_TOKEN_LENGTH];

    Vector3f position, color;
    Vector3f a, b, c;
    float falloff = 0;

    getToken(token); matchToken(token, "{");
    while (1)
    {
        getToken(token);
        if (!strcmp(token, "vertex0"))
        {
            a = readVector3f();
        }
        else if (!strcmp(token, "vertex1"))
        {
            b = readVector3f();
        }
        else if (!strcmp(token, "vertex2"))
        {
            c = readVector3f();
        }
        else if (strcmp(token, "color") == 0)
        {
            color = readVector3f();
        }
        else if (strcmp(token, "falloff") == 0)
        {
            falloff = readFloat();
        }
        else
        {
            matchToken(token, "}");
            break;
        }
    }
    return new LightTriangle(a, b, c, color, falloff, id);
}
LightSphere* SceneParser::parseLightSphere(int id)
{
    char token[MAX_PARSER_TOKEN_LENGTH];

    Vector3f center, color;
    float radius = 0;
    float falloff = 0;

    getToken(token); matchToken(token, "{"); 
    while (1)
    {
        getToken(token);
        if (!strcmp(token, "center"))
        {
            center = readVector3f();
        }
        else if (strcmp(token, "color") == 0)
        {
            color = readVector3f();
        }
        else if (strcmp(token, "radius") == 0)
        {
            radius = readFloat();
        }
        else if (strcmp(token, "falloff") == 0)
        {
            falloff = readFloat();
        }
        else
        {
            matchToken(token, "}");
            break;
        }
    }
    return new LightSphere(center, color, radius, falloff, id);
}
// ====================================================================
// ====================================================================
void SceneParser::parseMaterials() 
{
    char token[MAX_PARSER_TOKEN_LENGTH];
    getToken(token); assert(!strcmp(token, "{"));

    // read in the number of objects
    getToken(token); matchToken(token, "numMaterials");
    numMaterials = readInt();
    if (numMaterials <= 0)
        throw runtime_error("numMaterials <= 0");

    materials = new Material * [numMaterials];

    // read in the objects
    int count = 0;
    while (numMaterials > count) 
    {
        getToken(token);
        if (!strcmp(token, "PhongMaterial")||(!strcmp(token, "Material")))
        {
            materials[count] = parsePhongMaterial();
        }
        else if (!strcmp(token, "Glossy"))
        {
            stochastic = true;
            materials[count] = parseGlossyMaterial();
        }
        else if (!strcmp(token, "Ambient"))
        {
            stochastic = true;
            materials[count] = parseAmbientMaterial();
        }
        else if (!strcmp(token, "Mirror"))
        {
            materials[count] = parseMirror();
        }
        else if (!strcmp(token, "Glass"))
        {
            materials[count] = parseGlass();
        }
        else 
        {
            throw runtime_error("unknown token in parseMaterial: " + string(token));
        }
        count++;
    }
    getToken(token); matchToken(token, "}");
}

Material* SceneParser::parsePhongMaterial() 
{
    char token[MAX_PARSER_TOKEN_LENGTH];
    char filename[MAX_PARSER_TOKEN_LENGTH];
    filename[0] = 0;
    Vector3f diffuseColor(1, 1, 1), specularColor(0, 0, 0);

    float shininess = 0;
    float refractionIndex = 0;
    float reflectivity = 0;

    getToken(token); matchToken(token, "{");
    while (1) 
    {
        getToken(token);
        if (!strcmp(token, "diffuseColor")) 
        {
            diffuseColor = readVector3f();
        }
        else if (!strcmp(token, "specularColor"))
        {
            specularColor = readVector3f();
        }
        else if (!strcmp(token, "shininess")) 
        {
            shininess = readFloat();
        }
        else if (!strcmp(token, "refractionIndex")) 
        {
            refractionIndex = readFloat();
        }
        else if (!strcmp(token, "texture")) 
        {
            getToken(filename);
        }
        else 
        {
            matchToken(token, "}");
            break;
        }
    }

    Material* answer = new Phong(diffuseColor, specularColor, shininess, refractionIndex);
    if (filename[0] != 0) 
    {
        string texturePath = getTexturePath(filename);
        if (texturePath == "<file does not exist>")
            throw runtime_error("cannot find texture file " + string(filename));
        answer->loadTexture(texturePath.c_str());
    }
    return answer;
}
Material* SceneParser::parseGlossyMaterial()
{
    char token[MAX_PARSER_TOKEN_LENGTH];
    char filename[MAX_PARSER_TOKEN_LENGTH];
    filename[0] = 0;
    Vector3f diffuseColor(1, 1, 1), specularColor(0, 0, 0);

    float shininess = 0;
    float refractionIndex = 0;
    float reflectivity = 0;
    float roughness = 0.5;

    getToken(token); matchToken(token, "{");
    while (1)
    {
        getToken(token);
        if (!strcmp(token, "diffuseColor"))
        {
            diffuseColor = readVector3f();
        }
        else if (!strcmp(token, "specularColor"))
        {
            specularColor = readVector3f();
        }
        else if (!strcmp(token, "shininess"))
        {
            shininess = readFloat();
        }
        else if (!strcmp(token, "texture"))
        {
            getToken(filename);
        }
        else if (!strcmp(token, "roughness"))
        {
            roughness = readFloat();
        }
        else
        {
            matchToken(token, "}");
            break;
        }
    }

    Material* answer = new Glossy(diffuseColor, specularColor, shininess, roughness);
    if (filename[0] != 0)
    {
        string texturePath = getTexturePath(filename);
        if (texturePath == "<file does not exist>")
            throw runtime_error("cannot find texture file " + string(filename));
        answer->loadTexture(texturePath.c_str());
    }
    return answer;
}
Material* SceneParser::parseAmbientMaterial()
{
    char token[MAX_PARSER_TOKEN_LENGTH];
    char filename[MAX_PARSER_TOKEN_LENGTH];
    filename[0] = 0;
    Vector3f diffuseColor(1, 1, 1), specularColor(0, 0, 0);

    float shininess = 0;
    float refractionIndex = 0;
    float reflectivity = 0;

    bool hasEmit = false;
    Vector3f emittance;

    getToken(token); matchToken(token, "{");
    while (1)
    {
        getToken(token);
        if (!strcmp(token, "diffuseColor"))
        {
            diffuseColor = readVector3f();
        }
        else if (!strcmp(token, "specularColor"))
        {
            specularColor = readVector3f();
        }
        else if (!strcmp(token, "shininess"))
        {
            shininess = readFloat();
        }
        else if (!strcmp(token, "texture"))
        {
            getToken(filename);
        }
        else
        {
            matchToken(token, "}");
            break;
        }
    }

    Material* answer = new Ambient(diffuseColor, specularColor, shininess);
    if (filename[0] != 0)
    {
        string texturePath = getTexturePath(filename);
        if (texturePath == "<file does not exist>")
            throw runtime_error("cannot find texture file " + string(filename));
        answer->loadTexture(texturePath.c_str());
    }
    return answer;
}
Material* SceneParser::parseMirror()
{
    char token[MAX_PARSER_TOKEN_LENGTH];

    getToken(token); matchToken(token, "{");
    getToken(token); matchToken(token, "}");

    return new Mirror();
}
Material* SceneParser::parseGlass()
{
    char token[MAX_PARSER_TOKEN_LENGTH];
    float refractionIndex = 0;

    getToken(token); matchToken(token, "{");
    while (1)
    {
        getToken(token);
        if (!strcmp(token, "refractionIndex"))
        {
            refractionIndex = readFloat();
        }
        else
        {
            matchToken(token, "}");
            break;
        }
    }
    return new Glass(refractionIndex);
}
// ====================================================================
// ====================================================================
Object3D* SceneParser::parseObject(char token[MAX_PARSER_TOKEN_LENGTH]) 
{
    Object3D* answer = NULL;
    if (!strcmp(token, "Group")) 
    {
        answer = (Object3D*)parseGroup();
    }
    else if (!strcmp(token, "Sphere")) 
    {
        answer = (Object3D*)parseSphere();
    }
    else if (!strcmp(token, "Plane")) 
    {
        answer = (Object3D*)parsePlane();
    }
    else if (!strcmp(token, "Triangle")) 
    {
        answer = (Object3D*)parseTriangle();
    }
    else if (!strcmp(token, "TriangleMesh")) 
    {
        answer = (Object3D*)parseTriangleMesh();
    }
    else if (!strcmp(token, "Transform")) 
    {
        answer = (Object3D*)parseTransform();
    }
    else if (!strcmp(token, "Velocity"))
    {
        stochastic = true;
        answer = (Object3D*)parseVelocity();
    }
    else 
    {
        throw runtime_error("unknown token in parseObject: " + string(token));
    }

    return answer;
}

Group* SceneParser::parseGroup()
{
    char token[MAX_PARSER_TOKEN_LENGTH];
    getToken(token); matchToken(token, "{"); 

    // read in the number of objects
    getToken(token); matchToken(token, "numObjects");
    numObjects = readInt();
    if (numObjects <= 0)
    {
        numObjects = 0;
    }

    Group* answer = new Group();

    // read in the objects
    int count = 0;
    while (numObjects > count) 
    {
        getToken(token);
        if (!strcmp(token, "MaterialIndex")) 
        {
            // change the current material
            int index = readInt();
            currentMaterial = getMaterial(index);
        }
        else 
        {
            auto object = parseObject(token);
            if (object == NULL)
                throw runtime_error("a NULL object is produced in parseGroup");
            answer->addObject(object);

            count++;
        }
    }
    getToken(token); matchToken(token, "}");

    return answer;
}
Sphere* SceneParser::parseSphere()
{
    char token[MAX_PARSER_TOKEN_LENGTH];
    getToken(token); matchToken(token, "{");
    getToken(token); matchToken(token, "center");
    Vector3f center = readVector3f();
    getToken(token); matchToken(token, "radius");
    float radius = readFloat();
    getToken(token); matchToken(token, "}");

    if (currentMaterial == NULL)
        throw runtime_error("material for sphere is not specified");
    return new Sphere(center, radius, currentMaterial);
}
Plane* SceneParser::parsePlane() 
{
    char token[MAX_PARSER_TOKEN_LENGTH];
    getToken(token); matchToken(token, "{");
    getToken(token); matchToken(token, "normal");
    Vector3f normal = readVector3f();
    getToken(token); matchToken(token, "offset");
    float offset = readFloat();
    getToken(token); matchToken(token, "}");

    if (currentMaterial == NULL)
        throw runtime_error("material for plane is not specified");
    return new Plane(normal, offset, currentMaterial);
}
Triangle* SceneParser::parseTriangle() 
{
    char token[MAX_PARSER_TOKEN_LENGTH];
    getToken(token); matchToken(token, "{");
    getToken(token); matchToken(token, "vertex0");
    Vector3f v0 = readVector3f();
    getToken(token); matchToken(token, "vertex1");
    Vector3f v1 = readVector3f();
    getToken(token); matchToken(token, "vertex2");
    Vector3f v2 = readVector3f();
    getToken(token); matchToken(token, "}");

    if (currentMaterial == NULL)
        throw runtime_error("material for triangle is not specified");
    return new Triangle(v0, v1, v2, currentMaterial);
}
Mesh* SceneParser::parseTriangleMesh() 
{
    char token[MAX_PARSER_TOKEN_LENGTH];
    char filename[MAX_PARSER_TOKEN_LENGTH];

    // get the filename
    getToken(token); matchToken(token, "{"); 
    getToken(token); matchToken(token, "obj_file");
    getToken(filename);
    getToken(token); matchToken(token, "}");

    char* extension = &filename[strlen(filename) - 4];
    matchToken(extension, ".obj"); 

    if (currentMaterial == NULL)
        throw runtime_error("material for triangle mesh is not specified");

    string meshPath = getTriangleMeshPath(filename);
    if (meshPath == "<file does not exist>")
        throw runtime_error("cannot find mesh file " + string(filename));

    Mesh* answer = new Mesh(meshPath.c_str(), currentMaterial);

    return answer;
}
Transform* SceneParser::parseTransform() 
{
    char token[MAX_PARSER_TOKEN_LENGTH];
    Matrix4f matrix = Matrix4f::identity();
    Object3D* object = NULL;
    getToken(token); matchToken(token, "{");
    // read in transformations: 
    // apply to the LEFT side of the current matrix (so the first
    // transform in the list is the last applied to the object)
    getToken(token);

    while (1) {
        if (!strcmp(token, "Scale")) 
        {
            Vector3f s = readVector3f();
            matrix = matrix * Matrix4f::scaling(s[0], s[1], s[2]);
        }
        else if (!strcmp(token, "UniformScale")) 
        {
            float s = readFloat();
            matrix = matrix * Matrix4f::uniformScaling(s);
        }
        else if (!strcmp(token, "Translate")) 
        {
            matrix = matrix * Matrix4f::translation(readVector3f());
        }
        else if (!strcmp(token, "XRotate")) 
        {
            matrix = matrix * Matrix4f::rotateX(DegreesToRadians(readFloat()));
        }
        else if (!strcmp(token, "YRotate")) 
        {
            matrix = matrix * Matrix4f::rotateY(DegreesToRadians(readFloat()));
        }
        else if (!strcmp(token, "ZRotate")) 
        {
            matrix = matrix * Matrix4f::rotateZ(DegreesToRadians(readFloat()));
        }
        else if (!strcmp(token, "Rotate")) 
        {
            getToken(token); matchToken(token, "{"); 
            Vector3f axis = readVector3f();
            float degrees = readFloat();
            float radians = DegreesToRadians(degrees);
            matrix = matrix * Matrix4f::rotation(axis, radians);
            getToken(token); matchToken(token, "}");
        }
        else if (!strcmp(token, "Matrix4f"))
        {
            Matrix4f matrix2 = Matrix4f::identity();
            getToken(token); matchToken(token, "{"); 
            for (int j = 0; j < 4; j++) 
            {
                for (int i = 0; i < 4; i++) 
                {
                    float v = readFloat();
                    matrix2(i, j) = v;
                }
            }
            getToken(token); matchToken(token, "}");
            matrix = matrix2 * matrix;
        }
        else 
        {
            // otherwise this must be an object,
            // and there are no more transformations
            object = parseObject(token);
            break;
        }
        getToken(token);
    }

    if (object == NULL)
        throw runtime_error("a NULL object is produced in parseTransform");
    getToken(token); matchToken(token, "}");
    return new Transform(matrix, object);
}
Velocity* SceneParser::parseVelocity()
{
    char token[MAX_PARSER_TOKEN_LENGTH];
    Vector3f velocity;
    Object3D* object = NULL;

    getToken(token); matchToken(token, "{");
    getToken(token);

    while (1)
    {
        if (!strcmp(token, "velocity"))
        {
            velocity = readVector3f();
        }
        else
        {
            object = parseObject(token);
            break;
        }

        getToken(token);
    }

    if (object == NULL)
        throw runtime_error("a NULL object is produced in parseVelocity");
    getToken(token); matchToken(token, "}");

    return new Velocity(velocity, object);
}
// ====================================================================
// ====================================================================
int SceneParser::getToken(char token[MAX_PARSER_TOKEN_LENGTH]) 
{
    //for simplicity, tokens must be separated by whitespace
    //tokens starting with '#' will be ignored
    if (file == NULL)
        throw runtime_error("cannot get token, file is NULL");
    while (true)
    {
        int success = fscanf(file, "%s ", token);
        if (success == EOF)
        {
            token[0] = '\0';
            return 0;
        }
        else if (token[0] != '#')
            return 1;
    }
}

void SceneParser::matchToken(char token[MAX_PARSER_TOKEN_LENGTH], const char* target)
{
    if (strcmp(token, target))
        throw runtime_error("unknown token " + string(token) + ", expect " + target);
}

Vector3f SceneParser::readVector3f() 
{
    float x, y, z;
    int count = fscanf(file, "%f %f %f", &x, &y, &z);
    if (count != 3) 
    {
        throw runtime_error("failed to read in a Vector3f");
    }
    return Vector3f(x, y, z);
}

Vector2f SceneParser::readVector2f()
{
    float u, v;
    int count = fscanf(file, "%f %f", &u, &v);
    if (count != 2) 
    {
        throw runtime_error("failed to read in a Vector2f");
    }
    return Vector2f(u, v);
}

float SceneParser::readFloat() 
{
    float answer;
    int count = fscanf(file, "%f", &answer);
    if (count != 1) 
    {
        throw runtime_error("failed to read in a float");
    }
    return answer;
}

int SceneParser::readInt() 
{
    int answer;
    int count = fscanf(file, "%d", &answer);
    if (count != 1) 
    {
        throw runtime_error("failed to read in a int");
    }
    return answer;
}
