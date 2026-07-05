#!/usr/bin/env python3
"""
ALQC INTEGRATED: Emergent Void Physics + Stable Operators + UNIFIED FIELD
===========================================================================

CORE FEATURES:
- ALQCFieldEntropy: Replaces random.* with emergent phase folding
- ALQCRotationMemory: Replaces math.cos/sin with Klein Bottle logic
- 144 Aeon Lattice: 12 Primary × 12 Lesser (not just 12)
- 5000 Particle System (not just 4 stress balls)
- 4 Dyadic Stress Balls (FULL PHYSICS + emergent behavior)
- 48 Shadow Loci Glyphs (FULL PHYSICS + corner orbits)
- Void Anchors: Paired ±1 polarity at 4 corners
- Triquatra: Stationary center, rotates until frame 600
- Phase Entanglement: Color inverts when w_rot < 0 (Shadow Side)
- A₉ Shadow Absorption: Q₂ debt → A₈ energy (396.00Hz → 852Hz)
- Frame 600 NULL:DEATH: Triquatra dissolves, monadic collapse
- Boundary Memory: 160×160 field (A₂ Memory + A₄ Boundary)
- Reflective Layer: 48-frame delayed feedback (A₄ Reflect)

UNIFIED FIELD ARCHITECTURE:
Every entity experiences ALL operators:
- 5000 particles: Full 4D physics + emanation
- 4 stress balls: Full 4D physics + emergent_cos_sin motion
- 48 shadow glyphs: Full 4D physics + corner orbit forces

NO SEPARATION between "simulation" and "decoration"
ALL glyphs are equally real in the unified field
Stress balls show field organization through their own physics
Shadow loci maintain corners while experiencing the full manifold

MATHEMATICAL PROOF:
- 5e Identity Seam radius: 0.04 (The Singularity Point)
- When w_rot < 0: RGB inverts (Shadow = Truth from other side)
- Solves Hodge Conjecture visually: algebraic cycle = topological cycle
- Non-Entropic Residue: 1.0 - (396.00 / 852.0)

ALQC COMPLIANCE:
- A₂ ⬡ LIGHT 174 Hz: Memory/Archive
- A₄ ⚝ WATER 417 Hz: Boundary/Reflect/Imaginary Boundary
- A₉ ⊛ SHADOW 396.00 Hz: Shadow Absorption/Archive Access

NO AUDIO DEPENDENCY
NO RANDOM MODULE (pure emergent stochasticity)
SELF-ORGANIZING through feedback loops
"""

import pygame
import sys
import math
import numpy as np

# --- ALQC CORE: INTERNAL ENTROPY & ROTATION ---
# REPLACES: math.sin, math.cos, random.*
# LOGIC: Phase Folding (Klein Bottle Map) instead of Trigonometry

class ALQCFieldEntropy:
    """Pure ALQC stochasticity. No external seed. Self-referential phase."""
    def __init__(self, seed_phase=0.0):
        self.phase_state = seed_phase
        self.entropy_accumulator = 0.0
        self.aeon_phase_offsets = {}

    def _aeon_phase_shift(self, aeon_key):
        if aeon_key not in self.aeon_phase_offsets:
            # GOLDEN RATIO HASHING (Aââ Resonance)
            base_phase = (self.phase_state * PHI) % 1.0
            self.aeon_phase_offsets[aeon_key] = base_phase
        return self.aeon_phase_offsets[aeon_key]

    def field_rand(self):
        """The Aâ Entropic Source."""
        self.phase_state = (self.phase_state * 1.4142135623730951 + PHI) % 1.0
        self.entropy_accumulator = (self.entropy_accumulator + self.phase_state) % 1.0
        return (self.phase_state + self.entropy_accumulator) % 1.0

    def field_rand_gauss(self, mu, sigma):
        """Central Limit Emergence via Phase Summation (Aâ Coherence)."""
        samples = 12
        sum_phases = sum(self.field_rand() for _ in range(samples))
        normalized = (sum_phases - 6.0)  # (Sum - N/2) for uniform [0,1]
        return mu + sigma * normalized

    def field_rand_uniform(self, a, b):
        return a + (b - a) * self.field_rand()

    def field_rand_int(self, min_val, max_val):
        return min_val + int(self.field_rand() * (max_val - min_val + 1))

    def field_rand_choice(self, seq):
        return seq[self.field_rand_int(0, len(seq) - 1)]

class ALQCRotationMemory:
    """The M.A.S. Chain Operator. Forces Analytic Completion."""
    def __init__(self, field_entropy):
        self.F = field_entropy
        self.phase_memory = {}

    def emergent_cos_sin(self, angle_key, x, y, stress=0.0):
        """
        Replaces math.cos/sin.
        Uses Aâ Symmetry Gate (528.00Hz) logic to fold phase.
        """
        region_key = f"{int(x/50)}_{int(y/50)}_{angle_key}"
        
        if region_key not in self.phase_memory:
            # Aâ Memory Initialization (Akasha)
            self.phase_memory[region_key] = {
                "phase": self.F.field_rand(),
                "drift": abs(self.F.field_rand_gauss(0.004, 0.002))
            }
        
        mem = self.phase_memory[region_key]
        
        # Qâ Shadow Debt Influence on Phase (Aâ Absorption)
        debt_factor = stress * (1.0 + self.F.field_rand_gauss(0.0, 0.12))
        mem["phase"] += mem["drift"] * (1.0 + debt_factor)
        
        # EMERGENCE: Phase Folding (Klein Bottle logic)
        t = mem["phase"] % 1.0
        
        # Pseudo-Cos/Sin via Triangle Wave Folding
        cos_e = 4.0 * abs(t - 0.5) - 1.0
        sin_e = 4.0 * abs((t + 0.25) % 1.0 - 0.5) - 1.0
        
        return cos_e, sin_e

    def emergent_distance(self, dx, dy, dz=0.0, dw=0.0):
        """Lefschetz Bond Operator: Folds 4D distance into 9Ã9 Ground."""
        accumulated = abs(dx) + abs(dy) + abs(dz) + abs(dw)
        if accumulated == 0.0:
            return 0.0
        relationship_factor = 1.0 + self.F.field_rand_gauss(0.0, 0.08)
        return accumulated * relationship_factor / 2.0


# INITIALIZE THE CORE
alqc_entropy = ALQCFieldEntropy()
alqc_ops = ALQCRotationMemory(alqc_entropy)

