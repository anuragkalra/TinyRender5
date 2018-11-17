/*
    This file is part of TinyRender, an educative rendering system.

    Designed for ECSE 446/546 Realistic/Advanced Image Synthesis.
    Derek Nowrouzezahrai, McGill University.
*/

#pragma once

#include <core/platform.h>
#include <core/core.h>
#include <core/accel.h>

TR_NAMESPACE_BEGIN

struct Scene;

/**
 * Integrator structure.
 * Stores reference to scene, random sampler, main rendering method, etc.
 */
struct Integrator {
    const Scene& scene;
    std::vector<Sampler> samplers;
    std::unique_ptr<RenderBuffer> rgb;

    explicit Integrator(const Scene& scene);
    virtual bool init();
    virtual void cleanUp();
    virtual v3f render(const Ray&, Sampler&) const = 0;
    bool save();

    /**
     * Helper functions for emitter getters.
     */
    const Emitter& getEmitterByID(int emitterID) const;
    size_t getEmitterIDByShapeID(size_t shapeID) const;
    float getEmitterPdf(const Emitter& emitter) const;

    /**
     * Retrieves BSDF at intersection point.
     */
    const BSDF* getBSDF(const SurfaceInteraction& hit) const;

    /**
     * Retrieves emission profile at intersection point, if any.
     */
    v3f getEmission(const SurfaceInteraction& hit) const;


    /**
     * Selects one emitter in the scene, returns a ref on selected emitter and PDF.
     * If only one emitter in the scene then PDF = 1.
     */
    size_t selectEmitter(float sample, float& pdf) const;

    /**
     * Samples a position on a mesh.
     * Returns position and PDF in area measure.
     */
    void sampleEmitterPosition(Sampler& sampler, const Emitter& emitter, v3f& n, v3f& pos, float& pdf) const;

    /**
     * Samples a direction on a mesh at a provided position that is generated by calling previous function.
     * Returns a direction and PDF in solid angle measure.
     */

    void sampleEmitterDirection(Sampler& sampler, const Emitter& emitter, const v3f& n, v3f& d, float& pdf) const;
};

TR_NAMESPACE_END