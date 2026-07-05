#version 430

// Fullscreen quad for trail fade pass
const vec2 VERTS[4] = vec2[4](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0,  1.0)
);
const vec2 UVS[4] = vec2[4](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
);

out vec2 v_uv;

void main() {
    v_uv = UVS[gl_VertexID];
    gl_Position = vec4(VERTS[gl_VertexID], 0.0, 1.0);
}
