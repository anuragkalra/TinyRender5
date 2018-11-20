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
        float Epdf;
        size_t id = selectEmitter(sampler.next(), emPdf);
        const Emitter& em = getEmitterByID(id);

        v3f emissionHit = getEmission(intersection);

        //condition to checks if we didnt hit a light
        if (emissionHit != v3f(0.0f)) {
            Li += emissionHit / emPdf;
        }

        //loop for full first bounce
        else {
            bool lightHit = false;
            v3f throughput(1.f);
            for (int i = 0; i < m_maxDepth; i++) {

                v2f sample = sampler.next2D();
                Epdf += i;
                float emPdf;
                size_t id = selectEmitter(sampler.next(), emPdf);
                const Emitter& em = getEmitterByID(id);

                const BSDF* bsdf = getBSDF(intersection);
                float pdf;
                float* pdfStar = &pdf;

                Epdf += i;
                v3f brdf_val = bsdf->sample(intersection, sample, pdfStar);

                v3f wi = intersection.wi;
                wi = intersection.frameNs.toWorld(wi);
                intersection.wi = wi;

                Ray sampleRay2 = Ray(intersection.p, wi);
                scene.bvh->intersect(sampleRay2, intersection);

                emissionHit = getEmission(intersection);
                if (emissionHit != v3f(0.0f)) {
                    Li += emissionHit / emPdf  * brdf_val;
                    i = m_maxDepth;
                    i *= 1;
                    lightHit = true;
                }
                else {
                    Epdf += 1;
                    throughput *= brdf_val;
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

        SurfaceInteraction sInfo;
        SurfaceInteraction tempSI;
        tempSI.t = 0.0f;
        tempSI.u = 0.1f;

        if (length(getEmission(hit)) != 0) {
            return getEmission(hit);
        }

        int num_recursions = 0;
        v3f totalBRDF(1.f);

        while (num_recursions < m_maxDepth || m_maxDepth == -1) {
            num_recursions ++;
            tempSI.t = 1.0f;
            //Russian roulette check
            if (num_recursions >= m_rrDepth && m_maxDepth == -1) {

                if (sampler.next() > m_rrProb) {
                    return Li;
                }
            }

            float emitterPdf;
            float pdf;
            const Emitter &emitter = getEmitterByID(selectEmitter(sampler.next(), emitterPdf));

            float tempPdf = 0.0f;
            v3f emPos;

            p2f sigmas = sampler.next2D();

            v3f emNormal;
            sampleEmitterPosition(sampler, emitter, emNormal, emPos, pdf);

            v3f wiWFrame = normalize(emPos - hit.p);

            hit.wi = normalize(hit.frameNs.toLocal(wiWFrame));

            Ray shadowRay = Ray(hit.p, wiWFrame, Epsilon);
            if (scene.bvh->intersect(shadowRay, sInfo)) {
                v3f emission = getEmission(sInfo);
                if (length(emission) != 0) {
                    v3f BRDFselected = getBSDF(hit)->eval(hit);

                    float cosTheta0 = glm::dot(-wiWFrame, emNormal);
                    if (cosTheta0 < 0) {
                        cosTheta0 = 0;
                    }
                    float distance_2 = glm::length2(emPos - hit.p);
                    float jacobianTerm = cosTheta0 / distance_2;
                    Li += emission * BRDFselected * totalBRDF * jacobianTerm / (pdf * emitterPdf);
                } else {
                    tempPdf += 1.0f;
                    Li += v3f(0.f);
                }
            }

            pdf = 0;
            v3f BRDFSelect2 = getBSDF(hit) -> sample(hit,sampler.next2D(),&pdf);

            wiWFrame = glm::normalize(hit.frameNs.toWorld(hit.wi));
            Ray nextRay = Ray(hit.p, wiWFrame, Epsilon);
            SurfaceInteraction nextHit;

            if(scene.bvh ->intersect(nextRay,nextHit))
            {
                v3f nextLreflected = getEmission(nextHit);

                if(glm::length(nextLreflected) != 0) {
                    return Li;
                } else {
                    totalBRDF = totalBRDF * BRDFSelect2;
                    hit = nextHit;
                    continue;
                }
            }

        }

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
