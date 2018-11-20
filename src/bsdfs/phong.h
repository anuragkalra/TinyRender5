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

        // TODO - COMPLETE
        v3f eval(const SurfaceInteraction& i) const override {
            v3f val(0.f);
            v3f wo = i.wo;
            v3f wi = i.wi;

            float z_in = Frame::cosTheta(i.wi);
            float z_out = Frame::cosTheta(i.wo);

            if (z_in > 0 && z_out > 0) {
                v3f rho_d = diffuseReflectance->eval(worldData, i);
                v3f rho_s = specularReflectance->eval(worldData, i);
                float n = exponent->eval(worldData, i);
                float cos_a = fmax(glm::dot(glm::normalize(reflect(i.wo)), i.wi), 0.0);
                val = rho_d * INV_PI + rho_s * INV_TWOPI * (n + 2)*pow(cos_a, n);
                return val * z_in * scale;
            }
            else {
                return v3f(0.f);
            }
        }

        // TODO - COMPLETE
        float pdf(const SurfaceInteraction& i) const override {

            v3f wOut = i.wo;
            v3f wIn = i.wi;
            v3f refl = reflect(wOut);
            v3f v = glm::toMat4(glm::quat(refl, v3f(0, 0, 1))) * v4f(wIn, 1);

            return Warp::squareToPhongLobePdf(v, exponent->eval(worldData, i));
        }

        // TODO - COMPLETE
        v3f sample(SurfaceInteraction& i, const v2f& _sample, float* pdf_param) const override {
            v3f val(0.f);
            float EXP = exponent->eval(worldData, i);
            v3f wi = Warp::squareToPhongLobe(_sample, EXP);

            v3f refl = reflect(i.wo);
            wi = glm::toMat4(glm::quat(v3f(0, 0, 1), refl)) * v4f(wi, 1);
            i.wi = wi;

            float pdf_val = pdf(i);
            *pdf_param = pdf_val;

            if (pdf_val == 0.f) {
                return v3f(0.f);
            } else {
                v3f brdf_scaling_factor = eval(i);
                val = brdf_scaling_factor / pdf_val;
                return val;
            }
        }

        std::string toString() const override { return "Phong"; }
    };

TR_NAMESPACE_END
