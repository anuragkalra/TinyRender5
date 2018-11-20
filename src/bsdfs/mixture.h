/*
    This file is part of TinyRender, an educative rendering system.

    Designed for ECSE 446/546 Realistic/Advanced Image Synthesis.
    Derek Nowrouzezahrai, McGill University.
*/

#pragma once

#include "core/core.h"

TR_NAMESPACE_BEGIN

/**
 * Modified Phong reflectance model + Diffuse
 */
struct MixtureBSDF : BSDF {
    std::unique_ptr<Texture < v3f>> specularReflectance;
    std::unique_ptr<Texture < v3f>> diffuseReflectance;
    std::unique_ptr<Texture < float>> exponent;
    float specularSamplingWeight;
    float scale;

    MixtureBSDF(const WorldData& scene, const Config& config, const size_t& matID) : BSDF(scene, config, matID) {
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

    //Common BRDF mixture model combines and diffuse and glossy (Phong) reflection effects
    //Two Implementation strategies
        //1. Explicitly treat the mixture BRDF as a sum of the two individual BRDFs

        //2. Stochastic interpretation of the mixture using Russian Roulette
            //Stochastically choose between performing EITHER diffuse or glossy reflection
            //Purely diffuse 50% of the time
            //Purely specular 50% of the time
    // TODO - COMPLETE
    v3f eval(const SurfaceInteraction& i) const override {
        v3f val = v3f(0);
        v3f wo = i.wo;
        v3f wi = i.wi;
        float z_in = Frame::cosTheta(wi);
        float z_out = Frame::cosTheta(wo);
        if (z_in > 0 && z_out > 0) { //check correct direction
            v3f rho_d = diffuseReflectance->eval(worldData, i);
            v3f rho_s = specularReflectance->eval(worldData, i);
            float n = exponent->eval(worldData, i);
            float cos_a = fmax(glm::dot(glm::normalize(reflect(wo)), wi), 0.0);
            cos_a = fmin(1, cos_a); //make sure cos_a is not more than 1 due to precision errors
            val = rho_d * INV_PI + rho_s * INV_TWOPI * (n + 2)*pow(cos_a, n);
            return val * z_in * scale;
        }
        else {
            return v3f(0);
        }
    }

    // TODO - COMPLETE
    float pdf(const SurfaceInteraction& i) const override {
        float pdf = 0.f;

        int expo = exponent->eval(worldData, i);

        v3f wrW = normalize(i.frameNs.toWorld(reflect(i.wo)));
        Frame wrFrame = Frame(wrW);
        v3f wiR =  wrFrame.toLocal(i.frameNs.toWorld(i.wi));
        float specularPdf = max(Warp :: squareToPhongLobePdf(wiR, expo),0.f);  // TODO: put this back to reflected frame
        //float specularPdf = max(Warp :: squareToPhongLobePdf(i.wi, expo),0.f);  // TODO: put this back to reflected frame
        float diffusePdf = max(Warp::squareToCosineHemispherePdf(i.wi),0.f);
        pdf = specularPdf * specularSamplingWeight + diffusePdf *(1-specularSamplingWeight);

        return pdf;
    }

    // TODO - need to do
    v3f sample(SurfaceInteraction& i, const v2f& _sample, float* pdf_param) const override {
        v3f val(0.f);

        if (_sample.x < specularSamplingWeight) { //do specular
            v2f new_sample = v2f(_sample.x / specularSamplingWeight, _sample.y);
            v3f wi = Warp::squareToPhongLobe(new_sample, exponent->eval(worldData, i));
            v3f refl = reflect(i.wo); //get reflection dir in local coords
            wi = glm::toMat4(glm::quat(v3f(0, 0, 1), refl)) * v4f(wi, 1); //rotate the lobe to be around the reflection dir in local
            i.wi = wi; //set the i.wi to be rotated around the relection of wo
        }
        else { //do diffuse
            v2f new_sample = v2f((_sample.x-specularSamplingWeight) / (1-specularSamplingWeight), _sample.y);
            i.wi = Warp::squareToCosineHemisphere(new_sample);
        }
        float pdf_val = pdf(i);
        *pdf_param = pdf_val;
        if (pdf_val == 0.f)// check if pdf val is well defined
            val = v3f(0.f);
        else
            return eval(i) / pdf_val;

    }

    std::string toString() const override { return "Mixture"; }
};

TR_NAMESPACE_END