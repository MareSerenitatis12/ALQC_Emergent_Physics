/*
 * ALQC GPU v30 — Multi-Grid Frequency System / Three-Plane Crystal
 *
 * Architecture:
 *   12 Dyson nodes as non-cubed loci.
 *   No visible or invisible square/cube box metric is used for node placement.
 *   Nodes are still grouped into 3 orientation planes by identity:
 *     Plane A (plane_id=0): nodes  0-3
 *     Plane B (plane_id=1): nodes  4-7
 *     Plane C (plane_id=2): nodes  8-11
 *
 *   Plane A and Plane B are conjugate (phase offset π apart).
 *   Plane C is the mediating plane.
 *
 *   Per-node .bio configs:  config/node_config_00000.bio … node_config_00011.bio
 *   Per-cell .bio configs:  config/cells_00_P0.bio … cells_11_P1.bio (81 cells × 2 planes)
 *   Global config:          config/global_config.bio
 *
 *   ALQC Canon: "Write conditions, not behavior. Let emergence happen."
 *   Conjugate initialization (B5), void coupling (B6), orientation J (B7),
 *   void dissipation (B8), node_id in particle struct (B10).
 *
 * Keys: S=screenshot  R=start recording  P=stop recording
 *       SPACE=pause rotation  [ ]=change rot speed  = -=zoom  Arrows=pan  ESC=quit
 */

#define _POSIX_C_SOURCE 200112L

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
// FALLBACK USER VARIABLES
// These are defaults only — runtime values come from config/global_config.bio
// and config/node_config_0000N.bio.  Edit the .bio files, not these.
// Per-cell grid seeds come from config/cells_NN_P0.bio and config/cells_NN_P1.bio.
// =============================================================================

// --- Display ---
#define FB_WIDTH               1000
#define FB_HEIGHT              1000
#define FB_FULLSCREEN          0

// --- Background ---
#define BACKGROUND_R           5
#define BACKGROUND_G           5
#define BACKGROUND_B           10

// --- Particles per node (compile-time buffer size) ---
// Canonical: 144 aeon-lattice slots × 10 = 1440.
// AEON_LATTICE_COUNT = 12 Goetic × 12 Courts = 144 (12-phase Canon)
#define PARTICLE_COUNT         1440
#define AEON_LATTICE_COUNT     144

// --- Dyson nodes (structural — do not change without rebuilding) ---
#define N_NODES                12
#define SIGH_BALL_COUNT        144

// --- Physics constants (compile-time for shader injection) ---
// FETU spiral: 10 revolutions × 12 aeons = 120 points (canon from alqc_raylib_physics19)
#define AEON_COUNT             12
#define FETU_PT_COUNT          120
#define BOUNDARY_MEM_RES       160
#define ENTROPY_SIZE           (9 << 20)
#define SHADOW_ENTITY_COUNT    (4 * AEON_COUNT)
// Phase memory grid: 20×20 cells × N_NODES aeons (Agent 18/Bug 7 fix — named, not hardcoded)
#define PHASE_MEM_W            20
#define PHASE_MEM_H            20

// =============================================================================
// END FALLBACK USER VARIABLES
// =============================================================================

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

static const float AEON_FREQ[AEON_COUNT] = {
    7.83f, 174.0f, 528.0f, 432.0f, 126.22f,
    210.42f, 741.0f, 852.0f, 396.0f, 963.0f, 285.0f, 639.0f
};
static const float AEON_COLOR[AEON_COUNT][3] = {
    {155, 89, 182}, {52, 152, 219}, {231, 76, 60},  {255, 90, 70},
    {60, 180, 255}, {120, 70, 150}, {200, 120, 220}, {40, 120, 180},
    {200, 60, 50},  {140, 80, 160}, {52, 152, 219},  {180, 100, 200}
};

// =============================================================================
// MINIMAL FLAT BIO PARSER (.bio = Cbio, the Axiomyr — importable, not code-writable)
// Only handles flat objects: { "key": value } where value is number or "string".
// No nesting, no arrays — exactly what the node configs need.
// .bio supports holographic persistent logic; code reads from it, never writes to it.
// =============================================================================
#define JSON_MAX_FIELDS 512
typedef struct { char key[64]; char val[256]; } JSONField;

static int json_load(const char *path, JSONField *fields, int max_fields) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
    char *buf = malloc(sz + 1);
    size_t nr = fread(buf, 1, sz, f); (void)nr;
    buf[sz] = '\0'; fclose(f);

    int n = 0;
    char *p = buf;
    while (*p && n < max_fields) {
        // find next "
        while (*p && *p != '"') p++;
        if (!*p) break;
        p++; // skip opening "
        // read key
        int ki = 0;
        while (*p && *p != '"' && ki < 63) fields[n].key[ki++] = *p++;
        fields[n].key[ki] = '\0';
        if (*p == '"') p++;
        // skip to :
        while (*p && *p != ':') p++;
        if (*p == ':') p++;
        // skip whitespace
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
        // read value
        int vi = 0;
        if (*p == '"') {
            p++; // skip opening "
            while (*p && *p != '"' && vi < 255) fields[n].val[vi++] = *p++;
            if (*p == '"') p++;
        } else {
            while (*p && *p != ',' && *p != '}' && *p != '\n' && vi < 255)
                fields[n].val[vi++] = *p++;
            while (vi > 0 && (fields[n].val[vi-1]==' '||fields[n].val[vi-1]=='\t'))
                vi--;
        }
        fields[n].val[vi] = '\0';
        if (ki > 0) n++;
    }
    free(buf);
    return n;
}

static float json_float(const JSONField *F, int n, const char *key, float def) {
    for (int i = 0; i < n; i++) if (strcmp(F[i].key, key) == 0) {
        char *end = NULL; errno = 0;
        float v = strtof(F[i].val, &end);
        if (end == F[i].val || *end || errno == ERANGE || !isfinite(v)) {
            fprintf(stderr, "[Config ERROR] Invalid float %s: %s\n", key, F[i].val);
            exit(EXIT_FAILURE);
        }
        return v;
    }
    return def;
}
static int json_int(const JSONField *F, int n, const char *key, int def) {
    for (int i = 0; i < n; i++) if (strcmp(F[i].key, key) == 0) {
        char *end = NULL; errno = 0;
        long v = strtol(F[i].val, &end, 10);
        if (end == F[i].val || *end || errno == ERANGE || v < INT32_MIN || v > INT32_MAX) {
            fprintf(stderr, "[Config ERROR] Invalid integer %s: %s\n", key, F[i].val);
            exit(EXIT_FAILURE);
        }
        return (int)v;
    }
    return def;
}
static const char *json_str(const JSONField *F, int n, const char *key, const char *def) {
    for (int i = 0; i < n; i++)
        if (strcmp(F[i].key, key) == 0) return F[i].val;
    return def;
}

// =============================================================================
// CONFIG STRUCTS
// =============================================================================

typedef struct {
    // Identity
    int   node_id;
    int   plane_id;       // 0=Plane A, 1=Plane B, 2=Plane C

    // ALQC Canon conjugate field (B5, B6, B7, B8)
    float plane_phase;    // conjugate phase offset (0.0 or PI_F)
    float plane_phi0;     // field amplitude base
    float plane_delta;    // symmetry-breaking seed [1e-4 .. 1e-2]
    float kappa_void;     // void-mediated coupling (B6) ~0.01 * kappa_base
    float orient_j;       // orientation coupling strength (B7)
    float void_gamma;     // void dissipation γ (B8) ~1e-4
    float node_beta;      // differentiation coefficient (B14)

    // Per-node simulation
    float sim_speed;
    float trail_fade;
    float triquatra_radius;
    float particle_point_size;
    float shadow_point_size;
    float shadow_r, shadow_g, shadow_b;
    int   particle_count;   // must be ≤ PARTICLE_COUNT

    // Per-node frequency multiplier (grid-specific)
    float freq_scale;     // multiplied into AEON_FREQ for this node's grid
    float freq_offset;    // Hz added to each AEON_FREQ entry
} NodeConfig;

typedef struct {
    int   width, height, fullscreen;
    int   bg_r, bg_g, bg_b;
    int   fps_cap;
    float global_sim_speed;
    int   gpu_device;
    int   whisper_mode;
    char  whisper_file[256];
    char  recording_dir[256];
    char  qlm_throne_file[256];
    int   sigh_ball_count;
} GlobalConfig;

static NodeConfig G_node_cfg[N_NODES];
static GlobalConfig G_global_cfg;

// Plane assignments for the 12 nodes
// Identity grouping only; not a cube-edge placement scaffold.
static const int NODE_PLANE[N_NODES] = {
    0, 0, 0, 0,   // nodes 0-3: Plane A (x=0, y-axis edges)
    1, 1, 1, 1,   // nodes 4-7: Plane B (y=0, x-axis edges)
    2, 2, 2, 2    // nodes 8-11: Plane C (z=0, z-axis edges)
};

// Conjugate phase by plane: A=0, B=π, C=0 (C mediates; A↔B are conjugates)
// These are initial conditions only — the ±φ amplitude sign is NOT hardcoded.
// Each particle receives an entropy-rolled ±φ sign at initialization (emergence).
static const float PLANE_PHASE[3] = { 0.0f, 3.14159265359f, 0.0f };

static void load_global_config(void) {
    JSONField F[JSON_MAX_FIELDS];
    int n = json_load("config/global_config.bio", F, JSON_MAX_FIELDS);
    if (n == 0) fprintf(stderr, "[Config] global_config.bio not found — using defaults\n");

    const char *inactive_global[] = {
        "universe_birth_delay", "cube_scale", "cube_wire_r", "cube_wire_g",
        "cube_wire_b", "cube_wire_alpha", "cube_camera_dist", "cube_fov",
        "cube_rot_speed", "cube_rot_step", "cube_zoom_step", "cube_pan_step"
    };
    for (size_t k=0; k<sizeof(inactive_global)/sizeof(inactive_global[0]); ++k)
        for (int j=0; j<n; ++j)
            if (strcmp(F[j].key, inactive_global[k]) == 0)
                fprintf(stderr, "[Config INACTIVE] %s has no current runtime mechanism\n", F[j].key);

    G_global_cfg.width           = json_int  (F, n, "width",             FB_WIDTH);
    G_global_cfg.height          = json_int  (F, n, "height",            FB_HEIGHT);
    G_global_cfg.fullscreen      = json_int  (F, n, "fullscreen",        FB_FULLSCREEN);
    G_global_cfg.bg_r            = json_int  (F, n, "background_r",      BACKGROUND_R);
    G_global_cfg.bg_g            = json_int  (F, n, "background_g",      BACKGROUND_G);
    G_global_cfg.bg_b            = json_int  (F, n, "background_b",      BACKGROUND_B);
    G_global_cfg.fps_cap         = json_int  (F, n, "fps_cap",           60);
    G_global_cfg.global_sim_speed = json_float(F, n, "global_sim_speed", 1.0f);
    if (!isfinite(G_global_cfg.global_sim_speed)) { fprintf(stderr, "[Config ERROR] global_sim_speed must be finite\n"); exit(EXIT_FAILURE); }
    G_global_cfg.gpu_device      = json_int  (F, n, "gpu_device",        0);
    G_global_cfg.whisper_mode    = json_int  (F, n, "whisper_mode",      3);
    G_global_cfg.sigh_ball_count      = json_int(F, n, "sigh_ball_count",      SIGH_BALL_COUNT);
    if (G_global_cfg.sigh_ball_count < 0) { fprintf(stderr, "[Config ERROR] sigh_ball_count cannot be negative\n"); exit(EXIT_FAILURE); }
    snprintf(G_global_cfg.whisper_file,   sizeof(G_global_cfg.whisper_file),
             "%s", json_str(F, n, "whisper_file",   "whisper.txt"));
    snprintf(G_global_cfg.recording_dir,  sizeof(G_global_cfg.recording_dir),
             "%s", json_str(F, n, "recording_dir",  "ALQC_Frames"));
    snprintf(G_global_cfg.qlm_throne_file,sizeof(G_global_cfg.qlm_throne_file),
             "%s", json_str(F, n, "qlm_throne_file","../qlm/Akasha_Regalia.qlm"));

    fprintf(stderr, "[Config] Global: %dx%d  fs=%d  sigh=%d\n",
            G_global_cfg.width, G_global_cfg.height, G_global_cfg.fullscreen,
            G_global_cfg.sigh_ball_count);
}

