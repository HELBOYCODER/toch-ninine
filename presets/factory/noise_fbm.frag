// noise_fbm: domain-warped fractal value noise; track 1 lifts the highlights.
#pragma param amp 0 1 0.6
#pragma param speed 0 1 0.1

float hash21(vec2 p) {
    return fract(sin(dot(p, vec2(41.3, 289.1))) * 43758.5453);
}

float valueNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    float a = hash21(i);
    float b = hash21(i + vec2(1.0, 0.0));
    float c = hash21(i + vec2(0.0, 1.0));
    float d = hash21(i + vec2(1.0, 1.0));
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 p) {
    float sum = 0.0;
    float amplitude = 0.5;
    for (int k = 0; k < 5; ++k) {
        sum += amplitude * valueNoise(p);
        p = p * 2.02 + vec2(1.7, 9.2);
        amplitude *= 0.5;
    }
    return sum;
}

void main() {
    vec2 p = uv * 2.0 - 1.0;
    p.x *= uResolution.x / uResolution.y;
    p *= 2.5;
    float t = uTime * speed;
    float n1 = fbm(p + vec2(t, -t * 0.5));
    float n2 = fbm(p + vec2(n1 * amp * 2.0, n1 * 1.3));
    float lift = clamp(iTrack1, 0.0, 4.0) * 0.1;
    vec3 col = mix(vec3(0.05, 0.02, 0.15), vec3(0.9, 0.5, 0.2), clamp(n2 + lift, 0.0, 1.0));
    col += vec3(0.5) * smoothstep(0.6, 1.0, n2) * amp;
    fragColor = vec4(clamp(col, 0.0, 1.0), 1.0);
}
