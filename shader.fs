#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in float vAO;

uniform sampler2D texture1;

void main() {
    vec4 texColor = texture(texture1, TexCoord);

    // Convert the 0-3 AO level into a clean light multiplier scale
    // Level 3 -> 1.0 (Full Brightness)
    // Level 2 -> 0.8 (Light Shadow)
    // Level 1 -> 0.6 (Medium Shadow)
    // Level 0 -> 0.4 (Deep Corner Crack Shadow)
    float aoMultiplier = 0.4 + (vAO / 3.0) * 0.6;

    // 1. Map the UV coordinates back to the 16x16 atlas grid space
    int col = int(TexCoord.s * 16.0);
    int row = int((1.0 - TexCoord.t) * 16.0);

    // 2. Isolate the Grass Top texture slot (Column 0, Row 0)
    bool isGrassTop = (row == 0 && col == 0);

    if (isGrassTop) {
        // Multiply texture color by your custom grass green tint AND the ambient occlusion shadow
        FragColor = texColor * vec4(0.45, 0.85, 0.35, 1.0) * aoMultiplier;
    } else {
        // Leave all other textures untinted, but apply the ambient occlusion shadow
        FragColor = vec4(texColor.rgb * aoMultiplier, texColor.a);
    }
}