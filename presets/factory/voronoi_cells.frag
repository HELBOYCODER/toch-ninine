// voronoi_cells: drifting Voronoi cells with darkened borders; track 0 lifts brightness.
#pragma param jitter 0 1 0.8
#pragma param scale 1 12 5

vec2 hash22(vec2 p) {
    return fract(sin(vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)))) * 43758.5453);
}

void main() {
    vec2 p = uv * 2.0 - 1.0;
    p.x *= uResolution.x / uResolution.y;
    p *= scale;
    vec2 base = floor(p);
    vec2 f = fract(p);
    float dmin = 8.0;
    float idHash = 0.0;
    for (int j = 0; j < 3; ++j) {
        for (int i = 0; i < 3; ++i) {
            vec2 cell = vec2(float(i) - 1.0, float(j) - 1.0);
            vec2 h = hash22(base + cell);
            vec2 point = cell + 0.5 + jitter * (h - 0.5) + 0.2 * sin(uTime * (0.5 + h) + 6.2831 * h);
            float d = length(point - f);
            if (d < dmin) {
                dmin = d;
                idHash = h.x;
            }
        }
    }
    vec3 col = 0.5 + 0.5 * cos(vec3(0.0, 2.0, 4.0) + idHash * 6.2831);
    col *= 0.35 + 0.65 * smoothstep(0.0, 0.15, dmin);
    col += clamp(iTrack0, 0.0, 4.0) * 0.05;
    fragColor = vec4(clamp(col, 0.0, 1.0), 1.0);
}
