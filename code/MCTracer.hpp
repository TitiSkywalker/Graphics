//Monte Carlo ray tracer
#pragma once
#include <vector>
#include <cmath>
#include <iostream>
#include <random>

#include "SceneParser.hpp"
#include "Ray.hpp"
#include "Hit.hpp"
#include "Camera.hpp"
#include "Group.hpp"
#include "Material.hpp"
#include "Light.hpp"
#include "Configuration.hpp"

using namespace std;

class SceneParser;

//keep track of all refraction indexes along the way and build a BST
class MCNode
{
    friend class MCTracer;

    MCNode* reflect_node;
    MCNode* refract_node;
    MCNode* parent = NULL;

    float refraction_index;

public:
    MCNode(MCNode* reflect, MCNode* refract, float refr)
    {
        reflect_node = reflect;
        refract_node = refract;
        refraction_index = refr;
    }
};

class MCTracer
{
    //scene settings
    SceneParser* m_scene;
    Group* group;
    LightGroup* lightGroup;

    //used for recursive ray tracind tree
    MCNode* root;
    MCNode* NIL;

    //used for Russian roulette
    float stop_probability;
    int max_depth;

    //release trace tree memory
    void free(MCNode* p)
    {
        if (p->reflect_node != NIL)
            free(p->reflect_node);
        if (p->refract_node != NIL)
            free(p->refract_node);
        delete p;
    }
    
    //produce a random ray direction(used in traceAmbient and traceGlossy)
    Vector3f randomDir() 
    {
        static bool firstCall = true;
        if (firstCall) {
            srand(time(NULL));
            firstCall = false;
        }

        return Vector3f((rand() / (double)RAND_MAX) * 2 - 1, (rand() / (double)RAND_MAX) * 2 - 1, (rand() / (double)RAND_MAX) * 2 - 1);
    }

    Vector3f traceReflect(Ray& ray, Hit& hit, MCNode* current, int depth)
    {
        //perfect reflection
        Vector3f reflectDir = computeReflect(hit.getNormal(), ray.getDirection(), current);
        Ray reflectRay(ray.pointAtParameter(hit.getT()), reflectDir);
        Hit reflectHit;
        return traceRay(reflectRay, reflectHit, current->reflect_node, depth+1);
    }

    Vector3f traceReflectAndRefract(Ray& ray, Hit& hit, MCNode* current, int depth)
    {
        Material* material = hit.getMaterial();
        Vector3f reflectDir = computeReflect(hit.getNormal(), ray.getDirection(), current);
        Ray reflectRay(ray.pointAtParameter(hit.getT()), reflectDir);
        Hit reflectHit;
        Vector3f reflectColor = traceRay(reflectRay, reflectHit, current->reflect_node, depth+1);

        if (material->getRefractionIndex() > 0)
        {
            Vector3f refractDir = computeRefract(hit.getNormal(), ray.getDirection(), current, material->getRefractionIndex());
            Ray refractRay(ray.pointAtParameter(hit.getT()), refractDir);
            Hit refractHit;
            if (refractDir.length() < 0.5)
            {
                //no refraction, only reflection
                return reflectColor;
            }
            else
            {
                Vector3f refractColor = traceRay(refractRay, refractHit, current->refract_node, depth + 1);

                float n_current = current->refraction_index;
                float n_next = current->refract_node->refraction_index;
                float c = (n_current <= n_next) ?
                    abs(Vector3f::dot(ray.getDirection(), hit.getNormal())) :
                    abs(Vector3f::dot(refractDir, hit.getNormal()));

                //Schlick's approximation to Fresnel's law
                float R0 = pow(((n_next - n_current) / (n_next + n_current)), 2);
                float R = R0 + (1 - R0) * pow(1 - c, 5);

                return R * reflectColor + (1 - R) * refractColor;
            }
        }
        else
        {
            return reflectColor;
        }
    }

    Vector3f traceAmbient(Ray& ray, Hit& hit, MCNode* current, int depth)
    {
        //generate a random reflect ray and make it points outward
        Vector3f reflectDir = randomDir();
        if (Vector3f::dot(hit.getNormal(), reflectDir) < 0)
            reflectDir = -reflectDir;

        Ray reflectRay(ray.pointAtParameter(hit.getT()), reflectDir);
        Hit reflectHit;

        MCNode* reflect_node = new MCNode(NIL, NIL, current->refraction_index);
        current->reflect_node = reflect_node;
        reflect_node->parent = current;

        Vector3f traceColor = traceRay(reflectRay, reflectHit, current->reflect_node, depth + 1);
        float distance = reflectHit.getT();

        return traceColor / (1+FALLOFF * distance * distance);
    }

