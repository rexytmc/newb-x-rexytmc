$input v_pos, v_uvPos

#include <bgfx_shader.sh>

// 2D noise para variaciones sutiles en el efecto
float rand2(vec2 n) {
    return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453);
}

float noise(vec2 n) {
    const vec2 d = vec2(0.0, 1.0);
    vec2 b = floor(n), f = smoothstep(vec2(0.0), vec2(1.0), fract(n));
    return mix(mix(rand2(b), rand2(b + d.yx), f.x), mix(rand2(b + d.xy), rand2(b + d.yy), f.x), f.y);
}

// Generación del efecto de lens flare
vec3 lensflare(vec2 uv, vec2 pos) {
    vec2 main = uv - pos;
    vec2 uvd = uv * length(uv);

    float ang = atan(main.x, main.y);
    float dist = length(main);
    dist = pow(dist, .1);

    float f0 = 1.0 / (length(uv - pos) * 16.0 + 1.0);
    f0 += f0 * (dist * 0.1 + 0.8);

    float f2 = max(1.0 / (1.0 + 32.0 * pow(length(uvd + 0.8 * pos), 2.0)), 0.0) * 0.25;
    float f4 = max(0.01 - pow(length(uvd - 0.4 * pos), 2.4), 0.0) * 6.0;
    float f6 = max(0.01 - pow(length(uvd - 0.3 * pos), 1.6), 0.0) * 6.0;

    vec3 c = vec3(0.0);
    c.r += f2 + f4 + f6;
    c.g += f2 * 0.9;
    c.b += f2 * 0.8;
    c += vec3(f0);

    return c;
}

void main() {
    vec3 flareColor = lensflare(v_pos.xz, v_uvPos.xy);
    gl_FragColor = vec4(flareColor, 1.0);
}