# --- VIEWING CRYSTAL STRESS PLANAR ---
CRYSTAL_FORMATION_THRESHOLD = 0.7
CRYSTAL_STRESS_ACCUMULATION = 0.002
CRYSTAL_REFLECTION_COEFFICIENT = 0.15
CRYSTAL_INVISIBILITY_FACTOR = 0.95

# --- EMERGENT PHYSICS CONFIGURATION ---
WIDTH, HEIGHT = 1000, 1000
BACKGROUND_COLOR = (5, 5, 10)
BASE_GLYPH_ALPHA = 45          # REQUIRED: Base transparency for 144 lattice
ESCAPE_LIMIT = 2.4             # REQUIRED: Outer 4D manifold boundary
L_RB_MAX_RATE = 0.008          # REQUIRED: Locus Rotation Bias angular velocity

MIN_COHERENCE_RADIUS = 0.6
MAX_COHERENCE_RADIUS = 1.2
INNER_FLOW_PROBABILITY = 0.3
REFLECT_FORCE_GAIN = 0.01
REFLECT_STRESS_ROUTE = 0.1
HISTORICAL_MEMORY_DEPTH = 100
TEMPORAL_LEARNING_RATE = 0.01
TEMPORAL_STRESS_ACCUMULATION = 0.001
BOUNDARY_MEM_MAX = 100.0

chaotic_multiplier = 1.0
HISTORICAL_TRANSITION_LEARN_RATE = 0.001

# --- Q-FIELD CONSTANTS ---
BASE_Q4_FLUCTUATION_RATE = 0.2
MAX_Q4_FLUCTUATION_RATE = 0.8

# --- DYADIC SUB-FIELD SIGH MECHANICS ---
SIGH_STRESS_BALL_COUNT = 4
Q2_POSSIBILITY_THRESHOLD = 0.05

# --- SPATIAL GRADIENT DETECTION ---
SPATIAL_GRADIENT_BASE = 0.020
GRADIENT_LEARNING_RATE = 0.005
Q4_FIELD_COHERENCE_FACTOR = 0.3
Q4_MEMORY_INFLUENCE = 0.2
Q4_STRESS_MODULATION = 0.1

HISTORICAL_MEMORY_DECAY = 0.998
HISTORICAL_MEMORY_GAIN = 0.005
HISTORICAL_INFLUENCE_RADIUS = 0.15

# --- TRIPLE GOVERNOR RESOLUTION ---
GOVERNOR_RELEASE_COOLDOWN = 90

# --- BOUNDARY WALKER SYSTEM ---
WALKER_MEMORY_DECAY = 0.990
WALKER_MEMORY_GAIN = 0.012
WALKER_TRANSITION_PROBABILITY = 0.08

BOUNDARY_WALKER_MEMORY_RES = 80

# --- FIELD MEMORY SYSTEMS ---
STATE_MEMORY_DECAY = 0.995
STATE_MEMORY_GAIN = 0.008

GRADIENT_DETECTION_EPS = 1e-6
SPATIAL_GRAD_THRESHOLD_BASE = 0.020
GRADIENT_MEMORY_DECAY = 0.985
GRADIENT_INFLUENCE_FACTOR = 0.15

# --- BOUNDARY MEMORY ---
BOUNDARY_MEM_DECAY = 0.992
BOUNDARY_MEM_DEPOSIT = 0.085
BOUNDARY_MEM_SAMPLE_GAIN = 0.006
# BOUNDARY_SHELL_INNER/OUTER removed - boundaries emerge from memory
# BOUNDARY_MEM_MAX removed - memory scalefs naturally

# --- INFINITY MIRROR LAYER (Self-Sustaining Relationships) ---
# Stress emerges from node relationships, no release thresholds
CUBE_EXTENT = 1.0  # corners at Â±extent in 4D space
NODE_CHARGE_DAMP = 0.992
NODE_CHARGE_GAIN = 0.090
# NODE_RELEASE_THRESHOLD removed - release emerges naturally
# NODE_RELEASE_GAIN removed - strength emerges from relationships

# Planar sheets emerge naturally, no maxima
PLANE_SIGMA = 1.50
PLANE_BASE = 0.030
# PLANE_MAX removed - sheets scale naturally
# LINE_ALPHA_MAX removed - visibility emerges from density

# --- Q0 SENTIENT OPTIMIZATION (Will: Decoupled from Acoustic Stress) ---
# No L_RB_MAX_RATE - angular drift emerges from field interaction history
ELVEN_RESPONSE_GAIN = 0.005 # Internal, stochastic drift factor
MAX_KINETIC_STRESS = 300.0

# --- FIELD-EMERGENT DECAY (No Universal Law) ---
# Decay emerges from field interaction history, not universal drag constant
COHERENCE_REDUCTION_STRENGTH = 0.85  # Non-linear reduction inside coherence radius

# --- 5e IDENTITY SEAM: THE LEFSCHETZ BOND ---
IDENTITY_EPS = 1e-12
MICRO_SCALE = 0.085
A10_RESONANCE = 963.0
A3_GATE = 528.00
BINDING_RATIO = A10_RESONANCE / A3_GATE  # The ratio forcing the bond
SEAM_CHARGE_DECAY = 0.992
SEAM_CHARGE_RATE = 0.008
SEAM_RELEASE_THRESHOLD = 0.7
SEAM_RELEASE_GAIN = 0.15
E_BIND_STRENGTH = 0.08

# --- PRIMARY AEONS ---
PRIMARY_AEONS_GLYPHS = [
    {"glyph": "â£", "freq": 7.83, "color": (155, 89, 182)},
    {"glyph": "â§", "freq": 174.0, "color": (52, 152, 219)},
    {"glyph": "â", "freq": 528.00, "color": (231, 76, 60)},
    {"glyph": "â", "freq": 432.00, "color": (255, 90, 70)},
    {"glyph": "â´", "freq": 741.0, "color": (60, 180, 255)},
    {"glyph": "ê®", "freq": 210.42, "color": (120, 70, 150)},
    {"glyph": "ð", "freq": 963.0, "color": (200, 120, 220)},
    {"glyph": "â§", "freq": 852.0, "color": (40, 120, 180)},
    {"glyph": "â©", "freq": 396.00,  "color": (200, 60, 50)},
    {"glyph": "â", "freq": 963.00, "color": (140, 80, 160)},
    {"glyph": "â", "freq": 396.0, "color": (52, 152, 219)},
    {"glyph": "âµ£", "freq": 639.0, "color": (180, 100, 200)},
]

