#ifdef VERTEX_SHADER

in vec3 vert;
in vec2 vertTexCoord;
out vec2 fragTexCoord;

void main() {
  // Pass the texture coordinates straight to the fragment shader
  fragTexCoord = vertTexCoord;

  // does not alter the vertices at all
  gl_Position = vec4(vert, 1);
}

#endif


#ifdef FRAGMENT_SHADER

uniform sampler2D tex; // this is the texture slot
in vec2 fragTexCoord; // these are the texture coordinates
out vec4 finalColor; // this is the output color of the pixel

void main() {
    finalColor = texture(tex, fragTexCoord);
}

#endif
