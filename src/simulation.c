#include "simulation.h"
#include "quadtree.h"
#include <stdlib.h>

ParticleSystem* particles_create(int capacity) {
    ParticleSystem *ps = (ParticleSystem*)malloc(sizeof(ParticleSystem));
    if (!ps) return NULL;

    ps->x    = (float*)malloc(capacity * sizeof(float));
    ps->y    = (float*)malloc(capacity * sizeof(float));
    ps->vx   = (float*)malloc(capacity * sizeof(float));
    ps->vy   = (float*)malloc(capacity * sizeof(float));
    ps->ax   = (float*)malloc(capacity * sizeof(float));
    ps->ay   = (float*)malloc(capacity * sizeof(float));
    ps->mass = (float*)malloc(capacity * sizeof(float));

    ps->count    = 0;
    ps->capacity = capacity;
    return ps;
}

void particles_destroy(ParticleSystem *ps) {
    if (!ps) return;
    free(ps->x);
    free(ps->y);
    free(ps->vx);
    free(ps->vy);
    free(ps->ax);
    free(ps->ay);
    free(ps->mass);
    free(ps);
}

void simulation_step(ParticleSystem *ps, struct QuadTree *tree, float dt) {
    int n = ps->count;

    /* Half-kick: v += a * dt/2 */
    for (int i = 0; i < n; i++) {
        ps->vx[i] += ps->ax[i] * (dt * 0.5f);
        ps->vy[i] += ps->ay[i] * (dt * 0.5f);
    }

    /* Drift: x += v * dt */
    for (int i = 0; i < n; i++) {
        ps->x[i] += ps->vx[i] * dt;
        ps->y[i] += ps->vy[i] * dt;
    }

    /* Rebuild tree and recompute forces at new positions */
    quadtree_build(tree, ps);
    quadtree_compute_forces(tree, ps);

    /* Half-kick: v += a_new * dt/2 */
    for (int i = 0; i < n; i++) {
        ps->vx[i] += ps->ax[i] * (dt * 0.5f);
        ps->vy[i] += ps->ay[i] * (dt * 0.5f);
    }
}
