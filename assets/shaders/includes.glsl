#ifdef TRANSFORM

struct MyGL
{
  mat4 matProj;
  mat4 matView;
  mat4 matWorld;
  mat4 matProjViewWorld;
};

uniform MyGL mygl;

#define P    mygl.matProj
#define V    mygl.matView
#define W    mygl.matWorld
#define PVW  mygl.matProjViewWorld

#endif

#ifdef __vert__
#define vary out
#endif


#ifdef __frag__
#define vary in
#endif


#ifdef VTX_P_C

#ifdef __vert__
layout(location = 0) in vec4 vtx_p;
layout(location = 1) in vec4 vtx_c;
#endif

#endif


#ifdef VTX_P_T

#ifdef __vert__
layout(location = 0) in vec4 vtx_p;
layout(location = 1) in vec4 vtx_t;
#endif

#endif

#ifdef VTX_P_T2

#ifdef __vert__
layout(location = 0) in vec4 vtx_p;
layout(location = 1) in vec4 vtx_t0;
layout(location = 2) in vec4 vtx_t1;
#endif

#endif


#ifdef VTX_P_N_T2

#ifdef __vert__
layout(location = 0) in vec4 vtx_p;
layout(location = 1) in vec4 vtx_n;
layout(location = 2) in vec4 vtx_t0;
layout(location = 3) in vec4 vtx_t1;
#endif

#endif