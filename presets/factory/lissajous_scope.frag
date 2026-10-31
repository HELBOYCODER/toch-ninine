// lissajous_scope: a Lissajous figure drawn as dots; tracks 0..3 set its size.
#pragma param freqA 1 8 3
#pragma param freqB 1 8 2
#pragma param glow 0.002 0.05 0.012

void main() {
    vec2 p = uv * 2.0 - 1.0;
    p.x *= uResolution.x / uResolution.y;
    float energy = clamp(iTrack0 + iTrack1 + iTrack2 + iTrack3, 0.0, 16.0) / 16.0;
    float dist = 1e9;
    for (int k = 0; k < 64; ++k) {
        float t = float(k) / 64.0 * 6.2831853;
        vec2 c = vec2(sin(freqA * t + uTime * 0.5), sin(freqB * t)) * (0.6 + 0.4 * energy);
        dist = min(dist, length(p - c));
    }
    float line = exp(-dist * dist / glow);
    vec3 col = vec3(0.1, 1.0, 0.5) * line;
    fragColor = vec4(clamp(col, 0.0, 1.0), 1.0);
}
