[Vertex Shader]
attribute vec3 Coord;
uniform mat4 Matrix;

void main()
{
  vec4 P = Matrix*vec4(Coord,1.0);
  P.z = max( -P.w, P.z ); // Clamp to the near plane
  gl_Position = P;
}


[Fragment Shader]

void main()
{
  gl_FragColor = vec4(0.0,0.0,0.0,1.0);
}