#ifndef QUADTREE_H
#define QUADTREE_H

#include "simulation.h"

#define THETA          0.7f
#define SOFTENING      0.05f
#define MAX_TREE_DEPTH 30

typedef struct {
    float cx, cy;
    float total_mass;
    float x_min, x_max;
    float y_min, y_max;
    int   children[4];
    int   particle_idx;
} QuadNode;

typedef struct QuadTree {
    QuadNode *nodes;
    int       count;
    int       capacity;
} QuadTree;

QuadTree* quadtree_create(int particle_capacity);
void      quadtree_destroy(QuadTree *tree);
void      quadtree_build(QuadTree *tree, ParticleSystem *ps);
void      quadtree_compute_forces(QuadTree *tree, ParticleSystem *ps);

#endif
