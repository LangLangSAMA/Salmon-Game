#version 330

uniform sampler2D screen_texture;
uniform float time;
uniform float dead_timer;

in vec2 uv;

layout(location = 0) out vec4 color;

vec2 distort(vec2 uv) 
{
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// HANDLE THE WATER WAVE DISTORTION HERE (you may want to try sin/cos)
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// vec2 coord = uv.xy;
    float pi = 3.14159265;
    float amplitude_x = 0.02;
    float amplitude_y = 0.01;
    float frequency_x = 3 * pi;
    float frequency_y = 3 * pi;
    float shift_x = 20 * time / 180;
    float shift_y = 15 * time / 180;
    float x = cos(frequency_x * uv.y + shift_x) * amplitude_x;
    float y = sin(frequency_y * uv.x + shift_y) * amplitude_y;
    vec2 coord = uv.xy + vec2 (x, y);
    return coord;
}

vec4 color_shift(vec4 in_color) 
{
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// HANDLE THE COLOR SHIFTING HERE (you may want to make it blue-ish)
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	vec4 color = in_color;
    color *= vec4(0.67, 0.84, 0.9, 1);
	return color;
}

vec4 fade_color(vec4 in_color) 
{
	vec4 color = in_color;
	if (dead_timer > 0)
		color -= 0.1 * dead_timer * vec4(0.1, 0.1, 0.1, 0);

	return color;
}

void main()
{
	vec2 coord = distort(uv);

    vec4 in_color = texture(screen_texture, coord);
    color = color_shift(in_color);
    color = fade_color(color);
}