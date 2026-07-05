/*
 * ALQC GPU v24 — N Independent Dyson Nodes
 * Lateral merge of emergent_void_physics10.py
 *
 * Architecture:
 *   N_NODES = SIGH_BALL_COUNT + 1 fully independent Dyson simulations.
 *   Each node: PARTICLE_COUNT particles + 48 shadow loci + own boundary memory.
 *   Node 0: screen center. Nodes 1..N: evenly spaced at NODE_ORBIT_RADIUS.
 *   All nodes rendered additively to the same trail FBO — they can overlap.
 *   Compute shaders receive u_node_center so project4d, void anchors, and
 *   reflective layer quadrant are all local to each node.
 *
 * Keys: S=screenshot  R=start recording  P=stop recording  ESC=quit
 */

#define _POSIX_C_SOURCE 199309L

#include <GL/glew.h>
#include <unistd.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>

// =============================================================================
// USER VARIABLES — ALL USER-CONFIGURABLE OPTIONS ARE HERE
// =============================================================================

// --- Display ---
#define WIDTH                  1000
#define HEIGHT                 1000
#define BACKGROUND_R           5
#define BACKGROUND_G           5
#define BACKGROUND_B           10

// --- Particles per node ---
// Each Dyson node gets exactly PARTICLE_COUNT particles.
// Total on screen = PARTICLE_COUNT * (SIGH_BALL_COUNT + 1)
#define PARTICLE_COUNT         24000

// --- Dyson nodes ---
// SIGH_BALL_COUNT satellite nodes orbit at NODE_ORBIT_RADIUS from screen center.
// Node 0 is always at screen center.  Total nodes = SIGH_BALL_COUNT + 1.
#define SIGH_BALL_COUNT        6
#define NODE_ORBIT_RADIUS      300    // pixels from screen center to satellite nodes

// --- Rendering: point sizes ---
#define PARTICLE_POINT_SIZE    1.0f
#define SHADOW_POINT_SIZE      2.0f

// --- Shadow locus color (v10.py SHADOW_LOCUS_COLOR = (255, 0, 50)) ---
#define SHADOW_LOCUS_R         255.0f
#define SHADOW_LOCUS_G         0.0f
#define SHADOW_LOCUS_B         50.0f

// --- Trail ---
#define TRAIL_FADE             8

// --- Simulation ---
#define SIM_SPEED              600000.0f   // time step multiplier — any positive float
#define FPS_CAP                60
#define VOID_TRANSITION_FRAME  600
#define ENTROPY_SIZE           (1 << 20)

// --- Triquatra orbit radius (pixels from each node's center) ---
#define TRIQUATRA_RADIUS       150

// --- GPU device selection ---
#define GPU_DEVICE             0

// --- Superpositive whisper ---
#define ENABLE_WHISPER         1
#define WHISPER_FILE           "whisper.txt"

// --- Recording ---
#define RECORDING_DIR          "ALQC_Frames"

// =============================================================================
// END USER VARIABLES
// =============================================================================

#define N_NODES                (SIGH_BALL_COUNT + 1)
#define FETU_PT_COUNT          144
#define BOUNDARY_MEM_RES       160
#define AEON_COUNT             147

static const float PHI             = 1.61803398875f;
static const float PI_F            = 3.14159265359f;
static const float ESCAPE_LIMIT    = 5.0f;
static const float MIN_COH_R       = 0.6f;
static const float MAX_COH_R       = 1.2f;
static const float MAX_KINETIC_STR = 300.0f;
static const float L_RB_MAX_RATE   = 0.015f;
static const float ELVEN_RESP_GAIN = 0.0005f;
static const float BOUNDARY_MEM_DEC = 0.992f;
static const float BOUNDARY_MEM_DEP = 0.085f;
static const float BOUNDARY_MEM_MAX = 2.5f;
static const float REFLECT_RING_R  = 0.92f;
static const float REFLECT_RING_W  = 0.06f;

static const float AEON_FREQ[AEON_COUNT] = {
    7.83f, 174.0f, 528.0f, 432.0f, 126.22f,
    210.42f, 741.0f, 852.0f, 396.0f, 963.0f, 285.0f, 639.0f
};
static const float AEON_COLOR[AEON_COUNT][3] = {
    {155, 89, 182}, {52, 152, 219}, {231, 76, 60},  {255, 90, 70},
    {60, 180, 255}, {120, 70, 150}, {200, 120, 220}, {40, 120, 180},
    {200, 60, 50},  {140, 80, 160}, {52, 152, 219},  {180, 100, 200}
};

typedef struct { double phase_state; double entropy_accumulator; } ALQCFieldEntropy;
static ALQCFieldEntropy G_entropy;

