#include <cmath>
#include <cstring>

#include "vector.h"

//extern int printf(const char *format, ...);

using namespace::mbz::math;

float mbz::math::sqrtf( float f ){
  return std::sqrt(f);
}

Vector2 Matrix2::operator *(const Vector2 &rhs) const {
  Vector2 result;
  result.x = e00 * rhs.x + e01 * rhs.y;
  result.y = e10 * rhs.x + e11 * rhs.y;
  return result;

}

float Matrix2::determinant() const {
  return e00 * e11 - e01 * e10;
}

Matrix2 Matrix2::inverted() const {
  Matrix2 inverse;
  float det = determinant();

  if (det == 0.0f) {
    return inverse;
  }

  float invDet = 1.0f / det;
  inverse.e00 = +e11 * invDet;
  inverse.e01 = -e01 * invDet;
  inverse.e10 = -e10 * invDet;
  inverse.e11 = +e00 * invDet;

  return inverse;
}

// Matrix3
Vector3 Matrix3::operator *(const Vector3 &rhs) const {
  return Vector3 { e00 * rhs.x + e01 * rhs.y + e02 * rhs.z, e10 * rhs.x + e11 * rhs.y + e12 * rhs.z, e20 * rhs.x + e21 * rhs.y + e22 * rhs.z };
}

float Matrix3::determinant() const {
  float det = e00 * (e11 * e22 - e12 * e21) - e01 * (e10 * e22 - e12 * e20) + e02 * (e10 * e21 - e11 * e20);
  return det;
}

Matrix3 Matrix3::inverted() const {
  Matrix3 inverse;

  float det = determinant();

  // Check if determinant is zero
  if (det == 0.0f) {
    return inverse;
  }

  // Calculate adjugate matrix
  float invDet = 1.0f / det;
  inverse.e00 = (e11 * e22 - e12 * e21) * invDet;
  inverse.e01 = (e02 * e21 - e01 * e22) * invDet;
  inverse.e02 = (e01 * e12 - e02 * e11) * invDet;
  inverse.e10 = (e12 * e20 - e10 * e22) * invDet;
  inverse.e11 = (e00 * e22 - e02 * e20) * invDet;
  inverse.e12 = (e02 * e10 - e00 * e12) * invDet;
  inverse.e20 = (e10 * e21 - e11 * e20) * invDet;
  inverse.e21 = (e01 * e20 - e00 * e21) * invDet;
  inverse.e22 = (e00 * e11 - e01 * e10) * invDet;

  return inverse;
}
