#version 430

uniform vec3  u_wire_color;
uniform float u_wire_alpha;

out vec4 fragColor;

void main() {
    fragColor = vec4(u_wire_color, u_wire_alpha);
}
