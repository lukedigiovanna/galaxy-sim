# Galaxy N-Body Simulation — Implementation Spec

## Overview

A real-time 2D galaxy simulation using gravitational N-body dynamics.
- ~100,000 particles maximum
- Leapfrog (Störmer-Verlet) integration — energy-conserving, no drift
- Barnes-Hut quadtree — O(N log N) force calculation
- Density-grid renderer — maps particle density to pixel brightness

Built in C with Raylib.

---

## Units

Use dimensionless "simulation units" to keep numbers well-behaved for floating point:

| Quantity | Unit |
|---|---|
| Length | 1 unit ≈ 1 kpc (kiloparsec) |
| Mass | 1 unit ≈ 10^9 solar masses |
| Time | 1 unit ≈ 1 Myr (megayear) |
| G | tuned so G = 1.0 in sim units |

This avoids the enormous exponents you'd get with SI units and keeps forces in a sane range.

---

## Data Structures

### Particle Array (SoA layout)

Store particles in Structure-of-Arrays format for cache efficiency during force loops:

```c
typedef struct {
    float *x, *y;       // position
    float *vx, *vy;     // velocity
    float *ax, *ay;     // acceleration (current step)
    float *mass;
    int count;
    int capacity;
} ParticleSystem;
```

SoA means when iterating over x positions, they are contiguous in memory — avoids cache misses vs. AoS (array of structs).

### Quadtree Node

```c
typedef struct QuadNode {
    float cx, cy;           // center of mass
    float total_mass;
    float x_min, x_max;     // node boundary
    float y_min, y_max;
    int children[4];        // indices into node pool, -1 = empty
    int particle_idx;       // -1 if internal node, else index of single particle
} QuadNode;
```

Use a **flat node pool** (pre-allocated array) rather than dynamic malloc per node. Reset pool head to 0 each timestep — O(1) "free". This avoids heap fragmentation and is much faster than tree malloc/free each step.

```c
typedef struct {
    QuadNode *nodes;
    int count;
    int capacity;
} QuadTree;
```

### Density Grid

```c
typedef struct {
    float *cells;       // density accumulator [GRID_W * GRID_H]
    int width, height;
    Color *pixels;      // final RGBA output, same dimensions as render target
} DensityGrid;
```

---

## Simulation Loop

Each frame:

```
1. Build quadtree from current positions
2. Compute forces (accelerations) via Barnes-Hut tree walk
3. Leapfrog integrate: update velocities and positions
4. Rasterize particles into density grid
5. Map density -> color -> GPU texture -> draw
```

---

## Leapfrog Integration

The **kick-drift-kick** (KDK) form:

```
// Half-kick: advance velocity by half step
vx += ax * (dt / 2)
vy += ay * (dt / 2)

// Drift: advance position by full step using half-kicked velocity
x += vx * dt
y += vy * dt

// Recompute accelerations at new positions
compute_forces()

// Half-kick again with new accelerations
vx += ax * (dt / 2)
vy += ay * (dt / 2)
```

**Why this works**: The half-kick/drift/half-kick structure is symplectic — it conserves a modified energy exactly, preventing secular drift. Euler integrates energy; leapfrog conserves it.

**Timestep**: Fixed dt, tuned empirically. Start around dt = 0.01 sim-time-units. Too large → instability. Too small → slow. A variable dt (per particle, based on local acceleration) is possible later.

---

## Barnes-Hut Quadtree

### Building the Tree

```
1. Find bounding box of all particles
2. Allocate root node covering bounding box
3. For each particle:
   a. Start at root
   b. If node is empty → place particle here
   c. If node is a leaf (has 1 particle) → subdivide into 4 children,
      re-insert existing particle, then insert new particle
   d. If node is internal → recurse into appropriate child quadrant
4. On the way back up: accumulate center-of-mass and total mass
```

### Force Calculation (Tree Walk)

For particle p, walk the tree:

```
function compute_force(p, node):
    if node is empty: return
    if node is a leaf and leaf.particle != p:
        add direct pairwise force
        return

    s = node width
    d = distance from p to node center of mass

    if s/d < theta:
        // Node is "far enough" — treat as single point mass
        add force from (node.total_mass at node.cx, node.cy)
        return
    else:
        // Recurse into children
        for each child: compute_force(p, child)
```

**theta (θ)**: Opening angle parameter. θ = 0.5 is standard. Lower = more accurate, slower. Higher = faster, less accurate.

