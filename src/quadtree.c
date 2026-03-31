#include "quadtree.h"
#include "galaxy_init.h"  /* GALAXY_G */
#include <stdlib.h>
#include <math.h>

/* ---------- pool helpers ---------- */

static int node_alloc(QuadTree *tree) {
    if (tree->count >= tree->capacity) return -1;
    int idx = tree->count++;
    QuadNode *n = &tree->nodes[idx];
    n->cx = n->cy = n->total_mass = 0.0f;
    n->x_min = n->x_max = n->y_min = n->y_max = 0.0f;
    n->children[0] = n->children[1] = n->children[2] = n->children[3] = -1;
    n->particle_idx = -1;
    return idx;
}

/* Returns quadrant index 0-3 for position (px,py) relative to node midpoint.
   bit0 = right half (x >= mid), bit1 = bottom half (y >= mid). */
static int get_quadrant(QuadNode *n, float px, float py) {
    float mid_x = (n->x_min + n->x_max) * 0.5f;
    float mid_y = (n->y_min + n->y_max) * 0.5f;
    int q = 0;
    if (px >= mid_x) q |= 1;
    if (py >= mid_y) q |= 2;
    return q;
}

/* Allocate child node q of parent (if not already allocated), set its bounds. */
static int ensure_child(QuadTree *tree, int node_idx, int q) {
    QuadNode *n = &tree->nodes[node_idx];
    if (n->children[q] != -1) return n->children[q];

    int child_idx = node_alloc(tree);
    if (child_idx == -1) return -1;

    /* Re-fetch parent — pointer remains valid since pool array never reallocates */
    n = &tree->nodes[node_idx];
    float mid_x = (n->x_min + n->x_max) * 0.5f;
    float mid_y = (n->y_min + n->y_max) * 0.5f;

    QuadNode *child = &tree->nodes[child_idx];
    child->x_min = (q & 1) ? mid_x : n->x_min;
    child->x_max = (q & 1) ? n->x_max : mid_x;
    child->y_min = (q & 2) ? mid_y : n->y_min;
    child->y_max = (q & 2) ? n->y_max : mid_y;

    n->children[q] = child_idx;
    return child_idx;
}

/* ---------- tree build ---------- */

static void quadtree_insert(QuadTree *tree, int node_idx, int pidx,
                             ParticleSystem *ps, int depth) {
    if (node_idx == -1) return;
    QuadNode *n = &tree->nodes[node_idx];

    /* Case 1: empty leaf */
    if (n->particle_idx == -1 &&
        n->children[0] == -1 && n->children[1] == -1 &&
        n->children[2] == -1 && n->children[3] == -1) {
        n->particle_idx = pidx;
        n->cx           = ps->x[pidx];
        n->cy           = ps->y[pidx];
        n->total_mass   = ps->mass[pidx];
        return;
    }

    /* Case 2: leaf with existing particle — subdivide */
    if (n->particle_idx >= 0) {
        if (depth >= MAX_TREE_DEPTH) return; /* duplicate position safety guard */

        int old = n->particle_idx;
        n->particle_idx = -1; /* promote to internal */

        int q_old   = get_quadrant(n, ps->x[old], ps->y[old]);
        int c_old   = ensure_child(tree, node_idx, q_old);
        quadtree_insert(tree, c_old, old, ps, depth + 1);

        /* Re-fetch after potential pool growth */
        n = &tree->nodes[node_idx];
        int q_new = get_quadrant(n, ps->x[pidx], ps->y[pidx]);
        int c_new = ensure_child(tree, node_idx, q_new);
        quadtree_insert(tree, c_new, pidx, ps, depth + 1);
        return;
    }

    /* Case 3: internal node */
    int q = get_quadrant(n, ps->x[pidx], ps->y[pidx]);
    int c = ensure_child(tree, node_idx, q);
    quadtree_insert(tree, c, pidx, ps, depth + 1);
}

