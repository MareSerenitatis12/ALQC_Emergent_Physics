#version 430

// ---------------------------------------------------------------------------
// Vertex shader — reads particle SSBO via gl_VertexID (true GPU rendering)
// Projection and color entanglement from v10.py exactly
// ---------------------------------------------------------------------------

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

    // Projected position already computed by compute shader (px, py in pixels)
    float px = e.pout.y;
    float py = e.pout.z;
    float alpha = e.pout.w;

    // Phase entanglement — w_rot < 0 inverts color (v10.py exact)
    float w_rot = e.pos.x * sin(u_global_angle) + e.pos.w * cos(u_global_angle);
    vec3 col = e.aeon.yzw / 255.0;  // r,g,b packed in aeon.y,z,w
    if (w_rot < 0.0) col = 1.0 - col;

    v_color = vec4(col, alpha / 255.0);

    // Map pixel coords to NDC
    vec2 ndc = (vec2(px, py) / u_screen_size) * 2.0 - 1.0;
    ndc.y = -ndc.y;  // flip Y (screen Y is down, NDC Y is up)

    gl_Position  = vec4(ndc, 0.0, 1.0);
    gl_PointSize = u_point_size;
}
