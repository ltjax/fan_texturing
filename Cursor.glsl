[Vertex Shader]
attribute vec3 Vertex;
uniform mat4 Modelview;
uniform mat4 Projection;
varying vec3 Point;

void main()
{
  
  vec4 V=vec4( Vertex,1.0);
  Point = Vertex;
  gl_Position = Projection*(Modelview*V);
}


[Fragment Shader]

uniform vec4 Color;
varying vec3 Point;

void main()
{
  // vec4 Color = vec4(0.8,0.2,0.01,1.0)
  vec3 R=smoothstep(0.0,0.4,abs(Point));
  float C = 1.0-(R.x*R.y*R.z);
  gl_FragColor = Color*(C*C*C);
}