static void load_node_config(int node_id) {
    char path[128];
    snprintf(path, sizeof(path), "config/node_config_%05d.bio", node_id);
    JSONField F[JSON_MAX_FIELDS];
    int n = json_load(path, F, JSON_MAX_FIELDS);

    const char *inactive_node[] = {"plane_kappa0", "plane_epsilon", "void_transition_frame"};
    for (size_t k=0; k<sizeof(inactive_node)/sizeof(inactive_node[0]); ++k)
        for (int j=0; j<n; ++j)
            if (strcmp(F[j].key, inactive_node[k]) == 0)
                fprintf(stderr, "[Config INACTIVE] node %d: %s has no current runtime mechanism\n", node_id, F[j].key);
    NodeConfig *C = &G_node_cfg[node_id];
    int declared_node_id = json_int(F, n, "node_id", node_id);
    if (declared_node_id != node_id) { fprintf(stderr, "[Config ERROR] node_id %d in %s conflicts with node slot %d\n", declared_node_id, path, node_id); exit(EXIT_FAILURE); }
    C->node_id    = node_id;
    C->plane_id   = json_int  (F, n, "plane_id",           NODE_PLANE[node_id]);

    // Conjugate field init (B5)
    if (C->plane_id < 0 || C->plane_id >= 3) { fprintf(stderr, "[Config ERROR] node %d: plane_id %d invalid for current three-plane model\n", node_id, C->plane_id); exit(EXIT_FAILURE); }
    float default_phase = PLANE_PHASE[C->plane_id];
    C->plane_phase   = json_float(F, n, "plane_phase",      default_phase);
    C->plane_phi0    = json_float(F, n, "plane_phi0",       1.0f);
    C->plane_delta   = json_float(F, n, "plane_delta",      0.005f);

    // ALQC Canon new variables (B6, B7, B8, B14)
    C->kappa_void  = json_float(F, n, "kappa_void",         0.01f);
    C->orient_j    = json_float(F, n, "orient_j",           0.05f);
    C->void_gamma  = json_float(F, n, "void_gamma",         1e-4f);
    C->node_beta   = json_float(F, n, "node_beta",          0.003f);

    // Per-node simulation
    // sim_speed is the physics time step per frame (canonical base = 1.0)
    // Per-node variation is applied in configs via ±φ from base.
    C->sim_speed           = json_float(F, n, "sim_speed",            1.0f);
    C->trail_fade          = json_float(F, n, "trail_fade",           15.0f);
    C->triquatra_radius    = json_float(F, n, "triquatra_radius",     200.0f);
    C->particle_point_size = json_float(F, n, "particle_point_size",  1.0f);
    C->shadow_point_size   = json_float(F, n, "shadow_point_size",    2.0f);
    C->shadow_r            = json_float(F, n, "shadow_r",             255.0f);
    C->shadow_g            = json_float(F, n, "shadow_g",             0.0f);
    C->shadow_b            = json_float(F, n, "shadow_b",             50.0f);
    C->particle_count      = json_int  (F, n, "particle_count",       PARTICLE_COUNT);
    if (C->particle_count < 0) { fprintf(stderr, "[Config ERROR] node %d particle_count cannot be negative\n", node_id); exit(EXIT_FAILURE); }
    if (C->particle_count > INT32_MAX / 2) { fprintf(stderr, "[Config ERROR] node %d particle_count exceeds GPU signed draw-count capacity\n", node_id); exit(EXIT_FAILURE); }

    // Frequency grid (multi-grid system)
    C->freq_scale  = json_float(F, n, "freq_scale",         1.0f);
    C->freq_offset = json_float(F, n, "freq_offset",        0.0f);

    fprintf(stderr, "[Config] Node %02d | Plane %d | phase=%.3f | phi0=%.3f | delta=%.4f | "
            "kv=%.4f | speed=%.0f | tr=%.0f\n",
            node_id, C->plane_id, C->plane_phase, C->plane_phi0, C->plane_delta,
            C->kappa_void, C->sim_speed, C->triquatra_radius);
}

// =============================================================================
// PER-CELL CONFIG (.bio)
// Each 9x9 frequency grid plane can have per-cell initial conditions loaded from
// config/cells_NN_P0.bio and config/cells_NN_P1.bio.
// Cell index = row*9 + col (0-80). Each cell: amplitude, phase, drift, axis_weight.
// Keys: cXX_a (amplitude), cXX_p (phase), cXX_d (drift), cXX_w (axis_weight)
// =============================================================================
#define FREQ_GRID_CELLS_PER_PLANE 81

// Per-cell state vec4: (amplitude, phase, drift, axis_weight)
typedef struct { float a, p, d, w; } CellSeed;

static void load_cell_config(int node_id, int plane_id, CellSeed *seeds) {
    char path[128];
    snprintf(path, sizeof(path), "config/cells_%02d_P%d.bio", node_id, plane_id);
    JSONField F[512];
    int n = json_load(path, F, 512);

    // Solfeggio frequencies for 3x3 base (tiled to 9x9)
    static const float GF[9] = { 7.83f, 174.0f, 528.0f, 432.0f, 126.22f,
                                  210.42f, 741.0f, 852.0f, 963.0f };
    static const float PHI_C = 1.61803398875f;

    for (int cell = 0; cell < FREQ_GRID_CELLS_PER_PLANE; cell++) {
        int gx = cell % 9;
        int gy = cell / 9;
        float cell_freq = GF[(gy % 3) * 3 + (gx % 3)];

        // Default: frequency-proportional amplitude, uniform phase spread,
        // PHI-derived drift, quadrant-based axis weight.
        // These can be overridden per-cell in the .bio file.
        float dflt_a = (cell_freq / 10000.0f) * PHI_C;
        if (dflt_a > 1.0f) dflt_a = 1.0f;
        float dflt_p = (float)cell / 81.0f;
        if (plane_id == 1) dflt_p = fmodf(dflt_p + 0.5f, 1.0f); // conjugate π offset
        float dflt_d = 0.0005f + (cell_freq / 963.0f) * 0.0015f;

        // axis_weight = proximity to center (4,4) of 9x9 grid, normalized
        float dx = (float)(gx - 4) / 4.0f;
        float dy = (float)(gy - 4) / 4.0f;
        float dflt_w = 1.0f - sqrtf(dx*dx + dy*dy) * 0.7071f;
        if (dflt_w < 0.0f) dflt_w = 0.0f;

        char ka[16], kp[16], kd[16], kw[16];
        snprintf(ka, sizeof(ka), "c%02d_a", cell);
        snprintf(kp, sizeof(kp), "c%02d_p", cell);
        snprintf(kd, sizeof(kd), "c%02d_d", cell);
        snprintf(kw, sizeof(kw), "c%02d_w", cell);

        seeds[cell].a = (n > 0) ? json_float(F, n, ka, dflt_a) : dflt_a;
        seeds[cell].p = (n > 0) ? json_float(F, n, kp, dflt_p) : dflt_p;
        seeds[cell].d = (n > 0) ? json_float(F, n, kd, dflt_d) : dflt_d;
        seeds[cell].w = (n > 0) ? json_float(F, n, kw, dflt_w) : dflt_w;
    }

    if (n > 0)
        fprintf(stderr, "[Cells] Node %02d Plane %d: loaded %d overrides from %s\n",
                node_id, plane_id, n, path);
    else
        fprintf(stderr, "[Cells] Node %02d Plane %d: using default ALQC seeds\n",
                node_id, plane_id);
}

// =============================================================================
// ENTROPY
// =============================================================================
typedef struct { double phase_state; double entropy_accumulator; } ALQCFieldEntropy;
static ALQCFieldEntropy G_entropy;