    //Cook-Torrance BRDF function that take roughness into account
    Vector3f CookTorrance(const Vector3f& normal, const Vector3f& incoming, const Vector3f& reflect, const Vector3f& specularColor, float roughness)
    {
        Vector3f N = normal.normalized();
        Vector3f V = -incoming.normalized();
        Vector3f L = reflect.normalized();
        Vector3f H = (L + V).normalized();

        float H_N = Vector3f::dot(H, N);
        float H_V = Vector3f::dot(H, V);
        float N_L = Vector3f::dot(N, L);
        float N_V = Vector3f::dot(N, V);

        if (N_V == 0)
            return Vector3f(0, 0, 0);

        float delta = acos(H_N);    //angle between H and N
        float m = roughness;        //roughness
        float q = 5.0;              //specular reflection exponent

        float D = exp(-(tan(pow(delta / m, 2)))) / (m * m * pow(cos(delta), 4));
        float G = min(1.0, min(2.0 * H_N * N_V / H_V, 2.0 * H_N * N_L / H_V));
        float F = pow(H_N, q);

        float result = D * F * G / (N_L * N_V);

        if (isinf(result))
        {
            return Vector3f(1, 1, 1);
        }
        else if (result < 0)
        {
            result = -result;
        }

        return specularColor * result;
    }

    Vector3f traceGlossy(Ray& ray, Hit& hit, MCNode* current, int depth)
    {
        //generate a random reflect ray
        Vector3f reflectDir = randomDir();
        //point out
        if (Vector3f::dot(hit.getNormal(), reflectDir) < 0)
            reflectDir = -reflectDir;

        Ray reflectRay(ray.pointAtParameter(hit.getT()), reflectDir);
        Hit reflectHit;

        MCNode* reflect_node = new MCNode(NIL, NIL, current->refraction_index);
        current->reflect_node = reflect_node;
        reflect_node->parent = current;

        Vector3f traceColor = traceRay(reflectRay, reflectHit, current->reflect_node, depth + 1);
        float distance = reflectHit.getT();

        // Add BRDF function here
        Vector3f brdf = CookTorrance(hit.getNormal(), ray.getDirection(), reflectDir, hit.getMaterial()->getSpecularColor(), hit.getMaterial()->getRoughness());

        // Multiply the traceColor by the BRDF
        traceColor = Vector3f::pointwiseDot(traceColor, brdf);

        return traceColor / (1 + FALLOFF * distance * distance);
    }

    //call this function to get the color of hitting point
    Vector3f getLocalColor(Ray& ray, Hit& hit)
    {
        Material* material = hit.getMaterial();
        Vector3f localColor=material->shadeAmbient(ray, hit, m_scene->getAmbientLight());

        Vector3f localPoint = ray.pointAtParameter(hit.getT());

        //compute local color from virtual lights
        for (int l = 0; l < m_scene->getNumLights(); l++)
        {
            //compute a single color per light source
            Light* light = m_scene->getLight(l);

            Vector3f lightColor;
            Vector3f dir2light;
            float distance = 0;
            light->getIllumination(localPoint, dir2light, lightColor, distance);

            //cast shadow rays, dir2light aready normalized
            Ray shadowRay(localPoint, dir2light);
            Hit shadowHit;      //blocked by another 3D object
            Hit shadowLightHit; //blocked by another light object

            bool group_hit = group->intersect(shadowRay, shadowHit, EPSILON);
            if (group_hit && shadowHit.getT() < distance - EPSILON)
                continue;

            bool group_lights_hit = group->intersect(shadowRay, shadowLightHit, EPSILON);
            if (group_lights_hit && shadowLightHit.getT() < distance - EPSILON)
                continue;

            localColor = localColor + material->Shade(ray, hit, dir2light, lightColor);
        }
        
        //compute local color with 3D light objects
        for (int l = 0; l < lightGroup->getLightGroupSize(); l++)
        {
            LightObject* object = lightGroup->getLightObject(l);

            Vector3f lightColor;
            Vector3f dir2light;
            float distance = 0;
            //This function returns a random light sample at the light source
            object->getIllumination(localPoint, dir2light, lightColor, distance);

            Ray shadowRay(localPoint, dir2light);
            Hit shadowHit;      //blocked by another 3D object
            Hit shadowLightHit; //blocked by another light object

            bool isGroupHit = group->intersect(shadowRay, shadowHit, EPSILON);
            if (isGroupHit && shadowHit.getT() < distance - EPSILON)
                continue;

            bool isGroupLightHit = lightGroup->intersect(shadowRay, shadowLightHit, EPSILON);
            if (isGroupLightHit && shadowLightHit.getLightObject()->getID() != object->getID())
            {
                if (shadowLightHit.getT() < distance - EPSILON)
                    continue;
            }

            localColor = localColor + material->Shade(ray, hit, dir2light, lightColor);
        }

        return localColor;
    }