/* Post-order pass: accumulate center-of-mass from leaves up to root. */
static void propagate_mass(QuadTree *tree, int node_idx) {
    if (node_idx == -1) return;
    QuadNode *n = &tree->nodes[node_idx];

    if (n->particle_idx >= 0) return; /* leaf: already set during insert */

    n->total_mass = 0.0f;
    n->cx = 0.0f;
    n->cy = 0.0f;

    for (int q = 0; q < 4; q++) {
        int c = n->children[q];
        if (c == -1) continue;
        propagate_mass(tree, c);
        QuadNode *child = &tree->nodes[c];
        if (child->total_mass > 0.0f) {
            n->cx         += child->cx * child->total_mass;
            n->cy         += child->cy * child->total_mass;
            n->total_mass += child->total_mass;
        }
    }

    if (n->total_mass > 0.0f) {
        n->cx /= n->total_mass;
        n->cy /= n->total_mass;
    }
}

void quadtree_build(QuadTree *tree, ParticleSystem *ps) {
    if (ps->count == 0) return;

    /* Find bounding box */
    float x_min = ps->x[0], x_max = ps->x[0];
    float y_min = ps->y[0], y_max = ps->y[0];
    for (int i = 1; i < ps->count; i++) {
        if (ps->x[i] < x_min) x_min = ps->x[i];
        if (ps->x[i] > x_max) x_max = ps->x[i];
        if (ps->y[i] < y_min) y_min = ps->y[i];
        if (ps->y[i] > y_max) y_max = ps->y[i];
    }

    /* Pad and enforce minimum size */
    float px = (x_max - x_min) * 0.01f + 1e-4f;
    float py = (y_max - y_min) * 0.01f + 1e-4f;
    x_min -= px;  x_max += px;
    y_min -= py;  y_max += py;

    /* Reset pool, allocate root */
    tree->count = 0;
    int root = node_alloc(tree);
    tree->nodes[root].x_min = x_min;
    tree->nodes[root].x_max = x_max;
    tree->nodes[root].y_min = y_min;
    tree->nodes[root].y_max = y_max;

    /* Insert all particles */
    for (int i = 0; i < ps->count; i++) {
        quadtree_insert(tree, 0, i, ps, 0);
    }

    /* Propagate center-of-mass */
    propagate_mass(tree, 0);
}

/* ---------- force calculation ---------- */

static void compute_force_node(ParticleSystem *ps, int i,
                                QuadTree *tree, int node_idx,
                                float *ax, float *ay) {
    if (node_idx == -1) return;
    QuadNode *n = &tree->nodes[node_idx];
    if (n->total_mass == 0.0f) return;

    float dx = n->cx - ps->x[i];
    float dy = n->cy - ps->y[i];
    float d2 = dx * dx + dy * dy;

    /* Leaf node */
    if (n->particle_idx >= 0) {
        if (n->particle_idx == i) return; /* skip self */
        float r2     = d2 + SOFTENING * SOFTENING;
        float inv_r  = 1.0f / sqrtf(r2);
        float inv_r3 = inv_r / r2;
        *ax += GALAXY_G * n->total_mass * dx * inv_r3;
        *ay += GALAXY_G * n->total_mass * dy * inv_r3;
        return;
    }

    /* Barnes-Hut criterion: s²/d² < theta² — squared to avoid sqrt */
    float s = n->x_max - n->x_min;
    if (d2 > 0.0f && s * s < THETA * THETA * d2) {
        float r2     = d2 + SOFTENING * SOFTENING;
        float inv_r  = 1.0f / sqrtf(r2);
        float inv_r3 = inv_r / r2;
        *ax += GALAXY_G * n->total_mass * dx * inv_r3;
        *ay += GALAXY_G * n->total_mass * dy * inv_r3;
        return;
    }

    /* Recurse into children */
    for (int q = 0; q < 4; q++) {
        compute_force_node(ps, i, tree, n->children[q], ax, ay);
    }
}

void quadtree_compute_forces(QuadTree *tree, ParticleSystem *ps) {
    for (int i = 0; i < ps->count; i++) {
        ps->ax[i] = 0.0f;
        ps->ay[i] = 0.0f;
        compute_force_node(ps, i, tree, 0, &ps->ax[i], &ps->ay[i]);
    }
}

/* ---------- lifecycle ---------- */

QuadTree* quadtree_create(int particle_capacity) {
    QuadTree *tree = (QuadTree*)malloc(sizeof(QuadTree));
    tree->capacity = particle_capacity * 8;
    tree->count    = 0;
    tree->nodes    = (QuadNode*)malloc(tree->capacity * sizeof(QuadNode));
    return tree;
}

void quadtree_destroy(QuadTree *tree) {
    if (!tree) return;
    free(tree->nodes);
    free(tree);
}
