[Vertex Shader]
#version 150

in vec4 Position;
in vec3 Color;

out vec3 FragColor;

uniform mat4 ViewMatrix;
uniform mat4 ProjMatrix;

void main()
{
	FragColor = Color;
	gl_Position    = Position;
  gl_Position.xy = gl_Position.xy*2.0 - vec2(1.0);
}

[Fragment Shader]
#version 150

in vec3 FragColor;

void main()
{
	gl_FragColor = vec4(FragColor,1.0);
}