    //compute diffuse color separately, may not be used
    Vector3f getDiffuseColor(Ray& ray, Hit& hit)
    {
        Vector3f diffuseColor;
        Material* material = hit.getMaterial();

        //compute local color
        for (int l = 0; l < m_scene->getNumLights(); l++)
        {
            //compute a single color per light source
            Light* light = m_scene->getLight(l);

            Vector3f lightColor;
            Vector3f dir2light;
            float distance = 0;
            light->getIllumination(
                ray.pointAtParameter(hit.getT()),
                dir2light,
                lightColor,
                distance);

            //cast shadow rays, dir2light aready normalized
            Ray shadowRay(ray.pointAtParameter(hit.getT()), dir2light);
            Hit shadowHit;      //blocked by another 3D object
            Hit shadowLightHit; //blocked by another light object

            bool group_hit = group->intersect(shadowRay, shadowHit, EPSILON);
            bool group_lights_hit = group->intersect(shadowRay, shadowLightHit, EPSILON);

            if ((group_hit) || (group_lights_hit))
            {
                if ((shadowHit.getT() < distance) || (shadowLightHit.getT() < distance))
                {
                    continue;
                }
            }

            diffuseColor = diffuseColor + material->shadeDiffuse(ray, hit, dir2light, lightColor);
        }
        return diffuseColor;
    }

    //compute specular color separately, may not be used
    Vector3f getSpecularColor(Ray& ray, Hit& hit)
    {
        Vector3f specularColor;
        Material* material = hit.getMaterial();

        //compute local color
        for (int l = 0; l < m_scene->getNumLights(); l++)
        {
            //compute a single color per light source
            Light* light = m_scene->getLight(l);

            Vector3f lightColor;
            Vector3f dir2light;
            float distance = 0;
            light->getIllumination(
                ray.pointAtParameter(hit.getT()),
                dir2light,
                lightColor,
                distance);

            //cast shadow rays, dir2light aready normalized
            Ray shadowRay(ray.pointAtParameter(hit.getT()), dir2light);
            Hit shadowHit;      //blocked by another 3D object
            Hit shadowLightHit; //blocked by another light object

            bool group_hit = group->intersect(shadowRay, shadowHit, EPSILON);
            bool group_lights_hit = group->intersect(shadowRay, shadowLightHit, EPSILON);

            if ((group_hit) || (group_lights_hit))
            {
                if ((shadowHit.getT() < distance) || (shadowLightHit.getT() < distance))
                {
                    continue;
                }
            }

            specularColor = specularColor + material->shadeSpecular(ray, hit, dir2light, lightColor);
        }
        return specularColor;
    }

    //call this function when a light object is hit
    Vector3f getLightColor(Ray& ray, Hit& hit)
    {
        LightObject* lightObject = hit.getLightObject();
        if (lightObject == NULL)
        {
            cout << "Warning: hit.getLightObject() == NULL. Something is wrong" << endl;
            cout << "See MCTracer::getLightColor" << endl;
            return Vector3f(0, 0, 0);
        }

        return lightObject->getColor();
    }

public:
    MCTracer(SceneParser* scene, float refr)
    {
        m_scene = scene;
        group = scene->getGroup();
        lightGroup = scene->getLightGroup();
        stop_probability = STOPPROBABILITY;
        max_depth = 0;

        NIL = new MCNode(NULL, NULL, 0.0);
        root = new MCNode(NIL, NIL, refr);
    }

    ~MCTracer()
    {
        if (root != NIL)    
            free(root);
        delete NIL;
    }

    int maximumDepth() const
    {
        return max_depth;
    }

