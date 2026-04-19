# M_AccretionDisk — Material Setup Guide

## Overview
The accretion disk material uses `/Shaders/AccretionDisk.usf` for physically
accurate plasma rendering including Doppler shift, temperature gradient, and turbulence.

## Mesh Setup
- Create a flat torus or ring mesh in Blender/Houdini
  - Inner radius: 1.0 (maps to ISCO radius, set via scale in-engine)
  - Outer radius: 6.0 (maps to outer disk, ~20× ISCO)
  - Subdivisions: 256+ radial, 64+ height for smooth deformation
- UV channel 0: Cylindrical projection (U=azimuth 0..1, V=radius 0..1)
- Export as FBX to `Content/Meshes/SM_AccretionDisk.fbx`

## Material Creation in UE5

1. **Create Material** `M_AccretionDisk`
   - Blend Mode: `Translucent` (for opacity variation)
   - Shading Model: `Unlit` (disk is self-luminous)
   - Two Sided: `true`

2. **Add Custom node** calling `AccretionDiskMain(...)`:
   - Inputs: `UV` (TexCoord), `r_norm` (computed from UV.y or vertex colour),
     `phi` (U * 2π), `beta_orb` (from MPC), `Time` (from SceneTime)

3. **Material Parameters** (via MPC_AccretionDisk):
   - `BetaOrbital` (Scalar) — v/c at current camera distance, updated each frame
   - `DiskThickness` (Scalar, default 0.8)
   - `TurbulenceScale` (Scalar, default 2.0)
   - `TurbulenceSpeed` (Scalar, default 0.3)
   - `InnerEdgeBoost` (Scalar, default 1.5) — ISCO glow multiplier

4. **Emissive output**: Connect `AccretionDiskMain.rgb` → Emissive Color

## GPU Compatibility
- Pure standard HLSL — no NvAPI, no AMD AGS
- DX11 SM5 / DX12 SM6 / Vulkan SPIR-V (via DXC compiler)
- Works on AMD RX 5000+ and NVIDIA GTX 10+
- FBM octave count automatically reduces on lower-end hardware via material LOD

## Physics Parameters Update (C++)
```cpp
// In BlackHoleActor::Tick or SpaceshipPawn::Tick:
float r = DistanceFromBH;  // meters
float beta = sqrt(BH->GM / r) / BlackHolePhysics::C;  // v_circ / c
UKismetMaterialLibrary::SetScalarParameterValue(
    World, MPC_AccretionDisk, "BetaOrbital", beta);
```