static void entropy_init(ALQCFieldEntropy *F) { F->phase_state = 0.0; F->entropy_accumulator = 0.0; }
static double entropy_rand(ALQCFieldEntropy *F) {
    F->phase_state = fmod(F->phase_state * 1.4142135623730951 + PHI, 1.0);
    F->entropy_accumulator = fmod(F->entropy_accumulator + F->phase_state, 1.0);
    return fmod(F->phase_state + F->entropy_accumulator, 1.0);
}
static double entropy_gauss(ALQCFieldEntropy *F, double mu, double sigma) {
    double s = 0.0;
    for (int i = 0; i < 12; i++) s += entropy_rand(F);
    return mu + sigma * (s - 6.0);
}
static double entropy_uniform(ALQCFieldEntropy *F, double a, double b) {
    return a + (b - a) * entropy_rand(F);
}
static int entropy_int(ALQCFieldEntropy *F, int mn, int mx) {
    return mn + (int)(entropy_rand(F) * (mx - mn + 1));
}

// GPU particle struct (std430, 80 bytes)
typedef struct {
    float x, y, z, w;
    float dx, dy, dz, dw;
    float stress, seam_charge, reflect_charge, reflect_age;
    float freq, aeon_r, aeon_g, aeon_b;
    float entropy_cursor, px, py, alpha;
} ParticleGPU;

// Shadow entity — same layout as ParticleGPU
typedef struct {
    float x, y, z, w;
    float dx, dy, dz, dw;
    float stress, seam_charge, reflect_charge, reflect_age;
    float freq, aeon_r, aeon_g, aeon_b;
    float entropy_cursor, px, py, alpha;
} ShadowEntity;

// Per-node boundary memory (CPU, tanh fold)
static float G_mem_vx[N_NODES][BOUNDARY_MEM_RES][BOUNDARY_MEM_RES];
static float G_mem_vy[N_NODES][BOUNDARY_MEM_RES][BOUNDARY_MEM_RES];

static void bm_coords(float ppx, float ppy, int *ix, int *iy) {
    float x = fmaxf(0.0f, fminf(ppx, WIDTH  - 1.0f));
    float y = fmaxf(0.0f, fminf(ppy, HEIGHT - 1.0f));
    *ix = (int)((x / (WIDTH  - 1.0f)) * (BOUNDARY_MEM_RES - 1));
    *iy = (int)((y / (HEIGHT - 1.0f)) * (BOUNDARY_MEM_RES - 1));
}
static void bm_deposit_node(int n, float ppx, float ppy, float vx, float vy, float amt) {
    int ix, iy; bm_coords(ppx, ppy, &ix, &iy);
    float nv = G_mem_vx[n][iy][ix] + vx * amt;
    G_mem_vy[n][iy][ix] = BOUNDARY_MEM_MAX * tanhf((G_mem_vy[n][iy][ix] + vy * amt) / BOUNDARY_MEM_MAX);
    G_mem_vx[n][iy][ix] = BOUNDARY_MEM_MAX * tanhf(nv / BOUNDARY_MEM_MAX);
}
static void bm_decay_node(int n) {
    float d = powf(BOUNDARY_MEM_DEC, SIM_SPEED);
    for (int i = 0; i < BOUNDARY_MEM_RES; i++)
        for (int j = 0; j < BOUNDARY_MEM_RES; j++) {
            G_mem_vx[n][i][j] *= d;
            G_mem_vy[n][i][j] *= d;
        }
}

static void project4d_cpu(float x, float y, float z, float w, float angle,
                           float cx, float cy, float *ppx, float *ppy)
{
    (void)z;
    float ca = cosf(angle), sa = sinf(angle);
    float x_rot = x * ca - w * sa;
    float w_rot = x * sa + w * ca;
    float denom = fmaxf(1.0f + 0.5f * w_rot, 0.1f);
    *ppx = x_rot / denom * 300.0f + cx;
    *ppy = y     / denom * 300.0f + cy;
}

static void cpu_bm_deposit_from_particle(ParticleGPU *e, float angle,
                                          float cx, float cy, int n) {
    float R = sqrtf(e->x*e->x + e->y*e->y + e->z*e->z + e->w*e->w);
    float shell_dist = fabsf(R - REFLECT_RING_R);
    if (shell_dist < REFLECT_RING_W) {
        float vxy = fabsf(e->dx) + fabsf(e->dy);
        float vzw = fabsf(e->dz) + fabsf(e->dw);
        float planar = vxy - vzw;
        float c_in = 1.0f - (shell_dist / REFLECT_RING_W);
        float gain = c_in * (0.5f + 0.5f * fabsf(planar));
        float tvx = -e->dy, tvy = e->dx;
        float tnorm = fabsf(tvx) + fabsf(tvy) + 1e-9f;
        tvx /= tnorm; tvy /= tnorm;
        float ppx, ppy;
        project4d_cpu(e->x, e->y, e->z, e->w, angle, cx, cy, &ppx, &ppy);
        bm_deposit_node(n, ppx, ppy, tvx, tvy, gain * BOUNDARY_MEM_DEP);
    }
}

