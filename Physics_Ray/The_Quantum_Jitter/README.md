# ALQC GPU v22 — Emergent Void Physics

A real-time OpenGL 4.3 renderer of `emergent_void_physics10.py`. Every line of physics is translated exactly as written — nothing changed, nothing invented. All physics runs on the GPU via compute shaders.

---

## Quick Start

```bash
cd v22
make
./alqc_gpu
```

**Requirements:** GCC, libGL, libGLEW, libglfw3. On Ubuntu/Debian: `sudo apt install libglew-dev libglfw3-dev`

---

## Keys

| Key | Action |
|-----|--------|
| `S` | Save a screenshot to `ALQC_Frames/screenshot_NNNN.ppm` |
| `R` | Start recording (saves every frame to `ALQC_Frames/frame_NNNNNNN.ppm`) |
| `P` | Stop recording |
| `ESC` | Quit |

---

## Screen Recording — Converting Frames to Video

Press `R` to start, `P` to stop. Frames land in `ALQC_Frames/` as numbered PPM files. To turn them into a video:

```bash
# MP4 (H.264) — plays everywhere
ffmpeg -framerate 60 -i ALQC_Frames/frame_%07d.ppm -c:v libx264 -pix_fmt yuv420p output.mp4

# Higher quality / lossless
ffmpeg -framerate 60 -i ALQC_Frames/frame_%07d.ppm -c:v libx264 -crf 0 output.mp4

# If your framerate is different from 60, change -framerate to match FPS_CAP
ffmpeg -framerate 30 -i ALQC_Frames/frame_%07d.ppm -c:v libx264 -pix_fmt yuv420p output.mp4

# GIF (smaller, loops, good for sharing)
ffmpeg -framerate 30 -i ALQC_Frames/frame_%07d.ppm -vf "scale=500:-1" output.gif
```

Install ffmpeg on Ubuntu/Debian: `sudo apt install ffmpeg`

Screenshots (`screenshot_NNNN.ppm`) open directly in most image viewers, or convert with:
```bash
convert ALQC_Frames/screenshot_0001.ppm screenshot.png
```

---

## Every User Variable

All user-configurable options live at the **very top** of `alqc_gpu.c`, clearly marked. Change them, run `make`, and relaunch. No other files need to be touched.

---

### Display

| Variable | Default | What it does |
|----------|---------|--------------|
| `WIDTH` | `1000` | Window width in pixels |
| `HEIGHT` | `1000` | Window height in pixels |
| `BACKGROUND_R` | `5` | Background red tint (0–255). The original is nearly black with a blue-purple cast. |
| `BACKGROUND_G` | `5` | Background green tint (0–255) |
| `BACKGROUND_B` | `10` | Background blue tint (0–255) |

---

### Particles

| Variable | Default | What it does |
|----------|---------|--------------|
| `PARTICLE_COUNT` | `5000` | Number of main physics particles. The original uses 5000. Higher = richer field, slower if your GPU is weak. |
| `SIGH_BALL_COUNT` | `4` | Number of stress balls. The original uses 4. Stress balls are the larger colored dots that move through the particle field. Setting this to 0 disables them. |

---

### Point Sizes

These control how big each dot is on screen. Physics is unaffected.

| Variable | Default | What it does |
|----------|---------|--------------|
| `PARTICLE_POINT_SIZE` | `1.0` | Pixel diameter of each particle dot |
| `SHADOW_POINT_SIZE` | `2.0` | Pixel diameter of each shadow locus dot (the red clusters at the corners) |
| `BALL_POINT_SIZE` | `4.0` | Pixel diameter of each stress ball dot. Larger values make stress balls more visible. |

---

### Shadow Locus Color

The four corner clusters (Shadow Loci) glow in this color. The original is a deep red-orange.

| Variable | Default | What it does |
|----------|---------|--------------|
| `SHADOW_LOCUS_R` | `255.0` | Red component of shadow locus color (0–255) |
| `SHADOW_LOCUS_G` | `0.0` | Green component (0–255) |
| `SHADOW_LOCUS_B` | `50.0` | Blue component (0–255) |

---

### Trail

| Variable | Default | What it does |
|----------|---------|--------------|
| `TRAIL_FADE` | `15` | How fast the particle trails fade per frame (0–255). `1` = extremely long ghostly trails. `15` = original. `255` = trails disappear instantly, particles appear with no history. |

---

### Simulation