**Softening**: Always use softened gravity to prevent singularities:
```c
float r2 = dx*dx + dy*dy + SOFTENING*SOFTENING;
float inv_r3 = 1.0f / (r2 * sqrtf(r2));
ax += G * mass * dx * inv_r3;
ay += G * mass * dy * inv_r3;
```

SOFTENING ≈ 0.01–0.1 sim units (roughly the mean inter-particle spacing).

---

## Galaxy Initialization

### Two-Component Model: Bulge + Disk

**Bulge** (~20% of particles): Plummer sphere profile
```
r drawn from: r = a / sqrt(u^(-2/3) - 1)   where u = uniform(0,1), a = scale radius
angle: uniform [0, 2π]
velocity: roughly v = sqrt(G * M_bulge / r), randomized direction
```

**Disk** (~80% of particles): Exponential disk
```
r drawn by inverting exponential CDF: r = -h * ln(1 - u)   where u = uniform(0,1), h = scale height
angle: uniform [0, 2π]
velocity: circular orbit speed at radius r
    v_circ(r) = sqrt(G * M_enclosed(r) / r)
Add small random vertical (out-of-plane in 2D: tangential scatter) perturbation
```

**M_enclosed(r)**: For exponential disk, computed analytically or via lookup table.

### Circular Velocity Initialization

This is critical — without proper v_circ, particles either fly off or collapse:

```c
// For a particle at radius r in the disk
float v_circ = sqrtf(G * enclosed_mass(r) / r);
// Tangential direction (perpendicular to radial)
float tx = -sinf(angle);
float ty =  cosf(angle);
vx = v_circ * tx + small_random_perturbation;
vy = v_circ * ty + small_random_perturbation;
```

---

## Density Grid Renderer

### Rasterization

Each frame:
1. Zero out density grid cells
2. For each particle, map (x, y) in sim-space to grid cell (i, j):
   ```c
   int gi = (int)((px - view_x_min) / view_width  * GRID_W);
   int gj = (int)((py - view_y_min) / view_height * GRID_H);
   cells[gj * GRID_W + gi] += particle_mass;  // or += 1.0 for unweighted
   ```
3. (Optional) Apply a 1–2 pixel Gaussian blur to soften point artifacts
4. Find max density for normalization
5. Map density → color using a palette

### Color Palette

Map normalized density [0, 1] to color:
- 0.0 → black
- 0.05 → deep blue/purple (sparse arms)
- 0.3 → yellow-white
- 1.0 → pure white (dense core)

Use gamma correction or log scaling for density so the dim arms are visible alongside the bright core:
```c
float brightness = logf(1.0f + density * SCALE) / logf(1.0f + SCALE);
```

### GPU Upload

```c
// Each frame:
UpdateTexture(render_texture, pixels);  // push CPU pixel array to GPU
DrawTexture(render_texture, 0, 0, WHITE);
```

The pixel array is rewritten from the density grid every frame on the CPU.

---

## File Structure

```
src/
  main.c          — window init, main loop, input handling
  simulation.h/.c — particle system, leapfrog integration
  quadtree.h/.c   — Barnes-Hut tree build and force calculation
  galaxy_init.h/.c— initial condition generators (disk, bulge)
  renderer.h/.c   — density grid, color mapping, texture upload
  camera.h/.c     — pan/zoom view transform
```

---

## Controls (Planned)

| Key/Input | Action |
|---|---|
| Scroll wheel | Zoom in/out |
| Click + drag | Pan view |
| Space | Pause/resume |
| R | Reset simulation |
| +/- | Increase/decrease simulation speed (dt multiplier) |
| 1/2/3 | Switch color palettes |

---

## Performance Targets

| N | Target FPS |
|---|---|
| 10,000 | 60 fps |
| 50,000 | 30+ fps |
| 100,000 | 15+ fps (playable) |

Barnes-Hut tree build + walk is the bottleneck. If needed: OpenMP parallelism on the force loop is a natural next step (each particle's tree walk is independent).

---

## Implementation Phases

1. **Phase 1**: Particle system + leapfrog only, direct O(N²) force, N ≤ 1000, verify energy conservation
2. **Phase 2**: Barnes-Hut quadtree replaces direct force, scale to N = 50,000
3. **Phase 3**: Galaxy initialization (disk + bulge with proper velocities)
4. **Phase 4**: Density grid renderer replaces direct point drawing
5. **Phase 5**: Polish — color palettes, camera, UI overlays, perf tuning
