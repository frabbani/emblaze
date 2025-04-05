#if 0
Name "Vertex Position and Color"

Passes "Main"

#endif


Main {

/**
Cull Back
Blend SrcAlpha OneMinusSrcAlpha Add
Depth LEqual
**/

#define TRANSFORM
#define VTX_P_C
#include "includes.glsl"

vary vec4 var_c;

#ifdef __vert__

void main(){
  gl_Position = PVW * vtx_p;
  var_c = vtx_c;
}

#endif

#ifdef __frag__

void main(){
  gl_FragData[0] = var_c;
}

#endif

}