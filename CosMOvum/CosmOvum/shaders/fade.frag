#version 430

// Trail fade — subtracts u_fade_amount/255 from all channels per frame
// Exact match of: trail_surface.fill((0,0,0,TRAIL_FADE), pygame.BLEND_RGBA_SUB) in v10.py

uniform sampler2D u_trail;
uniform float u_fade_amount;  // fade per channel per frame (0-255 range, divided by 255 in shader)
in  vec2 v_uv;
out vec4 frag_color;

void main() {
    vec4 current = texture(u_trail, v_uv);
    frag_color = max(current - vec4(u_fade_amount / 255.0), vec4(0.0));
}
