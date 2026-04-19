# 🌌 Black Hole Space Game

A hyper-realistic space simulation built in **Unreal Engine 5.3** where you pilot a
spacecraft toward a **10 solar-mass Kerr black hole**, experiencing every physically
accurate relativistic effect along the way.

---

## Table of Contents
1. [Game Description](#game-description)
2. [System Requirements](#system-requirements)
3. [Setup Instructions](#setup-instructions)
4. [Physics Reference](#physics-reference)
5. [Controls](#controls)
6. [Project Architecture](#project-architecture)
7. [GPU Compatibility & Quality Presets](#gpu-compatibility--quality-presets)
8. [Known Limitations & Future Work](#known-limitations--future-work)

---

## Game Description

You start **1 million kilometres** from a supermassive rotating black hole.
Your ship has unlimited fuel. There is no speed limit.

As you approach:

- Stars **warp and bunch forward** as you accelerate (relativistic aberration)
- The **accretion disk** blazes in blue-white at its inner edge and deep red at its
  outer edge, with the approaching side blazing brighter than the receding side
  (relativistic Doppler beaming)
- **Gravitational lensing** bends light around the black hole into an **Einstein ring**
- Your ship **stretches** along the radial axis as tidal forces increase
  (*spaghettification*)
- Your **onboard clock slows** relative to the outside universe (time dilation)
- The HUD shows both your **ship time** and **universal time** diverging
- Cross the **photon sphere** and a warning fires: *POINT OF NO RETURN*
- Cross the **event horizon** — and escape becomes physically impossible

---

## System Requirements

### Minimum
| Component | Requirement |
|---|---|
| GPU | Any DX12-capable GPU with 8 GB VRAM |
| CPU | 8-core, 3.5 GHz (Intel i7-9700K / AMD Ryzen 7 3700X) |
| RAM | 16 GB |
| Storage | 10 GB SSD |
| OS | Windows 10 (64-bit) / Ubuntu 22.04 LTS |
| API | DirectX 12 or Vulkan 1.3 |
| UE5 | 5.3 or newer |

### Recommended (Full Quality)
| Component | Requirement |
|---|---|
| GPU | See GPU table below |
| CPU | 12-core, 4.0 GHz+ |
| RAM | 32 GB DDR5 |
| VRAM | 16 GB |
| Storage | NVMe SSD |

### GPU Tiers & Feature Support

| GPU | Tier | Ray Tracing | Lumen Mode | Quality Preset |
|---|---|---|---|---|
| NVIDIA RTX 4090 / 4080 | Cinematic | Hardware RT | HW Lumen | Cinematic |
| NVIDIA RTX 3080 / 3090 | Cinematic | Hardware RT | HW Lumen | Cinematic |
| NVIDIA RTX 2080 / 2080 Ti | Cinematic | Hardware RT | HW Lumen | Cinematic |
| AMD RX 7900 XTX / 7900 XT | High | None¹ | SW Lumen | High |
| AMD RX 6900 XT / 6800 XT | High | None¹ | SW Lumen | High |
| **AMD RX 6700 XT** *(recommended AMD)* | High | None¹ | SW Lumen | High |
| AMD RX 5700 XT | Medium | None | Screen Space | Medium |
| AMD RX 5700 | Medium | None | Screen Space | Medium |
| NVIDIA GTX 1080 Ti / 2060 (non-RT) | Medium | None | Screen Space | Medium |
| Intel Arc A770 / A750 | High | None¹ | SW Lumen | High |
| AMD Vega 64 / RX 580 (DX12) | Low | None | SSAO | Low |

> ¹ AMD hardware RT in UE5 is not yet stable; the game uses **Software Lumen RT**
> which delivers equivalent visual quality for space environments.

**The gravitational lensing shader, accretion disk shader, and star field shader
are all pure standard HLSL with no vendor-specific code.** They run identically
on NVIDIA, AMD, and Intel hardware.

---

## Setup Instructions

### Prerequisites
- [Unreal Engine 5.3](https://www.unrealengine.com) installed via Epic Games Launcher
- Visual Studio 2022 (Windows) or Clang 14+ (Linux) with UE5 workload
- Git

### Windows

```powershell
# 1. Clone the repository
git clone https://github.com/JoppeJoppinger/blackhole-game.git
cd blackhole-game

# 2. Generate project files
"C:\Program Files\Epic Games\UE_5.3\Engine\Build\BatchFiles\GenerateProjectFiles.bat" ^
    BlackHoleGame.uproject -Game

# 3. Build (Development Editor configuration)
"C:\Program Files\Epic Games\UE_5.3\Engine\Build\BatchFiles\Build.bat" ^
    BlackHoleGameEditor Win64 Development BlackHoleGame.uproject -Waitmutex

# 4. Launch the editor
"C:\Program Files\Epic Games\UE_5.3\Engine\Binaries\Win64\UnrealEditor.exe" ^
    "%CD%\BlackHoleGame.uproject"
```

### Linux

```bash
# 1. Clone
git clone https://github.com/JoppeJoppinger/blackhole-game.git
cd blackhole-game

# 2. Generate project files
~/UnrealEngine/Engine/Build/BatchFiles/Linux/GenerateProjectFiles.sh \
    BlackHoleGame.uproject -Game

# 3. Build
~/UnrealEngine/Engine/Build/BatchFiles/Linux/Build.sh \
    BlackHoleGameEditor Linux Development BlackHoleGame.uproject

# 4. Launch
~/UnrealEngine/Engine/Binaries/Linux/UnrealEditor BlackHoleGame.uproject
```

> See [SETUP.md](./SETUP.md) for detailed AMD driver notes, Vulkan configuration,
> and automated setup scripts.

---

## Physics Reference

### The Black Hole

This game simulates a **Kerr black hole** (rotating) with:
- **Mass**: 10 M☉ = 1.989 × 10³¹ kg
- **Schwarzschild radius**: *rs* = 2GM/c² ≈ **29,540 metres** (~29.5 km)
- **Spin parameter**: a = 0.7 (70% of maximum possible spin)
- **ISCO** (Innermost Stable Circular Orbit): ~3.2 × rs ≈ 94 km
- **Photon sphere**: 1.5 × rs ≈ 44 km

---

### Schwarzschild Radius

The "surface" of a non-rotating black hole — the event horizon.

```
rs = 2GM/c²
```
- G = 6.674 × 10⁻¹¹ N·m²/kg²
- M = mass of black hole
- c = 299,792,458 m/s

For 10 solar masses: rs ≈ 29,540 m (≈ 30 km)

The Schwarzschild radius scales **linearly** with mass. A black hole of 1 billion solar
masses (like M87*) would have rs ≈ 2.95 × 10¹² m ≈ 20 AU.

---

### Gravitational Time Dilation

From the Schwarzschild metric, time passes slower near massive objects:

```
dt_proper = dt_coordinate × √(1 - rs/r)
```

- At r = 2rs (twice the horizon): clocks run at 70.7% of far-away rate
- At r = 1.1rs: clocks run at 30.2% of far-away rate
- At r = rs: time stops completely

The game also includes **special relativistic** (kinematic) dilation from velocity:

```
dt_proper = dt_coord × √(1 - rs/r) × √(1 - v²/c²)
```

Both effects are tracked independently on the HUD.

---

### Tidal Forces (Spaghettification)

The difference in gravity between the near and far sides of your ship:

```
a_tidal = 2GM × L / r³
```

Where L is ship length (50 m default). The tidal acceleration grows as **r⁻³**,
becoming catastrophically strong near the horizon.

For a 10 M☉ black hole, spaghettification becomes fatal at approximately **r = 10 × rs**.

> Stellar-mass black holes kill you *before* you cross the horizon.
> Supermassive black holes (>10⁸ M☉) have weak enough tidal forces that you
> could cross the horizon alive — the game uses artistic licence here.

---

### Photon Sphere

At **r = 1.5 × rs**, photons orbit the black hole in unstable circular orbits.
This creates a glowing ring visible from outside — light that has orbited multiple
times. Crossing the photon sphere is "the point of no return" in the visual sense;
escape is still technically possible, but the lensing effects become extreme.

---

### Frame Dragging (Kerr Effect)

A rotating black hole drags spacetime itself into rotation — the **Lense-Thirring effect**.

The angular velocity of frame dragging:

```
Ω_LT = GJ / (c² r³) × (3(Ĵ·r̂)r̂ - Ĵ)
```

Where J = angular momentum of the black hole = a × M × c × (rs/2)

This manifests in-game as a sideways force that grows dramatically as you approach,
nudging your ship into a spiral even without engine thrust.

---

### Escape Velocity

```
v_escape = √(2GM/r)
```

At the event horizon, v_escape = c. This is why nothing can escape — you'd need
to travel faster than light.

The HUD shows current escape velocity and a green/red indicator for whether your
current speed exceeds it.

---

### Gravitational Lensing

The deflection angle of a photon passing at impact parameter b:

```
α ≈ 2rs/b  (weak field)
```

For small b (near the photon sphere), the full strong-field formula is used
(Keeton & Petri 2006 approximation). The post-process shader distorts screen UVs
to simulate this bending in real-time.

**Einstein Ring**: When the ship is perfectly aligned with the black hole and a
background light source, the source appears as a complete ring at a specific angular
radius — the "Einstein ring radius."

---

### Gravitational Redshift

Light climbing out of a gravitational well loses energy:

```
z = 1/√(1 - rs/r) - 1
```

- At r = 10rs: z ≈ 0.054 (5.4% wavelength shift — just visible)
- At r = 2rs:  z ≈ 0.414 (41.4% — dramatic reddening)
- At r = 1.1rs: z ≈ 2.08 (208% — deep infrared)

The shader applies this as a colour tint to lensed background objects.

---

### Accretion Disk Temperature

The Shakura-Sunyaev thin disk model gives temperature as a function of radius:

```
T(r) ∝ r^(-3/4)
```

- Inner edge (ISCO, ~94 km): **~10 million K** → blue-white
- Middle ring (~200 km): **~100,000 K** → white-yellow
- Outer edge (~600 km): **~3,000 K** → deep orange-red

The Doppler effect brightens the approaching side by a factor of D⁴ (relativistic beaming).

---

## Controls

| Key | Action |
|---|---|
| **W** | Thrust forward |
| **S** | Thrust backward |
| **A** | Strafe left |
| **D** | Strafe right |
| **Space** | Thrust up / Emergency brake |
| **Left Ctrl** | Thrust down |
| **Q** | Roll left |
| **E** | Roll right |
| **Mouse** | Pitch / Yaw look |
| **Left Shift** | Boost (5× thrust) |
| **V** | Toggle camera (cockpit ↔ chase) |
| **H** | Toggle HUD visibility |
| **Esc** | Pause / Menu |

---

## Project Architecture

```
blackhole-game/
├── Source/BlackHoleGame/
│   ├── BlackHoleActor          — Black hole physics & spatial queries
│   ├── BlackHolePhysicsComponent — Gravity integration, orbital mechanics, spaghettification
│   ├── TidalForceComponent     — Mesh deformation from tidal stress
│   ├── TimeDilationComponent   — Proper time vs coordinate time tracking
│   ├── SpaceshipPawn           — 6DOF player ship, camera, input
│   ├── SpaceEnvironmentActor   — Procedural star field, aberration
│   ├── BlackHoleGameMode       — Game state machine
│   ├── BlackHoleHUD            — Full physics HUD (Canvas-based)
│   └── GraphicsQualityManager  — Auto GPU detection, preset application
├── Shaders/
│   ├── GravitationalLens.usf   — Post-process lensing (standard HLSL)
│   ├── AccretionDisk.usf       — Disk material shader (standard HLSL)
│   └── StarField.usf           — Procedural skybox (standard HLSL)
├── Config/
│   ├── DefaultEngine.ini       — Lumen SW-RT default, DX12+DX11+Vulkan
│   ├── DefaultGame.ini         — Game mode / map assignments
│   ├── DefaultInput.ini        — All input bindings
│   └── DefaultEditorPerProjectUserSettings.ini
└── Content/                    — (Populated in UE5 Editor)
    ├── Materials/              — See *_README.md files
    ├── Blueprints/             — See BP_Setup_README.md
    ├── Particles/              — See Niagara_README.md
    └── Maps/                   — See Maps_README.md
```

---

## GPU Compatibility & Quality Presets

The `UGraphicsQualityManager` component auto-detects GPU vendor via `GRHIAdapterName`
and applies the appropriate preset:

| Preset | Conditions | Ray Tracing | Lumen | Shadows | AA |
|---|---|---|---|---|---|
| **Cinematic** | NVIDIA RTX + HW-RT supported | Hardware RT | HW Lumen | VSM | TSR |
| **High** | AMD RDNA2+, Intel Arc | Disabled | SW Lumen | VSM | TSR |
| **Medium** | AMD RDNA1, NVIDIA GTX | Disabled | Screen Space | CSM | TAA |
| **Low** | Legacy DX12 | Disabled | Disabled (SSAO) | CSM | TAA |

To override: set `bUseForcePreset = true` on `BP_BlackHole`'s `GraphicsQualityManager`
component and choose `ForcedPreset`.

All shaders (`GravitationalLens.usf`, `AccretionDisk.usf`, `StarField.usf`) are
**100% standard HLSL** — no `NvAPI`, no `AMD AGS`, no vendor intrinsics.
They compile to DXBC (DX11), DXIL (DX12), and SPIR-V (Vulkan) via the DXC compiler
included with UE5.

---

## Known Limitations & Future Work

### Current Limitations

1. **Geodesic integration**: Gravity uses the Newtonian inverse-square law rather
   than full geodesic integration in the Kerr metric. For visual purposes this is
   nearly indistinguishable far from the horizon. A Runge-Kutta Kerr geodesic
   integrator is planned.

2. **No speed cap**: The game intentionally removes the speed of light limit for
   gameplay reasons. Velocities above ~0.99c will produce extreme time dilation
   but the kinematic term is clamped to avoid NaN.

3. **Spaghettification mesh deformation**: Currently uses uniform scale along the
   radial axis. A proper vertex deformation shader would give more accurate results.

4. **Lensing is post-process only**: The gravitational lensing is a 2D screen-space
   UV distortion. It does not correctly handle objects occluded by the lens or
   multiple-orbit photon paths (beyond the secondary image approximation).

5. **Accretion disk is a static mesh**: The disk geometry does not dynamically
   deform. A Niagara GPU simulation driving geometry would be more accurate.

6. **No multiplayer**.

### Planned Features

- [ ] Full Kerr metric geodesic integration via RK4 solver
- [ ] GPU particle accretion disk replacing the static mesh
- [ ] Sound design: gravitational wave audio (chirp/rumble from Lorentz factor)
- [ ] Hawking temperature display (extremely subtle for 10 M☉, shown as UI curiosity)
- [ ] Mission system: orbital manoeuvres, escape challenges, scientific observations
- [ ] VR support (OpenXR)
- [ ] Multiple black holes (binary system with gravitational wave emission)