    Vector3f traceRay(Ray& ray, Hit& hit, MCNode* current = NULL, int depth = 0, float tmin = EPSILON)
    {
        if (depth > max_depth)
            max_depth = depth;

        if (current == NULL)
            current = root;

        //clear nodes left by previous traces
        if (current->reflect_node != NIL)
        {
            free(current->reflect_node);
            current->reflect_node = NIL;
        }
        if (current->refract_node != NIL)
        {
            free(current->refract_node);
            current->refract_node = NIL;
        }

        Ray lightRay = ray;
        Hit lightHit = hit;
        bool group_intersect = group->intersect(ray, hit, tmin);
        bool light_intersect = lightGroup->intersect(lightRay, lightHit, tmin);

        if ((group_intersect) || (light_intersect))
        {
            if (hit.getT() < lightHit.getT())
            {
                //hit a normal object
                Vector3f localColor = getLocalColor(ray, hit);
                Material* material = hit.getMaterial();

                //Russian roulette
                static random_device rd;
                static mt19937 gen(rd());
                uniform_real_distribution<> dis(0, 1);

                if ((dis(gen) < stop_probability)&&(depth>5))
                    return Vector3f::clamp(localColor);
                
                if (depth > MAXDEPTH)
                {
                    return Vector3f::clamp(localColor);
                }

                auto materiatlType = material->getType();
                if (materiatlType == MIRROR)
                {
                    return Vector3f::clamp(traceReflect(ray, hit, current, depth));
                }
                else if (materiatlType == GLASS)
                {
                    Vector3f secondaryColor = traceReflectAndRefract(ray, hit, current, depth);
                    return Vector3f::clamp(secondaryColor);
                }
                else if (materiatlType == AMBIENT)
                {
                    Vector3f secondaryColor = traceAmbient(ray, hit, current, depth);
                    return Vector3f::clamp(localColor + secondaryColor);
                }
                else if (materiatlType == PHONG)
                {
                    Vector3f secondaryColor = traceReflectAndRefract(ray, hit, current, depth);
                    secondaryColor = Vector3f::pointwiseDot(secondaryColor, material->getSpecularColor());
                    return Vector3f::clamp(localColor + secondaryColor);
                }
                else if (materiatlType == GLOSSY)
                {
                    Vector3f secondaryColor = traceGlossy(ray, hit, current, depth);
                    if (isnan(secondaryColor[0]))
                        cout << "Warning: nan detected" << endl;
                    if (isinf(secondaryColor[0]))
                        cout << "Warning: inf detected" << endl;
                    if (secondaryColor[0] < 0)
                        cout << "Warning: negative color detected" << endl;
                    return Vector3f::clamp(localColor + secondaryColor);
                }
            }
            else
            {
                //hit a light object
                LightObject* light = lightHit.getLightObject();
                return light->getColor();
            }
        }
        else
        {
            return m_scene->getBackgroundColor();
        }
    }

    Vector3f computeReflect(const Vector3f& normal, const Vector3f& incoming, MCNode* current)
    {
        MCNode* reflect_node = new MCNode(NIL, NIL, current->refraction_index);
        current->reflect_node = reflect_node;
        reflect_node->parent = current;

        return (incoming - normal * 2 * Vector3f::dot(incoming, normal)).normalized();
    }

    Vector3f computeRefract(const Vector3f& normal, const Vector3f& incoming, MCNode* current, float n_material)
    {
        Vector3f V = incoming;
        Vector3f N = normal;

        float V_N = Vector3f::dot(V, N);

        if (V_N < 0)
        {
            //shoot in, use next material's refraction index
            float n_ratio = current->refraction_index / n_material;
            float delta = 1.0 - n_ratio * n_ratio * (1.0 - V_N * V_N);

            if (delta <= 0)
                return Vector3f(0, 0, 0);
            else
            {
                MCNode* refract_node = new MCNode(NIL, NIL, n_material);
                current->refract_node = refract_node;
                refract_node->parent = current;

                return (n_ratio * (V - N * V_N) - N * sqrt(delta)).normalized();
            }
        }
        else
        {
            //shoot out, find the refraction index before shooting in
            MCNode* p = current->parent;
            while (p != NULL)
            {
                if (p->refraction_index == current->refraction_index)
                    p = p->parent;
                else
                    break;
            }
            float n_previous = (p == NULL) ? 1.0 : p->refraction_index;
            float n_ratio = current->refraction_index / n_previous;
            float delta = 1.0 - n_ratio * n_ratio * (1.0 - V_N * V_N);

            if (delta <= 0)
                return Vector3f(0, 0, 0);
            else
            {
                MCNode* refract_node = new MCNode(NIL, NIL, n_previous);
                current->refract_node = refract_node;
                refract_node->parent = current;

                return (n_ratio * (V - N * V_N) + N * sqrt(delta)).normalized();
            }
        }
    }
};