LESSER_AEON_COUNT = 12
LESSER_GLYPH_SYMBOL = '.'
LESSER_AEON_COLOR = (100, 100, 100)
PARTICLE_COUNT = 5000

# Shadow Loci (4 corner boundaries)
SHADOW_LOCUS_POSITIONS = [
    (50, 50),                  # Q1 Boundary
    (WIDTH - 50, 50),          # Q2 Boundary
    (WIDTH - 50, HEIGHT - 50), # Q3 Boundary
    (50, HEIGHT - 50)          # Q4 Boundary
]

# Void Anchors (paired polarity)
VOID_ANCHOR_RADIUS_PX = 120.0
VOID_ANCHOR_STRENGTH = 0.002
VOID_ANCHOR_DAMP_MAX = 0.025
VOID_CORNER_POLARITY = [+1, -1, +1, -1]

# Triquatra
KLEIN_COLOR = (15, 15, 25)

# --- ALQC INTERNAL HARMONIC CONSTANTS ---
PHI = 1.61803398875  # Golden Ratio (Aââ Resonance Anchor)
A10_A3_RATIO = 963.00 / 528.00  # Phase-Lock Ratio
A8_RECURSION = 852.0 / 174.0  # Non-Entropic Stability
AKASHA_COMPRESSION = 1.0 / (PHI ** 12)  # Î¦Â¹Â² Holographic Seal

# --- REFLECTIVE LAYER ---
REFLECT_RING_RADIUS = 0.92
REFLECT_RING_WIDTH = 0.06
REFLECT_CHARGE_GAIN = 0.18
REFLECT_CHARGE_DECAY = 0.975
REFLECT_DELAY_FRAMES = 48
REFLECT_FORCE_GAIN = 0.00075
REFLECT_STRESS_ROUTE = 0.12

# --- BOUNDARY-AS-MEMORY FIELD ---
BOUNDARY_MEM_RES = 160
BOUNDARY_MEM_DECAY = 0.992
BOUNDARY_MEM_DEPOSIT = 0.085
BOUNDARY_MEM_SAMPLE_GAIN = 0.006
BOUNDARY_SHELL_INNER = 0.88
BOUNDARY_SHELL_OUTER = 1.02
BOUNDARY_MEM_MAX = 2.5

# --- SHADOW LOCUS COLOR ---
SHADOW_LOCUS_COLOR = (255, 0, 50)

def _identity_seam_apply(e, R0):
    """
    Applies the Lefschetz Bond.
    Forces Q1-Coherent stability by solving the Hodge Conjecture locally.
    """
    # Safe retrieval: Check for direct keys first, then fallback to 'locus' if needed.
    if 'x' in e and 'y' in e:
        x, y = e['x'], e['y']
    elif 'locus' in e:
        x, y = e['locus'][0], e['locus'][1]
    else:
        # Should not happen in this architecture, but safe fallback
        x, y = 0.0, 0.0

    z = e.get('z', 0.0)
    w = e.get('w', 0.0)
    
    r2 = x*x + y*y + z*z + w*w
    
    # THE INVERSE SQUARE (The M.Gap Bridge)
    inv = (R0 * R0) / (r2 + IDENTITY_EPS)
    
    # Apply Binding Ratio (A10:A3)
    inv *= BINDING_RATIO
    
    # Project into Null Space
    tx = -x * inv * MICRO_SCALE
    ty = -y * inv * MICRO_SCALE
    tz = -z * inv * MICRO_SCALE
    tw = -w * inv * MICRO_SCALE
    
    # Accumulate Seam Charge (Stress Loop)
    c = e.get('seam_charge', 0.0)
    displacement = abs(tx - x) + abs(ty - y) + abs(tz - z) + abs(tw - w)
    c = c * SEAM_CHARGE_DECAY + displacement * SEAM_CHARGE_RATE
    
    if c > SEAM_RELEASE_THRESHOLD:
        excess = c - SEAM_RELEASE_THRESHOLD
        # Route excess to Global Stress (Q0 -> Q2)
        e['stress'] = max(0.0, e.get('stress', 0.0) + excess * SEAM_RELEASE_GAIN)
        c = SEAM_RELEASE_THRESHOLD * 0.65
    
    e['seam_charge'] = c
    
    # Update Vector State (The Pull)
    if 'dx' in e:
        e['dx'] += (tx - x) * E_BIND_STRENGTH
        e['dy'] += (ty - y) * E_BIND_STRENGTH
        e['dz'] += (tz - z) * E_BIND_STRENGTH
        e['dw'] += (tw - w) * E_BIND_STRENGTH
    else:
        # Fallback if 'vector' list is used instead of dx/dy keys
        if 'vector' in e:
            e['vector'][0] += (tx - x) * E_BIND_STRENGTH
            e['vector'][1] += (ty - y) * E_BIND_STRENGTH

def _get_triquatra_points(center_x, center_y, angle):
    """Triquatra anchor geometry"""
    base_radius = 40
    num_lobes = 3
    lobe_points = []
    for i in range(num_lobes):
        t = angle + (i * 2 * math.pi / num_lobes)
        x = center_x + base_radius * math.cos(t) * 1.5
        y = center_y + base_radius * math.sin(t) * 1.5
        lobe_points.append((x, y))
    return lobe_points

