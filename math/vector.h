#pragma once

#include <type_traits>

namespace mbz {
namespace math {

float sqrtf(float);

const float tol = 1e-8f;
const float tolSq = 1e-16f;
const float NaN = 1.0f / 0.0f;
constexpr float Pi = 3.1415927f;  //32 bit precision

struct v2 {
  union {
    struct {
      float x, y;
    };
    float xy[2];
    float es[2];
  };

  static constexpr int dim = 2;
};

struct v3 {
  union {
    struct {
      float x, y, z;
    };
    float xyz[3];
    float es[3];

  };

  static constexpr int dim = 3;
};

struct v4 {
  union {
    struct {
      float x, y, z, w;
    };
    float xyzw[4];
    float es[4];
  };

  static constexpr int dim = 4;
};

template<typename T>
struct Vector : public T {
 public:
  static_assert( std::is_same<T, v2>::value || std::is_same<T, v3>::value || std::is_same<T, v4>::value, "invalid vector type." );
  Vector() {
    for (int i = 0; i < T::dim; i++)
      this->es[i] = 0.0f;
  }

  Vector(const Vector<T> &rhs) {
    for (int i = 0; i < T::dim; i++)
      this->es[i] = rhs.es[i];
  }

  Vector<T>& operator =(const Vector<T> &rhs) {
    for (int i = 0; i < T::dim; i++)
      this->es[i] = rhs.es[i];
    return *this;
  }

  Vector<T>& operator =(const T &rhs) {
    for (int i = 0; i < T::dim; i++)
      this->es[i] = rhs.es[i];
    return *this;
  }

  bool operator >(const Vector<T> &rhs) const {
    for (int i = 0; i < T::dim; i++)
      if (this->es[i] < rhs.es[i])
        return false;
    return true;
  }

  bool operator <(const Vector<T> &rhs) const {
    for (int i = 0; i < T::dim; i++)
      if (this->es[i] > rhs.es[i])
        return false;
    return true;
  }

  bool operator ==(const Vector<T> &rhs) const {
    for (int i = 0; i < T::dim; i++)
      if (this->es[i] < rhs.es[i] || this->es[i] > rhs.es[i])
        return false;
    return true;
  }

  bool closeTo(const Vector<T> &rhs, float range) const {
    for (int i = 0; i < T::dim; i++) {
      float dif = this->es[i] - rhs.es[i];
      if (dif < range || dif > range)
        return false;
    }
    return true;
  }

  Vector<T> operator +(const Vector<T> &rhs) const {
    Vector<T> r;
    for (int i = 0; i < T::dim; i++)
      r.es[i] = this->es[i] + rhs.es[i];
    return r;
  }

  Vector<T>& operator +=(const Vector<T> &rhs) {
    for (int i = 0; i < T::dim; i++)
      this->es[i] += rhs.es[i];
    return *this;
  }

  Vector<T> operator -(const Vector<T> &rhs) const {
    Vector<T> r;
    for (int i = 0; i < T::dim; i++)
      r.es[i] = this->es[i] - rhs.es[i];
    return r;
  }

  Vector<T>& operator -=(const Vector<T> &rhs) {
    for (int i = 0; i < T::dim; i++)
      this->es[i] -= rhs.es[i];
    return *this;
  }

  Vector<T> operator *(float rhs) const {
    Vector<T> r;
    for (int i = 0; i < T::dim; i++)
      r.es[i] = this->es[i] * rhs;
    return r;
  }

  Vector<T>& operator *=(float rhs) {
    for (int i = 0; i < T::dim; i++)
      this->es[i] *= rhs;
    return *this;
  }

  float dot(const Vector<T> &rhs) const {
    float sum = 0.0f;
    for (int i = 0; i < T::dim; i++)
      sum += this->es[i] * rhs.es[i];
    return sum;
  }

  float lengthSq() const {
    return this->dot(*this);
  }

  Vector<T> point(const Vector<T> &other) const {
    return other - *this;
  }

  float length() const {
    float s = lengthSq();
    if (s > tolSq)
      return sqrtf(s);
    return 0.0f;
  }

  Vector<T> normalized() const {
    float len = length();
    if (0.0f == len)
      return Vector<T>();
    float invLen = 1.0f / len;
    Vector<T> r(*this);
    for (int i = 0; i < T::dim; i++)
      r.es[i] *= invLen;
    return r;
  }

  Vector<T>& normalize() {
    *this = this->normalized();
    return *this;
  }

};

template<typename T>
Vector<T> operator *(float lhs, const Vector<T> &rhs) {
  static_assert( std::is_same<T, v2>::value || std::is_same<T, v3>::value || std::is_same<T, v4>::value, "vector mul invalid type." );
  return rhs * lhs;
}

struct Vector2 : public Vector<v2> {
  Vector2(const Vector<v2> &other)
      :
      Vector<v2>(other) {
  }

