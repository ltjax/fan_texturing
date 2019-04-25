[Vertex Shader]

attribute vec4 BaseU;
attribute vec4 BaseV;
attribute vec4 Coord;
attribute vec3 Offset;

varying vec4 Uvec;
varying vec4 Vvec;
varying vec3 ModelPos;
varying float Scale;

uniform float HalfTexel;
//const float HalfTexel = (1.0/4096.0)*0.5;

void main( void )
{
  ModelPos = Coord.xyz;
  
  Uvec = BaseU*Offset.z;
  Uvec.w += Offset.x;
  Vvec = BaseV*Offset.z;
  Vvec.w += Offset.y;
  
  Scale = Offset.z;
  
  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}

[Fragment Shader]
#version 150
#extension GL_EXT_texture_array : enable
//#define NO_SMOOTH_BLEND
uniform sampler2D Texture;
#ifdef RUNTIME_SHADOWS
uniform sampler2DArrayShadow ShadowTexture;
uniform mat4 ShadowMatrix;
#endif
uniform float HalfTexel;

varying vec2 Tc0;
varying vec2 Tc1;
varying vec2 Tc2;
varying vec2 Weight;
varying vec3 WorldCoord;
varying vec3 PScale;
#ifdef RUNTIME_SHADOWS
float ShadowCoeff()
{
  // Transform into texture map space
  vec4 C = ShadowMatrix*vec4(WorldCoord,1.0);
  
  // Do perspective division
  C.xyz = C.xyz / C.w;
  //C.z -= 0.001;
  
  // Set the layer in w
  C.w = 0.0;
  
  return shadow2DArray( ShadowTexture, C.xywz ).x;
}
#endif

vec2 LeftTurn( vec2 v )
{
  return vec2( -v.y, v.x  );
}

float section( float left, float right, float v )
{
  return 1.0 - step( left, v )*step(-right,-v);
}

float EdgeWeight( vec3 BarycentricCoords, float w )
{ 
  vec3 Rim = smoothstep(vec3(0.0),vec3(w),BarycentricCoords);
  return Rim.x*Rim.y*Rim.z;
}

float SeamWeight( vec3 Dist, float Width )
{
  float w =
    step(Width,Dist.x)*(1.0-step(-Width,Dist.y)) +
    step(Width,Dist.y)*(1.0-step(-Width,Dist.z)) +
    step(Width,Dist.z)*(1.0-step(-Width,Dist.x));
  
  return w;
}

vec3 ComputeDists( vec2 BarycentricCoords )
{
  const float a = 1.0/6.0;
  const float b = 1.0/3.0;
  
  const vec3 A0=vec3(-a,b,-a);
  const vec3 A1=vec3(-b,a,a);
  
  const vec2 m = vec2(1.0/3.0);
  
  vec2 st = BarycentricCoords - m;
  
  return A0*st.x+A1*st.y;
}

vec3 SquishWeights( vec3 w, float FadeWidth )
{
#ifdef NO_SMOOTH_BLEND
  w = step(0.0,w);
#else
  w = smoothstep( -FadeWidth, FadeWidth, w );
#endif
  w = w*(vec3(1.0)-w.yzx);
  w /= dot(w,vec3(1.0));
  return w;
}



vec4 tile_texture2D( sampler2D tex, vec2 coord, float scale )
{
  vec2 low = mod( coord, scale );
  vec2 high = coord-low;
  coord = high + clamp(low, HalfTexel, scale-HalfTexel);
  return texture2D( Texture, coord );
}

void main( void )
{
  const float FadeWidth = 0.01;
   
  vec3 Dist = ComputeDists(Weight);
  vec3 LocalWeight = SquishWeights( Dist, FadeWidth );
  
  vec4 Color =
    tile_texture2D( Texture, Tc0, PScale.x ) * LocalWeight.x +
    tile_texture2D( Texture, Tc1, PScale.y ) * LocalWeight.y +
    tile_texture2D( Texture, Tc2, PScale.z ) * LocalWeight.z;

  //Color.xyz *= 0.8;
  //Color.xyz += vec3(0.2);
  //Color = min(Color,vec4(1.0));
  
  //Color *= mix( 0.8, 1.0, SeamWeight( Dist, FadeWidth ));
  //Color *= mix( 0.5, 1.0, ShadowCoeff() );
  gl_FragColor =  Color;
}

[Geometry Shader]
#version 120
#extension GL_EXT_geometry_shader4 : enable

varying in vec4 Uvec[];
varying in vec4 Vvec[];
varying in vec3 ModelPos[];
varying in vec4 SampleRange[];
varying in float Scale[];

varying out vec2 Tc0;
varying out vec2 Tc1;
varying out vec2 Tc2;
varying out vec2 Weight;
varying out vec3 WorldCoord;
varying out vec3 PScale;

vec2 project( vec4 point, vec4 u, vec4 v )
{
  return vec2( dot(point,u), dot(point,v) );
}

vec2 hproject( vec3 point, vec4 u, vec4 v )
{
  return vec2( dot(point,u.xyz)+u.w, dot(point,v.xyz)+v.w );
}


void main( void )
{
  const vec3 Origin=vec3(0.0);
  
  PScale = vec3( Scale[0],Scale[1],Scale[2] );  
  Tc0 = vec2( Uvec[0].w, Vvec[0].w );
  Tc1 = hproject( ModelPos[0]-ModelPos[1], Uvec[1], Vvec[1] );
  Tc2 = hproject( ModelPos[0]-ModelPos[2], Uvec[2], Vvec[2] );
  Weight = vec2( 0.0, 0.0 );
  gl_Position = gl_PositionIn[0];
  WorldCoord = ModelPos[0];
  EmitVertex();
  
  PScale = vec3( Scale[0],Scale[1],Scale[2] );  
  Tc0 = hproject( ModelPos[1]-ModelPos[0], Uvec[0], Vvec[0] );
  Tc1 = vec2( Uvec[1].w, Vvec[1].w );
  Tc2 = hproject( ModelPos[1]-ModelPos[2], Uvec[2], Vvec[2] );
  Weight = vec2( 1.0, 0.0 );
  gl_Position = gl_PositionIn[1];
  WorldCoord = ModelPos[1];
  EmitVertex();
  
  PScale = vec3( Scale[0],Scale[1],Scale[2] );  
  Tc0 = hproject( ModelPos[2]-ModelPos[0], Uvec[0], Vvec[0] );
  Tc1 = hproject( ModelPos[2]-ModelPos[1], Uvec[1], Vvec[1] );
  Tc2 = vec2( Uvec[2].w, Vvec[2].w );
  Weight = vec2( 0.0, 1.0 );
  gl_Position = gl_PositionIn[2];
  WorldCoord = ModelPos[2];
  EmitVertex();
  
  EndPrimitive();  
}



