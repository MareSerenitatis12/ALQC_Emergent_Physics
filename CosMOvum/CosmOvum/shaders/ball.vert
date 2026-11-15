#version 430

struct BallEntity {
    vec4 pos;    // x, y, z, w
    vec4 vel;    // dx, dy, dz, dw
    vec4 state;  // stress, seam_charge, reflect_charge, reflect_age
    vec4 aeon;   // aeon_idx (as float), r, g, b (0-255)
    vec4 pout;   // entropy_cursor, px, py, alpha
    vec4 extra;  // charge, _pad, _pad, _pad
};

layout(std430, binding = 6) readonly buffer BallBuf {
    BallEntity b[];
};

uniform float u_global_angle;
uniform vec2  u_screen_size;

out vec4 v_color;

void main() {
    BallEntity e = b[gl_VertexID];
    float px    = e.pout.y;
    float py    = e.pout.z;
    float alpha = e.pout.w;

    // Phase entanglement — w_rot < 0 inverts color (v10.py lines 740-741)
    float w_rot = e.pos.x * sin(u_global_angle) + e.pos.w * cos(u_global_angle);
    vec3 col = e.aeon.yzw / 255.0;
    if (w_rot < 0.0) col = 1.0 - col;

    v_color = vec4(col, alpha / 255.0);

    vec2 ndc = (vec2(px, py) / u_screen_size) * 2.0 - 1.0;
    ndc.y = -ndc.y;
    gl_Position  = vec4(ndc, 0.0, 1.0);
    gl_PointSize = 2.0;
}
