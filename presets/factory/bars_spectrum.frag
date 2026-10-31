// bars_spectrum: eight bars, one per track, rising from the bottom edge.
#pragma param gain 0.1 2 0.8
#pragma param glow 0 1 0.3

void main() {
    float levels[8];
    levels[0] = iTrack0;
    levels[1] = iTrack1;
    levels[2] = iTrack2;
    levels[3] = iTrack3;
    levels[4] = iTrack4;
    levels[5] = iTrack5;
    levels[6] = iTrack6;
    levels[7] = iTrack7;

    float x = uv.x * 8.0;
    int idx = clamp(int(floor(x)), 0, 7);
    float level = clamp(levels[idx], 0.0, 4.0);
    float height = clamp(level * gain * 0.5, 0.0, 1.0);
    float local = fract(x);
    float inBar = step(uv.y, height) * step(0.1, local) * step(local, 0.9);
    vec3 barCol = mix(vec3(0.1, 0.8, 1.0), vec3(1.0, 0.2, 0.5), uv.y);
    vec3 col = inBar * barCol + glow * height * 0.2 * vec3(0.3, 0.2, 0.5) * (1.0 - uv.y);
    fragColor = vec4(clamp(col, 0.0, 1.0), 1.0);
}
