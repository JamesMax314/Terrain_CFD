#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) out vec2 fragColor;

vec2 positions[4] = vec2[](vec2 (-1.0, -1.0), vec2 (1.0, -1.0), vec2 (-1.0, 1.0), vec2 (1.0, 1.0));

vec3 colors[4] = vec3[](vec3 (1.0, 0.0, 0.0), vec3 (0.0, 1.0, 0.0), vec3 (0.0, 0.0, 1.0), vec3 (0.0, 0.0, 0.0));

void main ()
{
	gl_Position = vec4 (positions[gl_VertexIndex], 0.0, 1.0);
	// fragColor = positions[gl_VertexIndex];
	fragColor = (positions[gl_VertexIndex] + vec2(1.0, 1.0)) * 0.5;
}