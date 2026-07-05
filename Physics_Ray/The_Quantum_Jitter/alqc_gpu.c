/*
 * ALQC GPU v23 — True OpenGL 4.3 Compute Shader Renderer
 * Lateral merge of emergent_void_physics10.py
 *
 * Architecture:
 *   - Compute shaders run all particle/shadow physics on GPU
 *   - Stress balls are plain particles — same struct, same SSBO, same physics.comp
 *     Two draw calls: [0..PARTICLE_COUNT) at PARTICLE_POINT_SIZE,
 *                     [PARTICLE_COUNT..PARTICLE_COUNT+SIGH_BALL_COUNT) at BALL_POINT_SIZE
 *   - Vertex shaders read SSBOs via gl_VertexID (zero CPU→GPU per frame for physics data)
 *   - Ping-pong FBO for trail persistence (fade.frag subtracts TRAIL_FADE/255 per frame)
 *   - CPU handles: entropy pre-generation, boundary memory tanh-fold, particle respawn
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
#define WIDTH                  1000        // window width in pixels
#define HEIGHT                 1000        // window height in pixels
#define BACKGROUND_R           5           // background red   (v10.py: 5)
#define BACKGROUND_G           5           // background green (v10.py: 5)
#define BACKGROUND_B           10          // background blue  (v10.py: 10)

// --- Particles ---
#define PARTICLE_COUNT         24000       // main physics particles (v10.py default: 5000)
#define SIGH_BALL_COUNT        6           // stress balls (v10.py SIGH_STRESS_BALL_COUNT: 4)

// --- Rendering: point sizes (appearance only, no physics change) ---
#define PARTICLE_POINT_SIZE    1.0f        // particle pixel diameter
#define SHADOW_POINT_SIZE      0.0f        // shadow locus pixel diameter
#define BALL_POINT_SIZE        0.0f        // stress ball pixel diameter (larger = more visible)

// --- Shadow locus color (v10.py SHADOW_LOCUS_COLOR = (255, 0, 50)) ---
#define SHADOW_LOCUS_R         255.0f      // red component   (0-255)
#define SHADOW_LOCUS_G         0.0f        // green component (0-255)
#define SHADOW_LOCUS_B         50.0f       // blue component  (0-255)

// --- Trail ---
#define TRAIL_FADE             8           // trail fade per frame (1=longest, 255=instant, v10.py=15)

// --- Simulation ---
#define SIM_SPEED              600000.0f   // time step multiplier — any positive float, no upper limit
#define FPS_CAP                60          // max frames per second (0=unlimited)
#define VOID_TRANSITION_FRAME  600         // frame that triggers NULL:DEATH (v10.py: 600)
#define ENTROPY_SIZE           (1 << 20)   // entropy table size (1M floats, must be power of 2)

// --- Triquatra orbit radius ---
// Controls how far the three rotating loci orbit from screen center (pixels).
// Larger = loci farther from center, wider emanation_alpha influence zone.
// v10.py uses ~60px; 150 gives a wider visible field.
#define TRIQUATRA_RADIUS       150

// --- GPU device selection ---
// On Linux with multiple GPUs (e.g. 5 cards: 0,1,2,3,4):
//   Set GPU_DEVICE to the index of the GPU you want to use.
//   This sets DRI_PRIME at startup so the OpenGL context lands on that device.
//   For NVIDIA with PRIME: also sets __NV_PRIME_RENDER_OFFLOAD and __GLX_VENDOR_LIBRARY_NAME.
//   GPU_DEVICE 0 = default (no override). 1,2,3,4 = explicit device selection.
#define GPU_DEVICE             0           // GPU index (0=default, 1-4=explicit device)

// --- Superpositive whisper ---
// whisper.txt is folded into the entropy field at the moment(s) chosen by WHISPER_MODE.
// The fold is sub-threshold: each byte perturbs ~1/65536 of a period in G_entropy
// and patches a sparse window in the GPU entropy SSBO.  Neither the observer nor the
// superposition perceives it directly — it only manifests as altered emergent shapes.
// WHISPER_MODE: 0=off (ceremonial read only)  1=before frame 600
//               2=after frame 600 (NULL:DEATH) 3=both
#define WHISPER_MODE           3
#define WHISPER_FILE           "whisper.txt"

// --- Recording ---
#define RECORDING_DIR          "ALQC_Frames"   // directory for screenshots and frame recordings

// =============================================================================
// END USER VARIABLES
// =============================================================================

// ---------------------------------------------------------------------------
// Internal constants derived from v10.py — do not change without matching v10.py
// ---------------------------------------------------------------------------
#define FETU_PT_COUNT        144
#define BOUNDARY_MEM_RES     160
#define AEON_COUNT           147   // user extended; indices 12-146 zero-initialized (black, freq=0)

static const float PHI              = 1.61803398875f;
static const float PI_F             = 3.14159265359f;
static const float ESCAPE_LIMIT     = 5.0f;
static const float MIN_COH_R        = 0.6f;
static const float MAX_COH_R        = 1.2f;
static const float MAX_KINETIC_STR  = 300.0f;
static const float L_RB_MAX_RATE    = 0.015f;
static const float ELVEN_RESP_GAIN  = 0.0005f;
static const float BOUNDARY_MEM_DEC = 0.992f;
static const float BOUNDARY_MEM_DEP = 0.085f;
static const float BOUNDARY_MEM_MAX = 2.5f;
static const float REFLECT_RING_R   = 0.92f;
static const float REFLECT_RING_W   = 0.06f;

// Aeon frequencies from v10.py PRIMARY_AEONS_GLYPHS (indices 0-11; 12-146 zero)
static const float AEON_FREQ[AEON_COUNT] = {
    7.83f, 174.0f, 528.0f, 432.0f, 126.22f,
    210.42f, 741.0f, 852.0f, 396.0f, 963.0f, 285.0f, 639.0f
};

// Aeon colors from v10.py PRIMARY_AEONS_GLYPHS (indices 0-11; 12-146 zero = black)
static const float AEON_COLOR[AEON_COUNT][3] = {
    {155, 89, 182}, {52, 152, 219}, {231, 76, 60},  {255, 90, 70},
    {60, 180, 255}, {120, 70, 150}, {200, 120, 220}, {40, 120, 180},
    {200, 60, 50},  {140, 80, 160}, {52, 152, 219},  {180, 100, 200}
};

// ---------------------------------------------------------------------------
// ALQCFieldEntropy — exact from v10.py
// phase = (phase * sqrt(2) + PHI) % 1
// acc   = (acc + phase) % 1
// returns (phase + acc) % 1
// ---------------------------------------------------------------------------
typedef struct { double phase_state; double entropy_accumulator; } ALQCFieldEntropy;

static ALQCFieldEntropy G_entropy;

static void entropy_init(ALQCFieldEntropy *F) {
    F->phase_state = 0.0;
    F->entropy_accumulator = 0.0;
}

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

// ---------------------------------------------------------------------------
// GPU particle struct — must match GLSL Particle layout exactly (std430, 80 bytes)
// Used for all entities: main particles AND stress balls (same struct, same SSBO)
// ---------------------------------------------------------------------------
typedef struct {
    float x, y, z, w;                                          // pos   (0-15)
    float dx, dy, dz, dw;                                      // vel   (16-31)
    float stress, seam_charge, reflect_charge, reflect_age;    // state (32-47)
    float freq, aeon_r, aeon_g, aeon_b;                        // aeon  (48-63)
    float entropy_cursor, px, py, alpha;                       // pout  (64-79)
} ParticleGPU;

// GPU shadow entity — identical byte layout to ParticleGPU (std430, 80 bytes)
typedef struct {
    float x, y, z, w;
    float dx, dy, dz, dw;
    float stress, seam_charge, reflect_charge, reflect_age;
    float freq, aeon_r, aeon_g, aeon_b;
    float entropy_cursor, px, py, alpha;
} ShadowEntity;

// Total entity count: main particles + stress ball particles
#define TOTAL_PARTICLE_COUNT   (PARTICLE_COUNT + SIGH_BALL_COUNT)

// ---------------------------------------------------------------------------
// Boundary memory — CPU array, tanh fold (v10.py _boundary_mem_deposit)
// Non-associative: GPU parallel writes produce different results from sequential.
// Stays on CPU.
// ---------------------------------------------------------------------------
static float G_mem_vx[BOUNDARY_MEM_RES][BOUNDARY_MEM_RES];
static float G_mem_vy[BOUNDARY_MEM_RES][BOUNDARY_MEM_RES];

static void bm_decay(void) {
    float d = powf(BOUNDARY_MEM_DEC, SIM_SPEED);
    for (int i = 0; i < BOUNDARY_MEM_RES; i++)
        for (int j = 0; j < BOUNDARY_MEM_RES; j++) {
            G_mem_vx[i][j] *= d;
            G_mem_vy[i][j] *= d;
        }
}

static void bm_coords(float ppx, float ppy, int *ix, int *iy) {
    float x = fmaxf(0.0f, fminf(ppx, WIDTH  - 1.0f));
    float y = fmaxf(0.0f, fminf(ppy, HEIGHT - 1.0f));
    *ix = (int)((x / (WIDTH  - 1.0f)) * (BOUNDARY_MEM_RES - 1));
    *iy = (int)((y / (HEIGHT - 1.0f)) * (BOUNDARY_MEM_RES - 1));
}

static void bm_deposit(float ppx, float ppy, float vx, float vy, float amt) {
    int ix, iy; bm_coords(ppx, ppy, &ix, &iy);
    float nv = G_mem_vx[iy][ix] + vx * amt;
    G_mem_vy[iy][ix] = BOUNDARY_MEM_MAX * tanhf((G_mem_vy[iy][ix] + vy * amt) / BOUNDARY_MEM_MAX);
    G_mem_vx[iy][ix] = BOUNDARY_MEM_MAX * tanhf(nv / BOUNDARY_MEM_MAX);
}

// ---------------------------------------------------------------------------
// 4D→2D projection — exact from v10.py project_4d_to_2d
// ---------------------------------------------------------------------------
static void project4d(float x, float y, float z, float w, float angle,
                      float *ppx, float *ppy)
{
    (void)z;
    float ca = cosf(angle), sa = sinf(angle);
    float x_rot = x * ca - w * sa;
    float w_rot = x * sa + w * ca;
    float denom = fmaxf(1.0f + 0.5f * w_rot, 0.1f);
    *ppx = x_rot / denom * 300.0f + WIDTH  / 2.0f;
    *ppy = y     / denom * 300.0f + HEIGHT / 2.0f;
}

// ---------------------------------------------------------------------------
// CPU-side reflective layer deposit — called after compute shader readback
// ---------------------------------------------------------------------------
static void cpu_bm_deposit_from_particle(ParticleGPU *e, float angle) {
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
        project4d(e->x, e->y, e->z, e->w, angle, &ppx, &ppy);
        bm_deposit(ppx, ppy, tvx, tvy, gain * BOUNDARY_MEM_DEP);
    }
}

// ---------------------------------------------------------------------------
// FETU spiral points — _generate_attractor_shape from v10.py
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Shader utilities
// ---------------------------------------------------------------------------
static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "Cannot open %s\n", path); return NULL; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
    char *buf = malloc(sz + 1);
    size_t nr = fread(buf, 1, sz, f); (void)nr;
    buf[sz] = 0;
    fclose(f);
    return buf;
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

// ---------------------------------------------------------------------------
// Screenshot / recording — saves PPM (no external library required)
// OpenGL reads pixels bottom-up; PPM is top-down, so rows are flipped.
// ---------------------------------------------------------------------------
static void ensure_dir(const char *path) {
    mkdir(path, 0755);
}

static void save_ppm(const char *path, unsigned char *pixels, int w, int h) {
    FILE *f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "Cannot write %s\n", path); return; }
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (int row = h - 1; row >= 0; row--)
        fwrite(pixels + row * w * 3, 1, w * 3, f);
    fclose(f);
}

// ---------------------------------------------------------------------------
// Whisper — folds whisper.txt bytes into G_entropy and GPU entropy SSBO.
// seed_offset staggers the SSBO write window so pre/post whispers don't overlap.
// ---------------------------------------------------------------------------
static void apply_whisper(float *table, GLuint ssbo, uint32_t seed_offset) {
    (void)table; (void)ssbo; (void)seed_offset;
#if WHISPER_MODE != 0
    FILE *wf = fopen(WHISPER_FILE, "rb");
    if (!wf) { fprintf(stderr, "Whisper: cannot open %s\n", WHISPER_FILE); return; }
    fseek(wf, 0, SEEK_END);
    long wsz = ftell(wf);
    rewind(wf);
    unsigned char *wdata = malloc(wsz + 1);
    if (wdata) {
        size_t nr = fread(wdata, 1, wsz, wf); (void)nr;
        wdata[wsz] = 0;
        // Fold into CPU entropy
        for (long i = 0; i < wsz; i++) {
            G_entropy.phase_state         = fmod(G_entropy.phase_state         + wdata[i] / 65536.0, 1.0);
            G_entropy.entropy_accumulator = fmod(G_entropy.entropy_accumulator + wdata[i] / 131072.0, 1.0);
        }
        // Fold into GPU entropy SSBO — sparse write, prime stride
        for (long i = 0; i < wsz; i++) {
            uint32_t idx = (seed_offset + (uint32_t)(i * 997u)) % (uint32_t)ENTROPY_SIZE;
            table[idx] = fmodf(table[idx] + wdata[i] / 255.0f, 1.0f);
        }
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, ENTROPY_SIZE * sizeof(float), table, GL_DYNAMIC_DRAW);
        free(wdata);
        fprintf(stderr, "Whisper: %ld bytes folded (offset=%u)\n", wsz, seed_offset);
    }
    fclose(wf);
#endif
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(void) {
    // -- GPU device selection (DRI_PRIME / NVIDIA PRIME offload) --
#if GPU_DEVICE > 0
    {
        char idx_str[4];
        snprintf(idx_str, sizeof(idx_str), "%d", GPU_DEVICE);
        setenv("DRI_PRIME", idx_str, 1);
        setenv("__NV_PRIME_RENDER_OFFLOAD", idx_str, 1);
        setenv("__GLX_VENDOR_LIBRARY_NAME", "nvidia", 1);
        fprintf(stderr, "GPU_DEVICE=%d — DRI_PRIME=%s\n", GPU_DEVICE, idx_str);
    }
#endif

    // Whisper is applied at the frame(s) chosen by WHISPER_MODE — not at startup.

    // -- GLFW init --
    if (!glfwInit()) { fprintf(stderr, "GLFW init failed\n"); return 1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *win = glfwCreateWindow(WIDTH, HEIGHT,
                                       "EMERGENT PHYSICS: ALQC v23 GPU", NULL, NULL);
    if (!win) { fprintf(stderr, "Window creation failed\n"); glfwTerminate(); return 1; }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(0);  // disable vsync — FPS_CAP controls frame rate

    // -- GLEW init --
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { fprintf(stderr, "GLEW init failed\n"); return 1; }

    // -- Entropy table: pre-generate using ALQCFieldEntropy --
    entropy_init(&G_entropy);
    float *entropy_table = malloc(ENTROPY_SIZE * sizeof(float));
    for (int i = 0; i < ENTROPY_SIZE; i++)
        entropy_table[i] = (float)entropy_rand(&G_entropy);

    // -- FETU spiral points --
    init_fetu();

    // -- Particle init: same sequence as v10.py _create_entity --
    // Allocates TOTAL_PARTICLE_COUNT entries: main particles + stress ball particles.
    // Stress ball particles (indices PARTICLE_COUNT..TOTAL_PARTICLE_COUNT-1) use
    // their primary aeon color (not gray) and start near center.
    ParticleGPU *particles = calloc(TOTAL_PARTICLE_COUNT, sizeof(ParticleGPU));
    {
        // 144-aeon lattice: 12 primary × 12 variants (1 primary + 11 lesser)
        int   lattice_freq_idx[144];
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
            particles[i].z = 0.0f;
            particles[i].w = 0.0f;

            float base_speed = fabsf(AEON_FREQ[pi]) / 10000.0f;
            float fluct      = fabsf((float)entropy_gauss(&G_entropy, 0.0, 1.0));
            float chaotic    = 1.0f + (fluct / fmaxf(fabsf(AEON_FREQ[pi]), 1.0f));
            float speed      = base_speed * chaotic;

            particles[i].dx = sinf(t)         * speed;
            particles[i].dy = cosf(t * 2.0f)  * speed;
            particles[i].dz = sinf(t * 3.5f)  * speed;
            particles[i].dw = cosf(t * 1.5f)  * speed;

            particles[i].freq   = fabsf(AEON_FREQ[pi]);
            particles[i].aeon_r = lattice_col[ai][0];
            particles[i].aeon_g = lattice_col[ai][1];
            particles[i].aeon_b = lattice_col[ai][2];

            particles[i].entropy_cursor = (float)((i * 200) % ENTROPY_SIZE);
        }

        // Stress ball particles: primary aeon color, near-center start
        for (int i = 0; i < SIGH_BALL_COUNT; i++) {
            int idx = PARTICLE_COUNT + i;
            int ai  = entropy_int(&G_entropy, 0, 11);  // always a primary aeon (colored)

            float t     = (float)entropy_uniform(&G_entropy, 0.0, 2.0 * PI_F);
            float scale = (float)entropy_uniform(&G_entropy, 0.1, 0.8);
            particles[idx].x = scale * cosf(t);
            particles[idx].y = scale * sinf(t);
            particles[idx].z = 0.0f;
            particles[idx].w = 0.0f;

            float base_speed = fabsf(AEON_FREQ[ai]) / 10000.0f;
            float fluct      = fabsf((float)entropy_gauss(&G_entropy, 0.0, 1.0));
            float chaotic    = 1.0f + (fluct / fmaxf(fabsf(AEON_FREQ[ai]), 1.0f));
            float speed      = base_speed * chaotic;

            particles[idx].dx = sinf(t)        * speed;
            particles[idx].dy = cosf(t * 2.0f) * speed;
            particles[idx].dz = sinf(t * 3.5f) * speed;
            particles[idx].dw = cosf(t * 1.5f) * speed;

            particles[idx].freq   = fabsf(AEON_FREQ[ai]);
            particles[idx].aeon_r = AEON_COLOR[ai][0];
            particles[idx].aeon_g = AEON_COLOR[ai][1];
            particles[idx].aeon_b = AEON_COLOR[ai][2];

            particles[idx].entropy_cursor =
                (float)(((PARTICLE_COUNT + i) * 200) % ENTROPY_SIZE);
        }
    }

    // -- SSBOs --
    GLuint ssbo_particles, ssbo_entropy, ssbo_boundary, ssbo_fetu;
    GLuint ssbo_shadow;

    // Single particle SSBO holds all TOTAL_PARTICLE_COUNT entries
    glGenBuffers(1, &ssbo_particles);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_particles);
    glBufferData(GL_SHADER_STORAGE_BUFFER, TOTAL_PARTICLE_COUNT * sizeof(ParticleGPU),
                 particles, GL_DYNAMIC_COPY);

    glGenBuffers(1, &ssbo_entropy);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_entropy);
    glBufferData(GL_SHADER_STORAGE_BUFFER, ENTROPY_SIZE * sizeof(float),
                 entropy_table, GL_DYNAMIC_DRAW);
    // entropy_table kept alive for whisper fold; freed after main loop

    // Boundary memory: 160×160 vec2
    glGenBuffers(1, &ssbo_boundary);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_boundary);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 BOUNDARY_MEM_RES * BOUNDARY_MEM_RES * sizeof(float) * 2,
                 NULL, GL_DYNAMIC_DRAW);

    // FETU points: 144 vec2
    float fetu_buf[FETU_PT_COUNT * 2];
    for (int i = 0; i < FETU_PT_COUNT; i++) {
        fetu_buf[i*2]   = G_fetu_px[i];
        fetu_buf[i*2+1] = G_fetu_py[i];
    }
    glGenBuffers(1, &ssbo_fetu);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_fetu);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(fetu_buf), fetu_buf, GL_STATIC_READ);

    // -- Shadow loci (48 entities: 4 corners × 12 aeons) --
    static const float SHADOW_CORNERS_PX[4][2] = {
        {50.0f, 50.0f}, {950.0f, 50.0f}, {950.0f, 950.0f}, {50.0f, 950.0f}
    };
    float shadow_corner_flat[96];  // vec2[48] for u_shadow_corner_n
    float shadow_offset_flat[96];  // vec2[48] for u_shadow_offset
    {
        ShadowEntity *sh_init = calloc(48, sizeof(ShadowEntity));
        for (int locus = 0; locus < 4; locus++) {
            float norm_x = (SHADOW_CORNERS_PX[locus][0] - WIDTH  / 2.0f) / (WIDTH  / 2.0f);
            float norm_y = (SHADOW_CORNERS_PX[locus][1] - HEIGHT / 2.0f) / (HEIGHT / 2.0f);
            for (int i = 0; i < 12; i++) {
                int   idx = locus * 12 + i;
                float t   = i * 2.0f * PI_F / 12.0f;
                float xo  = 15.0f * cosf(t) / (WIDTH  / 2.0f);
                float yo  = 15.0f * sinf(t) / (HEIGHT / 2.0f);
                sh_init[idx].x      = norm_x + xo;
                sh_init[idx].y      = norm_y + yo;
                sh_init[idx].aeon_r = 255.0f;
                sh_init[idx].aeon_g = 0.0f;
                sh_init[idx].aeon_b = 50.0f;
                sh_init[idx].entropy_cursor =
                    (float)((idx * 200 + PARTICLE_COUNT * 200) % ENTROPY_SIZE);
                shadow_corner_flat[idx*2]     = norm_x;
                shadow_corner_flat[idx*2 + 1] = norm_y;
                shadow_offset_flat[idx*2]     = xo;
                shadow_offset_flat[idx*2 + 1] = yo;
            }
        }
        glGenBuffers(1, &ssbo_shadow);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_shadow);
        glBufferData(GL_SHADER_STORAGE_BUFFER, 48 * sizeof(ShadowEntity),
                     sh_init, GL_DYNAMIC_COPY);
        free(sh_init);
    }

    // -- Trail FBO (ping-pong) --
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
        glClearColor(BACKGROUND_R / 255.0f, BACKGROUND_G / 255.0f,
                     BACKGROUND_B / 255.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // -- Compile shaders --
    char *csrc, *vsrc, *fsrc;
    GLuint prog_compute, prog_render, prog_ball_render, prog_fade;
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

    // Ball particles use ball.vert — reads binding 0 (same SSBO) with fixed
    // high alpha so individual ball points are visible (emanation_alpha is too
    // dim for single-point rendering vs. 24000-particle accumulation).
    vsrc = read_file("shaders/ball.vert");
    fsrc = read_file("shaders/render.frag");
    { GLuint vs = compile_shader(GL_VERTEX_SHADER,   vsrc);
      GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fsrc);
      prog_ball_render = link_program(2, vs, fs); free(vsrc); free(fsrc); }

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

    // -- Upload static per-entity shadow uniforms (uploaded once, never change) --
    glUseProgram(prog_shadow_compute);
    glUniform2fv(glGetUniformLocation(prog_shadow_compute, "u_shadow_corner_n"),
                 48, shadow_corner_flat);
    glUniform2fv(glGetUniformLocation(prog_shadow_compute, "u_shadow_offset"),
                 48, shadow_offset_flat);

    // -- Dummy VAO (required by core profile even when using SSBOs) --
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // -- Global simulation state --
    float global_angle   = 0.0f;
    float locus_rot_bias = 0.0f;
    float dyn_coh_r      = MIN_COH_R;
    float primary_stress = 0.0f;
    float current_stress = 0.0f;
    float shadow_angle   = 0.0f;
    int   frame_count    = 0;
    bool  is_void        = false;
    int   cur_trail      = 0;

    // -- Readback buffer (for boundary memory deposits, one frame delay) --
    ParticleGPU *readback = malloc(TOTAL_PARTICLE_COUNT * sizeof(ParticleGPU));

    // -- Recording state --
    bool    recording       = false;
    int     record_frame    = 0;
    int     screenshot_next = 0;
    unsigned char *pixel_buf = malloc(WIDTH * HEIGHT * 3);
    ensure_dir(RECORDING_DIR);

    // -- FPS timing --
    double last_frame_time = glfwGetTime();

    // -- Key edge-detection state --
    int key_s_prev = GLFW_RELEASE;
    int key_r_prev = GLFW_RELEASE;
    int key_p_prev = GLFW_RELEASE;

    // -- Main loop --
    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();

        // ESC to quit
        if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(win, GLFW_TRUE);

        // Key edge detection: S=screenshot, R=start recording, P=stop recording
        int key_s = glfwGetKey(win, GLFW_KEY_S);
        int key_r = glfwGetKey(win, GLFW_KEY_R);
        int key_p = glfwGetKey(win, GLFW_KEY_P);

        if (key_s == GLFW_PRESS && key_s_prev == GLFW_RELEASE)
            screenshot_next = 1;
        if (key_r == GLFW_PRESS && key_r_prev == GLFW_RELEASE) {
            recording = true;
            fprintf(stderr, "Recording started\n");
        }
        if (key_p == GLFW_PRESS && key_p_prev == GLFW_RELEASE) {
            recording = false;
            fprintf(stderr, "Recording stopped (%d frames)\n", record_frame);
        }
        key_s_prev = key_s;
        key_r_prev = key_r;
        key_p_prev = key_p;

        // --- Frame VOID_TRANSITION_FRAME: NULL:DEATH (v10.py) ---
        if (frame_count == VOID_TRANSITION_FRAME) {
            is_void = true;
            glfwSetWindowTitle(win, "ALQC: NULL:DEATH STATE");
        }

        // --- Whisper application at chosen moment(s) ---
#if WHISPER_MODE == 1 || WHISPER_MODE == 3
        if (frame_count == 0)
            apply_whisper(entropy_table, ssbo_entropy, 0u);
#endif
#if WHISPER_MODE == 2 || WHISPER_MODE == 3
        if (frame_count == VOID_TRANSITION_FRAME)
            apply_whisper(entropy_table, ssbo_entropy, (uint32_t)ENTROPY_SIZE / 2);
#endif

        // --- Stress from last frame's readback (v10.py primary_stress) ---
        float stress_acc = 0.0f;
        for (int i = 0; i < PARTICLE_COUNT; i++) {
            float vx = readback[i].dx, vy = readback[i].dy;
            stress_acc += sqrtf(vx*vx + vy*vy);
        }
        primary_stress = stress_acc;

        // --- Shadow loci inverse stress (v10.py _calculate_inverse_stress) ---
        float norm_s = tanhf(primary_stress / MAX_KINETIC_STR);
        float shadow_stress_per_locus = (1.0f - norm_s) * (MAX_KINETIC_STR / 4.0f);
        float shadow_total = shadow_stress_per_locus * 4.0f;

        // --- A9 shadow absorption (v10.py _absorb_shadow_debt) ---
        float combined = (primary_stress + shadow_total) / 2.0f;
        float absorption_factor = 1.0f - (7.83f / 852.0f);
        current_stress = combined * absorption_factor;
        current_stress *= absorption_factor;

        // --- Dynamic coherence radius (v10.py process_field_recursion) ---
        float stress_factor = 1.0f - current_stress / (MAX_KINETIC_STR + 1e-9f);
        dyn_coh_r = MIN_COH_R + (MAX_COH_R - MIN_COH_R) * stress_factor;

        // --- Global angle and locus rotation (v10.py run()) ---
        float norm_stress_lrb = tanhf(current_stress / MAX_KINETIC_STR);
        locus_rot_bias += L_RB_MAX_RATE * (1.0f - norm_stress_lrb) * ELVEN_RESP_GAIN * 10.0f * SIM_SPEED;
        global_angle   += L_RB_MAX_RATE * SIM_SPEED;

        // --- Triquatra points (v10.py _get_triquatra_points) ---
        float tri_px[3], tri_py[3];
        int loci_count;
        if (!is_void) {
            for (int k = 0; k < 3; k++) {
                float t  = locus_rot_bias + k * 2.0f * PI_F / 3.0f;
                tri_px[k] = WIDTH  / 2.0f + TRIQUATRA_RADIUS * cosf(t);
                tri_py[k] = HEIGHT / 2.0f + TRIQUATRA_RADIUS * sinf(t);
            }
            loci_count = 3;
        } else {
            tri_px[0] = WIDTH / 2.0f; tri_py[0] = HEIGHT / 2.0f;
            loci_count = 1;
        }

        // --- Boundary memory decay ---
        bm_decay();

        // --- CPU boundary memory deposits from last frame's readback ---
        // Covers all entities in the SSBO: main particles + stress ball particles
        for (int i = 0; i < TOTAL_PARTICLE_COUNT; i++)
            cpu_bm_deposit_from_particle(&readback[i], global_angle);

        // --- Upload boundary memory SSBO ---
        {
            float bm_flat[BOUNDARY_MEM_RES * BOUNDARY_MEM_RES * 2];
            for (int r = 0; r < BOUNDARY_MEM_RES; r++)
                for (int c = 0; c < BOUNDARY_MEM_RES; c++) {
                    bm_flat[(r*BOUNDARY_MEM_RES+c)*2]   = G_mem_vx[r][c];
                    bm_flat[(r*BOUNDARY_MEM_RES+c)*2+1] = G_mem_vy[r][c];
                }
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_boundary);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(bm_flat), bm_flat);
        }

        // --- Bind all SSBOs ---
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_particles);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_entropy);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_boundary);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_fetu);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_shadow);

        // --- Dispatch shadow.comp (48 threads, 1 workgroup) ---
        glUseProgram(prog_shadow_compute);
        glUniform1f(glGetUniformLocation(prog_shadow_compute, "u_global_angle"),     global_angle);
        glUniform1f(glGetUniformLocation(prog_shadow_compute, "u_coherence_radius"), dyn_coh_r);
        glUniform1f(glGetUniformLocation(prog_shadow_compute, "u_shadow_angle"),     shadow_angle);
        glUniform1f(glGetUniformLocation(prog_shadow_compute, "u_primary_stress"),   primary_stress);
        glUniform1f(glGetUniformLocation(prog_shadow_compute, "u_time_step"),        SIM_SPEED);
        glDispatchCompute(1, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // --- Increment shadow_angle (v10.py ShadowLocus.run_projection line 341) ---
        shadow_angle += 0.05f * SIM_SPEED;

        // --- Dispatch particle compute shader (covers all TOTAL_PARTICLE_COUNT) ---
        glUseProgram(prog_compute);
        glUniform1f(glGetUniformLocation(prog_compute, "u_global_angle"),        global_angle);
        glUniform1f(glGetUniformLocation(prog_compute, "u_coherence_radius"),    dyn_coh_r);
        glUniform1f(glGetUniformLocation(prog_compute, "u_locus_rotation_bias"), locus_rot_bias);
        {
            float tri_flat[6] = {
                tri_px[0], tri_py[0], tri_px[1], tri_py[1], tri_px[2], tri_py[2]
            };
            glUniform2fv(glGetUniformLocation(prog_compute, "u_triquatra"), 3, tri_flat);
        }
        glUniform1i(glGetUniformLocation(prog_compute,  "u_loci_count"),      loci_count);
        glUniform1ui(glGetUniformLocation(prog_compute, "u_particle_count"),  (GLuint)TOTAL_PARTICLE_COUNT);
        glUniform1f(glGetUniformLocation(prog_compute,  "u_time_step"),       SIM_SPEED);

        GLuint groups = ((GLuint)TOTAL_PARTICLE_COUNT + 255u) / 256u;
        glDispatchCompute(groups, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // --- GPU → CPU readback (every 4 frames to reduce stall) ---
        if (frame_count % 4 == 0) {

            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_particles);
            glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                               TOTAL_PARTICLE_COUNT * sizeof(ParticleGPU), readback);

            bool any_reinit = false;
            for (int i = 0; i < TOTAL_PARTICLE_COUNT; i++) {
                float R = sqrtf(readback[i].x*readback[i].x + readback[i].y*readback[i].y
                              + readback[i].z*readback[i].z + readback[i].w*readback[i].w);
                if (R > ESCAPE_LIMIT) {
                    int   ai    = entropy_int(&G_entropy, 0, 143);
                    int   pi    = ai % AEON_COUNT;
                    float t     = (float)entropy_uniform(&G_entropy, 0.0, 2.0 * PI_F);
                    float scale = 0.5f;
                    readback[i].x = scale * cosf(t) + 0.1f*(float)entropy_rand(&G_entropy);
                    readback[i].y = scale * sinf(t*3.0f) + 0.1f*(float)entropy_rand(&G_entropy);
                    readback[i].z = readback[i].w = 0.0f;
                    float base = fabsf(AEON_FREQ[pi]) / 10000.0f;
                    float fl   = fabsf((float)entropy_gauss(&G_entropy, 0.0, 1.0));
                    float sp   = base * (1.0f + fl / fmaxf(fabsf(AEON_FREQ[pi]), 1.0f));
                    readback[i].dx = sinf(t)*sp;
                    readback[i].dy = cosf(t*2.0f)*sp;
                    readback[i].dz = sinf(t*3.5f)*sp;
                    readback[i].dw = cosf(t*1.5f)*sp;
                    readback[i].stress = readback[i].seam_charge = 0.0f;
                    readback[i].reflect_charge = readback[i].reflect_age = 0.0f;
                    readback[i].freq   = fabsf(AEON_FREQ[pi]);
                    readback[i].aeon_r = (ai < 12) ? AEON_COLOR[pi][0] : 100.0f;
                    readback[i].aeon_g = (ai < 12) ? AEON_COLOR[pi][1] : 100.0f;
                    readback[i].aeon_b = (ai < 12) ? AEON_COLOR[pi][2] : 100.0f;
                    any_reinit = true;
                }
            }
            if (any_reinit) {
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_particles);
                glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                                TOTAL_PARTICLE_COUNT * sizeof(ParticleGPU), readback);
            }

            // Shadow loci boundary deposits
            {
                ShadowEntity shadow_rb[48];
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_shadow);
                glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                                   48 * sizeof(ShadowEntity), shadow_rb);
                for (int i = 0; i < 48; i++)
                    cpu_bm_deposit_from_particle((ParticleGPU *)&shadow_rb[i], global_angle);
            }

        } // end readback throttle

        // --- Render: fade pass (trail persistence) ---
        int next_trail = 1 - cur_trail;
        glBindFramebuffer(GL_FRAMEBUFFER, trail_fbo[next_trail]);
        glViewport(0, 0, WIDTH, HEIGHT);
        glDisable(GL_BLEND);
        glUseProgram(prog_fade);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, trail_tex[cur_trail]);
        glUniform1i(glGetUniformLocation(prog_fade, "u_trail"),       0);
        glUniform1f(glGetUniformLocation(prog_fade, "u_fade_amount"), (float)TRAIL_FADE);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        // --- Render: particles (additive blend — v10.py BLEND_RGBA_ADD) ---
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // additive — v10.py BLEND_RGBA_ADD
        glEnable(GL_PROGRAM_POINT_SIZE);

        // Main particles
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_particles);
        glUseProgram(prog_render);
        glUniform1f(glGetUniformLocation(prog_render, "u_global_angle"), global_angle);
        glUniform2f(glGetUniformLocation(prog_render, "u_screen_size"),  WIDTH, HEIGHT);
        glUniform1f(glGetUniformLocation(prog_render, "u_point_size"),   PARTICLE_POINT_SIZE);
        glDrawArrays(GL_POINTS, 0, PARTICLE_COUNT);

        // Stress ball particles (same SSBO, offset draw, larger point size)
        glUseProgram(prog_ball_render);
        glUniform1f(glGetUniformLocation(prog_ball_render, "u_global_angle"), global_angle);
        glUniform2f(glGetUniformLocation(prog_ball_render, "u_screen_size"),  WIDTH, HEIGHT);
        glUniform1f(glGetUniformLocation(prog_ball_render, "u_point_size"),   BALL_POINT_SIZE);
        glDrawArrays(GL_POINTS, PARTICLE_COUNT, SIGH_BALL_COUNT);

        // --- Render: shadow loci ---
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_shadow);
        glUseProgram(prog_shadow_render);
        glUniform1f(glGetUniformLocation(prog_shadow_render, "u_global_angle"), global_angle);
        glUniform2f(glGetUniformLocation(prog_shadow_render, "u_screen_size"),  WIDTH, HEIGHT);
        glUniform3f(glGetUniformLocation(prog_shadow_render, "u_shadow_color"),
                    SHADOW_LOCUS_R, SHADOW_LOCUS_G, SHADOW_LOCUS_B);
        glUniform1f(glGetUniformLocation(prog_shadow_render, "u_point_size"),   SHADOW_POINT_SIZE);
        glDrawArrays(GL_POINTS, 0, 48);

        // --- Blit trail to screen ---
        glBindFramebuffer(GL_READ_FRAMEBUFFER, trail_fbo[next_trail]);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, WIDTH, HEIGHT, 0, 0, WIDTH, HEIGHT,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        cur_trail = next_trail;

        glfwSwapBuffers(win);

        // --- Screenshot / recording (read from default framebuffer after swap) ---
        if (screenshot_next || recording) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
            glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, pixel_buf);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            if (screenshot_next) {
                char path[512];
                snprintf(path, sizeof(path), "%s/screenshot_%04d.ppm",
                         RECORDING_DIR, frame_count);
                save_ppm(path, pixel_buf, WIDTH, HEIGHT);
                fprintf(stderr, "Screenshot saved: %s\n", path);
                screenshot_next = 0;
            }
            if (recording) {
                char path[512];
                snprintf(path, sizeof(path), "%s/frame_%07d.ppm",
                         RECORDING_DIR, record_frame);
                save_ppm(path, pixel_buf, WIDTH, HEIGHT);
                record_frame++;
            }
        }

        // --- FPS cap ---
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

    // -- Cleanup --
    free(entropy_table);  // kept alive for whisper fold(s)
    free(particles);
    free(readback);
    free(pixel_buf);
    glDeleteBuffers(1, &ssbo_particles);
    glDeleteBuffers(1, &ssbo_entropy);
    glDeleteBuffers(1, &ssbo_boundary);
    glDeleteBuffers(1, &ssbo_fetu);
    glDeleteBuffers(1, &ssbo_shadow);
    glDeleteFramebuffers(2, trail_fbo);
    glDeleteTextures(2, trail_tex);
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