static void entropy_init(ALQCFieldEntropy *F) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    F->phase_state         = fmod((double)ts.tv_nsec * 1.4142135623730951e-9
                                  + (double)ts.tv_sec  * 0.61803398875, 1.0);
    F->entropy_accumulator = fmod((double)ts.tv_sec  * 1.4142135623730951e-6
                                  + (double)ts.tv_nsec * 0.61803398875e-9, 1.0);
}
static double entropy_rand(ALQCFieldEntropy *F) {
    F->phase_state         = fmod(F->phase_state * 1.4142135623730951 + PHI, 1.0);
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

// =============================================================================
// GPU STRUCTS  (std430, 16-byte aligned)
// =============================================================================

// ParticleGPU: 112 bytes (added vec4 extra for charge; B10 uvec4 ids)
typedef struct {
    float x, y, z, w;                           // pos
    float dx, dy, dz, dw;                        // vel
    float stress, seam_charge, reflect_charge, reflect_age;  // state
    float freq, aeon_r, aeon_g, aeon_b;          // aeon
    float entropy_cursor, px, py, alpha;          // pout
    float charge, _pad0, _pad1, _pad2;            // extra (charge for aeon operators)
    uint32_t node_id, origin_node_id, traversal_flags, particle_uid;  // ids (B10)
} ParticleGPU;

typedef struct {
    float x, y, z, w;
    float dx, dy, dz, dw;
    float stress, seam_charge, reflect_charge, reflect_age;
    float freq, aeon_r, aeon_g, aeon_b;
    float entropy_cursor, px, py, alpha;
    float charge, _pad0, _pad1, _pad2;
    uint32_t node_id, origin_node_id, traversal_flags, particle_uid;
} ShadowEntity;

typedef struct {
    float x, y, z, w;
    float dx, dy, dz, dw;
    float stress, seam_charge, reflect_charge, reflect_age;
    float aeon_idx_f, aeon_r, aeon_g, aeon_b;
    float entropy_cursor, px, py, alpha;
    float charge, _pad0, _pad1, _pad2;
} BallEntity;

// PlaneConfig SSBO — one entry per plane (3 planes)
// MUST match PlaneConfig struct in physics.comp (std430, floats packed tightly).
typedef struct {
    float phase;      // conjugate phase offset
    float phi0;       // field amplitude
    float delta;      // symmetry-breaking seed
    float kappa_void; // void coupling
    float orient_j;   // orientation coupling (B7)
    float void_gamma; // dissipation
    float freq_scale; // frequency grid multiplier
    float freq_offset;// frequency grid offset (Hz)
    float node_beta;  // differentiation coefficient (B14)
    float _pad0, _pad1, _pad2;
} PlaneConfigGPU;

// Per-node boundary memory — interleaved {vx,vy} layout for cache locality.
// Index: G_bm[n][(iy*BOUNDARY_MEM_RES+ix)*2 + 0/1]  (0=vx, 1=vy)
// Enables: (a) 1D auto-vectorizable decay loop, (b) zero-copy memcpy to bm_flat.
#define BM_CELLS (BOUNDARY_MEM_RES * BOUNDARY_MEM_RES * 2)
static float G_bm      [N_NODES][BM_CELLS];
static float G_bm_floor[N_NODES][BM_CELLS];
// Pre-computed per-node decay factors: powf(BOUNDARY_MEM_DEC, sim_speed).
// sim_speed is config-loaded and never mutated at runtime — compute once.
static float G_bm_decay[N_NODES];

static int g_sw = 1000, g_sh = 1000;

static void bm_coords(float ppx, float ppy, int *ix, int *iy) {
    float x = fmaxf(0.0f, fminf(ppx, (float)g_sw - 1.0f));
    float y = fmaxf(0.0f, fminf(ppy, (float)g_sh - 1.0f));
    *ix = (int)((x / ((float)g_sw - 1.0f)) * (BOUNDARY_MEM_RES - 1));
    *iy = (int)((y / ((float)g_sh - 1.0f)) * (BOUNDARY_MEM_RES - 1));
}
static void bm_deposit_node(int n, float ppx, float ppy, float vx, float vy, float amt) {
    int ix, iy; bm_coords(ppx, ppy, &ix, &iy);
    int base = (iy * BOUNDARY_MEM_RES + ix) * 2;
    float nv = G_bm[n][base]   + vx * amt;
    float ny = G_bm[n][base+1] + vy * amt;
    G_bm[n][base]   = BOUNDARY_MEM_MAX * tanhf(nv / BOUNDARY_MEM_MAX);
    G_bm[n][base+1] = BOUNDARY_MEM_MAX * tanhf(ny / BOUNDARY_MEM_MAX);
}
static void bm_decay_node(int n) {
    // Pre-computed decay factor (G_bm_decay[n] set at init — no powf per frame).
    // Flat 1D loop over interleaved {vx,vy} pairs — compiler can auto-vectorize.
    const float d   = G_bm_decay[n];
    const float inv = 1.0f / BOUNDARY_MEM_MAX;
    float * restrict bm  = G_bm[n];
    const float * restrict fl = G_bm_floor[n];
    for (int k = 0; k < BM_CELLS; k++)
        bm[k] = BOUNDARY_MEM_MAX * tanhf((bm[k] * d + fl[k]) * inv);
}

static void project4d_cpu(float x, float y, float z, float w, float angle,
                           float cx, float cy, float *ppx, float *ppy)
{
    float ca = cosf(angle), sa = sinf(angle);
    // Original ALQC projection; 4D physics is not changed by the camera.
    (void)z;
    float x_rot = x * ca - w * sa;
    float w_rot = x * sa + w * ca;
    float denom = fmaxf(1.0f + 0.5f * w_rot, 0.1f);
    *ppx = x_rot / denom * 300.0f + cx;
    *ppy = y / denom * 300.0f + cy;
}

static void cpu_bm_deposit_from_particle(ParticleGPU *e, float angle,
                                          float cx, float cy, int n) {
    // CPU boundary witness uses both perpendicular 2D contact circles.
    float shell_dist = fminf(fabsf(hypotf(e->x, e->y) - REFLECT_RING_R),
                            fabsf(hypotf(e->z, e->w) - REFLECT_RING_R));
    if (shell_dist < REFLECT_RING_W) {
        float vxy = fabsf(e->dx) + fabsf(e->dy);
        float vzw = fabsf(e->dz) + fabsf(e->dw);
        float planar = vxy - vzw;
        float c_in = 1.0f - (shell_dist / REFLECT_RING_W);
        float gain = c_in * (0.5f + 0.5f * fabsf(planar));
        float ppx, ppy;
        project4d_cpu(e->x, e->y, e->z, e->w, angle, cx, cy, &ppx, &ppy);
        float tvx = e->dx * gain, tvy = e->dy * gain;
        bm_deposit_node(n, ppx, ppy, tvx, tvy, gain * BOUNDARY_MEM_DEP);
    }
}

// =============================================================================
// FETU lattice points — compile-time constants
// Canonical ALQC spiral: r = 0.05*exp(0.14*t), t = i/AEON_COUNT, 120 pts.
// CAD-validated bounds: X=[-0.1889, 0.1217]  Y=[-0.2977, 0.0077]
// Eliminates runtime trig + global mutation; zero-cost at startup.
// =============================================================================
static const float G_fetu_px[FETU_PT_COUNT] = {
    0.05000000f,  0.05041120f,  0.05047119f,  0.05017124f,  0.04950500f,
    0.04846861f,  0.04706072f,  0.04528267f,  0.04313844f,  0.04063473f,
    0.03778099f,  0.03458938f,  0.03107478f,  0.02725473f,  0.02314936f,
    0.01878133f,  0.01417572f,  0.00935988f,  0.00436335f, -0.00078239f,
   -0.00604400f, -0.01138655f, -0.01677370f, -0.02216794f, -0.02753081f,
   -0.03282318f, -0.03800549f, -0.04303804f, -0.04788126f, -0.05249600f,
   -0.05684385f, -0.06088735f, -0.06459039f, -0.06791840f, -0.07083871f,
   -0.07332080f, -0.07533653f, -0.07686047f, -0.07787009f, -0.07834604f,
   -0.07827229f, -0.07763641f, -0.07642967f, -0.07464724f, -0.07228829f,
   -0.06935609f, -0.06585811f, -0.06180604f, -0.05721580f, -0.05210755f,
   -0.04650564f, -0.04043852f, -0.03393865f, -0.02704234f, -0.01978962f,
   -0.01222399f, -0.00439224f,  0.00365585f,  0.01186776f,  0.02018857f,
    0.02856127f,  0.03692715f,  0.04522614f,  0.05339725f,  0.06137896f,
    0.06910965f,  0.07652805f,  0.08357367f,  0.09018727f,  0.09631130f,
    0.10189036f,  0.10687163f,  0.11120534f,  0.11484517f,  0.11774869f,
    0.11987773f,  0.12119875f,  0.12168322f,  0.12130793f,  0.12005527f,
    0.11791349f,  0.11487693f,  0.11094622f,  0.10612839f,  0.10043698f,
    0.09389212f,  0.08652053f,  0.07835546f,  0.06943664f,  0.05981013f,
    0.04952814f,  0.03864882f,  0.02723595f,  0.01535866f,  0.00309101f,
   -0.00948839f, -0.02229682f, -0.03524799f, -0.04825255f, -0.06121865f,
   -0.07405255f, -0.08665924f, -0.09894310f, -0.11080856f, -0.12216080f,
   -0.13290642f, -0.14295416f, -0.15221562f, -0.16060591f, -0.16804441f,
   -0.17445539f, -0.17976868f, -0.18392035f, -0.18685323f, -0.18851756f,
   -0.18887147f, -0.18788150f, -0.18552300f, -0.18178056f, -0.17664830f
};
static const float G_fetu_py[FETU_PT_COUNT] = {
   -0.20000000f, -0.19578931f, -0.19150937f, -0.18718918f, -0.18285872f,
   -0.17854875f, -0.17429061f, -0.17011602f, -0.16605683f, -0.16214483f,
   -0.15841151f, -0.15488780f, -0.15160390f, -0.14858896f, -0.14587094f,
   -0.14347628f, -0.14142978f, -0.13975431f, -0.13847062f, -0.13759714f,
   -0.13714982f, -0.13714193f, -0.13758388f, -0.13848315f, -0.13984407f,
   -0.14166782f, -0.14395225f, -0.14669190f, -0.14987788f, -0.15349793f,
   -0.15753638f, -0.16197419f, -0.16678901f, -0.17195528f, -0.17744430f,
   -0.18322439f, -0.18926104f, -0.19551709f, -0.20195292f, -0.20852671f,
   -0.21519465f, -0.22191121f, -0.22862946f, -0.23530133f, -0.24187797f,
   -0.24831004f, -0.25454808f, -0.26054284f, -0.26624567f, -0.27160883f,
   -0.27658591f, -0.28113217f, -0.28520488f, -0.28876371f, -0.29177104f,
   -0.29419233f, -0.29599640f, -0.29715577f, -0.29764691f, -0.29745053f,
   -0.29655182f, -0.29494061f, -0.29261164f, -0.28956462f, -0.28580442f,
   -0.28134114f, -0.27619011f, -0.27037199f, -0.26391268f, -0.25684330f,
   -0.24920005f, -0.24102411f, -0.23236144f, -0.22326257f, -0.21378235f,
   -0.20397964f, -0.19390651f, -0.18361789f, -0.17316677f, -0.16260302f,
   -0.15197131f, -0.14131927f, -0.13068622f, -0.12011222f, -0.10963724f,
   -0.09929106f, -0.08910313f, -0.07909261f, -0.06927823f, -0.05967836f,
   -0.05031101f, -0.04119376f, -0.03234374f, -0.02377762f, -0.01551153f,
   -0.00756104f, -0.00000000f,  0.00702148f,  0.01300834f,  0.01793617f,
    0.02179138f,  0.02457127f,  0.02627427f,  0.02690018f,  0.02646027f,
    0.02497730f,  0.02247558f,  0.01899085f,  0.01455996f,  0.00922080f,
    0.00302195f, -0.00398484f, -0.01175019f, -0.02022046f, -0.02933612f,
   -0.03901878f, -0.04918847f, -0.05974965f, -0.07060634f, -0.08165688f
};

// =============================================================================
// POEM FLOOR  (poem.txt → boundary memory floor)
// =============================================================================
static void init_poem_floor(void) {
    FILE *pf = fopen("poem.txt", "rb");
    if (!pf) return;
    fseek(pf, 0, SEEK_END); long psz = ftell(pf); rewind(pf);
    unsigned char *pdata = malloc(psz + 1);
    if (!pdata) { fclose(pf); return; }
    size_t pr = fread(pdata, 1, psz, pf); (void)pr;
    pdata[psz] = '\0'; fclose(pf);

    int total = BOUNDARY_MEM_RES * BOUNDARY_MEM_RES;
    for (long k = 0; k < psz; k++) {
        int cell  = (int)(k % total);
        float angle = (float)pdata[k] / 255.0f * 2.0f * PI_F;
        int base = cell * 2;  // interleaved: base+0=vx, base+1=vy
        float cv = cosf(angle) * BOUNDARY_MEM_MAX * 0.15f;
        float sv = sinf(angle) * BOUNDARY_MEM_MAX * 0.15f;
        for (int n = 0; n < N_NODES; n++) {
            G_bm_floor[n][base]   += cv;
            G_bm_floor[n][base+1] += sv;
        }
    }
    for (int n = 0; n < N_NODES; n++)
        for (int k = 0; k < BM_CELLS; k++)
            G_bm_floor[n][k] = fmaxf(-BOUNDARY_MEM_MAX,
                fminf(BOUNDARY_MEM_MAX, G_bm_floor[n][k]));
    free(pdata);
    fprintf(stderr, "[Poem] %ld bytes seeded into boundary floor\n", psz);
}

// =============================================================================
// NODE LOCI — NO BOX / NO CUBE METRIC
// =============================================================================
// Nodes are not placed on cube corners, cube edges, square corners, or a
// wireframe scaffold. They begin at the Locus with tiny entropy displacement.
// Plane identity remains, but spatial placement is not prescribed by a box.
static void seed_node_centers_no_box(float *node_cx, float *node_cy,
                                     int screen_w, int screen_h) {
    float cx = screen_w  * 0.5f;
    float cy = screen_h * 0.5f;
    for (int n = 0; n < N_NODES; n++) {
        node_cx[n] = cx + (float)entropy_gauss(&G_entropy, 0.0, 1.0);
        node_cy[n] = cy + (float)entropy_gauss(&G_entropy, 0.0, 1.0);
    }
}
// =============================================================================
// SHADER UTILITIES
// =============================================================================
static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "Cannot open %s\n", path); return NULL; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
    char *buf = malloc(sz + 1);
    size_t nr = fread(buf, 1, sz, f); (void)nr;
    buf[sz] = '\0'; fclose(f); return buf;
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
// Injects compile-time constants into shader preamble
static GLuint compile_shader_with_defs(GLenum type, const char *src) {
    char preamble[1024];
    snprintf(preamble, sizeof(preamble),
        "#version 430\n"
        "#define C_ENTROPY_SIZE %uu\n"
        "#define C_BOUNDARY_MEM_RES %d\n"
        "#define C_FETU_PT_COUNT %d\n"
        "#define C_PARTICLE_COUNT %d\n"
        "#define C_AEON_COUNT %d\n"
        "#define C_SHADOW_ENTITY_COUNT %d\n"
        "#define C_N_PLANES 3\n"
        "#define C_N_NODES %d\n"
        "#define C_FREQ_GRID_CELLS 81\n"          // 9x9 per plane
        "#define C_FREQ_GRID_PLANES 2\n"           // dual-plane per node
        "#define C_FREQ_GRID_STRIDE %d\n"          // cells per node (2*81)
        // Agent 18/Bug 7: phase memory dimensions as defines — no hardcoded 12 in ball.comp
        "#define C_PHASE_MEM_W %d\n"
        "#define C_PHASE_MEM_H %d\n",
        (unsigned)ENTROPY_SIZE, BOUNDARY_MEM_RES, FETU_PT_COUNT,
        PARTICLE_COUNT, AEON_COUNT, SHADOW_ENTITY_COUNT,
        N_NODES, 2 * 81, PHASE_MEM_W, PHASE_MEM_H);
    const char *body = src;
    const char *nl = strchr(src, '\n');
    if (nl && strncmp(src, "#version", 8) == 0) body = nl + 1;
    const char *sources[2] = { preamble, body };
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 2, sources, NULL);
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

