#pragma once

#include <vec.h>

struct Camera {
  float yawAngle = 0.0f, pitchAngle = 0.0f;
  float yawSpeed, pitchSpeed;
  MyGL_Vec3 position = MyGL_Vec3 { .x = 0.0f, .y = 0.0f, .z = 0.0f };
  float forwardSpeed, strafeSpeed, upSpeed;
  float fov = 90.0f;
  float nearPlane = 0.01f;
  float farPlane = 1000.0f;

  void look(int yaw, int pitch, float dt);
  void move(int forward, int side, int up, float dt);
  MyGL_Mat4 worldMatrix() const;
  MyGL_Mat4 viewMatrix() const;
  MyGL_Mat4 stereoViewMatrix(float ipd = 0.065, float focalDist = 15.0, bool leftEye = true) const;
  MyGL_Mat4 projectionMatrix(float aspect = 1.0f) const;
  MyGL_Vec3 lookVector();
};
