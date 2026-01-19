#version 400
out vec4 fragColor;
// Input from vertex shader: the 3D direction on the sky sphere
in vec3 fragPos; 

// --- A simple pseudo-random function ---
// Input: A 3D coordinate
// Output: A single random number between 0.0 and 1.0
// It works by doing messy math that results in deterministic "static"
float hash31(vec3 p) {
    p  = fract(p * 0.3183099 + .1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

void main() {
    // 1. Normalize direction
    // Ensure the direction vector has a length of exactly 1.0.
    vec3 dir = normalize(fragPos);

    // --- PART 1: BACKGROUND GRADIENT ---

    // Define colors
    vec3 deepSpaceBlack = vec3(0.0, 0.0, 0.02); // Almost pure black at zenith
    vec3 horizonBlue = vec3(0.05, 0.09, 0.2);   // Dark blue near horizon

    // Calculate blend factor based on up/down direction (dir.y)
    // dir.y ranges from -1.0 (down) to 1.0 (up).
    // smoothstep creates a soft transition between -0.1 and 0.4 altitude.
    float t = smoothstep(-0.1, 0.4, dir.y);

    // Mix the colors based on 't'
    vec3 bg = mix(horizonBlue, deepSpaceBlack, t);


    // --- PART 2: STARS ---

    // Generate base noise
    // We multiply 'dir' by a large number (150.0) to make the noise pattern tiny/dense.
    float starNoise = hash31(dir * 150.0);

    // Thresholding
    // We only want the very brightest specks of noise to be stars.
    // smoothstep(0.995, 1.0, ...) takes only the top 0.5% of noise values
    // and ramps them quickly from black to white.
    float stars = smoothstep(0.995, 1.0, starNoise);
    
    // Make them brighter
    stars *= 1.5;


    // --- FINAL COMPOSITION ---
    // Add stars on top of the background gradient
    vec3 finalColor = bg + vec3(stars);

    fragColor = vec4(finalColor, 1.0);
}