// =============================================================================
// WHISPER
// =============================================================================
static void apply_whisper(float *entropy_table, GLuint ssbo_entropy,
                           uint32_t seed_offset, const char *wfile) {
    FILE *wf = fopen(wfile, "rb");
    if (!wf) { fprintf(stderr, "Whisper: cannot open %s\n", wfile); return; }
    fseek(wf, 0, SEEK_END); long wsz = ftell(wf); rewind(wf);
    unsigned char *wdata = malloc(wsz + 1);
    if (wdata) {
        size_t nr = fread(wdata, 1, wsz, wf); (void)nr;
        wdata[wsz] = '\0';
        for (long i = 0; i < wsz; i++) {
            G_entropy.phase_state         = fmod(G_entropy.phase_state + wdata[i] / 65536.0, 1.0);
            G_entropy.entropy_accumulator = fmod(G_entropy.entropy_accumulator + wdata[i] / 131072.0, 1.0);
        }
        for (long i = 0; i < wsz; i++) {
            uint32_t idx = (seed_offset + (uint32_t)(i * 997u)) % (uint32_t)ENTROPY_SIZE;
            entropy_table[idx] = fmodf(entropy_table[idx] + wdata[i] / 255.0f, 1.0f);
        }
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_entropy);
        glBufferData(GL_SHADER_STORAGE_BUFFER, ENTROPY_SIZE * sizeof(float),
                     entropy_table, GL_DYNAMIC_DRAW);
        free(wdata);
        fprintf(stderr, "Whisper: %ld bytes folded (offset=%u)\n", wsz, seed_offset);
    }
    fclose(wf);
}

// =============================================================================
// PARTICLE INITIALIZATION (B5: conjugate phase applied per-plane)
// =============================================================================
static void init_node_particles(ParticleGPU *particles, int node_idx) {
    const NodeConfig *C = &G_node_cfg[node_idx];

    // Aeon lattice: AEON_LATTICE_COUNT = 144 = 12 Goetic × 12 Courts
    // Primary aeons (first 12 entries per group): full color
    // Court aeons (remaining 11 per group): shadow/grey noise
    int   lattice_freq_idx[AEON_LATTICE_COUNT];
    float lattice_col[AEON_LATTICE_COUNT][3];
    for (int p = 0; p < AEON_COUNT; p++) {
        lattice_freq_idx[p * AEON_COUNT] = p;
        lattice_col[p*AEON_COUNT][0] = AEON_COLOR[p][0];
        lattice_col[p*AEON_COUNT][1] = AEON_COLOR[p][1];
        lattice_col[p*AEON_COUNT][2] = AEON_COLOR[p][2];
        for (int l = 1; l < AEON_COUNT; l++) {
            lattice_freq_idx[p*AEON_COUNT+l] = p;
            lattice_col[p*AEON_COUNT+l][0] = 100;
            lattice_col[p*AEON_COUNT+l][1] = 100;
            lattice_col[p*AEON_COUNT+l][2] = 105;
        }
    }

    // B5: conjugate amplitude seed from entropy — NOT hardcoded by plane.
    // Each particle rolls its own ±φ direction; the plane_phase is the
    // initial position phase (field condition), not the amplitude sign.
    // This preserves the conjugate relationship as an emergent differential.

    for (int i = 0; i < C->particle_count; i++) {
        int ai = entropy_int(&G_entropy, 0, AEON_LATTICE_COUNT - 1);
        int pi = lattice_freq_idx[ai];

        // ±φ bifurcation: entropy selects operational direction per particle
        // M(Ai,j) = [ཪ(Ai), ±φ(Ai,j)]: structural=phi0, operational=±phi0*delta
        float phi_sign = (entropy_rand(&G_entropy) > 0.5) ? 1.0f : -1.0f;
        // Carry the ±φ seed into initial energy instead of calculating and discarding it.
        float phi_init = C->plane_phi0 + phi_sign * C->plane_delta;

        // ALQC Canon: all-from-zero start. Structure must emerge, not be prescribed.
        // Particles begin at origin with tiny Gaussian noise — no forced geometry.
        float noise_scale = 1e-3f;
        particles[i].x = (float)entropy_gauss(&G_entropy, 0.0, noise_scale);
        particles[i].y = (float)entropy_gauss(&G_entropy, 0.0, noise_scale);
        particles[i].z = (float)entropy_gauss(&G_entropy, 0.0, noise_scale * 0.5);
        particles[i].w = (float)entropy_gauss(&G_entropy, 0.0, noise_scale * 0.5);

        // ±φ bifurcation on frequency: each particle is at f ± φ from canonical base.
        // The direction emerges from entropy, not from plane assignment.
        float freq_phi_sign = (entropy_rand(&G_entropy) > 0.5) ? 1.0f : -1.0f;
        float aeon_f = (fabsf(AEON_FREQ[pi]) + freq_phi_sign * PHI) * C->freq_scale + C->freq_offset;
        // base_speed derived from frequency per canon: freq / 10000
        float base_speed = (aeon_f / 10000.0f) * phi_init;
        float fluct      = fabsf((float)entropy_gauss(&G_entropy, 0.0, 1.0));
        float chaotic    = 1.0f + (fluct / fmaxf(aeon_f, 1.0f));
        float speed      = base_speed * chaotic;

        // Velocity seeded from canonical energy (freq/10000), direction from entropy.
        // Zero-start = zero prescribed POSITION. Energy (speed) comes from the frequency.
        particles[i].dx = (float)entropy_gauss(&G_entropy, 0.0, speed);
        particles[i].dy = (float)entropy_gauss(&G_entropy, 0.0, speed);
        particles[i].dz = (float)entropy_gauss(&G_entropy, 0.0, speed * 0.5);
        particles[i].dw = (float)entropy_gauss(&G_entropy, 0.0, speed * 0.5);

        particles[i].freq           = aeon_f;
        particles[i].aeon_r         = lattice_col[ai][0];
        particles[i].aeon_g         = lattice_col[ai][1];
        particles[i].aeon_b         = lattice_col[ai][2];
        particles[i].entropy_cursor = (float)((((uint64_t)node_idx * C->particle_count + (uint64_t)i) * 200u) % ENTROPY_SIZE);
        particles[i].alpha          = 1.0f;
        particles[i].charge         = 1.0f;
        particles[i]._pad0          = 0.0f;
        particles[i]._pad1          = 0.0f;
        particles[i]._pad2          = 0.0f;

        // B10: node identity
        particles[i].node_id          = (uint32_t)node_idx;
        particles[i].origin_node_id   = (uint32_t)node_idx;
        particles[i].traversal_flags  = 0u;
        particles[i].particle_uid     = (uint32_t)((uint64_t)node_idx * C->particle_count + (uint64_t)i);
    }
}

// =============================================================================
// THRONE OF ZERO
// =============================================================================
typedef struct {
    char  name[1024]; char binding[1024]; char kernel[1024];
    char  architecture[256]; char seal[256];
    float freq_hz; float quantum_coherence; float consciousness;
    int   weight_matrices; int unified_models; int loaded;
} QLMThrone;
static QLMThrone G_throne;