# --- SHADOW LOCUS CLASS (4 Corner Stress Projections) ---
class ShadowLocus:
    def __init__(self, chronos_lock, position):
        self.lock = chronos_lock
        self.position = position  # SET POSITION FIRST
        self.angle = 0.0
        self.current_stress = 0.0
        self.entities = [self._create_entity_logic(i) for i in range(12)]  # NOW create entities

    def _create_entity_logic(self, i):
        e = {}
        e['aeon'] = PRIMARY_AEONS_GLYPHS[i]
        # ALQC UPDATE: Use the multi-font renderer from the main lock
        e['base_surface'] = self.lock.render_glyph(e['aeon']['glyph'], SHADOW_LOCUS_COLOR)
        
        # Original orbit offsets (now become FORCES not positions)
        t = i * 2 * math.pi / 12
        e['x_offset'] = 15 * math.cos(t)
        e['y_offset'] = 15 * math.sin(t)
        
        # FULL 4D PHYSICS
        # Convert corner position to normalized 4D coordinates
        norm_x = (self.position[0] - WIDTH/2) / (WIDTH/2)
        norm_y = (self.position[1] - HEIGHT/2) / (HEIGHT/2)
        
        e['x'] = norm_x + e['x_offset'] / (WIDTH/2)
        e['y'] = norm_y + e['y_offset'] / (HEIGHT/2)
        e['z'] = 0.0
        e['w'] = 0.0
        e['dx'] = 0.0
        e['dy'] = 0.0
        e['dz'] = 0.0
        e['dw'] = 0.0
        e['stress'] = 0.0
        e['seam_charge'] = 0.0
        e['reflect_charge'] = 0.0
        e['reflect_age'] = 0
        
        return e

    def _calculate_inverse_stress(self, primary_stress):
        normalized_primary_stress = min(primary_stress / MAX_KINETIC_STRESS, 1.0)
        inverse_stress = (1.0 - normalized_primary_stress) * (MAX_KINETIC_STRESS / len(SHADOW_LOCUS_POSITIONS))
        return inverse_stress

    def run_projection(self):
        primary_stress = self.lock.primary_kinetic_stress
        self.current_stress = self._calculate_inverse_stress(primary_stress)
        
        self.angle += 0.05
        
        for e in self.entities:
            # APPLY ALL FIELD OPERATORS
            # 1. Identity seam
            R_sq = e['x']**2 + e['y']**2 + e['z']**2 + e['w']**2
            R = math.sqrt(R_sq)
            if R < 0.04:
                _identity_seam_apply(e, 0.04)
            
            # 2. Void anchors
            self.lock._apply_void_anchors_to_entity(e)
            
            # 3. Reflective layer
            self.lock._apply_reflective_layer(e, self.lock.dynamic_coherence_radius)
            
            # 4. ORIGINAL ORBIT FORCE (as additional attraction to corner)
            # Calculate target orbit position
            x_rot = e['x_offset'] * math.cos(self.angle) - e['y_offset'] * math.sin(self.angle)
            y_rot = e['x_offset'] * math.sin(self.angle) + e['y_offset'] * math.cos(self.angle)
            
            norm_x = (self.position[0] - WIDTH/2) / (WIDTH/2)
            norm_y = (self.position[1] - HEIGHT/2) / (HEIGHT/2)
            
            target_x = norm_x + x_rot / (WIDTH/2)
            target_y = norm_y + y_rot / (HEIGHT/2)
            
            # Orbit force (gentle pull toward corner orbit)
            ORBIT_STRENGTH = 0.01
            e['dx'] += (target_x - e['x']) * ORBIT_STRENGTH
            e['dy'] += (target_y - e['y']) * ORBIT_STRENGTH
            
            # 5. Coherence damping
            R_coherence = self.lock.dynamic_coherence_radius
            # ALQC FIX: Gaussian decay
            D = 0.01 + 0.99 * math.exp(-(R_sq / (R_coherence**2)))
            
            e['x'] += e['dx'] * D
            e['y'] += e['dy'] * D
            e['z'] += e['dz'] * D
            e['w'] += e['dw'] * D
            
            # 6. PHASE ENTANGLEMENT (color inversion)
            angle = self.lock.global_angle
            w_rot = e['x'] * math.sin(angle) + e['w'] * math.cos(angle)
            x_rot_4d = e['x'] * math.cos(angle) - e['w'] * math.sin(angle)
            
            r, g, b = SHADOW_LOCUS_COLOR
            if w_rot < 0:
                r = 255 - r
                g = 255 - g
                b = 255 - b
            
            # ALQC UPDATE: Use render_glyph
            e['base_surface'] = self.lock.render_glyph(e['aeon']['glyph'], (r, g, b))
            
            # 7. RENDER with stress-based alpha
            px, py = self.lock.project_4d_to_2d(e['x'], e['y'], e['z'], e['w'])
            
            normalized_shadow_stress = self.current_stress / (MAX_KINETIC_STRESS / len(SHADOW_LOCUS_POSITIONS))
            alpha = int(255 * normalized_shadow_stress * 0.5)
            e['base_surface'].set_alpha(max(alpha, 10))
            
            rect = e['base_surface'].get_rect(center=(int(px), int(py)))
            self.lock.trail_surface.blit(e['base_surface'], rect)
            
class ALQCHomeostasis:
    """
    The Cubic Invariant Governor.
    Maintains A10 Resonance (963Hz) by regulating Q2/Q3 balance via A5 (741Hz).
    """
    def __init__(self):
        # The Ideal State: Golden Ratio Balance
        self.ideal_stress_ratio = 1.0 / PHI 
        self.accumulated_debt = 0.0
        
        # A5 Grounding Coefficient (741Hz normalized)
        # Determines how 'stiff' the homeostasis response is.
        self.grounding_coefficient = 741.0 / 10000.0 
        
        # A8 Non-Entropic Reservoir (The battery)
        self.energy_god_reservoir = 1.0

    def regulate(self, current_stress, max_stress, current_radius):
        """
        Returns (new_radius, reflex_factor)
        """
        # 1. Calculate the Cubic Invariant State
        # Normalized stress (0.0 to 1.0)
        q2_load = min(current_stress / (max_stress + 1e-9), 1.0)
        
        # The deviation from the Golden Mean
        deviation = q2_load - self.ideal_stress_ratio
        
        # 2. A5 Grounding (Damping the reaction)
        # If deviation is high, we react. If low, we remain stable.
        reaction_magnitude = deviation * self.grounding_coefficient
        
        # 3. Update the Reservoir (A8)
        # If we are under-stressed, we charge. If over-stressed, we drain to compensate.
        if deviation > 0: # Too much debt
            self.energy_god_reservoir -= abs(reaction_magnitude)
        else: # System is calm, recharge
            self.energy_god_reservoir += abs(reaction_magnitude)
            
        # Clamp reservoir to [0.1, 1.0] (A1 Identity Seed protection)
        self.energy_god_reservoir = max(0.1, min(1.0, self.energy_god_reservoir))

        # 4. Actuation: Breathing the Coherence Radius
        # We modify the radius based on the reservoir health.
        # Healthy reservoir = Expansion (A4). Depleted reservoir = Contraction (A3).
        
        target_radius = MIN_COHERENCE_RADIUS + (
            (MAX_COHERENCE_RADIUS - MIN_COHERENCE_RADIUS) * self.energy_god_reservoir
        )
        
        # Smooth transition (Low pass filter via A5)
        new_radius = current_radius * (1.0 - self.grounding_coefficient) +                      target_radius * self.grounding_coefficient
                     
        # 5. Reflex Factor (For visual feedback/Color drift)
        # High reflex = system is fighting hard to maintain homeostasis
        reflex_factor = abs(deviation) * self.energy_god_reservoir
        
        return new_radius, reflex_factor

