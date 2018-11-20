/*
    This file is part of TinyRender, an educative rendering system.

    Designed for ECSE 446/546 Realistic/Advanced Image Synthesis.
    Derek Nowrouzezahrai, McGill University.
*/

#pragma once

#include <core/core.h>
#include "core/renderpass.h"
#include "tiny_obj_loader.h"
#include "integrators/path.h"

TR_NAMESPACE_BEGIN

/**
 * Global Illumination baking renderpass.
 */
struct GIPass : RenderPass {
    GLuint shader{0};

    GLuint modelMatUniform{0};
    GLuint viewMatUniform{0};
    GLuint projectionMatUniform{0};

    int m_samplePerVertex;

    std::unique_ptr<PathTracerIntegrator> m_ptIntegrator;

    explicit GIPass(const Scene& scene) : RenderPass(scene) {
        m_ptIntegrator = std::unique_ptr<PathTracerIntegrator>(new PathTracerIntegrator(scene));
        m_ptIntegrator->m_maxDepth = scene.config.integratorSettings.gi.maxDepth;
        m_ptIntegrator->m_rrProb = scene.config.integratorSettings.gi.rrProb;
        m_ptIntegrator->m_rrDepth = scene.config.integratorSettings.gi.rrDepth;
        m_samplePerVertex = scene.config.integratorSettings.gi.samplesByVertex;
    }

    virtual void buildVBO(size_t objectIdx) override {
        GLObject& obj = objects[objectIdx];

        // TODO: Implement this
        // TODO: Task 2.1.1 Precomputation Pass: Computing and Storing Radiance in VBOs
        // NOTE: see renderpass.cpp (229-251)

        // Use getPrimitiveID() and getMaterialID() to properly initialize
            //some of the SurfaceInteraction attributes

        // You can access a given object's vertex data using
            //scene.getObjectVertexPosition()
            //scene.getObjectVertexNormal()

        obj.nVerts = scene.getObjectNbVertices(objectIdx);
        obj.vertices.resize(obj.nVerts * N_ATTR_PER_VERT);

        int k= 0;

        for (int j = 0; j < obj.nVerts; j++) {
            size_t i = j;
            //printf("Object %d/%d: %d/%d\n", objectIdx+1, objects.size(), j+1, obj.nVerts);
            Sampler sampler = Sampler(260631195);

            v3f normal = scene.getObjectVertexNormal(objectIdx, i);
            v3f pos = scene.getObjectVertexPosition(objectIdx, i);

            SurfaceInteraction hit = SurfaceInteraction();
            hit.wo = v3f(0, 0, 1); //local normal
            hit.p = pos+normal*Epsilon;
            hit.primID = scene.getPrimitiveID(i);
            hit.matID = scene.getMaterialID(objectIdx, hit.primID);
            hit.shapeID = objectIdx;
            hit.frameNg = Frame(normal);
            hit.frameNs = Frame(normal);

            // Position
            obj.vertices[k + 0] = pos.x;
            obj.vertices[k + 1] = pos.y;
            obj.vertices[k + 2] = pos.z;
            Ray _ray(v3f(0), v3f(1, 0, 0));
            v3f RGB = v3f(0);

            for (int l = 0; l < m_samplePerVertex; l++) {
                RGB += m_ptIntegrator->renderExplicit(_ray, sampler, hit);
            }

            RGB /= m_samplePerVertex;
            // RGB
            obj.vertices[k + 3] = RGB.x;
            obj.vertices[k + 4] = RGB.y;
            obj.vertices[k + 5] = RGB.z;

            k += N_ATTR_PER_VERT;
        }

        // VBO
        glGenVertexArrays(1, &obj.vao);
        glBindVertexArray(obj.vao);

        glGenBuffers(1, &obj.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, obj.vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     sizeof(GLfloat) * obj.nVerts * N_ATTR_PER_VERT,
                     (GLvoid*) (&obj.vertices[0]),
                     GL_STATIC_DRAW);
    }

    bool init(const Config& config) override {
        RenderPass::init(config);

        // Create shader
        GLuint vs = compileShader("gi.vs", GL_VERTEX_SHADER);
        GLuint fs = compileShader("gi.fs", GL_FRAGMENT_SHADER);
        shader = compileProgram(vs, fs);
        glDeleteShader(vs);
        glDeleteShader(fs);

        // Create uniforms
        modelMatUniform = GLuint(glGetUniformLocation(shader, "model"));
        viewMatUniform = GLuint(glGetUniformLocation(shader, "view"));
        projectionMatUniform = GLuint(glGetUniformLocation(shader, "projection"));

        // Create vertex buffers
        objects.resize(scene.worldData.shapes.size());
        for (size_t i = 0; i < objects.size(); i++) {
            buildVBO(i);
            buildVAO(i);
        }

        return true;
    }

    void cleanUp() override {
        // Delete vertex buffers
        for (size_t i = 0; i < objects.size(); i++) {
            glDeleteBuffers(1, &objects[i].vbo);
            glDeleteVertexArrays(1, &objects[i].vao);
        }

        RenderPass::cleanUp();
    }

    void render() override {
        glBindFramebuffer(GL_FRAMEBUFFER, postprocess_fboScreen);
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // TODO: Implement this
        // TODO: Task 2.1.2 Runtime Pass: Precomputed Radiance Lookup

        // NOTE: see renderpasses/normal.h (85-94)
        // Define shader to use
        glUseProgram(shader);

        // Update camera
        glm::mat4 model, view, projection;
        camera.Update();
        camera.GetMatricies(projection, view, model);

        // Pass uniforms
        glUniformMatrix4fv(modelMatUniform, 1, GL_FALSE, &(modelMat[0][0]));
        glUniformMatrix4fv(viewMatUniform, 1, GL_FALSE, &(view[0][0]));
        glUniformMatrix4fv(projectionMatUniform, 1, GL_FALSE, &(projection[0][0]));

        // Draw
        for (auto& object : objects) {
            /**
             * 1) Bind vertex array of current object.
             * 2) Draw its triangles.
             * 3) Bind vertex array to 0.
             */
            glBindVertexArray(object.vao);
            glDrawArrays(GL_TRIANGLES, 0, object.nVerts);
            glBindVertexArray(0);
        }

        // NOTE: attributes in the VBOs are position and color
        RenderPass::render();
    }

};

TR_NAMESPACE_END
