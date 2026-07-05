#version 430
uniform sampler2D u_trail;
in vec2 v_uv;
out vec4 fragColor;
void main() { fragColor = texture(u_trail, v_uv); }
