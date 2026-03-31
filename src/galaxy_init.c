#include "galaxy_init.h"
#include <math.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static float randf(void) {
    return (float)rand() / ((float)RAND_MAX + 1.0f);
}

void galaxy_init(ParticleSystem *ps) {
    int n_bulge = (int)(GALAXY_N * GALAXY_BULGE_FRAC);
    int n_disk  = GALAXY_N - n_bulge;

    float m_bulge = GALAXY_M_TOTAL * GALAXY_BULGE_FRAC;
    float m_disk  = GALAXY_M_TOTAL * (1.0f - GALAXY_BULGE_FRAC);

    /* --- Bulge particles (Plummer profile) --- */
    for (int i = 0; i < n_bulge; i++) {
        float u = randf();
        if (u < 1e-6f) u = 1e-6f;

        float r = GALAXY_R_CORE / sqrtf(powf(u, -2.0f/3.0f) - 1.0f);
        float theta = randf() * 2.0f * (float)M_PI;

        ps->x[i] = r * cosf(theta);
        ps->y[i] = r * sinf(theta);

        float v_bulge = sqrtf(GALAXY_G * m_bulge / sqrtf(r*r + GALAXY_R_CORE*GALAXY_R_CORE));
        float phi = randf() * 2.0f * (float)M_PI;
        float scatter = 1.0f + GALAXY_DISPERSION * (randf() - 0.5f);
        ps->vx[i] = v_bulge * cosf(phi) * scatter;
        ps->vy[i] = v_bulge * sinf(phi) * scatter;

        ps->ax[i]   = 0.0f;
        ps->ay[i]   = 0.0f;
        ps->mass[i] = m_bulge / (float)n_bulge;
    }

    /* --- Disk particles (exponential profile) --- */
    for (int i = 0; i < n_disk; i++) {
        int idx = n_bulge + i;

        float u = randf();
        if (u > 1.0f - 1e-6f) u = 1.0f - 1e-6f;

        float r     = -GALAXY_R_SCALE * logf(1.0f - u);
        float theta = randf() * 2.0f * (float)M_PI;

        ps->x[idx] = r * cosf(theta);
        ps->y[idx] = r * sinf(theta);

        float xi    = r / GALAXY_R_SCALE;
        float m_enc = m_disk * (1.0f - expf(-xi) * (1.0f + xi));

        float v_circ = 0.0f;
        if (r > 1e-4f) {
            v_circ = sqrtf(GALAXY_G * m_enc / r);
        }

        float tx = -sinf(theta);
        float ty =  cosf(theta);
        float scatter = GALAXY_DISPERSION * v_circ;
        ps->vx[idx] = v_circ * tx + scatter * (randf() - 0.5f);
        ps->vy[idx] = v_circ * ty + scatter * (randf() - 0.5f);

        ps->ax[idx]   = 0.0f;
        ps->ay[idx]   = 0.0f;
        ps->mass[idx] = m_disk / (float)n_disk;
    }

    ps->count = GALAXY_N;
}
