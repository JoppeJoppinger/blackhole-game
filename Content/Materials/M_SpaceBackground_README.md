# M_SpaceBackground — Skybox Material Setup

## Overview
The space background uses `/Shaders/StarField.usf` for the procedural star field
with relativistic aberration. It wraps a large inverted sphere (skybox).

## Setup

1. **Create a large inverted sphere** in `Content/Meshes/SM_Skybox.fbx`
   - Radius: 1e8 UU (far clip distance)
   - Normals pointing INWARD
   - Single UV island (spherical/lat-long unwrap)

2. **Create Material** `M_SpaceBackground`
   - Domain: `Surface`, Blend Mode: `Opaque`, Shading: `Unlit`
   - Disable depth write: check `Disable Depth Test`
   - Two Sided: true

3. **Custom node** calling `StarFieldMain(...)`:
   - ViewDir: derived from object world position and camera position
   - ShipVelocity: from MPC_Ship
   - StarDensity: 80.0 (adjust for performance)
   - AngularStarSize: 0.003 radians

4. **MPC_Ship parameters** (updated by SpaceshipPawn each frame):
   - `ShipVelocityX/Y/Z` (Scalar ×3) — velocity components in m/s
   - `Time` (Scalar)

## AMD / Vulkan Note
The shader uses no texture samples for star generation — all procedural via
hash functions. This avoids texture cache bottlenecks on AMD RDNA hardware.

## Performance Tuning
- Reduce `StarDensity` from 80 → 40 for Medium preset
- Reduce FBM octave counts by modifying the .usf (search for `FBM3(..., 6)`)
- On Low preset, replace with a pre-baked cubemap texture
