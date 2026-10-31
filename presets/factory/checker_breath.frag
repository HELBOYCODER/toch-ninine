// checker_breath: a checkerboard that breathes in and out; track 7 adds a pulse.
#pragma param cells 2 32 8
#pragma param breathe 0 1 0.5

void main() {
    vec2 p = (uv * 2.0 - 1.0);
    p.x *= uResolution.x / uResolution.y;
    float breath = 0.5 + 0.5 * sin(uTime * 1.5);
    float scale = cells * (1.0 + breathe * 0.2 * (breath - 0.5) + 0.02 * clamp(iTrack7, 0.0, 4.0));
    vec2 q = (p + 1.0) * scale;
    vec2 g = floor(q);
    float c = mod(g.x + g.y, 2.0);
    vec2 f = fract(q) - 0.5;
    float edge = smoothstep(0.45, 0.5, max(abs(f.x), abs(f.y)));
    vec3 a = vec3(0.95, 0.9, 0.85);
    vec3 b = vec3(0.1, 0.1, 0.2);
    vec3 col = mix(a, b, c);
    col = mix(col, vec3(0.0), edge * 0.5);
    col *= 0.6 + 0.4 * breath;
    fragColor = vec4(clamp(col, 0.0, 1.0), 1.0);
}
