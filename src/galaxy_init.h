#ifndef GALAXY_INIT_H
#define GALAXY_INIT_H

#include "simulation.h"

#define GALAXY_N          10000
#define GALAXY_G          1.0f
#define GALAXY_M_TOTAL    1.0f
#define GALAXY_BULGE_FRAC 0.20f
#define GALAXY_R_CORE     0.5f
#define GALAXY_R_SCALE    2.0f
#define GALAXY_DISPERSION 0.05f

void galaxy_init(ParticleSystem *ps);

#endif
