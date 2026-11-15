#version 430

// ---------------------------------------------------------------------------
// Shadow Loci vertex shader — reads ShadowBuf SSBO via gl_VertexID
// Fixed color: SHADOW_LOCUS_COLOR = (255, 0, 50)
// Phase entanglement inversion same as render.vert (v10.py lines 370-379)
// ---------------------------------------------------------------------------

struct ShadowEntity {
    vec4  pos;    // x, y, z, w
    vec4  vel;    // dx, dy, dz, dw
    vec4  state;  // stress, seam_charge, reflect_charge, reflect_age
    vec4  aeon;   // (unused color slots — color is fixed)
    vec4  pout;   // entropy_cursor, px, py, alpha
    vec4  extra;  // charge, _pad, _pad, _pad
    uvec4 ids;    // B10: node_id, origin_node_id, traversal_flags, particle_uid
};

layout(std430, binding = 4) readonly buffer ShadowBuf {
    ShadowEntity s[];
};

uniform float u_global_angle;
uniform vec2  u_screen_size;
uniform vec3  u_shadow_color;
uniform float u_point_size;

out vec4 v_color;

void main() {
    ShadowEntity e = s[gl_VertexID];
    float px    = e.pout.y;
    float py    = e.pout.z;
    float alpha = e.pout.w;

    // Phase entanglement — w_rot < 0 inverts color (v10.py exact)
    float w_rot = e.pos.x * sin(u_global_angle) + e.pos.w * cos(u_global_angle);
    vec3 col = u_shadow_color / 255.0;
    if (w_rot < 0.0) col = 1.0 - col;

    v_color = vec4(col, alpha / 255.0);

    vec2 ndc = (vec2(px, py) / u_screen_size) * 2.0 - 1.0;
    ndc.y = -ndc.y;
    gl_Position  = vec4(ndc, 0.0, 1.0);
    gl_PointSize = u_point_size;
}
