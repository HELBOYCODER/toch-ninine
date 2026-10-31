// starfield_zoom: three depth layers of stars flying toward the viewer; track 3 adds speed.
#pragma param speed 0 4 1
#pragma param density 10 80 40

float hash21(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

void main() {
    vec2 p = uv * 2.0 - 1.0;
    p.x *= uResolution.x / uResolution.y;
    float flow = uTime * speed * 0.2 + clamp(iTrack3, 0.0, 4.0) * 0.05;
    float cellScale = density * 0.1;
    vec3 col = vec3(0.0);
    for (int layer = 0; layer < 3; ++layer) {
        float fl = float(layer);
        float depth = fract(flow + fl / 3.0);
        vec2 q = p / (depth + 0.05) * cellScale;
        vec2 cell = floor(q);
        vec2 local = fract(q) - 0.5;
        float h = hash21(cell + fl * 31.0);
        vec2 offset = vec2(fract(h * 13.1), fract(h * 37.7)) - 0.5;
        float d = length(local - offset * 0.6);
        float star = (1.0 - smoothstep(0.0, 0.08, d)) * step(0.6, h) * (1.0 - depth);
        col += star * vec3(0.8, 0.9, 1.0);
    }
    fragColor = vec4(clamp(col, 0.0, 1.0), 1.0);
}
