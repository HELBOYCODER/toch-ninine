// kaleido_fold: polar mirror folding into N segments; track 0 pushes the colour phase.
#pragma param segments 3 12 6
#pragma param spin 0 2 0.2

void main() {
    vec2 p = uv * 2.0 - 1.0;
    p.x *= uResolution.x / uResolution.y;
    float a = atan(p.y, p.x) + uTime * spin;
    float r = length(p);
    float seg = 6.2831853 / segments;
    a = mod(a, seg);
    a = abs(a - seg * 0.5);
    vec2 q = r * vec2(cos(a), sin(a));
    float pattern = sin(q.x * 9.0 + uTime) * cos(q.y * 9.0 - uTime * 0.7);
    float bass = clamp(iTrack0, 0.0, 4.0) * 0.25;
    vec3 col = 0.5 + 0.5 * cos(vec3(0.0, 2.0, 4.0) + pattern * 2.0 + r * 3.0 + bass * 3.0);
    col *= 0.1 + 0.9 * (1.0 - smoothstep(0.2, 1.4, r));
    fragColor = vec4(clamp(col, 0.0, 1.0), 1.0);
}