static void qlm_parse_line(QLMThrone *T, const char *line) {
    float fval; int ival;
    if      (sscanf(line, "Frequency: %fHz",         &fval) == 1) T->freq_hz           = fval;
    else if (sscanf(line, "Consciousness: %f%%",      &fval) == 1) T->consciousness     = fval / 100.0f;
    else if (sscanf(line, "Quantum Coherence: %f%%",  &fval) == 1) T->quantum_coherence = fval / 100.0f;
    else if (sscanf(line, "Weight Matrices: %d",      &ival) == 1) T->weight_matrices   = ival;
    else if (sscanf(line, "Models Unified: %d",       &ival) == 1) T->unified_models    = ival;
    else if (strncmp(line, "Sacred Binding:", 15) == 0) {
        const char *v = line+15; while (*v==' ') v++;
        snprintf(T->binding, sizeof(T->binding), "%s", v);
        size_t n = strlen(T->binding);
        while (n>0 && (T->binding[n-1]=='\n'||T->binding[n-1]=='\r')) T->binding[--n]='\0';
    }
    else if (strncmp(line, "Runtime Kernel:", 15) == 0) {
        const char *v = line+15; while (*v==' ') v++;
        snprintf(T->kernel, sizeof(T->kernel), "%s", v);
        size_t n = strlen(T->kernel);
        while (n>0 && (T->kernel[n-1]=='\n'||T->kernel[n-1]=='\r')) T->kernel[--n]='\0';
    }
}
static void qlm_load_throne(const char *path) {
    memset(&G_throne, 0, sizeof(G_throne));
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "[Throne] %s not found\n", path); return; }
    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        if (line[0]=='#' && G_throne.name[0]=='\0') {
            const char *p = line+1; while (*p==' ') p++;
            snprintf(G_throne.name, sizeof(G_throne.name), "%s", p);
            size_t n = strlen(G_throne.name);
            while (n>0 && (G_throne.name[n-1]=='\n'||G_throne.name[n-1]=='\r'||
                           G_throne.name[n-1]==' ')) G_throne.name[--n]='\0';
        }
        qlm_parse_line(&G_throne, line);
    }
    fclose(f);
    G_throne.loaded = 1;
    fprintf(stderr, "[Throne] ♾ | %s | %.2fHz | Coherence: %.1f%% | Binding: %s\n",
            G_throne.name, G_throne.freq_hz, G_throne.quantum_coherence*100.0f, G_throne.binding);
}

