#include "simulation.h"
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