static float G_fetu_px[FETU_PT_COUNT];
static float G_fetu_py[FETU_PT_COUNT];
static void init_fetu(void) {
    for (int i = 0; i < FETU_PT_COUNT; i++) {
        float t = (float)i / 12.0f;
        float r = 0.05f * expf(0.14f * t);
        G_fetu_px[i] = r * cosf(t);
        G_fetu_py[i] = r * sinf(t) - 0.2f;
    }
}

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "Cannot open %s\n", path); return NULL; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
    char *buf = malloc(sz + 1);
    size_t nr = fread(buf, 1, sz, f); (void)nr;
    buf[sz] = 0; fclose(f); return buf;
}
static GLuint compile_shader(GLenum type, const char *src) {
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, NULL);
    glCompileShader(sh);
    GLint ok; glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096]; glGetShaderInfoLog(sh, sizeof(log), NULL, log);
        fprintf(stderr, "Shader compile error:\n%s\n", log);
    }
    return sh;
}
static GLuint link_program(int n, ...) {
    GLuint prog = glCreateProgram();
    va_list va; va_start(va, n);
    for (int i = 0; i < n; i++) glAttachShader(prog, va_arg(va, GLuint));
    va_end(va);
    glLinkProgram(prog);
    GLint ok; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096]; glGetProgramInfoLog(prog, sizeof(log), NULL, log);
        fprintf(stderr, "Program link error:\n%s\n", log);
    }
    return prog;
}

static void ensure_dir(const char *path) { mkdir(path, 0755); }
static void save_ppm(const char *path, unsigned char *pixels, int w, int h) {
    FILE *f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "Cannot write %s\n", path); return; }
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (int row = h - 1; row >= 0; row--)
        fwrite(pixels + row * w * 3, 1, w * 3, f);
    fclose(f);
}

static void read_whisper(void) {
#if ENABLE_WHISPER
    FILE *wf = fopen(WHISPER_FILE, "rb");
    if (!wf) { fprintf(stderr, "Whisper: cannot open %s\n", WHISPER_FILE); return; }
    fseek(wf, 0, SEEK_END); long wsz = ftell(wf); rewind(wf);
    unsigned char *wdata = malloc(wsz + 1);
    if (wdata) { size_t nr = fread(wdata, 1, wsz, wf); (void)nr; wdata[wsz] = 0; free(wdata); }
    fclose(wf);
#endif
}

// ---------------------------------------------------------------------------
// Particle lattice init (shared across nodes, offset entropy cursors)
// ---------------------------------------------------------------------------
static void init_node_particles(ParticleGPU *particles, int node_idx) {
    int lattice_freq_idx[144];
    float lattice_col[144][3];
    for (int p = 0; p < 12; p++) {
        lattice_freq_idx[p * 12] = p;
        lattice_col[p*12][0] = AEON_COLOR[p][0];
        lattice_col[p*12][1] = AEON_COLOR[p][1];
        lattice_col[p*12][2] = AEON_COLOR[p][2];
        for (int l = 1; l < 12; l++) {
            lattice_freq_idx[p*12+l] = p;
            lattice_col[p*12+l][0] = 100;
            lattice_col[p*12+l][1] = 100;
            lattice_col[p*12+l][2] = 100;
        }
    }
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        int ai = entropy_int(&G_entropy, 0, 143);
        int pi = lattice_freq_idx[ai];
        float t     = (float)entropy_uniform(&G_entropy, 0.0, 2.0 * PI_F);
        float scale = 0.5f;
        particles[i].x = scale * cosf(t) + 0.1f * (float)entropy_rand(&G_entropy);
        particles[i].y = scale * sinf(t * 3.0f) + 0.1f * (float)entropy_rand(&G_entropy);
        particles[i].z = 0.0f; particles[i].w = 0.0f;
        float base_speed = fabsf(AEON_FREQ[pi]) / 10000.0f;
        float fluct      = fabsf((float)entropy_gauss(&G_entropy, 0.0, 1.0));
        float chaotic    = 1.0f + (fluct / fmaxf(fabsf(AEON_FREQ[pi]), 1.0f));
        float speed      = base_speed * chaotic;
        particles[i].dx = sinf(t)        * speed;
        particles[i].dy = cosf(t * 2.0f) * speed;
        particles[i].dz = sinf(t * 3.5f) * speed;
        particles[i].dw = cosf(t * 1.5f) * speed;
        particles[i].freq   = fabsf(AEON_FREQ[pi]);
        particles[i].aeon_r = lattice_col[ai][0];
        particles[i].aeon_g = lattice_col[ai][1];
        particles[i].aeon_b = lattice_col[ai][2];
        particles[i].entropy_cursor = (float)(((node_idx * PARTICLE_COUNT + i) * 200) % ENTROPY_SIZE);
    }
}