  Vector2(float x_ = 0.0f, float y_ = 0.0f) {
    this->x = x_;
    this->y = y_;
  }
};

struct Vector3 : public Vector<v3> {
  Vector3(const Vector<v3> &other)
      :
      Vector<v3>(other) {
  }

  Vector3(float x_ = 0.0f, float y_ = 0.0f, float z_ = 0.0f) {
    this->x = x_;
    this->y = y_;
    this->z = z_;
  }

  Vector3 point(const Vector3 &other) const {
    return Vector3(Vector<v3>::point(other));
  }

  Vector3 cross(const Vector3 &rhs) const {
    Vector3 r;
    r.xyz[0] = (xyz[1] * rhs.xyz[2] - xyz[2] * rhs.xyz[1]);
    //r.xyz[1] = -(xyz[0] * rhs.xyz[2] - xyz[2] * rhs.xyz[0]);
    r.xyz[1] = (xyz[2] * rhs.xyz[0] - xyz[0] * rhs.xyz[2]);
    r.xyz[2] = (xyz[0] * rhs.xyz[1] - xyz[1] * rhs.xyz[0]);
    return r;
  }
};

//Vector3 operator *(float lhs, const Vector3 &rhs) {
//  return rhs * lhs;
//}

struct Vector4 : public Vector<v4> {
  Vector4(const Vector<v4> &other)
      :
      Vector<v4>(other) {
  }
  Vector4(float x_ = 0.0f, float y_ = 0.0f, float z_ = 0.0f, float w_ = 0.0f) {
    this->x = x_;
    this->y = y_;
    this->z = z_;
    this->w = w_;
  }
};

struct m2 {
  union {
    struct {
      float e00, e01;
      float e10, e11;
    };
    float es[2][2];
  };
  static constexpr int dim = 2;
};

struct m3 {
  union {
    struct {
      float e00, e01, e02;
      float e10, e11, e12;
      float e20, e21, e22;
    };
    float es[3][3];
  };
  static constexpr int dim = 3;
};

struct m4 {
  union {
    struct {
      float e00, e01, e02, e03;
      float e10, e11, e12, e13;
      float e20, e21, e22, e23;
      float e30, e31, e32, e33;
    };
    float es[4][4];
  };
  static constexpr int dim = 4;
};

template<typename T>
struct Matrix : public T {
  static_assert( std::is_same<T, m2>::value || std::is_same<T, m3>::value || std::is_same<T, m4>::value, "invalid matrix type." );

  Matrix() {
    for (int i = 0; i < T::dim; i++)
      for (int j = 0; j < T::dim; j++)
        this->es[i][j] = 0.0f;
  }

  Matrix(const Matrix<T> &other) {
    for (int i = 0; i < T::dim; i++)
      for (int j = 0; j < T::dim; j++)
        this->es[i][j] = other.es[i][j];
  }

  Matrix<T>& operator =(const Matrix<T> &rhs) {
    for (int i = 0; i < T::dim; i++)
      for (int j = 0; j < T::dim; j++)
        this->es[i][j] = rhs.es[i][j];
    return *this;
  }

  Matrix<T>& scale(float by) {
    for (int i = 0; i < T::dim; i++)
      for (int j = 0; j < T::dim; j++)
        this->es[i][j] = i == j ? by : 0.0f;
    return *this;
  }

  Matrix<T>& identity() {
    return scale(1.0f);
  }

  Matrix<T> operator *(const Matrix<T> &rhs) const {
    Matrix<T> m;
    for (int i = 0; i < T::dim; i++)
      for (int j = 0; j < T::dim; j++) {
        m.es[i][j] = 0;
        for (int k = 0; k < T::dim; k++)
          m.es[i][j] += this->es[i][k] * rhs.es[k][j];
      }
    return m;
  }

  Matrix<T> transposed() const {
    Matrix<T> m;
    for (int i = 0; i < T::dim; i++)
      for (int j = 0; j < T::dim; j++)
        m.es[i][j] = this->es[j][i];
    return m;
  }

};

struct Matrix2 : public Matrix<m2> {

  Matrix2()
      :
      Matrix<m2>() {
  }
  Matrix2(const Matrix<m2> &other)
      :
      Matrix<m2>(other) {
  }

  Vector2 operator *(const Vector2 &rhs) const;
  float determinant() const;
  Matrix2 inverted() const;
};

struct Matrix3 : public Matrix<m3> {

  Matrix3()
      :
      Matrix<m3>() {
  }
  Matrix3(const Matrix<m3> &other)
      :
      Matrix<m3>(other) {
  }

  Vector3 operator *(const Vector3 &rhs) const;
  float determinant() const;
  Matrix3 inverted() const;

};

}
}
