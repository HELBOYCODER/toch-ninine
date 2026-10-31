// grid_warp: a line grid bent by sine waves; track 0 drives brightness.
#pragma param warp 0 1 0.3
#pragma param density 2 30 10

void main() {
    vec2 p = uv * 2.0 - 1.0;
    p.x *= uResolution.x / uResolution.y;
    float wob = clamp(iTrack2, 0.0, 4.0) * 0.25 + 0.2;
    p += warp * wob * vec2(sin(p.y * 3.0 + uTime), cos(p.x * 3.0 - uTime));
    vec2 g = abs(fract(p * density) - 0.5);
    float edgeDist = 0.5 - max(g.x, g.y);
    float line = 1.0 - smoothstep(0.0, 0.04, edgeDist);
    float pulse = clamp(iTrack0, 0.0, 4.0) * 0.25;
    vec3 col = mix(vec3(0.03), vec3(1.0, 0.4, 0.1) * (0.6 + pulse), line);
    fragColor = vec4(clamp(col, 0.0, 1.0), 1.0);
}