| Variable | Default | What it does |
|----------|---------|--------------|
| `SIM_SPEED` | `1.0` | Time step multiplier — any positive float, no upper limit. `1.0` = original speed. `0.1` = slow motion. `12.0`, `50.0`, `500.0` — all valid. Higher values make the physics step further each frame; the simulation remains stable until the integration diverges at extreme values. |
| `FPS_CAP` | `60` | Maximum frames per second. `0` = unlimited (runs as fast as possible). `60` = smooth 60fps. `30` = half speed refresh. |
| `VOID_TRANSITION_FRAME` | `600` | The frame number at which NULL:DEATH begins. At this frame, the simulation enters its void collapse state — shadow loci continue orbiting their corners, stress balls collapse to the center of the screen, and the title bar changes. The original triggers this at frame 600. |
| `ENTROPY_SIZE` | `1048576` | Size of the pre-generated entropy table (must be a power of 2). Larger = more entropy variety before the table repeats. Default is 1 million floats. Do not reduce below 65536. |

---

### Superpositive Whisper

| Variable | Default | What it does |
|----------|---------|--------------|
| `ENABLE_WHISPER` | `1` | `1` = read `whisper.txt` at startup. `0` = skip entirely. The whisper file contains only invisible zero-width Unicode characters. The program reads it but does nothing computational with the data. It is a superpositive consent signal embedded in the rendering context. |
| `WHISPER_FILE` | `"whisper.txt"` | Path to the whisper file. Change this if you move the file. |

---

### Recording

| Variable | Default | What it does |
|----------|---------|--------------|
| `RECORDING_DIR` | `"ALQC_Frames"` | Directory where screenshots and recorded frames are saved. Created automatically if it does not exist. |

---

## What You Are Looking At

The simulation runs three physics layers simultaneously, all on the GPU:

**Particles (5000 default)** — The main field. Each particle lives in 4D space and is projected down to your 2D screen at every frame. They are organized into 12 aeons (frequency resonance groups), each with a distinct color. Lesser aeons appear gray. Particles interact with the void anchors at the four screen corners, a reflective shell at radius 0.92 in 4D space, and each other's boundary memory field.

**Shadow Loci (48 entities)** — Four clusters of 12 entities each, one cluster per corner. They orbit their corner and glow in `SHADOW_LOCUS_COLOR`. Their brightness is the *inverse* of the particle field stress — when the particles are highly stressed, the shadow loci dim; when particles are calm, the shadow loci are brightest. They continue running during NULL:DEATH.

**Stress Balls (4 default)** — Larger colored points that wander through the field. Each one is assigned a random aeon and uses that aeon's color. They decay slowly — their brightness (`charge`) diminishes over time. During NULL:DEATH, they collapse visually to the screen center.

**NULL:DEATH (frame 600)** — The void transition. The three rotating anchor points collapse to a single point at the screen center. This changes how particles compute their brightness (they all flock toward the center locus). Stress balls collapse visually to center. Shadow loci keep orbiting. The simulation does not stop — it continues indefinitely in this state.

**Boundary Memory** — An invisible 160×160 grid of velocity vectors covering the screen. As particles pass through the reflective shell, they deposit their tangential velocity into this grid. The grid decays slowly each frame. Particles in the shell zone sample this grid and are nudged by what previous particles left behind. This creates emergent coherent flow patterns.

**Trail** — The screen is not cleared each frame. Instead, all colors are dimmed by `TRAIL_FADE/255` per channel. This creates the glowing trail effect. Setting TRAIL_FADE lower makes trails longer and more ghostly.

---

## Architecture Notes

- All physics runs in GPU compute shaders (`shaders/physics.comp`, `shaders/shadow.comp`, `shaders/ball.comp`)
- The CPU handles only what cannot be parallelized: boundary memory tanh-fold deposits (order-dependent), particle respawn (entropy-dependent sequential logic), entropy table pre-generation
- Vertex shaders read particle state directly from GPU memory via `gl_VertexID` — no per-frame CPU-to-GPU data transfer for rendering
- Entropy is pre-generated on the CPU using the original `ALQCFieldEntropy` algorithm (golden-ratio phase accumulator), then uploaded once as a read-only SSBO. Each entity keeps its own cursor into the table.
- The whisper file (`whisper.txt`) contains only zero-width Unicode characters (U+200B and U+200C) encoding consent text using 1-bit-per-character steganography. It is invisible in any text editor. The program reads it at startup as a superpositive rendering context signal.
