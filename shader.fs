#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

uniform sampler2D texture1;
uniform sampler2D texture2;

// in vec3 position; // exercise 3

void main() {
    // FragColor = vec4(ourColor, 1.0);
    // FragColor = vec4(position, 1.0);
    vec4 tex1 = texture(texture1, TexCoord);
    vec4 tex2 = texture(texture2, TexCoord);

    FragColor = mix(tex1, tex2, 0.2);
}