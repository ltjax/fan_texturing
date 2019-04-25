[Vertex Shader]

varying vec2 TexCoord;
void main()
{
  vec2 Vertex = gl_Vertex.xy;
  Vertex = Vertex*2.0 - vec2(1.0);
  TexCoord = Vertex;


  gl_Position.xy = Vertex;
  gl_Position.zw = vec2(0.0,1.0);
}


[Fragment Shader]

varying vec2 TexCoord;
float snoise(vec2 P);

float mnoise(vec2 P)
{
  float sum = 1.0 + 0.5 + 0.25 + 0.125;
  return (snoise(P)+0.5*snoise(P*2.0)+0.25*snoise(P*4.0)+0.125*snoise(P*8.0))/sum;
}

float m2noise(vec2 P)
{
  return (mnoise(P)+0.0625*mnoise(P*16.0))/(1.0+0.0625);
}

float sinwarp( float x )
{
  const float pi = 3.1415926535897932384626433832795;
  
  x = (x-0.5)*pi;
  x = sin(x);
  //x = x*x*x;
  return x*0.5+0.5;
}

vec3 tri_gradient( vec3 c0, vec3 c1, vec3 c2, float x0, float x )
{
  return vec3(0.0,0.0,0.0);
}

float major_blend_value( vec2 X )
{ 
  vec2 C = X*vec2(5.0,11.0);
  float Value = m2noise(C);
  Value = Value*0.5 + 0.5;
  //Value = pow(Value,4.5);
  Value = sinwarp(Value);
  Value = smoothstep( 0.2, 0.65, Value );
  return Value;
}

float low_value( vec2 C )
{
  return m2noise(C*33.7+vec2(0.14,0.67));
}

float high_value( vec2 C )
{
  return m2noise(C*27.7+vec2(0.44,0.27));
  
}

void main()
{
  float d=0.009;
  float Value = major_blend_value(TexCoord);
  float ValueX = major_blend_value(TexCoord+vec2(d,0.0));
  float ValueY = major_blend_value(TexCoord+vec2(0.0,d));
  vec3 normal = normalize( vec3((Value-ValueX),(Value-ValueY),0.8) );
  
  
  float ds=0.01;
  float LowValue = low_value(TexCoord);
  float LowValueX = low_value(TexCoord+vec2(ds,0.0));
  float LowValueY = low_value(TexCoord+vec2(0.0,ds));
  normal += normalize( vec3(LowValue-LowValueX,LowValue-LowValueY,0.4) )*(1.0-Value);
  vec3 LowColor = mix(vec3(142.0,130.0,120.0),vec3(85.0,88.0,41.0),LowValue) / 255.0;
  
  float HighValue = high_value(TexCoord);
  float HighValueX = high_value(TexCoord+vec2(ds,0.0));
  float HighValueY = high_value(TexCoord+vec2(0.0,ds));
  normal += normalize( vec3(LowValue-LowValueX,LowValue-LowValueY,0.5) )*Value;
  
  vec3 HighColor = mix(vec3(107.0,99.0,92.0),vec3(140.0,110.0,100.0),HighValue) / 255.0;
  
  normal = normalize(normal);
  vec3 Color =mix( LowColor, HighColor, Value )*0.7*normal.z;
  
  gl_FragColor = vec4(Color,1.0);
}