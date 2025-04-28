#if 0
Name "Vertex Position and Texture"

Passes "Main"

#endif


Main {

/**
Cull Back
Blend SrcAlpha OneMinusSrcAlpha Add
Depth LEqual
**/

#define TRANSFORM
#define VTX_P_T
#include "includes.glsl"

vary vec4 var_t;

#ifdef __vert__

void main(){
  gl_Position = PVW * vtx_p;
  var_t = vtx_t;
}

#endif

#ifdef __frag__

layout(binding = 0) uniform sampler2D tex;
layout(binding = 1) uniform sampler2D lightmap;


void main(){
  vec3 c = texture( tex, var_t.xy ).rgb * texture( lightmap, var_t.zw ).rgb;
  gl_FragData[0] = vec4( c, 1.0 );
}

#endif

}