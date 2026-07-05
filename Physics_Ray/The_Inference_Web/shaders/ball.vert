#version 430

// Stress ball vertex shader — reads from the unified ParticleBuf (binding 0).
// gl_VertexID is offset by the draw call base (PARTICLE_COUNT), so it lands on
// the ball entries at the tail of the SSBO.
// Uses fixed high alpha: balls are individual points, not an accumulated field,
// so emanation_alpha (from pout.w) would leave them nearly invisible.

struct Particle {
    vec4 pos;    // x, y, z, w
    vec4 vel;    // dx, dy, dz, dw
    vec4 state;  // stress, seam_charge, reflect_charge, reflect_age
    vec4 aeon;   // freq, r, g, b (0-255)
    vec4 pout;   // entropy_cursor, px, py, alpha
};

layout(std430, binding = 0) readonly buffer ParticleBuf {
    Particle p[];
};

uniform float u_global_angle;
uniform vec2  u_screen_size;
uniform float u_point_size;

out vec4 v_color;

void main() {
    Particle e = p[gl_VertexID];
    float px = e.pout.y;
    float py = e.pout.z;

    // Phase entanglement — w_rot < 0 inverts color (v10.py exact)
    float w_rot = e.pos.x * sin(u_global_angle) + e.pos.w * cos(u_global_angle);
    vec3 col = e.aeon.yzw / 255.0;
    if (w_rot < 0.0) col = 1.0 - col;

    // Fixed alpha: balls must be individually visible, not accumulated
    v_color = vec4(col, 0.85);

    vec2 ndc = (vec2(px, py) / u_screen_size) * 2.0 - 1.0;
    ndc.y = -ndc.y;
    gl_Position  = vec4(ndc, 0.0, 1.0);
    gl_PointSize = u_point_size;
}
