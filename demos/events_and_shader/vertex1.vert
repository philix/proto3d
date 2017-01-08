#version 330

in vec3 vert;
in vec2 vertTexCoord;
out vec2 fragTexCoord;

void main() {
  // Pass the texture coordinates straight to the fragment shader
  fragTexCoord = vertTexCoord;

  // does not alter the vertices at all
  gl_Position = vec4(vert, 1);
}
