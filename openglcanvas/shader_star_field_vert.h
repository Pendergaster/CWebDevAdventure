
"#version 300 es\n"
"precision highp float;\n"


STRINGIFY(

layout (location = 0) in vec2 position;

out vec2  pixel_coords;
out vec2  iResolution;
out vec2  iMouse;
out float iTime;

uniform float u_time;
uniform vec2 u_resolution;

// const vec2 resolution = vec2(860, 640);

void main() {

	gl_Position.xy = position.xy;
	gl_Position.z = 0.f;
	gl_Position.w = 1.f;
	vec2 screen_pos = position.xy;
	screen_pos.y *= -1.0;
	pixel_coords = ((screen_pos.xy + 1.0) / 2.0) * u_resolution;

	iResolution = u_resolution;
	iTime = u_time;
	iMouse = vec2(0.0, 0.0);
})

