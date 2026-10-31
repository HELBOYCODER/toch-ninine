// plasma_bands: layered sine plasma; tracks 1 and 2 brighten and shift the palette.
#pragma param speed 0 2 0.4
#pragma param scale 0.5 8 3

void main() {
    vec2 p = (uv * 2.0 - 1.0) * scale;
    p.x *= uResolution.x / uResolution.y;
    float t = uTime * speed;
    float v = sin(p.x + t);
    v += sin(p.y * 1.3 - t * 0.7);
    v += sin((p.x + p.y) * 0.8 + t * 0.5);
    v += sin(length(p) * 1.7 - t);
    v = v * 0.125 + 0.5;
    float boost = 0.25 * (clamp(iTrack1, 0.0, 4.0) + clamp(iTrack2, 0.0, 4.0));
    vec3 col = 0.5 + 0.5 * cos(vec3(0.0, 2.1, 4.2) + v * 6.2831 + boost);
    fragColor = vec4(clamp(col, 0.0, 1.0), 1.0);
}
