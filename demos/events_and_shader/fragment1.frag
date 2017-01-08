#version 330

uniform sampler2D tex; // this is the texture slot
in vec2 fragTexCoord; // these are the texture coordinates
out vec4 finalColor; // this is the output color of the pixel

void main() {
    finalColor = texture(tex, fragTexCoord);
}
