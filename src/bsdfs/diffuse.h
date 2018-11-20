/*
    This file is part of TinyRender, an educative rendering system.

    Designed for ECSE 446/546 Realistic/Advanced Image Synthesis.
    Derek Nowrouzezahrai, McGill University.
*/

#pragma once

#include "core/core.h"

TR_NAMESPACE_BEGIN

/**
 * Perfectly diffuse, Lambertian reflectance model
 */
    struct DiffuseBSDF : BSDF {
        std::unique_ptr<Texture < v3f>> albedo;

        DiffuseBSDF(const WorldData& scene, const Config& config, const size_t& matID) : BSDF(scene, config, matID) {
            const tinyobj::material_t& mat = scene.materials[matID];

            if (mat.diffuse_texname.empty())
                albedo = std::unique_ptr<Texture<v3f>>(new ConstantTexture3f(glm::make_vec3(mat.diffuse)));
            else
                albedo = std::unique_ptr<Texture<v3f>>(new BitmapTexture3f(config, mat.diffuse_texname));

            components.push_back(EDiffuseReflection);

            combinedType = 0;
            for (size_t i = 0; i < components.size(); ++i)
                combinedType |= components[i];
        }

        // TODO - COMPLETE
        v3f eval(const SurfaceInteraction& i) const override {
            v3f val(0.f);

            if(Frame::cosTheta(i.wo) <= 0 || Frame::cosTheta(i.wi) <= 0) {
                return val;
            }

            return (albedo->eval(worldData, i)) * INV_PI * max(0.f, Frame::cosTheta(i.wi));
        }

        // TODO - COMPLETE
        float pdf(const SurfaceInteraction& i) const override {
            float pdf = 0.f;

            if(Frame::cosTheta(i.wo) <= 0 || Frame::cosTheta(i.wi) <= 0) {
                return pdf;
            }
            pdf = max(i.wi.z * INV_PI, 0.f);
            return pdf;
        }

        // TODO - COMPLETE
        v3f sample(SurfaceInteraction& i, const v2f& sample, float* pdf_param) const override {
            v3f val(0.f);

            i.wi = Warp::squareToCosineHemisphere(sample);

            float pdf_val = pdf(i);
            *pdf_param = pdf_val;

            v3f brdf_scaling_factor = eval(i);

            if (pdf_val > 0.f) {
                val = (brdf_scaling_factor / pdf_val);
            } else {
                val = v3f(0.f);
            }
            return val;
        }

        std::string toString() const override { return "Diffuse"; }
    };

TR_NAMESPACE_END
