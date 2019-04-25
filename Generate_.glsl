[Vertex Shader]

varying vec3 Normal;
varying vec4 ModelPos;
uniform vec4 Center;
uniform mat4 ShadowMatrix;
varying vec4 ShadowCoord;

vec4 UniformToScreen( vec4 Coord )
{
  return vec4(Coord.x*2.0-1.0,Coord.y*2.0-1.0,0.0,1.0);
}

void main( void )
{
  ShadowCoord = ShadowMatrix*gl_Vertex;

  ModelPos = gl_Vertex;
  Normal = gl_Normal;
  gl_Position = UniformToScreen(gl_ModelViewMatrix * (gl_Vertex-Center));
}

[Fragment Shader]

float snoise(vec3 P);

varying vec3 Normal;
varying vec4 ModelPos;
varying vec4 ShadowCoord;
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
  //vec3 Color = (Normal + vec3(1.0)) * 0.5;
  
  //float Sign = Check3d(ModelPos.xyz,0.03);
  //float Sign2 = Check3d(ModelPos.xyz,0.12);
  //float Sign3 = Check3d(ModelPos.xyz,0.36);
  
  //Color = Color*((0.5+0.5*Sign)*(0.9+0.1*Sign2)*(0.95+0.05*Sign3));
  
  vec3 SunDir = normalize(vec3( -0.8, -0.2, 0.7 ));
  //vec3 GrassGreen = vec3( 0.07, 0.27, 0.0 )*mix( 0.7, 1.0, MultiNoise(ModelPos.xyz*0.56));
  vec3 GrassGreen = texture2D( GrassTexture, ModelPos.xy / 70.0 ).xyz;
  vec3 SandColor = texture2D( SandTexture, ModelPos.xy / 66.0 ).xyz;
  
  float Noise = GetWeight(ModelPos.xyz);
  
  float SandW = 1.0 - smoothstep( 55.0, 80.0, ModelPos.z );
  float GrassW = clamp( Normal.z + ((snoise(ModelPos.xyz*0.3)*0.5)+0.5)*0.2, 0.0, 1.0 );
  GrassW = GrassW*GrassW; GrassW = GrassW*GrassW; GrassW = GrassW*GrassW; GrassW = GrassW*GrassW;

  vec3 PerturbedNormal = normalize(normalize(Normal) + normalize(GetWeight_d(ModelPos.xyz,0.05))*(1.0-GrassW)*0.5);
  float Light = max(dot(SunDir,PerturbedNormal),0.0);
  
  vec4 RockColor = texture2D( RockTexture, vec2(Noise,0.0) );
  
  vec3 FloorColor = mix( GrassGreen, SandColor, SandW );
  vec3 Color = mix( RockColor.xyz, FloorColor, GrassW);
  
  float f = unit_step(ShadowCoord.x)*unit_step(ShadowCoord.y);
  vec4 Shadow = shadow2DProj(ShadowTexture,ShadowCoord);
  Light*=Shadow.r;
  //vec3 Marker = mix(vec3(1.0,0.3,0.3)*mix(0.6,1.0,0.5*(cos(l*300.0)+1.0)),vec3(1.0),f);
  //vec3 Marker = mix(vec3(1.0),vec3(1.0,0.7,0.7),f);
  gl_FragColor = vec4( Light*Color, 1.0 );
}



