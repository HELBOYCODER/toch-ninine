// scanline_glitch: CRT scanlines with row jitter; track 5 raises the glitch amount.
#pragma param jitter 0 1 0.3
#pragma param lines 50 400 180

void main() {
    vec2 p = uv;
    float row = floor(p.y * lines);
    float h = fract(sin(row * 12.9898 + floor(uTime * 12.0)) * 43758.5453);
    float glitch = clamp(iTrack5, 0.0, 4.0) * 0.25;
    float shift = (h - 0.5) * jitter * (0.05 + glitch * 0.1) * step(0.8, h);
    vec2 q = vec2(fract(p.x + shift), p.y);
    float scan = 0.5 + 0.5 * sin(q.y * lines * 3.14159);
    float bars = step(0.5, fract(q.x * 8.0 + uTime * 0.2));
    vec3 col = vec3(0.1, 0.9, 0.4) * (0.3 + 0.7 * scan) * (0.7 + 0.3 * bars);
    col += vec3(0.8, 0.1, 0.6) * step(0.995, h) * (1.0 - uv.y);
    fragColor = vec4(clamp(col, 0.0, 1.0), 1.0);
}
