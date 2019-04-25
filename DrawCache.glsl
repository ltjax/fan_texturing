[Vertex Shader]
attribute vec2 Vertex;
uniform vec4 Range;
uniform vec3 OffsetAndScale;
varying vec2 TexCoord;

void main()
{
  vec4 Range2 = (Range*2.0)-vec4(1.0);  
  TexCoord = (Vertex*OffsetAndScale.z)+OffsetAndScale.xy;
  gl_Position.xy = Range2.xy + Vertex*(Range2.zw-Range2.xy);
  gl_Position.zw = vec2(0.0,1.0);
}


[Fragment Shader]

uniform sampler2D CacheTexture;
varying vec2 TexCoord;

void main()
{
  if ( TexCoord.x < 0.0 || TexCoord.x > 1.0 || TexCoord.y < 0.0 || TexCoord.y > 1.0 )
    discard;
  
  gl_FragColor = texture2D( CacheTexture, TexCoord );
}