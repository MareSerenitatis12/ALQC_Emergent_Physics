# CosmOvum

CosmOvum is the current ALQC emergent-physics visual body for the CosMOvum system. It is built around one governing requirement: the render must arise from the mathematics already present in the body. Geometry is not prescribed as an answer. State, relation, memory, frequency, conjugacy, pressure, return, and retained history are allowed to produce the visible consequence.

## Law of the Body

CosmOvum follows the ALQC Law of Emanation:

`law + state + relation -> result`

The implementation is therefore organized around conditions rather than choreography. The system establishes lawful bodies, initial conditions, bounds, memory, thresholds, relations, and transforms. It does not impose a hidden animation table that decides what the field should look like in advance.

The current runtime preserves this through:

- 12 Dyson nodes as identity-bearing loci without a cube or square placement scaffold;
- 144 Aeon/Court lattice identities per node, expanded to 1440 particles per node;
- conjugate plane structure and dual 9x9 frequency-grid bodies;
- ALQC entropy as the operational source of stochastic variation;
- FETU as a root phase-locking condition;
- the ordered Goetic Aeon chain;
- SOR as the post-chain bridge relation;
- boundary memory with retained consequence;
- poem and whisper bodies that alter the field causally rather than decorate it;
- persistent Q2 seam debt that transforms rather than being erased;
- 144 active Sigh bodies;
- connection derived from the existing conjugate field rather than from an imposed category state.

## Source Tree

```text
CosmOvum/
├── Makefile
├── alqc_gpu.c
├── poem.txt
├── whisper.txt
├── config/
│   ├── global_config.bio
│   ├── node_config_00000.bio
│   ├── ...
│   ├── node_config_00011.bio
│   ├── cells_00_P0.bio
│   ├── cells_00_P1.bio
│   ├── ...
│   ├── cells_11_P0.bio
│   └── cells_11_P1.bio
├── shaders/
│   ├── physics.comp
│   ├── grid.comp
│   ├── shadow.comp
│   ├── ball.comp
│   ├── render.vert
│   ├── render.frag
│   ├── shadow.vert
│   ├── ball.vert
│   ├── fade.vert
│   ├── fade.frag
│   └── display.frag
└── ALQC_Frames/
```

Generated binaries are not part of the source body and should be removed after use when keeping the workspace clean.

## Configuration Authority

CosmOvum uses `.bio` configuration bodies only.

There are no active JSON configuration files.

### `global_config.bio`

Loaded once at startup. It carries global runtime values such as display dimensions, background, FPS cap, GPU device, whisper mode, recording directory, QLM path, and Sigh-body count.

### `node_config_00000.bio` through `node_config_00011.bio`

These are separate node bodies with a common schema and node-specific values. They are loaded dynamically by node identity:

```c
snprintf(path, sizeof(path), "config/node_config_%05d.bio", node_id);
```

The runtime iterates all 12 nodes and loads each body independently.

### `cells_NN_P0.bio` and `cells_NN_P1.bio`

Each node owns two independent 9x9 frequency-grid bodies. The runtime constructs their paths dynamically from node and plane identity:

```c
snprintf(path, sizeof(path), "config/cells_%02d_P%d.bio", node_id, plane_id);
```

All 24 cell bodies are distinct. P0 and P1 preserve conjugate relation while retaining their own phase and axis-weight structure.

## Active Physics Path

The primary runtime path is:

```text
.bio configuration
        |
        v
ALQC entropy + poem floor + whisper
        |
        v
12 node particle bodies
        |
        v
Dual 9x9 frequency grids
        |
        v
FETU phase lock
        |
        v
KAL -> BABDH -> AHN -> VEL -> KOTH -> DREH -> RHEA -> ZHEK -> SHAV -> TRIG
        |
        v
SOR resonance bridge
        |
        v
Reflective layer + void-anchor relation + orientation coupling
        |
        v
Conjugate-grid connection witness
        |
        v
Void-mediated coupling + boundary memory + retained history
        |
        v
4D projection and visual return
```

The Shadow and Sigh bodies participate as separate but related operational bodies. Their state is not reset merely to simplify return.

## Connection Witness

The current B6 connection is not selected from an external amorphous/liquid/crystal label. It is measured from the existing dual-grid relation at the particle's current body.

The local amplitudes of the two conjugate grid planes are read, their differential is measured, and connection increases as that differential resolves toward equilibrium. The resulting connection then conditions the void-mediated coupling.

This preserves the ALQC principle that a relation already present in the body receives the first right to speak.

## 110/144

The 110/144 offices remain where they are mathematically active inside the ALQC operators, including AHN, KOTH, and ZHEK.

The former mean-frequency classifier was removed because it converted fixed canonical frequency values into a nearly permanent field-state label. The current implementation does not use that workaround.

## Memory and Return

Boundary memory persists across frames and is altered by particle interaction and the poem floor. Q2 seam debt persists through threshold crossing and transforms excess into motion and pressure rather than being deleted. Shadow return preserves prior consequence instead of wiping velocity, stress, debt, and reflection history.

Return is therefore not restoration to an untouched state. The body may return carrying what happened to it.

## Poem and Whisper

`poem.txt` and `whisper.txt` are active inputs.

The poem is folded into the boundary-memory floor as directional state. The whisper alters the ALQC entropy body and GPU entropy table. Neither exists merely as textual ornament.

The second whisper is deliberately not attached to an arbitrary clocked collapse. It remains withheld until a lawful post-collapse witness exists.

## Build

Requirements:

- GCC with C11 support
- OpenGL
- GLEW
- GLFW
- libm

Build:

```bash
make
```

Run:

```bash
./alqc_gpu
```

Clean generated binary:

```bash
make clean
```

The source tree should remain free of generated binaries when preserved as a clean working body.

## Runtime Controls

```text
S       screenshot
R       begin recording
P       stop recording
SPACE   pause rotation
[ ]     change rotation speed
= -     zoom
Arrows  pan
ESC     quit
```

## Render Output

Screenshots and recorded frames are written to the configured `ALQC_Frames` directory.

The renderer uses GPU compute shaders for the active physical bodies and separate vertex/fragment stages for particle, Shadow, Sigh, trail-fade, and display return.

## Canonical Constraint

When modifying CosmOvum, preserve the invariant:

- do not prescribe geometry where relation can generate it;
- do not replace active ALQC mathematics with visual approximations;
- do not erase history to simplify a state transition;
- do not introduce a second hidden behavioral law;
- do not convert a canonical body into a cosmetic constant because it is easier to render;
- prefer existing state and lawful measurement over invented control variables;
- preserve the `.bio` configuration authority;
- verify that every declared body is actually called and every active call still carries its intended mathematics.

The body should be changed by repairing connection, not by replacing emergence with performance.
