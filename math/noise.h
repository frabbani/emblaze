#pragma once

#include "../thirdparty/mtwister/mtwister.h"

#include <inttypes.h>
#include "vector.h"

namespace mbz {
namespace math {
namespace mc {

Vector3 randomPointOnHemisphere(MTRandWrapper& mtRand);
Vector3 randomPointOnSphere(MTRandWrapper& mtRand);

}
}
}

/*
 void generate_random_point(double* x, double* y, double* z) {
 // Random yaw angle between 0 and 360 degrees
 double yaw = random_double(0, 2 * PI);

 // Random r between 0 and 1 for pitch angle distribution
 double r = random_double(0, 1);

 // Pitch angle between 0 and pi/2 radians, adjusted for uniform distribution
 double pitch = asin(sqrt(r));  // pitch = arcsin(sqrt(r))

 // Convert spherical coordinates (pitch, yaw) to Cartesian coordinates (x, y, z)
 *x = sin(pitch) * cos(yaw);
 *y = sin(pitch) * sin(yaw);
 *z = cos(pitch);
 }
 */
