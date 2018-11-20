/*
    This file is part of TinyRender, an educative rendering system.

    Designed for ECSE 446/546 Realistic/Advanced Image Synthesis.
    Derek Nowrouzezahrai, McGill University.
*/

#pragma once

TR_NAMESPACE_BEGIN

/**
 * Path tracer integrator
 */
struct PathTracerIntegrator : Integrator {
    explicit PathTracerIntegrator(const Scene& scene) : Integrator(scene) {
        m_isExplicit = scene.config.integratorSettings.pt.isExplicit;   //implicit of explicit boolean toggle
        m_maxDepth = scene.config.integratorSettings.pt.maxDepth;   //maximum path depth
        m_rrDepth = scene.config.integratorSettings.pt.rrDepth; //Russian roulette probability (e.g. 0.95 means a 95% chance of recursion)
        m_rrProb = scene.config.integratorSettings.pt.rrProb;   //Path depth at which Russian roulette path termination is employed
    }


    v3f renderImplicit(const Ray& ray, Sampler& sampler, SurfaceInteraction& hit) const {
        v3f Li(0.f);
        SurfaceInteraction intersection = hit;
        v3f throughput(1.0f);
        v2f sample = sampler.next2D();

        float emPdf;
        size_t id = selectEmitter(sampler.next(), emPdf);
        const Emitter& em = getEmitterByID(id);

        //if we hit a light just return light value
        v3f emissionHit = getEmission(intersection);
        if (emissionHit != v3f(0.0f)) {
            Li += emissionHit / emPdf;
        }
            //if we dont hit light then we do our first bounce
        else {
            bool lightHit = false;
            v3f throughput(1.f);
            for (int i = 0; i < m_maxDepth; i++) {

                v2f sample = sampler.next2D();
                float emPdf;
                size_t id = selectEmitter(sampler.next(), emPdf);
                const Emitter& em = getEmitterByID(id);

                const BSDF* bsdf = getBSDF(intersection);
                float pdf;
                float* pdfPointer = &pdf;
                v3f fr = bsdf->sample(intersection, sample, pdfPointer);
                v3f wi = intersection.wi;
                wi = intersection.frameNs.toWorld(wi);
                intersection.wi = wi;

                Ray sampleRay = Ray(intersection.p, wi);
                scene.bvh->intersect(sampleRay, intersection);

                //if our bounce hits light return light
                emissionHit = getEmission(intersection);
                if (emissionHit != v3f(0.0f)) {
                    Li += emissionHit / emPdf  * fr;
                    i = m_maxDepth; //if we hit a light we  get our stuff  set the loop to max so we end
                    lightHit = true;
                }
                else {
                    throughput *= fr;
                }

            }
            if (lightHit == false) {
                return v3f(0.f);
            }
        }

        return Li;
    }

    //Surface area importance sampling for the direct lighting contribution
    //BRDF importance sampling for the indirect lighting contribution
    v3f renderExplicit(const Ray& ray, Sampler& sampler, SurfaceInteraction& hit) const {
        v3f Li(0.f);
        //Pseudocode

        /**
         * direct illumination
         * for all lights {
         *  L_d += contribution from light //use surface area importance sampling
         * }
         */

        // TODO: Implement this
        //use sampleEmitterPosition() to retrieve a uniformly sampled point on an arbitrary mesh emitter
            //along with corresponding normal, sampling PDF, and radiance
        //

        // TODO: Task 1.3.2 Russian Roulette Path Termination
        //m_rrDepth to determine whether to employ Russian Roulette termination at your current path vertex
        //m_rrProb as your RR termination probability
        return Li;
    }

    //Notes:
    //m_maxDepth is to stop the recursion to a fixed number of bounces
    //m_rrDepth is the minimum number of bounces before Russian Roulette starts
    //To enable Russian Roulette maxDepth should be set to -1

    v3f render(const Ray& ray, Sampler& sampler) const override {
        Ray r = ray;
        SurfaceInteraction hit;

        if (scene.bvh->intersect(r, hit)) {
            if (m_isExplicit)
                return this->renderExplicit(ray, sampler, hit);
            else
                return this->renderImplicit(ray, sampler, hit);
        }
        return v3f(0.0);
    }

    int m_maxDepth;     // Maximum number of bounces
    int m_rrDepth;      // When to start Russian roulette
    float m_rrProb;     // Russian roulette probability
    bool m_isExplicit;  // Implicit or explicit
};

TR_NAMESPACE_END