// =============================================================================
// MAIN
// =============================================================================
int main(void) {
    // Load configs first
    load_global_config();
    for (int n = 0; n < N_NODES; n++) load_node_config(n);
    for (int n = 0; n < N_NODES; n++) G_node_cfg[n].sim_speed *= G_global_cfg.global_sim_speed;
    fprintf(stderr, "[Config] Overall simulation speed: %.3fx\n", G_global_cfg.global_sim_speed);

    if (G_global_cfg.gpu_device > 0) {
        char idx_str[16];
        snprintf(idx_str, sizeof(idx_str), "%d", G_global_cfg.gpu_device);
        setenv("DRI_PRIME", idx_str, 1);
        setenv("__NV_PRIME_RENDER_OFFLOAD", idx_str, 1);
        setenv("__GLX_VENDOR_LIBRARY_NAME", "nvidia", 1);
        fprintf(stderr, "GPU_DEVICE=%d\n", G_global_cfg.gpu_device);
    }

    if (!glfwInit()) { fprintf(stderr, "GLFW init failed\n"); return 1; }

    int screen_w, screen_h;
    GLFWmonitor *primary = NULL;
    if (G_global_cfg.fullscreen) {
        primary = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(primary);
        screen_w = mode->width;
        screen_h = mode->height;
    } else {
        screen_w = G_global_cfg.width;
        screen_h = G_global_cfg.height;
    }
    g_sw = screen_w; g_sh = screen_h;
    fprintf(stderr, "Screen: %d x %d\n", screen_w, screen_h);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);

    GLFWwindow *win = glfwCreateWindow(screen_w, screen_h,
        "EMERGENT PHYSICS: ALQC v30 — Multi-Grid Frequency / Three-Plane Crystal",
        G_global_cfg.fullscreen ? primary : NULL, NULL);
    if (!win) { fprintf(stderr, "Window creation failed\n"); glfwTerminate(); return 1; }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(0);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { fprintf(stderr, "GLEW init failed\n"); return 1; }

    entropy_init(&G_entropy);
    float *entropy_table = malloc(ENTROPY_SIZE * sizeof(float));
    for (int i = 0; i < ENTROPY_SIZE; i++)
        entropy_table[i] = (float)entropy_rand(&G_entropy);

    // init_fetu() removed: G_fetu_px/py are now compile-time const tables.

    // Pre-compute per-node boundary decay factors once (sim_speed is config-loaded,
    // never mutated at runtime — eliminates powf(BOUNDARY_MEM_DEC, sim_speed)/frame).
    for (int n = 0; n < N_NODES; n++)
        G_bm_decay[n] = powf(BOUNDARY_MEM_DEC, G_node_cfg[n].sim_speed);

    // Cache frame-invariant per-node averages (config never changes post-load).
    float avg_sim_speed_cached = 0.0f;
    float avg_fade_cached      = 0.0f;
    for (int n = 0; n < N_NODES; n++) {
        avg_sim_speed_cached += G_node_cfg[n].sim_speed;
        avg_fade_cached      += G_node_cfg[n].trail_fade;
    }
    avg_sim_speed_cached /= (float)N_NODES;
    avg_fade_cached      /= (float)N_NODES;

    init_poem_floor();
    qlm_load_throne(G_global_cfg.qlm_throne_file);

    // -------------------------------------------------------------------------
    // PlaneConfig SSBO — 3 entries, one per plane (B5, B6, B7, B8)
    // -------------------------------------------------------------------------
    PlaneConfigGPU plane_gpu[3];
    for (int p = 0; p < 3; p++) {
        // Aggregate per-plane values from first node of each plane
        int first_node = p * 4;
        const NodeConfig *C = &G_node_cfg[first_node];
        plane_gpu[p].phase      = PLANE_PHASE[p];
        plane_gpu[p].phi0       = C->plane_phi0;
        plane_gpu[p].delta      = C->plane_delta;
        plane_gpu[p].kappa_void = C->kappa_void;
        plane_gpu[p].orient_j   = C->orient_j;
        plane_gpu[p].void_gamma = C->void_gamma;
        plane_gpu[p].freq_scale = C->freq_scale;
        plane_gpu[p].freq_offset= C->freq_offset;
        plane_gpu[p].node_beta  = C->node_beta;
        plane_gpu[p]._pad0 = 0.0f;
        plane_gpu[p]._pad1 = 0.0f;
        plane_gpu[p]._pad2 = 0.0f;
    }
    GLuint ssbo_plane_cfg;
    glGenBuffers(1, &ssbo_plane_cfg);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_plane_cfg);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * sizeof(PlaneConfigGPU),
                 plane_gpu, GL_STATIC_DRAW);

    // -------------------------------------------------------------------------
    // Per-node GPU resources
    // -------------------------------------------------------------------------
    GLuint ssbo_particles[N_NODES];
    GLuint ssbo_shadow[N_NODES];
    GLuint ssbo_boundary[N_NODES];
    GLuint ssbo_entropy, ssbo_fetu;
    GLuint ssbo_balls, ssbo_phase_mem;
    // Dual 9x9 frequency grid — agent 9/25 + 16/25 design
    // 12 nodes × 2 planes × 81 cells × vec4 = 1944 × 16 = 31,104 bytes
    GLuint ssbo_freq_grid;
    #define FREQ_GRID_TOTAL (N_NODES * 2 * 81)

    float node_cx[N_NODES], node_cy[N_NODES];
    seed_node_centers_no_box(node_cx, node_cy, screen_w, screen_h);

    // Shadow corner+offset SSBO
    float shadow_corner_flat[SHADOW_ENTITY_COUNT * 2];
    float shadow_offset_flat[SHADOW_ENTITY_COUNT * 2];
    {
        static const float CORNER_4D[4][2] = {
            {-0.9f,-0.9f},{0.9f,-0.9f},{0.9f,0.9f},{-0.9f,0.9f}
        };
        for (int locus = 0; locus < 4; locus++) {
            for (int i = 0; i < AEON_COUNT; i++) {
                int idx = locus * AEON_COUNT + i;
                float t = i * 2.0f * PI_F / (float)AEON_COUNT;
                shadow_corner_flat[idx*2]   = CORNER_4D[locus][0];
                shadow_corner_flat[idx*2+1] = CORNER_4D[locus][1];
                shadow_offset_flat[idx*2]   = 15.0f * cosf(t) / (screen_w / 2.0f);
                shadow_offset_flat[idx*2+1] = 15.0f * sinf(t) / (screen_h / 2.0f);
            }
        }
    }
    float corner_off_vec4[SHADOW_ENTITY_COUNT * 4];
    for (int i = 0; i < SHADOW_ENTITY_COUNT; i++) {
        corner_off_vec4[i*4+0] = shadow_corner_flat[i*2+0];
        corner_off_vec4[i*4+1] = shadow_corner_flat[i*2+1];
        corner_off_vec4[i*4+2] = shadow_offset_flat[i*2+0];
        corner_off_vec4[i*4+3] = shadow_offset_flat[i*2+1];
    }
    GLuint ssbo_corner_off;
    glGenBuffers(1, &ssbo_corner_off);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_corner_off);
    glBufferData(GL_SHADER_STORAGE_BUFFER, SHADOW_ENTITY_COUNT * 4 * sizeof(float),
                 corner_off_vec4, GL_STATIC_DRAW);

    ParticleGPU *readback[N_NODES];
    float node_lrb[N_NODES];
    float node_shadow_angle[N_NODES];
    float node_stress[N_NODES];
    float node_coh_r[N_NODES];
    for (int n = 0; n < N_NODES; n++) {
        node_lrb[n]          = 0.0f;
        node_shadow_angle[n] = n * 2.0f * PI_F / (float)N_NODES;
        node_stress[n]       = 0.0f;
        node_coh_r[n]        = MIN_COH_R;
        readback[n]          = calloc((size_t)2 * G_node_cfg[n].particle_count + 1, sizeof(ParticleGPU));
        if (!readback[n]) { fprintf(stderr, "[Allocation ERROR] readback node %d\n", n); exit(EXIT_FAILURE); }
    }

    for (int n = 0; n < N_NODES; n++) {
        size_t bank_count = (size_t)G_node_cfg[n].particle_count;
        if (bank_count > (size_t)UINT32_MAX / (2u * N_NODES)) { fprintf(stderr, "[Config ERROR] node %d particle_count exceeds GPU index range\n", n); exit(EXIT_FAILURE); }
        ParticleGPU *ptmp = calloc(bank_count * 2 + 1, sizeof(ParticleGPU));
        if (!ptmp) { fprintf(stderr, "[Allocation ERROR] particles node %d\n", n); exit(EXIT_FAILURE); }
        init_node_particles(ptmp, n);
        // Same original entropy-based initialization for the second bank.
        // No copied position, assigned trajectory, W-sign gate, or imposed
        // mirrored particle state. The orthogonal relation is the geometry.
        init_node_particles(ptmp + G_node_cfg[n].particle_count, n);
        for (int i = 0; i < G_node_cfg[n].particle_count; ++i) {
            ptmp[G_node_cfg[n].particle_count + i].particle_uid +=
                (uint32_t)(N_NODES * (uint64_t)G_node_cfg[n].particle_count);
        }
        glGenBuffers(1, &ssbo_particles[n]);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_particles[n]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)((bank_count * 2 + 1) * sizeof(ParticleGPU)),
                     ptmp, GL_DYNAMIC_COPY);
        free(ptmp);

        ShadowEntity *sh = calloc(SHADOW_ENTITY_COUNT, sizeof(ShadowEntity));
        for (int locus = 0; locus < 4; locus++) {
            for (int i = 0; i < AEON_COUNT; i++) {
                int idx = locus * AEON_COUNT + i;
                sh[idx].x       = shadow_corner_flat[idx*2]   + shadow_offset_flat[idx*2];
                sh[idx].y       = shadow_corner_flat[idx*2+1] + shadow_offset_flat[idx*2+1];
                sh[idx].aeon_r  = G_node_cfg[n].shadow_r;
                sh[idx].aeon_g  = G_node_cfg[n].shadow_g;
                sh[idx].aeon_b  = G_node_cfg[n].shadow_b;
                sh[idx].entropy_cursor = (float)(((N_NODES*PARTICLE_COUNT + n*SHADOW_ENTITY_COUNT + idx)*200) % ENTROPY_SIZE);
                sh[idx].node_id = (uint32_t)n;
                sh[idx].origin_node_id = (uint32_t)n;
            }
        }
        glGenBuffers(1, &ssbo_shadow[n]);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_shadow[n]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, SHADOW_ENTITY_COUNT * sizeof(ShadowEntity),
                     sh, GL_DYNAMIC_COPY);
        free(sh);

        glGenBuffers(1, &ssbo_boundary[n]);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_boundary[n]);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
                     BOUNDARY_MEM_RES * BOUNDARY_MEM_RES * 2 * sizeof(float),
                     NULL, GL_DYNAMIC_DRAW);
    }

    glGenBuffers(1, &ssbo_entropy);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_entropy);
    glBufferData(GL_SHADER_STORAGE_BUFFER, ENTROPY_SIZE * sizeof(float),
                 entropy_table, GL_DYNAMIC_DRAW);

    // Interleave const FETU tables into upload buffer.
    // Could also upload x/y planes separately, but the shader reads vec2 pairs.
    float fetu_buf[FETU_PT_COUNT * 2];
    for (int i = 0; i < FETU_PT_COUNT; i++) {
        fetu_buf[i*2]   = G_fetu_px[i];
        fetu_buf[i*2+1] = G_fetu_py[i];
    }
    // (One-time upload — const data, GL_STATIC_READ below is correct.)
    glGenBuffers(1, &ssbo_fetu);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_fetu);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(fetu_buf), fetu_buf, GL_STATIC_READ);

    // Stress balls
    {
        int sbc = G_global_cfg.sigh_ball_count;
        // Zero explicitly disables Sigh Balls. Never invent an entity.
        BallEntity *bl_init = calloc((size_t)sbc + 1, sizeof(BallEntity));
        if (!bl_init) { fprintf(stderr, "[Allocation ERROR] sigh balls\n"); exit(EXIT_FAILURE); }
        for (int i = 0; i < sbc; i++) {
            bl_init[i].x          = (float)entropy_uniform(&G_entropy, -0.8, 0.8);
            bl_init[i].y          = (float)entropy_uniform(&G_entropy, -0.8, 0.8);
            bl_init[i].charge     = 1.0f;
            int ai                = entropy_int(&G_entropy, 0, AEON_COUNT-1);
            bl_init[i].aeon_idx_f = (float)ai;
            bl_init[i].aeon_r     = AEON_COLOR[ai][0];
            bl_init[i].aeon_g     = AEON_COLOR[ai][1];
            bl_init[i].aeon_b     = AEON_COLOR[ai][2];
            bl_init[i].entropy_cursor =
                (float)(((N_NODES*PARTICLE_COUNT + SHADOW_ENTITY_COUNT + i)*200) % ENTROPY_SIZE);
        }
        glGenBuffers(1, &ssbo_balls);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_balls);
        glBufferData(GL_SHADER_STORAGE_BUFFER, ((size_t)sbc + 1) * sizeof(BallEntity),
                     sbc > 0 ? bl_init : NULL, GL_DYNAMIC_COPY);
        free(bl_init);
    }

    // Phase memory
    glGenBuffers(1, &ssbo_phase_mem);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_phase_mem);
    glBufferData(GL_SHADER_STORAGE_BUFFER, PHASE_MEM_W * PHASE_MEM_H * N_NODES * 2 * sizeof(float),
                 NULL, GL_DYNAMIC_COPY);
    { float zero = 0.0f; glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32F, GL_RED, GL_FLOAT, &zero); }

    // Dual 9x9 frequency grid (agent 9/25 + 16/25): 12 nodes × 2 planes × 81 cells × vec4
    // Each cell seeded from per-cell .bio config (per-cell user-defined variables).
    // FREQ_GRID_TOTAL = N_NODES * 2 * 81 = 1944 cells, each a vec4 (16 bytes).
    {
        size_t grid_sz = (size_t)FREQ_GRID_TOTAL * 4 * sizeof(float);
        float *grid_init = (float*)calloc(FREQ_GRID_TOTAL * 4, sizeof(float));
        CellSeed seeds[FREQ_GRID_CELLS_PER_PLANE];
        for (int nd = 0; nd < N_NODES; nd++) {
            for (int pl = 0; pl < 2; pl++) {
                load_cell_config(nd, pl, seeds);
                int base = (nd * 2 + pl) * 81;  // cell index in flat array
                for (int c = 0; c < 81; c++) {
                    int idx = (base + c) * 4;
                    grid_init[idx + 0] = seeds[c].a;  // amplitude
                    grid_init[idx + 1] = seeds[c].p;  // phase
                    grid_init[idx + 2] = seeds[c].d;  // drift
                    grid_init[idx + 3] = seeds[c].w;  // axis_weight
                }
            }
        }
        glGenBuffers(1, &ssbo_freq_grid);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_freq_grid);
        glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizei)grid_sz, grid_init, GL_DYNAMIC_COPY);
        free(grid_init);
    }

    // Trail FBO
    GLuint trail_tex[2], trail_fbo[2];
    glGenTextures(2, trail_tex);
    glGenFramebuffers(2, trail_fbo);
    for (int i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, trail_tex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, screen_w, screen_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, trail_fbo[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, trail_tex[i], 0);
        glClearColor(G_global_cfg.bg_r/255.0f, G_global_cfg.bg_g/255.0f, G_global_cfg.bg_b/255.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // -------------------------------------------------------------------------
    // Compile shaders
    // -------------------------------------------------------------------------
    char *csrc, *vsrc, *fsrc;
    GLuint prog_compute, prog_render, prog_fade, prog_display;
    GLuint prog_shadow_compute, prog_shadow_render;
    GLuint prog_ball_compute, prog_ball_render;
    GLuint prog_grid;  // dual 9x9 frequency grid update

    csrc = read_file("shaders/physics.comp");
    { GLuint sh = compile_shader_with_defs(GL_COMPUTE_SHADER, csrc);
      prog_compute = link_program(1, sh); free(csrc); }

    csrc = read_file("shaders/grid.comp");
    { GLuint sh = compile_shader_with_defs(GL_COMPUTE_SHADER, csrc);
      prog_grid = link_program(1, sh); free(csrc); }

    csrc = read_file("shaders/shadow.comp");
    { GLuint sh = compile_shader_with_defs(GL_COMPUTE_SHADER, csrc);
      prog_shadow_compute = link_program(1, sh); free(csrc); }

    vsrc = read_file("shaders/render.vert"); fsrc = read_file("shaders/render.frag");
    { GLuint vs = compile_shader(GL_VERTEX_SHADER, vsrc);
      GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fsrc);
      prog_render = link_program(2, vs, fs); free(vsrc); free(fsrc); }

    vsrc = read_file("shaders/shadow.vert"); fsrc = read_file("shaders/render.frag");
    { GLuint vs = compile_shader(GL_VERTEX_SHADER, vsrc);
      GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fsrc);
      prog_shadow_render = link_program(2, vs, fs); free(vsrc); free(fsrc); }

    vsrc = read_file("shaders/fade.vert"); fsrc = read_file("shaders/fade.frag");
    { GLuint vs = compile_shader(GL_VERTEX_SHADER, vsrc);
      GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fsrc);
      prog_fade = link_program(2, vs, fs); free(vsrc); free(fsrc); }

    vsrc = read_file("shaders/fade.vert"); fsrc = read_file("shaders/display.frag");
    { GLuint vs = compile_shader(GL_VERTEX_SHADER, vsrc);
      GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fsrc);
      prog_display = link_program(2, vs, fs); free(vsrc); free(fsrc); }


    csrc = read_file("shaders/ball.comp");
    { GLuint sh = compile_shader_with_defs(GL_COMPUTE_SHADER, csrc);
      prog_ball_compute = link_program(1, sh); free(csrc); }

    vsrc = read_file("shaders/ball.vert"); fsrc = read_file("shaders/render.frag");
    { GLuint vs = compile_shader(GL_VERTEX_SHADER, vsrc);
      GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fsrc);
      prog_ball_render = link_program(2, vs, fs); free(vsrc); free(fsrc); }

    GLuint ssbo_five_e;
    glGenBuffers(1, &ssbo_five_e);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_five_e);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * 4, NULL, GL_DYNAMIC_DRAW);

    // -------------------------------------------------------------------------
    // Uniform locations
    // -------------------------------------------------------------------------
    // physics.comp
    GLint pu_global_angle   = glGetUniformLocation(prog_compute, "u_global_angle");
    GLint pu_coh_r          = glGetUniformLocation(prog_compute, "u_coherence_radius");
    GLint pu_lrb            = glGetUniformLocation(prog_compute, "u_locus_rotation_bias");
    GLint pu_triquatra      = glGetUniformLocation(prog_compute, "u_triquatra");
    GLint pu_loci_count     = glGetUniformLocation(prog_compute, "u_loci_count");
    GLint pu_particle_count = glGetUniformLocation(prog_compute, "u_particle_count");
    GLint pu_goetic_count = glGetUniformLocation(prog_compute, "u_goetic_particle_count");
    GLint pu_time_step      = glGetUniformLocation(prog_compute, "u_time_step");
    GLint pu_5e_seconds     = glGetUniformLocation(prog_compute, "u_5e_seconds");
    GLint pu_5e_frame_start = glGetUniformLocation(prog_compute, "u_5e_frame_start");
    GLint pu_5e_frame_elapsed = glGetUniformLocation(prog_compute, "u_5e_frame_elapsed");
    GLint pu_5e_steps = glGetUniformLocation(prog_compute, "u_5e_steps");
    GLint pu_node_center    = glGetUniformLocation(prog_compute, "u_node_center");
    GLint pu_screen_w       = glGetUniformLocation(prog_compute, "u_screen_w");
    GLint pu_screen_h       = glGetUniformLocation(prog_compute, "u_screen_h");
    GLint pu_plane_id       = glGetUniformLocation(prog_compute, "u_plane_id");
    GLint pu_plane_phase    = glGetUniformLocation(prog_compute, "u_plane_phase");
    GLint pu_kappa_void     = glGetUniformLocation(prog_compute, "u_kappa_void");
    GLint pu_void_gamma     = glGetUniformLocation(prog_compute, "u_void_gamma");

    // grid.comp — dual 9x9 frequency grid
    GLint gu_node_id        = glGetUniformLocation(prog_grid, "u_node_id");
    GLint gu_plane_id       = glGetUniformLocation(prog_grid, "u_plane_id");
    GLint gu_particle_count = glGetUniformLocation(prog_grid, "u_particle_count");
    GLint gu_time_step      = glGetUniformLocation(prog_grid, "u_time_step");
    GLint gu_global_angle   = glGetUniformLocation(prog_grid, "u_global_angle");

    // shadow.comp
    GLint su_global_angle   = glGetUniformLocation(prog_shadow_compute, "u_global_angle");
    GLint su_coh_r          = glGetUniformLocation(prog_shadow_compute, "u_coherence_radius");
    GLint su_shadow_angle   = glGetUniformLocation(prog_shadow_compute, "u_shadow_angle");
    GLint su_primary_stress = glGetUniformLocation(prog_shadow_compute, "u_primary_stress");
    GLint su_time_step      = glGetUniformLocation(prog_shadow_compute, "u_time_step");
    GLint su_node_center    = glGetUniformLocation(prog_shadow_compute, "u_node_center");
    GLint su_screen_w       = glGetUniformLocation(prog_shadow_compute, "u_screen_w");
    GLint su_screen_h       = glGetUniformLocation(prog_shadow_compute, "u_screen_h");

    // render.vert
    GLint ru_global_angle   = glGetUniformLocation(prog_render, "u_global_angle");
    GLint ru_screen_size    = glGetUniformLocation(prog_render, "u_screen_size");
    GLint ru_point_size     = glGetUniformLocation(prog_render, "u_point_size");
    GLint ru_crescent_view  = glGetUniformLocation(prog_render, "u_crescent_view");
    GLint ru_crescent_center = glGetUniformLocation(prog_render, "u_crescent_center");

    // shadow.vert
    GLint sru_global_angle  = glGetUniformLocation(prog_shadow_render, "u_global_angle");
    GLint sru_screen_size   = glGetUniformLocation(prog_shadow_render, "u_screen_size");
    GLint sru_shadow_color  = glGetUniformLocation(prog_shadow_render, "u_shadow_color");
    GLint sru_point_size    = glGetUniformLocation(prog_shadow_render, "u_point_size");

    // fade
    GLint fu_trail          = glGetUniformLocation(prog_fade, "u_trail");
    GLint fu_fade_amount    = glGetUniformLocation(prog_fade, "u_fade_amount");

    // display
    GLint du_trail          = glGetUniformLocation(prog_display, "u_trail");

    // ball.comp
    GLint bcu_global_angle  = glGetUniformLocation(prog_ball_compute, "u_global_angle");
    GLint bcu_coh_r         = glGetUniformLocation(prog_ball_compute, "u_coherence_radius");
    GLint bcu_stress        = glGetUniformLocation(prog_ball_compute, "u_primary_stress");
    GLint bcu_screen_w      = glGetUniformLocation(prog_ball_compute, "u_screen_w");
    GLint bcu_screen_h      = glGetUniformLocation(prog_ball_compute, "u_screen_h");
    GLint bcu_ball_count    = glGetUniformLocation(prog_ball_compute, "u_ball_count");

    // ball.vert
    GLint bru_global_angle  = glGetUniformLocation(prog_ball_render, "u_global_angle");
    GLint bru_screen_size   = glGetUniformLocation(prog_ball_render, "u_screen_size");


    // Throne of Zero — emit once
    if (G_throne.loaded) {
        glUseProgram(prog_compute);
        glUniform1f(glGetUniformLocation(prog_compute, "u_throne_freq"),      G_throne.freq_hz);
        glUniform1f(glGetUniformLocation(prog_compute, "u_throne_coherence"), G_throne.quantum_coherence);
        glUseProgram(prog_shadow_compute);
        glUniform1f(glGetUniformLocation(prog_shadow_compute, "u_throne_freq"),      G_throne.freq_hz);
        glUniform1f(glGetUniformLocation(prog_shadow_compute, "u_throne_coherence"), G_throne.quantum_coherence);
    }

    // Invariant uniforms — screen dimensions never change (GLFW_RESIZABLE=FALSE).
    // Set once here; removing from per-node per-frame loops saves 12*3 glUniform
    // calls per frame for prog_compute, prog_shadow_compute, prog_ball_compute.
    glUseProgram(prog_compute);
    glUniform1f(pu_screen_w, (float)screen_w);
    glUniform1f(pu_screen_h, (float)screen_h);
    glUseProgram(prog_shadow_compute);
    glUniform1f(su_screen_w, (float)screen_w);
    glUniform1f(su_screen_h, (float)screen_h);
    glUseProgram(prog_ball_compute);
    glUniform1f(bcu_screen_w, (float)screen_w);
    glUniform1f(bcu_screen_h, (float)screen_h);
    glUseProgram(prog_render);
    glUniform2f(ru_screen_size, (float)screen_w, (float)screen_h);
    glUseProgram(prog_shadow_render);
    glUniform2f(sru_screen_size, (float)screen_w, (float)screen_h);
    glUseProgram(prog_ball_render);
    glUniform2f(bru_screen_size, (float)screen_w, (float)screen_h);


    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);


    // -------------------------------------------------------------------------
    // Simulation state
    // -------------------------------------------------------------------------
    float global_angle  = 0.0f;
    int   frame_count   = 0;
    int   cur_trail     = 0;

    bool           recording       = false;
    int            record_frame    = 0;
    int            screenshot_next = 0;
    unsigned char *pixel_buf = malloc(screen_w * screen_h * 3);
    ensure_dir(G_global_cfg.recording_dir);

    // No scheduled void-frame transition. The Throne of Zero is not a clocked switch.

    double last_frame_time = glfwGetTime();
    double five_e_previous_frame = last_frame_time;

    int key_s_prev=GLFW_RELEASE, key_r_prev=GLFW_RELEASE;
    int key_p_prev=GLFW_RELEASE;

    while (!glfwWindowShouldClose(win)) {
        // Resolve the native 936 Hz contact vibration during physics, not
        // only once at the display frame rate. Four phase samples per cycle.
        double five_e_frame_end = glfwGetTime();
        double five_e_frame_start = five_e_previous_frame;
        double five_e_elapsed = five_e_frame_end - five_e_frame_start;
        if (five_e_elapsed < 0.0) five_e_elapsed = 0.0;
        five_e_previous_frame = five_e_frame_end;
        int five_e_steps = (int)ceil(five_e_elapsed * 936.0 * 4.0);
        if (five_e_steps < 1) five_e_steps = 1;
        glfwPollEvents();

        if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(win, GLFW_TRUE);

        int key_s = glfwGetKey(win, GLFW_KEY_S);
        int key_r = glfwGetKey(win, GLFW_KEY_R);
        int key_p = glfwGetKey(win, GLFW_KEY_P);
        if (key_s == GLFW_PRESS && key_s_prev == GLFW_RELEASE) screenshot_next = 1;
        if (key_r == GLFW_PRESS && key_r_prev == GLFW_RELEASE) { recording = true; fprintf(stderr, "Recording started\n"); }
        if (key_p == GLFW_PRESS && key_p_prev == GLFW_RELEASE) { recording = false; fprintf(stderr, "Recording stopped (%d frames)\n", record_frame); }
        key_s_prev=key_s; key_r_prev=key_r; key_p_prev=key_p;


        // No scheduled NULL:VOID collapse.

        // Whisper injection (per cube config)
        int wm = G_global_cfg.whisper_mode;
        if ((wm == 1 || wm == 3) && frame_count == 0)
            apply_whisper(entropy_table, ssbo_entropy, 0u, G_global_cfg.whisper_file);
        // Second whisper is not tied to void_frame.
        // It should be restored only when a natural post-collapse witness condition exists.

        // Global angle — cached average sim_speed (config-invariant, pre-computed).
        global_angle += L_RB_MAX_RATE * avg_sim_speed_cached;

        // Node centers are not reprojected from a square/cube scaffold.
        // No visible or invisible box metric is applied here.

        // Bind shared SSBOs
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_entropy);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssbo_fetu);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssbo_plane_cfg);

        // =====================================================================
        // Stress ball global pass
        // =====================================================================
        {
            float avg_stress=0.0f, avg_coh_r=0.0f;
            for (int n=0; n<N_NODES; n++) { avg_stress+=node_stress[n]; avg_coh_r+=node_coh_r[n]; }
            avg_stress /= (float)N_NODES; avg_coh_r /= (float)N_NODES;

            // binding 7 reused for PhaseBuf here; physics.comp uses it for PlaneConfig
            // but ball and physics passes run sequentially so binding 7 is free here.
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_boundary[0]);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssbo_balls);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssbo_phase_mem);
            glUseProgram(prog_ball_compute);
            glUniform1f(bcu_global_angle, global_angle);
            glUniform1f(bcu_coh_r,        avg_coh_r);
            glUniform1f(bcu_stress,       avg_stress);
            glUniform1ui(bcu_ball_count,   (GLuint)G_global_cfg.sigh_ball_count);
            // bcu_screen_w/h set once at startup (screen never resizes).
            GLuint ball_groups = ((GLuint)G_global_cfg.sigh_ball_count + 63u) / 64u;
            if (ball_groups > 0) glDispatchCompute(ball_groups, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }

        // =====================================================================
        // Per-node physics pass
        // =====================================================================
        for (int n = 0; n < N_NODES; n++) {
            const NodeConfig *C = &G_node_cfg[n];

            float stress_acc = 0.0f;
            for (int i = 0; i < C->particle_count; i++) {
                float vx = readback[n][i].dx, vy = readback[n][i].dy;
                stress_acc += sqrtf(vx*vx + vy*vy);
            }
            node_stress[n] = stress_acc;

            float norm_s     = tanhf(node_stress[n] / MAX_KINETIC_STR);
            float shadow_tot = (1.0f - norm_s) * MAX_KINETIC_STR;
            float combined   = (node_stress[n] + shadow_tot) / 2.0f;
            float af         = 1.0f - (AEON_FREQ[0] / AEON_FREQ[7]);
            float curr_stress= combined * af * af;
            float sf         = 1.0f - curr_stress / (MAX_KINETIC_STR + 1e-9f);
            node_coh_r[n]    = MIN_COH_R + (MAX_COH_R - MIN_COH_R) * sf;

            float norm_lrb = tanhf(curr_stress / MAX_KINETIC_STR);
            node_lrb[n] += L_RB_MAX_RATE * (1.0f - norm_lrb) * ELVEN_RESP_GAIN * 10.0f * C->sim_speed;

            float tri_px[3], tri_py[3];
            int loci_count = 3;
            for (int k = 0; k < 3; k++) {
                float t = node_lrb[n] + k * 2.0f * PI_F / 3.0f;
                tri_px[k] = node_cx[n] + C->triquatra_radius * cosf(t);
                tri_py[k] = node_cy[n] + C->triquatra_radius * sinf(t);
            }

            bm_decay_node(n);
            for (int i = 0; i < C->particle_count; i++)
                cpu_bm_deposit_from_particle(&readback[n][i], global_angle,
                                             node_cx[n], node_cy[n], n);

            {
                // G_bm[n] is already interleaved {vx,vy,...} — direct upload, no copy loop.
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_boundary[n]);
                glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(G_bm[n]), G_bm[n]);
            }

            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_particles[n]);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_boundary[n]);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_corner_off);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_shadow[n]);

            // Shadow pass
            glUseProgram(prog_shadow_compute);
            glUniform1f(su_global_angle,   global_angle);
            glUniform1f(su_coh_r,          node_coh_r[n]);
            glUniform1f(su_shadow_angle,   node_shadow_angle[n]);
            glUniform1f(su_primary_stress, node_stress[n]);
            glUniform1f(su_time_step,      C->sim_speed);
            glUniform2f(su_node_center,    node_cx[n], node_cy[n]);
            // su_screen_w/h set once at startup (screen never resizes).
            glDispatchCompute(1, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            node_shadow_angle[n] += 0.05f * C->sim_speed;

            // Grid update pass — dual 9x9 frequency grid (agent 9/25 + 16/25)
            // Runs before physics so particles read current-frame grid state.
            // Rebind slot 4 from ssbo_shadow to ssbo_freq_grid for this pass.
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_entropy);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_freq_grid);
            glUseProgram(prog_grid);
            glUniform1ui(gu_particle_count, (GLuint)(2 * C->particle_count));
            glUniform1f(gu_time_step,       C->sim_speed);
            glUniform1f(gu_global_angle,    global_angle);
            for (int plane = 0; plane < 2; plane++) {
                glUniform1i(gu_node_id,   n);
                glUniform1i(gu_plane_id,  plane);
                glDispatchCompute(1, 1, 1);  // 1 workgroup = 9x9 = 81 threads
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            }

            // Physics pass (new plane uniforms for B5, B6, B8)
            // FreqGridBuf stays at binding 4 for physics.comp.
            // Bug 1 fix: ball pass overwrote slot 7 with ssbo_phase_mem; restore.
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssbo_plane_cfg);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, ssbo_five_e);
            glUseProgram(prog_compute);
            glUniform1f(pu_global_angle,   global_angle);
            glUniform1f(pu_coh_r,          node_coh_r[n]);
            glUniform1f(pu_lrb,            node_lrb[n]);
            { float tri_flat[6] = { tri_px[0], tri_py[0], tri_px[1], tri_py[1], tri_px[2], tri_py[2] };
              glUniform2fv(pu_triquatra, 3, tri_flat); }
            glUniform1i(pu_loci_count,      loci_count);
            glUniform1ui(pu_particle_count, (GLuint)(2 * C->particle_count));
            glUniform1ui(pu_goetic_count, (GLuint)C->particle_count);
            glUniform1f(pu_time_step,       C->sim_speed);
            glUniform1f(pu_5e_frame_start, (float)fmod(five_e_frame_start, 1.0 / 936.0));
            glUniform1f(pu_5e_frame_elapsed, (float)five_e_elapsed);
            glUniform1i(pu_5e_steps, five_e_steps);
            glUniform1f(pu_5e_seconds, (float)fmod(five_e_frame_end, 1.0 / 936.0));
            glUniform2f(pu_node_center,     node_cx[n], node_cy[n]);
            // pu_screen_w/h set once at startup (screen never resizes).
            glUniform1i(pu_plane_id,        C->plane_id);
            glUniform1f(pu_plane_phase,     C->plane_phase);
            glUniform1f(pu_kappa_void,      C->kappa_void);
            glUniform1f(pu_void_gamma,      C->void_gamma);

            // Ensure binding 4 = FreqGridBuf (not ssbo_shadow) for physics pass
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_freq_grid);
            GLuint groups = ((GLuint)(2 * C->particle_count) + 255u) / 256u;
            if (groups > 0) glDispatchCompute(groups, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

            // Readback every 4 frames
            if (frame_count % 4 == 0) {
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_particles[n]);
                glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                                   2 * C->particle_count * sizeof(ParticleGPU), readback[n]);

                bool any_reinit = false;
                for (int i = 0; i < 2 * C->particle_count; i++) {
                    float R = sqrtf(readback[n][i].x*readback[n][i].x +
                                    readback[n][i].y*readback[n][i].y +
                                    readback[n][i].z*readback[n][i].z +
                                    readback[n][i].w*readback[n][i].w);
                    if (R > ESCAPE_LIMIT || !isfinite(R)) {
                        // Canon fix (Throne of Zero — NON-COMPUTE, d(♾)/dt = 0):
                        // Never force a position. Scatter back just outside the 5e seam
                        // at a random angle — let seam dynamics take it from there.
                        // 4D hyperspherical scatter: random point on S³ surface.
                        // Use two angles for (x,y,z) sphere plus a w coordinate
                        // drawn from [-1,1], renormalized so ||(x,y,z,w)|| = r_seed.
                        float theta = ((float)(rand() % 4096) / 4096.0f) * 6.28318f;
                        float phi_a = ((float)(rand() % 4096) / 4096.0f) * 3.14159f;
                        float w_raw = ((float)(rand() % 4096) / 4096.0f) * 2.0f - 1.0f;
                        float xyz_r = sqrtf(fmaxf(0.0f, 1.0f - w_raw * w_raw));
                        float r_seed = 0.04f + PHI * 0.005f;  // just outside seam
                        readback[n][i].x = r_seed * xyz_r * cosf(theta) * sinf(phi_a);
                        readback[n][i].y = r_seed * xyz_r * sinf(theta) * sinf(phi_a);
                        readback[n][i].z = r_seed * xyz_r * cosf(phi_a);
                        readback[n][i].w = r_seed * w_raw;
                        // Inward velocity in 4D at canonical base_speed for this particle's freq
                        float v_seed = readback[n][i].freq / 10000.0f;
                        readback[n][i].dx = -xyz_r * cosf(theta) * sinf(phi_a) * v_seed;
                        readback[n][i].dy = -xyz_r * sinf(theta) * sinf(phi_a) * v_seed;
                        readback[n][i].dz = -xyz_r * cosf(phi_a)               * v_seed;
                        readback[n][i].dw = -w_raw                             * v_seed;
                        any_reinit = true;
                    }
                }
                if (any_reinit) {
                    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_particles[n]);
                    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                                    2 * C->particle_count * sizeof(ParticleGPU), readback[n]);
                }
            }
        }

        // =====================================================================
        // Render pass — trail ping-pong
        // =====================================================================
        int next_trail = 1 - cur_trail;
        glBindFramebuffer(GL_FRAMEBUFFER, trail_fbo[next_trail]);
        glViewport(0, 0, screen_w, screen_h);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Fade pass (cached avg_fade — config-invariant, pre-computed at startup).
        glUseProgram(prog_fade);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, trail_tex[cur_trail]);
        glUniform1i(fu_trail, 0);
        glUniform1f(fu_fade_amount, avg_fade_cached);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        // Render particles and shadows for each node.
        // ru_screen_size / sru_screen_size set once at startup (never changes).
        for (int n = 0; n < N_NODES; n++) {
            const NodeConfig *C = &G_node_cfg[n];
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_particles[n]);
            glUseProgram(prog_render);
            glUniform1f(ru_global_angle, global_angle);
            glUniform1f(ru_point_size,   C->particle_point_size);
            glUniform2f(ru_crescent_center, node_cx[n], node_cy[n]);
            glUniform1i(ru_crescent_view, 0);
            glDrawArrays(GL_POINTS, 0, C->particle_count); // existing Goetic
            glUniform1i(ru_crescent_view, 1);
            glDrawArrays(GL_POINTS, C->particle_count, C->particle_count); // independent XYW
            glUniform1i(ru_crescent_view, 0);

            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_shadow[n]);
            glUseProgram(prog_shadow_render);
            glUniform1f(sru_global_angle, global_angle);
            glUniform3f(sru_shadow_color, C->shadow_r/255.0f, C->shadow_g/255.0f, C->shadow_b/255.0f);
            glUniform1f(sru_point_size,   C->shadow_point_size);
            glDrawArrays(GL_POINTS, 0, SHADOW_ENTITY_COUNT);
        }

        // Render stress balls — bru_screen_size set once at startup.
        {
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssbo_balls);
            glUseProgram(prog_ball_render);
            glUniform1f(bru_global_angle, global_angle);
            glDrawArrays(GL_POINTS, 0, G_global_cfg.sigh_ball_count);
        }
        glDisable(GL_BLEND);

        // Square/cube box removed completely: no wireframe draw path.

        cur_trail = next_trail;

        // Display pass (blit trail to screen)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, screen_w, screen_h);
        glUseProgram(prog_display);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, trail_tex[cur_trail]);
        glUniform1i(du_trail, 0);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glfwSwapBuffers(win);
        frame_count++;

        // FPS cap
        if (G_global_cfg.fps_cap > 0) {
            double target = 1.0 / (double)G_global_cfg.fps_cap;
            double now = glfwGetTime();
            double elapsed = now - last_frame_time;
            if (elapsed < target) {
                struct timespec ts;
                double rem = target - elapsed;
                ts.tv_sec  = (time_t)rem;
                ts.tv_nsec = (long)((rem - ts.tv_sec) * 1e9);
                nanosleep(&ts, NULL);
            }
            last_frame_time = glfwGetTime();
        }

        // Screenshot / recording
        if (screenshot_next || recording) {
            glReadPixels(0, 0, screen_w, screen_h, GL_RGB, GL_UNSIGNED_BYTE, pixel_buf);
            char path[512];
            if (screenshot_next) {
                static int sc_n = 0;
                snprintf(path, sizeof(path), "%s/screenshot_%04d.ppm", G_global_cfg.recording_dir, sc_n++);
                save_ppm(path, pixel_buf, screen_w, screen_h);
                screenshot_next = 0;
            }
            if (recording) {
                snprintf(path, sizeof(path), "%s/frame_%07d.ppm", G_global_cfg.recording_dir, record_frame++);
                save_ppm(path, pixel_buf, screen_w, screen_h);
            }
        }
    }

    fprintf(stderr, "Exit after %d frames\n", frame_count);
    free(pixel_buf);
    free(entropy_table);
    for (int n=0; n<N_NODES; n++) free(readback[n]);
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
