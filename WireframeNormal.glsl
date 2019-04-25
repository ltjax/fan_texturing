[Vertex Shader]
#version 150

in vec4 Position;
in vec3 Color;

out vec3 GeomColor;
out vec4 GeomPosition;

uniform mat4 ViewMatrix;
uniform mat4 ProjMatrix;

void main()
{
	GeomColor = Color;
	GeomPosition    = ProjMatrix * ViewMatrix * Position;
}

[Fragment Shader]
#version 150

in vec3 FragColor;

void main()
{
	gl_FragColor = vec4(FragColor,1.0);
}

[Geometry Shader]
#version 150

layout(lines) in;
layout(line_strip, max_vertices=6) out;

in vec4 GeomPosition[];
in vec3 GeomColor[];

out vec3 FragColor;

vec4 Assemble( vec4 P, vec2 D )
{
  P.xy += D.xy * P.w;
  return P;
}

void main()
{
  FragColor = GeomColor[0];
  gl_Position = GeomPosition[0];
  EmitVertex();
  gl_Position = GeomPosition[1];
  EmitVertex();
  
  vec2 P = GeomPosition[1].xy / GeomPosition[1].w;
  vec2 Q = GeomPosition[0].xy / GeomPosition[0].w;

  vec2 VDelta = normalize( P-Q )*0.01;
  vec2 Right = vec2( -VDelta.y, VDelta.x );
  //vec2 Right = vec2( 0.01, 0.0 );
  vec2 Up = VDelta;
  
  //gl_Position = vec4( P+Right, 0.0, 1.0 );
  gl_Position = Assemble(GeomPosition[1],Right);
  EmitVertex();
  //gl_Position = vec4( P+Up, 0.0, 1.0 );
  gl_Position = Assemble(GeomPosition[1],Up);
  EmitVertex();
 //gl_Position = vec4( P-Right, 0.0, 1.0 );
  gl_Position = Assemble(GeomPosition[1],-Right);
  EmitVertex();
  //gl_Position = vec4( P, 0.0, 1.0 );
  gl_Position = GeomPosition[1];
  EmitVertex();
  EndPrimitive();
}

