#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D texture_diffuse1;

void main() {
   vec3 color = texture(texture_diffuse1, TexCoords).rgb;

   if (length(color) < 0.1) {
      color = vec3(1.0, 0.9, 0.2);
   }

   FragColor = vec4(color, 1.0);
}