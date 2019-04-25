[Vertex Shader]

uniform vec3 OffsetAndScale;
varying vec2 CacheCoord;

vec4 UniformToScreen( vec4 Coord )
{
  return vec4(Coord.x*2.0-1.0,Coord.y*2.0-1.0,0.0,1.0);
}

void main( void )
{
  CacheCoord = gl_Vertex.xy*OffsetAndScale.z + OffsetAndScale.xy;
  gl_Position = UniformToScreen(gl_Vertex);
}

[Fragment Shader]

uniform sampler2D CacheTexture;
varying vec2 CacheCoord;

void main( void )
{
  // The box filter is applied by the bilinear filtering engine
  gl_FragColor = texture2D( CacheTexture, CacheCoord );
}



