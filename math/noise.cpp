#include "noise.h"
#include "../thirdparty/mtwister/mtwister.h"

#include <cmath>
#include <random>

namespace mbz {
namespace math {
namespace mc {

//std::random_device rd;  // For random seed
//std::mt19937 gen(rd());  // Mersenne Twister engine
//std::uniform_real_distribution<> dis(0.0, 1.0);  // Uniform distribution between 0

Vector3 randomPointOnHemisphere(MTRandWrapper& mtRand) {
  constexpr double pi = 3.14159265358979323846;

  double yaw = 2.0 * pi * mtRand.random();
  double pitch = asin(sqrt(mtRand.random()));
  Vector3 v;
  v.x = sin(pitch) * cos(yaw);
  v.y = sin(pitch) * sin(yaw);
  v.z = cos(pitch);
  return v;
}

Vector3 randomPointOnSphere(MTRandWrapper& mtRand) {
  constexpr double pi = 3.14159265358979323846;

  double yaw = 2.0 * pi * mtRand.random();
  double pitch = acos(2.0 * mtRand.random() - 1.0);

//  double yaw = 2.0 * pi * dis(gen);
//  double pitch = acos(2.0 * dis(gen) - 1.0);

  Vector3 v;
  v.x = sin(pitch) * cos(yaw);
  v.y = sin(pitch) * sin(yaw);
  v.z = cos(pitch);

  return v;
}

}
}
}