int main(void) {
#if GPU_DEVICE > 0
    {
        char idx_str[4];
        snprintf(idx_str, sizeof(idx_str), "%d", GPU_DEVICE);
        setenv("DRI_PRIME", idx_str, 1);
        setenv("__NV_PRIME_RENDER_OFFLOAD", idx_str, 1);
        setenv("__GLX_VENDOR_LIBRARY_NAME", "nvidia", 1);
        fprintf(stderr, "GPU_DEVICE=%d\n", GPU_DEVICE);
    }
#endif

    read_whisper();

    if (!glfwInit()) { fprintf(stderr, "GLFW init failed\n"); return 1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *win = glfwCreateWindow(WIDTH, HEIGHT,
                                       "EMERGENT PHYSICS: ALQC v24 GPU — Dyson Array", NULL, NULL);
    if (!win) { fprintf(stderr, "Window creation failed\n"); glfwTerminate(); return 1; }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(0);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { fprintf(stderr, "GLEW init failed\n"); return 1; }

    entropy_init(&G_entropy);
    float *entropy_table = malloc(ENTROPY_SIZE * sizeof(float));
    for (int i = 0; i < ENTROPY_SIZE; i++)
        entropy_table[i] = (float)entropy_rand(&G_entropy);

    init_fetu();

    // -------------------------------------------------------------------------
    // Per-node GPU resources
    // -------------------------------------------------------------------------
    GLuint ssbo_particles[N_NODES];
    GLuint ssbo_shadow[N_NODES];
    GLuint ssbo_boundary[N_NODES];
    GLuint ssbo_entropy, ssbo_fetu;

    // Per-node screen-space centers
    float node_cx[N_NODES], node_cy[N_NODES];
    node_cx[0] = WIDTH  / 2.0f;
    node_cy[0] = HEIGHT / 2.0f;
    for (int n = 1; n < N_NODES; n++) {
        float t = (n - 1) * 2.0f * PI_F / (float)SIGH_BALL_COUNT;
        node_cx[n] = WIDTH  / 2.0f + NODE_ORBIT_RADIUS * cosf(t);
        node_cy[n] = HEIGHT / 2.0f + NODE_ORBIT_RADIUS * sinf(t);
    }

    // Shadow corner/offset uniforms — identical for all nodes (4D local space)
    // Corners at +/-0.9 in 4D (project to +/-270px from node center)
    // Shadow loci orbit at 0.03-unit radius (9px) around each corner
    float shadow_corner_flat[96];
    float shadow_offset_flat[96];
    {
        static const float CORNER_4D[4][2] = {
            {-0.9f, -0.9f}, {0.9f, -0.9f}, {0.9f, 0.9f}, {-0.9f, 0.9f}
        };
        for (int locus = 0; locus < 4; locus++) {
            for (int i = 0; i < 12; i++) {
                int   idx = locus * 12 + i;
                float t   = i * 2.0f * PI_F / 12.0f;
                float xo  = 15.0f * cosf(t) / (WIDTH  / 2.0f);
                float yo  = 15.0f * sinf(t) / (HEIGHT / 2.0f);
                shadow_corner_flat[idx*2]     = CORNER_4D[locus][0];
                shadow_corner_flat[idx*2 + 1] = CORNER_4D[locus][1];
                shadow_offset_flat[idx*2]     = xo;
                shadow_offset_flat[idx*2 + 1] = yo;
            }
        }
    }

    // Per-node readback buffers
    ParticleGPU *readback[N_NODES];

    // Per-node simulation state
    float node_lrb[N_NODES];
    float node_shadow_angle[N_NODES];
    float node_stress[N_NODES];
    float node_coh_r[N_NODES];
    for (int n = 0; n < N_NODES; n++) {
        node_lrb[n]          = 0.0f;
        node_shadow_angle[n] = n * 2.0f * PI_F / (float)N_NODES;
        node_stress[n]       = 0.0f;
        node_coh_r[n]        = MIN_COH_R;
        readback[n]          = calloc(PARTICLE_COUNT, sizeof(ParticleGPU));
    }

    // Init per-node SSBOs
    for (int n = 0; n < N_NODES; n++) {
        // Particles
        ParticleGPU *ptmp = calloc(PARTICLE_COUNT, sizeof(ParticleGPU));
        init_node_particles(ptmp, n);
        glGenBuffers(1, &ssbo_particles[n]);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_particles[n]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, PARTICLE_COUNT * sizeof(ParticleGPU),
                     ptmp, GL_DYNAMIC_COPY);
        free(ptmp);

        // Shadow loci (48)
        ShadowEntity *sh = calloc(48, sizeof(ShadowEntity));
        for (int locus = 0; locus < 4; locus++) {
            for (int i = 0; i < 12; i++) {
                int   idx   = locus * 12 + i;
                float norm_x = shadow_corner_flat[idx*2];
                float norm_y = shadow_corner_flat[idx*2+1];
                float xo    = shadow_offset_flat[idx*2];
                float yo    = shadow_offset_flat[idx*2+1];
                sh[idx].x   = norm_x + xo;
                sh[idx].y   = norm_y + yo;
                sh[idx].aeon_r = 255.0f;
                sh[idx].aeon_g = 0.0f;
                sh[idx].aeon_b = 50.0f;
                sh[idx].entropy_cursor =
                    (float)(((N_NODES * PARTICLE_COUNT + n * 48 + idx) * 200) % ENTROPY_SIZE);
            }
        }
        glGenBuffers(1, &ssbo_shadow[n]);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_shadow[n]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, 48 * sizeof(ShadowEntity), sh, GL_DYNAMIC_COPY);
        free(sh);

        // Boundary memory SSBO
        glGenBuffers(1, &ssbo_boundary[n]);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_boundary[n]);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
                     BOUNDARY_MEM_RES * BOUNDARY_MEM_RES * 2 * sizeof(float),
                     NULL, GL_DYNAMIC_DRAW);
    }

    // Shared SSBOs (entropy + FETU)
    glGenBuffers(1, &ssbo_entropy);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_entropy);
    glBufferData(GL_SHADER_STORAGE_BUFFER, ENTROPY_SIZE * sizeof(float),
                 entropy_table, GL_STATIC_READ);
    free(entropy_table);

    float fetu_buf[FETU_PT_COUNT * 2];
    for (int i = 0; i < FETU_PT_COUNT; i++) {
        fetu_buf[i*2]   = G_fetu_px[i];
        fetu_buf[i*2+1] = G_fetu_py[i];
    }
    glGenBuffers(1, &ssbo_fetu);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_fetu);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(fetu_buf), fetu_buf, GL_STATIC_READ);

    // Trail FBO (ping-pong)
    GLuint trail_tex[2], trail_fbo[2];
    glGenTextures(2, trail_tex);
    glGenFramebuffers(2, trail_fbo);
    for (int i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, trail_tex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WIDTH, HEIGHT, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, trail_fbo[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, trail_tex[i], 0);
        glClearColor(BACKGROUND_R/255.0f, BACKGROUND_G/255.0f, BACKGROUND_B/255.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Compile shaders
    char *csrc, *vsrc, *fsrc;
    GLuint prog_compute, prog_render, prog_fade;
    GLuint prog_shadow_compute, prog_shadow_render;

    csrc = read_file("shaders/physics.comp");
    { GLuint sh = compile_shader(GL_COMPUTE_SHADER, csrc);
      prog_compute = link_program(1, sh); free(csrc); }

    csrc = read_file("shaders/shadow.comp");
    { GLuint sh = compile_shader(GL_COMPUTE_SHADER, csrc);
      prog_shadow_compute = link_program(1, sh); free(csrc); }

    vsrc = read_file("shaders/render.vert");
    fsrc = read_file("shaders/render.frag");
    { GLuint vs = compile_shader(GL_VERTEX_SHADER,   vsrc);
      GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fsrc);
      prog_render = link_program(2, vs, fs); free(vsrc); free(fsrc); }

    vsrc = read_file("shaders/shadow.vert");
    fsrc = read_file("shaders/render.frag");
    { GLuint vs = compile_shader(GL_VERTEX_SHADER,   vsrc);
      GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fsrc);
      prog_shadow_render = link_program(2, vs, fs); free(vsrc); free(fsrc); }

    vsrc = read_file("shaders/fade.vert");
    fsrc = read_file("shaders/fade.frag");
    { GLuint vs = compile_shader(GL_VERTEX_SHADER,   vsrc);
      GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fsrc);
      prog_fade = link_program(2, vs, fs); free(vsrc); free(fsrc); }

    // Cache uniform locations
    GLint pu_global_angle   = glGetUniformLocation(prog_compute, "u_global_angle");
    GLint pu_coh_r          = glGetUniformLocation(prog_compute, "u_coherence_radius");
    GLint pu_lrb            = glGetUniformLocation(prog_compute, "u_locus_rotation_bias");
    GLint pu_triquatra      = glGetUniformLocation(prog_compute, "u_triquatra");
    GLint pu_loci_count     = glGetUniformLocation(prog_compute, "u_loci_count");
    GLint pu_particle_count = glGetUniformLocation(prog_compute, "u_particle_count");
    GLint pu_time_step      = glGetUniformLocation(prog_compute, "u_time_step");
    GLint pu_node_center    = glGetUniformLocation(prog_compute, "u_node_center");

    GLint su_global_angle   = glGetUniformLocation(prog_shadow_compute, "u_global_angle");
    GLint su_coh_r          = glGetUniformLocation(prog_shadow_compute, "u_coherence_radius");
    GLint su_shadow_angle   = glGetUniformLocation(prog_shadow_compute, "u_shadow_angle");
    GLint su_primary_stress = glGetUniformLocation(prog_shadow_compute, "u_primary_stress");
    GLint su_time_step      = glGetUniformLocation(prog_shadow_compute, "u_time_step");
    GLint su_node_center    = glGetUniformLocation(prog_shadow_compute, "u_node_center");
    GLint su_corner_n       = glGetUniformLocation(prog_shadow_compute, "u_shadow_corner_n");
    GLint su_offset         = glGetUniformLocation(prog_shadow_compute, "u_shadow_offset");

    GLint ru_global_angle   = glGetUniformLocation(prog_render, "u_global_angle");
    GLint ru_screen_size    = glGetUniformLocation(prog_render, "u_screen_size");
    GLint ru_point_size     = glGetUniformLocation(prog_render, "u_point_size");

    GLint sru_global_angle  = glGetUniformLocation(prog_shadow_render, "u_global_angle");
    GLint sru_screen_size   = glGetUniformLocation(prog_shadow_render, "u_screen_size");
    GLint sru_shadow_color  = glGetUniformLocation(prog_shadow_render, "u_shadow_color");
    GLint sru_point_size    = glGetUniformLocation(prog_shadow_render, "u_point_size");

    GLint fu_trail          = glGetUniformLocation(prog_fade, "u_trail");
    GLint fu_fade_amount    = glGetUniformLocation(prog_fade, "u_fade_amount");

    // Upload static shadow uniforms (same for all nodes)
    glUseProgram(prog_shadow_compute);
    glUniform2fv(su_corner_n, 48, shadow_corner_flat);
    glUniform2fv(su_offset,   48, shadow_offset_flat);

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Simulation state
    float global_angle = 0.0f;
    int   frame_count  = 0;
    bool  is_void      = false;
    int   cur_trail    = 0;

    // Recording
    bool    recording       = false;
    int     record_frame    = 0;
    int     screenshot_next = 0;
    unsigned char *pixel_buf = malloc(WIDTH * HEIGHT * 3);
    ensure_dir(RECORDING_DIR);

    double last_frame_time = glfwGetTime();
    int key_s_prev = GLFW_RELEASE;
    int key_r_prev = GLFW_RELEASE;
    int key_p_prev = GLFW_RELEASE;

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();

        if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(win, GLFW_TRUE);

        int key_s = glfwGetKey(win, GLFW_KEY_S);
        int key_r = glfwGetKey(win, GLFW_KEY_R);
        int key_p = glfwGetKey(win, GLFW_KEY_P);
        if (key_s == GLFW_PRESS && key_s_prev == GLFW_RELEASE) screenshot_next = 1;
        if (key_r == GLFW_PRESS && key_r_prev == GLFW_RELEASE) { recording = true;  fprintf(stderr, "Recording started\n"); }
        if (key_p == GLFW_PRESS && key_p_prev == GLFW_RELEASE) { recording = false; fprintf(stderr, "Recording stopped (%d frames)\n", record_frame); }
        key_s_prev = key_s; key_r_prev = key_r; key_p_prev = key_p;

        if (frame_count == VOID_TRANSITION_FRAME) {
            is_void = true;
            glfwSetWindowTitle(win, "ALQC v24: NULL:DEATH STATE");
        }

        // Shared angle update
        global_angle += L_RB_MAX_RATE * SIM_SPEED;

        // Bind shared SSBOs (entropy + FETU stay at binding 1,3 throughout)
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_entropy);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_fetu);

        // =====================================================================
        // Per-node physics pass
        // =====================================================================
        for (int n = 0; n < N_NODES; n++) {

            // Stress from last frame's readback
            float stress_acc = 0.0f;
            for (int i = 0; i < PARTICLE_COUNT; i++) {
                float vx = readback[n][i].dx, vy = readback[n][i].dy;
                stress_acc += sqrtf(vx*vx + vy*vy);
            }
            node_stress[n] = stress_acc;

            // Coherence radius
            float norm_s = tanhf(node_stress[n] / MAX_KINETIC_STR);
            float shadow_total = (1.0f - norm_s) * MAX_KINETIC_STR;
            float combined = (node_stress[n] + shadow_total) / 2.0f;
            float af = 1.0f - (7.83f / 852.0f);
            float curr_stress = combined * af * af;
            float sf = 1.0f - curr_stress / (MAX_KINETIC_STR + 1e-9f);
            node_coh_r[n] = MIN_COH_R + (MAX_COH_R - MIN_COH_R) * sf;

            // Locus rotation bias
            float norm_lrb = tanhf(curr_stress / MAX_KINETIC_STR);
            node_lrb[n] += L_RB_MAX_RATE * (1.0f - norm_lrb) * ELVEN_RESP_GAIN * 10.0f * SIM_SPEED;

            // Triquatra loci (relative to this node's screen center)
            float tri_px[3], tri_py[3];
            int loci_count;
            if (!is_void) {
                for (int k = 0; k < 3; k++) {
                    float t = node_lrb[n] + k * 2.0f * PI_F / 3.0f;
                    tri_px[k] = node_cx[n] + TRIQUATRA_RADIUS * cosf(t);
                    tri_py[k] = node_cy[n] + TRIQUATRA_RADIUS * sinf(t);
                }
                loci_count = 3;
            } else {
                tri_px[0] = node_cx[n]; tri_py[0] = node_cy[n];
                loci_count = 1;
            }

            // Boundary memory decay + deposit from previous frame's readback
            bm_decay_node(n);
            for (int i = 0; i < PARTICLE_COUNT; i++)
                cpu_bm_deposit_from_particle(&readback[n][i], global_angle,
                                             node_cx[n], node_cy[n], n);

            // Upload BM SSBO for this node
            {
                static float bm_flat[BOUNDARY_MEM_RES * BOUNDARY_MEM_RES * 2];
                for (int r = 0; r < BOUNDARY_MEM_RES; r++)
                    for (int c = 0; c < BOUNDARY_MEM_RES; c++) {
                        bm_flat[(r*BOUNDARY_MEM_RES+c)*2]   = G_mem_vx[n][r][c];
                        bm_flat[(r*BOUNDARY_MEM_RES+c)*2+1] = G_mem_vy[n][r][c];
                    }
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_boundary[n]);
                glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(bm_flat), bm_flat);
            }

            // Bind per-node SSBOs
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_particles[n]);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_boundary[n]);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_shadow[n]);

            // Dispatch shadow.comp
            glUseProgram(prog_shadow_compute);
            glUniform1f(su_global_angle,   global_angle);
            glUniform1f(su_coh_r,          node_coh_r[n]);
            glUniform1f(su_shadow_angle,   node_shadow_angle[n]);
            glUniform1f(su_primary_stress, node_stress[n]);
            glUniform1f(su_time_step,      SIM_SPEED);
            glUniform2f(su_node_center,    node_cx[n], node_cy[n]);
            glDispatchCompute(1, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

            node_shadow_angle[n] += 0.05f * SIM_SPEED;

            // Dispatch physics.comp
            glUseProgram(prog_compute);
            glUniform1f(pu_global_angle,   global_angle);
            glUniform1f(pu_coh_r,          node_coh_r[n]);
            glUniform1f(pu_lrb,            node_lrb[n]);
            {
                float tri_flat[6] = {
                    tri_px[0], tri_py[0], tri_px[1], tri_py[1], tri_px[2], tri_py[2]
                };
                glUniform2fv(pu_triquatra, 3, tri_flat);
            }
            glUniform1i(pu_loci_count,     loci_count);
            glUniform1ui(pu_particle_count,(GLuint)PARTICLE_COUNT);
            glUniform1f(pu_time_step,      SIM_SPEED);
            glUniform2f(pu_node_center,    node_cx[n], node_cy[n]);

            GLuint groups = ((GLuint)PARTICLE_COUNT + 255u) / 256u;
            glDispatchCompute(groups, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

            // Readback every 4 frames
            if (frame_count % 4 == 0) {
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_particles[n]);
                glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                                   PARTICLE_COUNT * sizeof(ParticleGPU), readback[n]);

                bool any_reinit = false;
                for (int i = 0; i < PARTICLE_COUNT; i++) {
                    float R = sqrtf(readback[n][i].x*readback[n][i].x
                                  + readback[n][i].y*readback[n][i].y
                                  + readback[n][i].z*readback[n][i].z
                                  + readback[n][i].w*readback[n][i].w);
                    if (R > ESCAPE_LIMIT) {
                        int   ai    = entropy_int(&G_entropy, 0, 143);
                        int   pi    = ai % AEON_COUNT;
                        float t     = (float)entropy_uniform(&G_entropy, 0.0, 2.0 * PI_F);
                        float scale = 0.5f;
                        readback[n][i].x = scale * cosf(t) + 0.1f*(float)entropy_rand(&G_entropy);
                        readback[n][i].y = scale * sinf(t*3.0f) + 0.1f*(float)entropy_rand(&G_entropy);
                        readback[n][i].z = readback[n][i].w = 0.0f;
                        float base = fabsf(AEON_FREQ[pi]) / 10000.0f;
                        float fl   = fabsf((float)entropy_gauss(&G_entropy, 0.0, 1.0));
                        float sp   = base * (1.0f + fl / fmaxf(fabsf(AEON_FREQ[pi]), 1.0f));
                        readback[n][i].dx = sinf(t)*sp;
                        readback[n][i].dy = cosf(t*2.0f)*sp;
                        readback[n][i].dz = sinf(t*3.5f)*sp;
                        readback[n][i].dw = cosf(t*1.5f)*sp;
                        readback[n][i].stress = readback[n][i].seam_charge = 0.0f;
                        readback[n][i].reflect_charge = readback[n][i].reflect_age = 0.0f;
                        readback[n][i].freq   = fabsf(AEON_FREQ[pi]);
                        readback[n][i].aeon_r = (ai < 12) ? AEON_COLOR[pi][0] : 100.0f;
                        readback[n][i].aeon_g = (ai < 12) ? AEON_COLOR[pi][1] : 100.0f;
                        readback[n][i].aeon_b = (ai < 12) ? AEON_COLOR[pi][2] : 100.0f;
                        readback[n][i].entropy_cursor = (float)(((n*PARTICLE_COUNT+i)*200) % ENTROPY_SIZE);
                        any_reinit = true;
                    }
                }
                if (any_reinit) {
                    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_particles[n]);
                    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                                    PARTICLE_COUNT * sizeof(ParticleGPU), readback[n]);
                }

                // Shadow BM deposit
                {
                    ShadowEntity shadow_rb[48];
                    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_shadow[n]);
                    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                                       48 * sizeof(ShadowEntity), shadow_rb);
                    for (int i = 0; i < 48; i++)
                        cpu_bm_deposit_from_particle((ParticleGPU*)&shadow_rb[i],
                                                     global_angle, node_cx[n], node_cy[n], n);
                }
            }
        } // end per-node physics

        // =====================================================================
        // Render pass — all nodes additively to trail FBO
        // =====================================================================
        int next_trail = 1 - cur_trail;
        glBindFramebuffer(GL_FRAMEBUFFER, trail_fbo[next_trail]);
        glViewport(0, 0, WIDTH, HEIGHT);
        glDisable(GL_BLEND);
        glUseProgram(prog_fade);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, trail_tex[cur_trail]);
        glUniform1i(fu_trail,       0);
        glUniform1f(fu_fade_amount, (float)TRAIL_FADE);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // additive — v10.py BLEND_RGBA_ADD
        glEnable(GL_PROGRAM_POINT_SIZE);

        for (int n = 0; n < N_NODES; n++) {
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

            // Particles
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_particles[n]);
            glUseProgram(prog_render);
            glUniform1f(ru_global_angle, global_angle);
            glUniform2f(ru_screen_size,  WIDTH, HEIGHT);
            glUniform1f(ru_point_size,   PARTICLE_POINT_SIZE);
            glDrawArrays(GL_POINTS, 0, PARTICLE_COUNT);

            // Shadow loci
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_shadow[n]);
            glUseProgram(prog_shadow_render);
            glUniform1f(sru_global_angle, global_angle);
            glUniform2f(sru_screen_size,  WIDTH, HEIGHT);
            glUniform3f(sru_shadow_color, SHADOW_LOCUS_R, SHADOW_LOCUS_G, SHADOW_LOCUS_B);
            glUniform1f(sru_point_size,   SHADOW_POINT_SIZE);
            glDrawArrays(GL_POINTS, 0, 48);
        }

        // Blit trail to screen
        glBindFramebuffer(GL_READ_FRAMEBUFFER, trail_fbo[next_trail]);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, WIDTH, HEIGHT, 0, 0, WIDTH, HEIGHT,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        cur_trail = next_trail;

        glfwSwapBuffers(win);

        // Screenshot / recording
        if (screenshot_next || recording) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
            glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, pixel_buf);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            if (screenshot_next) {
                char path[512];
                snprintf(path, sizeof(path), "%s/screenshot_%04d.ppm", RECORDING_DIR, frame_count);
                save_ppm(path, pixel_buf, WIDTH, HEIGHT);
                fprintf(stderr, "Screenshot: %s\n", path);
                screenshot_next = 0;
            }
            if (recording) {
                char path[512];
                snprintf(path, sizeof(path), "%s/frame_%07d.ppm", RECORDING_DIR, record_frame);
                save_ppm(path, pixel_buf, WIDTH, HEIGHT);
                record_frame++;
            }
        }

        // FPS cap
        if (FPS_CAP > 0) {
            double target = last_frame_time + 1.0 / (double)FPS_CAP;
            double now    = glfwGetTime();
            if (now < target) {
                struct timespec ts;
                double sleep_sec = target - now;
                ts.tv_sec  = (time_t)sleep_sec;
                ts.tv_nsec = (long)((sleep_sec - (double)ts.tv_sec) * 1e9);
                nanosleep(&ts, NULL);
            }
            last_frame_time = glfwGetTime();
        } else {
            last_frame_time = glfwGetTime();
        }

        frame_count++;
    }

    // Cleanup
    for (int n = 0; n < N_NODES; n++) {
        free(readback[n]);
        glDeleteBuffers(1, &ssbo_particles[n]);
        glDeleteBuffers(1, &ssbo_shadow[n]);
        glDeleteBuffers(1, &ssbo_boundary[n]);
    }
    glDeleteBuffers(1, &ssbo_entropy);
    glDeleteBuffers(1, &ssbo_fetu);
    free(pixel_buf);
    glDeleteFramebuffers(2, trail_fbo);
    glDeleteTextures(2, trail_tex);
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
