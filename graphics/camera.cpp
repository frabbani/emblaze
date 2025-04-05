#include <vecdefs.h>

#include "camera.h"

constexpr float pi = 3.141592653589793;

void Camera::look(int yaw, int pitch, float dt) {
  if (yaw != 0) {
    float da = yawSpeed * dt;
    if (yaw < 0)
      da = -da;
    yawAngle += da;
    yawAngle = fmodf(yawAngle, 360.0f);
  }

  if (yawAngle < 0.0f)
    yawAngle += 360.0f;

  if (pitch != 0) {
    float da = pitchSpeed * dt;
    if (pitch < 0)
      da = -da;
    pitchAngle += da;
    pitchAngle = pitchAngle < -89.5f ? -89.5f : pitchAngle > 89.5f ? 89.5f : pitchAngle;

  }

}

void Camera::move(int forward, int side, int up, float dt) {
  MyGL_Vec3 look = MyGL_vec3Zero;  //MyGL_vec3Rotate( MyGL_vec3Y, MyGL_vec3Z, yawAngle * M_PI / 180.0f );
  MyGL_Vec3 right = MyGL_vec3Zero;  //MyGL_vec3Rotate( MyGL_vec3X, MyGL_vec3Z, yawAngle * M_PI / 180.0f );

  if (forward != 0) {
    look = MyGL_vec3Rotate(MyGL_vec3Y, MyGL_vec3Z, yawAngle * pi / 180.0f);
    float ds = forwardSpeed * dt;
    if (forward < 0)
      ds = -ds;
    position = MyGL_vec3Add(position, MyGL_vec3Scale(look, ds));
  }

  if (side != 0) {
    right = MyGL_vec3Rotate(MyGL_vec3X, MyGL_vec3Z, yawAngle * pi / 180.0f);
    float ds = strafeSpeed * dt;
    if (side < 0)
      ds = -ds;
    position = MyGL_vec3Add(position, MyGL_vec3Scale(right, ds));
  }

  if (up != 0) {
    float dz = upSpeed * dt;
    if (up < 0)
      dz = -dz;
    position.z += dz;
  }
}

static void axesFromAngles(float yawAngle, float pitchAngle, MyGL_Vec3 &r, MyGL_Vec3 &l, MyGL_Vec3 &u) {
  l = MyGL_vec3Rotate(MyGL_vec3Y, MyGL_vec3X, pitchAngle * pi / 180.0f);

  l = MyGL_vec3Rotate(l, MyGL_vec3Z, yawAngle * pi / 180.0f);
  r = MyGL_vec3Rotate(MyGL_vec3X, MyGL_vec3Z, yawAngle * pi / 180.0f);

  u = MyGL_vec3Norm(MyGL_vec3Cross(r, l));
}

MyGL_Mat4 Camera::worldMatrix() const {
  MyGL_Vec3 axes[3];
  axesFromAngles(yawAngle, pitchAngle, axes[0], axes[1], axes[2]);
  return MyGL_mat4World(position, axes[0], axes[1], axes[2]);
}

MyGL_Mat4 Camera::stereoViewMatrix(float ipd, float focalDist, bool leftEye) const {
  MyGL_Vec3 axes[3];
  axesFromAngles(yawAngle, pitchAngle, axes[0], axes[1], axes[2]);
  //return MyGL_mat4View(position, axes[0], axes[1], axes[2]);

  MyGL_Vec3 focalPoint = MyGL_vec3Add(position, axes[1]);  //MyGL_vec3Scale(axes[1], focalDist));
  MyGL_Vec3 eyePos = MyGL_vec3Add(position, MyGL_vec3Scale(axes[0], leftEye ? -ipd * 0.5f : ipd * 0.5f));

  MyGL_Vec3 look = MyGL_vec3Norm(MyGL_vec3Sub(focalPoint, eyePos));
  MyGL_Vec3 right = MyGL_vec3Norm(MyGL_vec3Cross(look, axes[2]));
  return MyGL_mat4View(eyePos, right, look, axes[2]);
}

MyGL_Mat4 Camera::viewMatrix() const {
  MyGL_Vec3 axes[3];
  axesFromAngles(yawAngle, pitchAngle, axes[0], axes[1], axes[2]);
  return MyGL_mat4View(position, axes[0], axes[1], axes[2]);
}

MyGL_Mat4 Camera::projectionMatrix(float aspect) const {
  return MyGL_mat4Perspective(aspect, fov * pi / 180.0f, nearPlane, farPlane);
}

MyGL_Vec3 Camera::lookVector(){
  MyGL_Vec3 axes[3];
  axesFromAngles(yawAngle, pitchAngle, axes[0], axes[1], axes[2]);
  return axes[1];
}
