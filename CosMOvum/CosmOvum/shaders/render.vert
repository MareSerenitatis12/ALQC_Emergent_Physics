#version 430

// ---------------------------------------------------------------------------
// Vertex shader — reads particle SSBO via gl_VertexID (true GPU rendering)
// Projection and color entanglement from v10.py exactly
// ---------------------------------------------------------------------------

struct Particle {
    vec4  pos;    // x, y, z, w
    vec4  vel;    // dx, dy, dz, dw
    vec4  state;  // stress, seam_charge, reflect_charge, reflect_age
    vec4  aeon;   // freq, r, g, b (0-255)
    vec4  pout;   // entropy_cursor, px, py, alpha
    vec4  extra;  // charge, _pad, _pad, _pad
    uvec4 ids;    // B10: node_id, origin_node_id, traversal_flags, particle_uid
};

layout(std430, binding = 0) readonly buffer ParticleBuf {
    Particle p[];
};

uniform float u_global_angle;
uniform vec2  u_screen_size;
uniform float u_point_size;
uniform int u_crescent_view;  // 0 = original; 1 = identical 90-degree mirrored copy
uniform vec2 u_crescent_center;

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

    // The second draw is an exact copy of the first particle rendering,
    // laid flat at 90 degrees around the same Identity Seam screen center.
    // Same points, alpha, colors, and physics: only the viewing plane changes.
    if (u_crescent_view == 1) {
        vec2 displacement = vec2(px, py) - u_crescent_center;
        vec2 laid_flat = vec2(-displacement.y, displacement.x);
        px = u_crescent_center.x + laid_flat.x;
        py = u_crescent_center.y + laid_flat.y;
    }

    // Map pixel coords to NDC
    vec2 ndc = (vec2(px, py) / u_screen_size) * 2.0 - 1.0;
    ndc.y = -ndc.y;  // flip Y (screen Y is down, NDC Y is up)

    gl_Position  = vec4(ndc, 0.0, 1.0);
    gl_PointSize = u_point_size;
}
