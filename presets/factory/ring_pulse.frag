// ring_pulse: concentric rings that expand from the centre and kick on track 0.
#pragma param thickness 0.005 0.2 0.05
#pragma param rings 1 12 6

void main() {
    vec2 p = uv * 2.0 - 1.0;
    p.x *= uResolution.x / uResolution.y;
    float d = length(p);
    float kick = clamp(iTrack0, 0.0, 4.0);
    float phase = fract(d * rings - uTime * 0.4 - kick * 0.25);
    float edge = min(phase, 1.0 - phase);
    float ring = 1.0 - smoothstep(thickness, thickness * 2.0 + 0.001, edge);
    vec3 base = vec3(0.02, 0.02, 0.05);
    vec3 glow = vec3(0.2, 0.9, 1.0) * (0.5 + 0.125 * kick);
    vec3 col = mix(base, glow, ring);
    col *= 1.0 - 0.6 * smoothstep(0.8, 1.6, d);
    fragColor = vec4(clamp(col, 0.0, 1.0), 1.0);
}
