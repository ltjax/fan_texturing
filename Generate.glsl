[Vertex Shader]

varying vec3 NormalIn;
varying vec4 ModelPosIn;
varying vec4 ShadowCoordIn;

uniform vec4 Center;
uniform mat4 ShadowMatrix;

vec4 UniformToScreen( vec4 Coord )
{
  return vec4(Coord.x*2.0-1.0,Coord.y*2.0-1.0,0.0,1.0);
}

void main( void )
{
  ShadowCoordIn = ShadowMatrix*gl_Vertex;

  ModelPosIn = gl_Vertex;
  NormalIn = gl_Normal;
  gl_Position = UniformToScreen(gl_ModelViewMatrix * (gl_Vertex-Center));
}

[Fragment Shader]

float snoise(vec3 P);

varying vec3 Normal;
varying vec4 ModelPos;
varying vec4 ShadowCoord;
varying vec2 Weight;
uniform sampler2D RockTexture;
uniform sampler2D GrassTexture;
uniform sampler2D SandTexture;
uniform sampler2DShadow ShadowTexture;

float MultiNoise( vec3 p )
{
  float v = snoise(p*0.47)*0.5+0.5;  
  v += snoise(p*1.06)*0.25+0.25;  
  v += snoise(p*1.96)*0.125+0.125;
  return v*0.57142857;
}

float Check3d( vec3 p, float f )
{
  vec3 Frac = fract( p*f );
  Frac = Frac - vec3(0.5);
  return sign(Frac.x*Frac.y*Frac.z)*0.5 + 0.5;
}

float GetWeight( vec3 P )
{
  P.z+=P.x*0.001+P.y*0.0011;
  float Noise = MultiNoise(P*vec3(0.037,0.04,0.23)+snoise(P*0.0005)*10.0);
  return Noise;
}

vec3 GetWeight_d( vec3 P, float d )
{
  float v = GetWeight(P);
  return vec3( v-GetWeight(P+vec3(d,0.0,0.0)),v-GetWeight(P+vec3(0.0,d,0.0)),v-GetWeight(P+vec3(0.0,0.0,d)));
}

float unit_step( float x )
{
  return step( 0.0, x )*step(-1.0,-x);
}

void main( void )
{
  vec3 Color = (Normal + vec3(1.0)) * 0.5;
  float f = 0.2;
  
  float Sign = Check3d(ModelPos.xyz,0.03*f);
  float Sign2 = Check3d(ModelPos.xyz,0.12*f);
  float Sign3 = Check3d(ModelPos.xyz,0.36*f);
  
  Color = Color*((0.5+0.5*Sign)*(0.9+0.1*Sign2)*(0.95+0.05*Sign3));
  
  vec3 C=vec3(1.0-Weight.x-Weight.y,Weight.x,Weight.y);
  C=smoothstep(0.0,0.005,C);
  
  gl_FragColor = vec4( Color, 1.0 );
}

[Geometry Shader]
#version 150
#extension GL_EXT_geometry_shader4 : enable

layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;

in vec3 NormalIn[];
in vec4 ModelPosIn[];
in vec4 ShadowCoordIn[];

out vec3 Normal;
out vec4 ModelPos;
out vec4 ShadowCoord;
out vec2 Weight;

void main( void )
{  
  Weight = vec2( 0.0, 0.0 );
  gl_Position = gl_PositionIn[0];
  ModelPos = ModelPosIn[0];
  Normal = NormalIn[0];
  ShadowCoord = ShadowCoordIn[0];
  EmitVertex();
  
  Weight = vec2( 1.0, 0.0 );
  gl_Position = gl_PositionIn[1];
  ModelPos = ModelPosIn[1];
  Normal = NormalIn[1];
  ShadowCoord = ShadowCoordIn[1];
  EmitVertex();

  Weight = vec2( 0.0, 1.0 );
  gl_Position = gl_PositionIn[2];
  ModelPos = ModelPosIn[2];
  Normal = NormalIn[2];
  ShadowCoord = ShadowCoordIn[2];
  EmitVertex();
  
  EndPrimitive();  
}

