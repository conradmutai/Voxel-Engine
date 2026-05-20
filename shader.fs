#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D texture1;

void main() {
    vec4 texColor = texture(texture1, TexCoord);
    
    // 1. Map the UV coordinates back to the 16x16 atlas grid space
    int col = int(TexCoord.s * 16.0);
    int row = int((1.0 - TexCoord.t) * 16.0);

    // 2. Isolate the Grass Top texture slot (Column 0, Row 0)
    bool isGrassTop = (row == 0 && col == 0);

    if (isGrassTop) {
        FragColor = texColor * vec4(0.45, 0.85, 0.35, 1.0);
    } else {
        // Leave the Grass Side (3,0), Dirt (2,0), Stone (1,0), and all gray bits untouched
        FragColor = texColor;
    }
}