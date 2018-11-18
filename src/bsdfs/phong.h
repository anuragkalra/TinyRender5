/*
    This file is part of TinyRender, an educative rendering system.

    Designed for ECSE 446/546 Realistic/Advanced Image Synthesis.
    Derek Nowrouzezahrai, McGill University.
*/

#pragma once

#include "core/core.h"

TR_NAMESPACE_BEGIN

/**
 * Modified Phong reflectance model
 */
struct PhongBSDF : BSDF {

    std::unique_ptr<Texture < v3f>> specularReflectance;
    std::unique_ptr<Texture < v3f>> diffuseReflectance;
    std::unique_ptr<Texture < float>> exponent;
    float specularSamplingWeight;
    float scale;

    PhongBSDF(const WorldData& scene, const Config& config, const size_t& matID) : BSDF(scene, config, matID) {
        const tinyobj::material_t& mat = scene.materials[matID];

        if (mat.specular_texname.empty())
            specularReflectance = std::unique_ptr<Texture<v3f>>(new ConstantTexture3f(glm::make_vec3(mat.specular)));
        else
            specularReflectance = std::unique_ptr<Texture<v3f>>(new BitmapTexture3f(config, mat.specular_texname));

        if (mat.diffuse_texname.empty())
            diffuseReflectance = std::unique_ptr<Texture<v3f>>(new ConstantTexture3f(glm::make_vec3(mat.diffuse)));
        else
            diffuseReflectance = std::unique_ptr<Texture<v3f>>(new BitmapTexture3f(config, mat.diffuse_texname));

        exponent = std::unique_ptr<Texture<float>>(new ConstantTexture1f(mat.shininess));

        //get scale value to ensure energy conservation
        v3f maxValue = specularReflectance->getMax() + diffuseReflectance->getMax();
        float actualMax = max(max(maxValue.x, maxValue.y), maxValue.z);
        scale = actualMax > 1.0f ? 0.99f * (1.0f / actualMax) : 1.0f;

        float dAvg = getLuminance(diffuseReflectance->getAverage() * scale);
        float sAvg = getLuminance(specularReflectance->getAverage() * scale);
        specularSamplingWeight = sAvg / (dAvg + sAvg);

        components.push_back(EGlossyReflection);
        components.push_back(EDiffuseReflection);

        combinedType = 0;
        for (unsigned int component : components)
            combinedType |= component;
    }

    inline v3f reflect(const v3f& d) const {
        return v3f(-d.x, -d.y, d.z);
    }

    v3f eval(const SurfaceInteraction& i) const override {
        v3f val(0.f);

        // TODO: Add previous assignment code (if needed)
        //Check that the incoming and outgoing rays are headed in the correct directions; if not return black.
        if(Frame::cosTheta(i.wo) <= 0 || Frame::cosTheta(i.wi) <= 0) {
            return val;
        }

//        //diffuse reflectivity, the fraction of the incoming energy that is reflected diffusely
        v3f rho_d = diffuseReflectance->eval(worldData,  i);
//
//        //specular reflectivity, the fraction of the incoming energy that is reflected specularly
        v3f rho_s = specularReflectance->eval(worldData, i);
//
//        //Phong exponent. Higher values give more mirror-like specular reflection
        float n = exponent->eval(worldData, i);
//
//        //the specular angle between the perfect specular reflect direction and the lighting direction (0 if negative).
//        float alpha = glm::dot(i.wo, reflect(i.wi));
//
//        //diffuse_component = rho_d * INV_PI
//        //specular_component = (rho_s) * (n+2) * INV_TWOPI * cos^n(alpha)
//        float specular_component = 0.0f;
//        if(alpha > 0) {
//            specular_component = rho_s * (n + 2) * INV_TWOPI *
//        }
//        //phong_brdf = diffuse_component + specular_component
        float alpha = glm::dot(i.wo, reflect(i.wi));
        float specular = 0.0f;
        if(alpha > 0) {
            specular = (n + 2) * INV_PI * pow(alpha, n);
        }
        return ((rho_d / M_PI) + rho_s * specular) * Frame::cosTheta(i.wo);
        //Evaluate the Phong BRDF
        //Return the value multiplied by the cosine factor

        return val;
    }

    float pdf(const SurfaceInteraction& i) const override {
        float pdf = 0.f;

        // TODO: Add previous assignment code (if needed)

        return pdf;
    }

    v3f sample(SurfaceInteraction& i, const v2f& _sample, float* pdf) const override {
        v3f val(0.f);

        // TODO: Add previous assignment code (if needed)

        return val;
    }

    std::string toString() const override { return "Phong"; }
};

TR_NAMESPACE_END