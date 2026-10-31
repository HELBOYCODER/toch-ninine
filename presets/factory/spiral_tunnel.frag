// spiral_tunnel: a twisting checkered tunnel; track 6 speeds up the travel.
#pragma param twist 0 4 1.5
#pragma param rings 2 30 12

void main() {
    vec2 p = uv * 2.0 - 1.0;
    p.x *= uResolution.x / uResolution.y;
    float r = length(p) + 0.0001;
    float a = atan(p.y, p.x);
    float depth = 1.0 / r;
    float u = a / 6.2831853 + twist * 0.1 * depth + uTime * 0.05;
    float v = depth * 0.5 + uTime * 0.3 + clamp(iTrack6, 0.0, 4.0) * 0.1;
    float checker = mod(floor(u * rings) + floor(v * 2.0), 2.0);
    vec3 col = mix(vec3(0.05, 0.0, 0.1), vec3(0.2, 0.8, 1.0), checker);
    col *= clamp(r * 2.0, 0.0, 1.0);
    fragColor = vec4(clamp(col, 0.0, 1.0), 1.0);
}
