[Vertex Shader]

uniform vec4 Center;
uniform vec3 OffsetAndScale;

varying vec3 Normal;
varying vec4 ModelPos;
varying vec2 CacheCoord;

vec4 UniformToScreen( vec4 Coord )
{
  return vec4(Coord.x*2.0-1.0,Coord.y*2.0-1.0,0.0,1.0);
}

void main( void )
{

  ModelPos = gl_Vertex;
  Normal = gl_Normal;
  vec4 NormalizedCoords = gl_ModelViewMatrix * (gl_Vertex-Center);
  CacheCoord = NormalizedCoords.xy*OffsetAndScale.z + OffsetAndScale.xy;
  gl_Position = UniformToScreen(NormalizedCoords);
}

[Fragment Shader]

uniform sampler2D CacheTexture;
varying vec2 CacheCoord;

varying vec3 Normal;
varying vec4 ModelPos;

uniform vec4 BrushColor;
uniform vec3 BrushPosition;
uniform vec2 BrushRadii;

void main( void )
{
  vec4 SourceColor = texture2D(CacheTexture,CacheCoord);
  
  float SourceWeight = length(BrushPosition-ModelPos.xyz);
  SourceWeight = smoothstep(BrushRadii.x,BrushRadii.y,SourceWeight);  
  
  gl_FragColor = mix(BrushColor,SourceColor,SourceWeight);
}