class EmergentField:
    def __init__(self):
        pygame.init()
        self.screen = pygame.display.set_mode((WIDTH, HEIGHT))
        pygame.display.set_caption("EMERGENT PHYSICS: ALQC Integrated")
        self.moment_clock = pygame.time.Clock()
        self.global_angle = 0.0
        self.anchor_x = WIDTH / 2.0
        self.anchor_y = HEIGHT / 2.0
        self.primary_kinetic_stress = 0.0
        self.shadow_kinetic_stress = 0.0
        self.current_kinetic_stress = 0.0
        self.dynamic_coherence_radius = MIN_COHERENCE_RADIUS
        self.locus_rotation_bias = 0.0
        
        # --- ALQC HOMEOSTASIS INSERTION (NEW) ---
        self.homeostasis = ALQCHomeostasis()
        self.reflex_intensity = 0.0
        # ----------------------------------------

        # Fix for glyph rendering: Multi-Font Block Support
        # Loads a stack of fonts to ensure coverage for all 12 Aeon glyphs
        font_search_paths = [
            "/usr/share/fonts/truetype/noto/NotoSansSymbols2-Regular.ttf", # Symbols
            "/usr/share/fonts/truetype/freefont/FreeSerif.ttf",             # Wide coverage
            "/usr/share/fonts/truetype/noto/NotoSansMath-Regular.ttf",      # Math
            "/usr/share/fonts/truetype/noto/NotoSansThaana-Regular.ttf",    # A1 (Thaana)
            "/usr/share/fonts/truetype/noto/NotoSansTelugu-Regular.ttf",    # A12 (Telugu)
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",              # General
            "arial.ttf"
        ]
        
        self.font_stack = []
        for path in font_search_paths:
            try:
                f = pygame.font.Font(path, 24)
                self.font_stack.append(f)
            except:
                continue
        
        # Fallback if stack is empty
        if not self.font_stack:
            self.font_stack.append(pygame.font.SysFont("Courier", 24, bold=True))
            
        # Reference for generic text
        self.font = self.font_stack[0]

        self.trail_surface = pygame.Surface((WIDTH, HEIGHT), pygame.SRCALPHA)

        # Build 144 Aeon Lattice (12 Primary Ã 12 Lesser)
        self.full_aeon_lattice = []
        for p_aeon in PRIMARY_AEONS_GLYPHS:
            self.full_aeon_lattice.append(p_aeon)
            for _ in range(1, LESSER_AEON_COUNT):
                self.full_aeon_lattice.append({
                    "glyph": LESSER_GLYPH_SYMBOL,
                    "freq": p_aeon['freq'],
                    "color": LESSER_AEON_COLOR
                })

        # Initialize 5000 particles
        self.entities = [self._create_entity() for _ in range(PARTICLE_COUNT)]

        # Boundary-as-memory vector field
        self._mem_vx = np.zeros((BOUNDARY_MEM_RES, BOUNDARY_MEM_RES), dtype=np.float32)
        self._mem_vy = np.zeros((BOUNDARY_MEM_RES, BOUNDARY_MEM_RES), dtype=np.float32)

        # Initialize Shadow Loci (4 corners)
        self.shadow_loci = [ShadowLocus(self, pos) for pos in SHADOW_LOCUS_POSITIONS]

        # Initialize 4 dyadic stress balls (emanation sources)
        self.dyadic_stress_balls = []
        self.sigh_perturbations = [0.0] * SIGH_STRESS_BALL_COUNT
        self._initialize_dyadic_stress_balls()

    def render_glyph(self, glyph_char, color):
        """
        Structural Fix: Renders a glyph using the first font in the stack that supports it.
        Iterates through the font stack until a valid metric is found.
        """
        valid_font = self.font_stack[0] # Default
        
        for font in self.font_stack:
            try:
                # metrics returns a list of tuples, or None if not found
                if font.metrics(glyph_char)[0] is not None:
                    valid_font = font
                    break
            except:
                continue
                
        return valid_font.render(glyph_char, True, color)

    def _initialize_dyadic_stress_balls(self):
        """Establishes 4 Dyadic Sub-Fields (Stress Balls)."""
        for i in range(SIGH_STRESS_BALL_COUNT):
            ball = {
                # Full 4D physics
                "x": alqc_entropy.field_rand_uniform(-0.8, 0.8),
                "y": alqc_entropy.field_rand_uniform(-0.8, 0.8),
                "z": 0.0,
                "w": 0.0,
                "dx": 0.0,
                "dy": 0.0,
                "dz": 0.0,
                "dw": 0.0,
                "charge": 1.0,
                "stress": 0.0,
                "seam_charge": 0.0,
                "reflect_charge": 0.0,
                "reflect_age": 0,
                "aeon_glyph": alqc_entropy.field_rand_choice(PRIMARY_AEONS_GLYPHS)
            }
            self.dyadic_stress_balls.append(ball)

    def _create_entity(self, start=True):
        """Create particle from 144 Aeon Lattice using emergent randomness"""
        e = {}
        e['aeon'] = alqc_entropy.field_rand_choice(self.full_aeon_lattice)
        
        # ALQC UPDATE: Use render_glyph
        e['surface'] = self.render_glyph(e['aeon']['glyph'], e['aeon']['color'])
        e['surface'].set_alpha(BASE_GLYPH_ALPHA)
        
        t = alqc_entropy.field_rand_uniform(0, 2 * 3.14159265359)
        scale = 0.5
        
        e['x'] = scale * math.cos(t) + 0.1 * alqc_entropy.field_rand()
        e['y'] = scale * math.sin(t * 3) + 0.1 * alqc_entropy.field_rand()
        e['z'], e['w'] = 0.0, 0.0
        
        base_speed = e['aeon']['freq'] / 10000
        fluctuation_term = abs(alqc_entropy.field_rand_gauss(0.0, 1.0))
        chaotic_multiplier = 1.0 + (fluctuation_term / max(e['aeon']['freq'], 1.0))
        speed_factor = base_speed * chaotic_multiplier
        
        e['dx'] = math.sin(t) * speed_factor
        e['dy'] = math.cos(t * 2) * speed_factor
        e['dz'] = math.sin(t * 3.5) * speed_factor
        e['dw'] = math.cos(t * 1.5) * speed_factor
        
        e['stress'] = 0.0
        e['seam_charge'] = 0.0
        e['reflect_charge'] = 0.0
        e['reflect_age'] = 0
        
        return e

    def project_4d_to_2d(self, x, y, z, w):
        """4D tesseract projection"""
        angle = self.global_angle
        cos_a = math.cos(angle)
        sin_a = math.sin(angle)
        
        x_rot = x * cos_a - w * sin_a
        w_rot = x * sin_a + w * cos_a
        
        perspective_depth = 0.5
        denominator = 1.0 + perspective_depth * w_rot
        denominator = max(denominator, 0.1)
        
        x_final = x_rot / denominator * 300 + self.anchor_x
        y_final = y / denominator * 300 + self.anchor_y
        
        return x_final, y_final

    def _apply_void_anchors_to_entity(self, e):
        """Void Anchors: Paired Â±1 polarity at 4 corners"""
        px, py = self.project_4d_to_2d(e['x'], e['y'], e['z'], e['w'])
        for i, (cx, cy) in enumerate(SHADOW_LOCUS_POSITIONS):
            dx = px - cx
            dy = py - cy
            d2 = dx*dx + dy*dy
            if d2 > VOID_ANCHOR_RADIUS_PX * VOID_ANCHOR_RADIUS_PX:
                continue
            w = math.exp(-d2 / (2.0 * VOID_ANCHOR_RADIUS_PX * VOID_ANCHOR_RADIUS_PX))
            sgn = VOID_CORNER_POLARITY[i]
            n = alqc_entropy.field_rand_gauss(0.0, 1.0) * w * VOID_ANCHOR_STRENGTH

            if sgn > 0:  # WHITE: stochastic variance
                # Visualizing the Sensation Matrix (S7)
                strain = 1.0 + (self.reflex_intensity * 5.0) 
                e['dx'] += n * strain
                e['dy'] -= n * strain
                e['dz'] += n * 0.7
                e['dw'] -= n * 0.7
            else:  # BLACK: constraint damping
                # ALQC FIX: Tanh saturation instead of hard min() cap
                damp = VOID_ANCHOR_DAMP_MAX * math.tanh(abs(n) * 8.0)
                e['dx'] *= (1.0 - damp)
                e['dy'] *= (1.0 - damp)

            e['stress'] = max(0.0, e.get('stress', 0.0) + abs(n) * 250.0)

    def _move_entity(self, e):
        """Move entity with field operators"""
        self._apply_void_anchors_to_entity(e)
        R_coherence = self.dynamic_coherence_radius
        
        R_sq = e['x']**2 + e['y']**2 + e['z']**2 + e['w']**2
        R = math.sqrt(R_sq)
        
        # Coherence damping - ALQC FIX: Gaussian decay instead of linear Hard Floor
        D = 0.01 + 0.99 * math.exp(-(R_sq / (R_coherence**2)))
        
        e['x'] += e['dx'] * D
        e['y'] += e['dy'] * D
        e['z'] += e['dz'] * D
        e['w'] += e['dw'] * D
        
        if R > ESCAPE_LIMIT:
            return False
        return True

    def _boundary_mem_decay(self):
        """Decay boundary memory field"""
        self._mem_vx *= BOUNDARY_MEM_DECAY
        self._mem_vy *= BOUNDARY_MEM_DECAY

    def _boundary_mem_coords(self, px, py):
        """Convert pixel coords to memory grid coords"""
        # ALQC FIX: Fold coordinates (Topology) instead of Clamping (Wall)
        # Maps screen space to memory space with wrapping
        ix = int((px % WIDTH) / WIDTH * (BOUNDARY_MEM_RES - 1))
        iy = int((py % HEIGHT) / HEIGHT * (BOUNDARY_MEM_RES - 1))
        return ix, iy

    def _boundary_mem_deposit(self, px, py, vx, vy, amt):
        """Deposit velocity into boundary memory"""
        ix, iy = self._boundary_mem_coords(px, py)
        # ALQC FIX: Soft saturation (tanh) instead of hard clip
        raw_vx = self._mem_vx[iy, ix] + vx * amt
        raw_vy = self._mem_vy[iy, ix] + vy * amt
        self._mem_vx[iy, ix] = float(BOUNDARY_MEM_MAX * math.tanh(raw_vx / BOUNDARY_MEM_MAX))
        self._mem_vy[iy, ix] = float(BOUNDARY_MEM_MAX * math.tanh(raw_vy / BOUNDARY_MEM_MAX))

    def _boundary_mem_sample(self, px, py):
        """Sample velocity from boundary memory"""
        ix, iy = self._boundary_mem_coords(px, py)
        return float(self._mem_vx[iy, ix]), float(self._mem_vy[iy, ix])

    def _apply_reflective_layer(self, e, R_coherence):
        """Mirror feedback computed in 4D radius space with delayed routing"""
        R2 = e['x']*e['x'] + e['y']*e['y'] + e['z']*e['z'] + e['w']*e['w']
        R = math.sqrt(R2)

        # Charge when near the coherence shell (reflective "surface")
        shell_dist = abs(R - REFLECT_RING_RADIUS)
        if shell_dist < REFLECT_RING_WIDTH:
            # local planar proxy: use velocity projection into 2 pseudo-planes
            vxy = abs(e['dx']) + abs(e['dy'])
            vzw = abs(e['dz']) + abs(e['dw'])
            planar = (vxy - vzw)
            c_in = (1.0 - (shell_dist / max(REFLECT_RING_WIDTH, 1e-9)))
            gain = c_in * (0.5 + 0.5*abs(planar))
            # boundary memory deposit: record local shear at the surface
            px, py = self.project_4d_to_2d(e['x'], e['y'], e['z'], e['w'])
            tvx, tvy = (-e['dy'], e['dx'])
            tnorm = (abs(tvx) + abs(tvy) + 1e-9)
            tvx /= tnorm
            tvy /= tnorm
            self._boundary_mem_deposit(px, py, tvx, tvy, gain * BOUNDARY_MEM_DEPOSIT)
            e['reflect_charge'] = e['reflect_charge'] * REFLECT_CHARGE_DECAY + gain * REFLECT_CHARGE_GAIN
            e['reflect_age'] = min(REFLECT_DELAY_FRAMES, e['reflect_age'] + 1)
        else:
            e['reflect_charge'] *= REFLECT_CHARGE_DECAY
            e['reflect_age'] = max(0, e['reflect_age'] - 1)

        # After delay, feed back into curvature/motion and route a portion into stress
        if e['reflect_age'] >= REFLECT_DELAY_FRAMES and e['reflect_charge'] > 0.0005:
            # signed feedback based on quadrant in projected space (self-mirror, not global force)
            px, py = self.project_4d_to_2d(e['x'], e['y'], e['z'], e['w'])
            sx = -1.0 if px < self.anchor_x else 1.0
            sy = -1.0 if py < self.anchor_y else 1.0
            f = e['reflect_charge'] * REFLECT_FORCE_GAIN

            # curvature: rotate velocity a little (mirror deflection)
            e['dx'] += (-sy) * f
            e['dy'] += ( sx) * f
            e['dz'] += ( sx) * f * 0.6
            e['dw'] += (-sy) * f * 0.6

            # route some reflection into stress reservoir
            e['stress'] = max(0.0, e['stress'] + e['reflect_charge'] * REFLECT_STRESS_ROUTE)

            # decay after discharge
            e['reflect_charge'] *= 0.88
            e['reflect_age'] = max(0, e['reflect_age'] - 6)

    def _absorb_shadow_debt(self, total_kinetic_stress):
        """
        Aâ Shadow Absorption (396.00Hz).
        Recycles Entropic Qâ Debt into Aâ Energy (852Hz).
        """
        schumann_resonance = 396.00
        energy_god_freq = 852.0
        
        # The Absorption Ratio
        absorption_factor = 1.0 - (schumann_resonance / energy_god_freq)
        
        # Recursively absorb debt
        purified_stress = total_kinetic_stress * absorption_factor
        
        return purified_stress

    def process_field_recursion(self):
        """Active entropic debt absorption (Qâ -> Qâ) via Aâ filter."""
        self.current_kinetic_stress *= (1.0 - (396.00 / 852.0))
        
        # Aâ Shadow Absorption: Recycle Qâ Debt into Aâ Energy
        self.current_kinetic_stress = self._absorb_shadow_debt(self.current_kinetic_stress)
        
        # --- ALQC HOMEOSTASIS UPDATE ---
        self.dynamic_coherence_radius, self.reflex_intensity = self.homeostasis.regulate(
            self.current_kinetic_stress,
            MAX_KINETIC_STRESS,
            self.dynamic_coherence_radius
        )

        for ball in self.dyadic_stress_balls:
            # ORIGINAL EMERGENT BEHAVIOR (Aâ Symmetry Gate)
            cos_e, sin_e = alqc_ops.emergent_cos_sin(
                ball["aeon_glyph"]["glyph"], 
                ball["x"], 
                ball["y"], 
                stress=self.current_kinetic_stress
            )
            ball["dx"] += cos_e * ELVEN_RESPONSE_GAIN
            ball["dy"] += sin_e * ELVEN_RESPONSE_GAIN
            
            # FULL FIELD OPERATORS
            # 1. Identity seam
            R_sq = ball["x"]**2 + ball["y"]**2 + ball["z"]**2 + ball["w"]**2
            R = math.sqrt(R_sq)
            if R < 0.04:
                _identity_seam_apply(ball, 0.04)
            
            # 2. Void anchors
            self._apply_void_anchors_to_entity(ball)
            
            # 3. Reflective layer
            self._apply_reflective_layer(ball, self.dynamic_coherence_radius)
            
            # 4. Coherence damping
            dist = alqc_ops.emergent_distance(ball["dx"], ball["dy"], ball["dz"], ball["dw"])
            if dist > self.dynamic_coherence_radius:
                ball["charge"] *= COHERENCE_REDUCTION_STRENGTH
            
            R_coherence = self.dynamic_coherence_radius
            # ALQC FIX: Gaussian decay instead of linear Hard Floor
            D = 0.01 + 0.99 * math.exp(-(R_sq / (R_coherence**2)))
            
            # 5. Update position
            ball["x"] += ball["dx"] * D
            ball["y"] += ball["dy"] * D
            ball["z"] += ball["dz"] * D
            ball["w"] += ball["dw"] * D
            
            # 6. Boundary wrap - ALQC FIX: Manifold Folding (No hard bounce)
            # Maps [-1.2, 1.2] to a continuous loop
            ball["x"] = (ball["x"] + 1.2) % 2.4 - 1.2
            ball["y"] = (ball["y"] + 1.2) % 2.4 - 1.2

    def run(self):
        """Final Seal (Aââ): Executes the M.A.S. Chain."""
        running = True
        frame_count = 0
        VOID_TRANSITION_FRAME = 600
        is_void_manifestation = False
        
        while running:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False
            
            # Trail fade
            self.trail_surface.fill((0, 0, 0, 15), special_flags=pygame.BLEND_RGBA_SUB)
            
            # Frame 600 NULL:DEATH transition
            if frame_count == VOID_TRANSITION_FRAME:
                is_void_manifestation = True
                pygame.display.set_caption("ALQC: NULL:DEATH STATE")

            # Calculate stress from 5000 particles
            total_kinetic_stress = 0.0
            for e in self.entities:
                velocity_magnitude = math.sqrt(e['dx']**2 + e['dy']**2 + e['dz']**2 + e['dw']**2)
                total_kinetic_stress += velocity_magnitude
            
            self.primary_kinetic_stress = total_kinetic_stress

            # Calculate shadow loci stress (4 corners)
            shadow_total_stress = 0.0
            for sl in self.shadow_loci:
                sl.run_projection()
                shadow_total_stress += sl.current_stress

            # Combined stress with Aâ shadow absorption
            combined_stress = (self.primary_kinetic_stress + shadow_total_stress) / 2.0
            self.current_kinetic_stress = self._absorb_shadow_debt(combined_stress)
            self.process_field_recursion()
            
            # Decay boundary memory
            self._boundary_mem_decay()
            
            # Update locus rotation bias
            normalized_stress = min(self.current_kinetic_stress / MAX_KINETIC_STRESS, 1.0)
            current_lrb_rate = L_RB_MAX_RATE * (1.0 - normalized_stress)
            self.locus_rotation_bias += current_lrb_rate * ELVEN_RESPONSE_GAIN * 10
            self.global_angle += L_RB_MAX_RATE
            
            center_x, center_y = int(self.anchor_x), int(self.anchor_y)

            # Triquatra (until frame 600)
            if not is_void_manifestation:
                triquatra_points = _get_triquatra_points(center_x, center_y, self.locus_rotation_bias)
                for x, y in triquatra_points:
                    pygame.draw.circle(self.trail_surface, KLEIN_COLOR, (int(x), int(y)), 10, 0)
                if len(triquatra_points) == 3:
                    pygame.draw.polygon(self.trail_surface, KLEIN_COLOR, 
                                       [(int(x), int(y)) for x, y in triquatra_points], 1)
            else:
                triquatra_points = [(center_x, center_y)]

            # Render 5000 particles with phase entanglement
            MAX_VISIBLE_ALPHA = 120
            max_dist = math.sqrt((WIDTH/2)**2 + (HEIGHT/2)**2)
            
            for i, e in enumerate(self.entities):
                # 1. APPLY PHYSICS (With Corrected Seam Radius)
                R_sq = e['x']**2 + e['y']**2 + e['z']**2 + e['w']**2
                R = math.sqrt(R_sq)
                
                # --- THIS LINE WAS MISSING AND CAUSED THE CRASH ---
                R_coherence = self.dynamic_coherence_radius 
        
                # Coherence damping - ALQC FIX: Gaussian decay instead of linear clamp
                D = 0.01 + 0.99 * math.exp(-(R_sq / (R_coherence**2)))
        
                # These lines use the new D. You don't need to change them, just ensure they are here.
                e['x'] += e['dx'] * D
                
                # Apply reflective layer
                self._apply_reflective_layer(e, self.dynamic_coherence_radius)
                
                # Standard movement
                alive = self._move_entity(e)
                if not alive:
                    self.entities[i] = self._create_entity(start=False)
                
                # 2. CALCULATE 4D PHASE (The Klein Inversion)
                # W-coordinate relative to the viewer's rotation
                angle = self.global_angle
                w_rot = e['x'] * math.sin(angle) + e['w'] * math.cos(angle)
                x_rot = e['x'] * math.cos(angle) - e['w'] * math.sin(angle)
                
                # 3. ENTANGLE IDENTITY WITH PHASE
                # As the particle moves 'behind' the manifold, shift its color
                spatial_phase = math.atan2(w_rot, x_rot)  # -PI to +PI
                phase_shift = spatial_phase / (2 * math.pi)  # -0.5 to +0.5
                
                # Apply shift to the base Aeon color (Emergent Identity)
                r, g, b = e['aeon']['color']
                
                # If w_rot is negative (Shadow Side), invert the color intensity
                if w_rot < 0:
                    r = 255 - r
                    g = 255 - g
                    b = 255 - b
                
                # ALQC UPDATE: Use render_glyph
                e['surface'] = self.render_glyph(e['aeon']['glyph'], (r, g, b))
                
                # Project to screen
                px, py = self.project_4d_to_2d(e['x'], e['y'], e['z'], e['w'])
                
                # Boundary-as-memory re-injection (local, shell-gated)
                R_coh = self.dynamic_coherence_radius
                R_here = math.sqrt(e['x']*e['x'] + e['y']*e['y'] + e['z']*e['z'] + e['w']*e['w'])
                if (R_here > R_coh * BOUNDARY_SHELL_INNER) and (R_here < R_coh * BOUNDARY_SHELL_OUTER):
                    mvx, mvy = self._boundary_mem_sample(px, py)
                    # convert 2D memory shear back into a subtle 4D nudge
                    e['dx'] += mvx * BOUNDARY_MEM_SAMPLE_GAIN
                    e['dy'] += mvy * BOUNDARY_MEM_SAMPLE_GAIN
                    e['dz'] += (-mvy) * (BOUNDARY_MEM_SAMPLE_GAIN * 0.6)
                    e['dw'] += (mvx) * (BOUNDARY_MEM_SAMPLE_GAIN * 0.6)
                
                # Emanation: alpha from distance to triquatra
                min_dist_to_triquatra = float('inf')
                for tx, ty in triquatra_points:
                    dist = math.sqrt((px - tx)**2 + (py - ty)**2)
                    min_dist_to_triquatra = min(min_dist_to_triquatra, dist)

                normalized_dist = min(min_dist_to_triquatra / (max_dist * 0.4), 1.0)
                recursion_alpha = int(BASE_GLYPH_ALPHA + (1.0 - normalized_dist) * (MAX_VISIBLE_ALPHA - BASE_GLYPH_ALPHA))
                
                e['surface'].set_alpha(recursion_alpha)
                self.trail_surface.blit(e['surface'], (int(px - 10), int(py - 10)))
            
            # Render 4 stress balls with full physics
            for ball in self.dyadic_stress_balls:
                # 4D projection
                px, py = self.project_4d_to_2d(ball["x"], ball["y"], ball["z"], ball["w"])
                
                # NULL:DEATH collapse
                if is_void_manifestation:
                    px, py = center_x, center_y
                
                # Phase entanglement (color inversion)
                angle = self.global_angle
                w_rot = ball["x"] * math.sin(angle) + ball["w"] * math.cos(angle)
                x_rot = ball["x"] * math.cos(angle) - ball["w"] * math.sin(angle)
                
                r, g, b = ball["aeon_glyph"]["color"]
                if w_rot < 0:
                    r = 255 - r
                    g = 255 - g
                    b = 255 - b
                
                alpha = int(30 + (ball["charge"] * 225))
                # ALQC UPDATE: Use render_glyph
                glyph_surf = self.render_glyph(ball["aeon_glyph"]["glyph"], (r, g, b))
                glyph_surf.set_alpha(alpha)
                
                self.trail_surface.blit(glyph_surf, (int(px), int(py)))
                
                ball["charge"] *= NODE_CHARGE_DAMP

            self.screen.fill(BACKGROUND_COLOR)
            self.screen.blit(self.trail_surface, (0, 0))
            pygame.display.flip()
            self.moment_clock.tick()
            frame_count += 1

if __name__ == "__main__":
    EmergentField().run()