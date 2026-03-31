#ifndef SIMULATION_H
#define SIMULATION_H

typedef struct {
    float *x, *y;
    float *vx, *vy;
    float *ax, *ay;
    float *mass;
    int count;
    int capacity;
} ParticleSystem;

ParticleSystem* particles_create(int capacity);
void            particles_destroy(ParticleSystem *ps);

/* Forward declaration — avoids circular include with quadtree.h */
struct QuadTree;
void simulation_step(ParticleSystem *ps, struct QuadTree *tree, float dt);

#endif
