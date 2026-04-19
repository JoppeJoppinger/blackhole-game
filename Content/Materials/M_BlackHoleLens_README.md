# M_BlackHoleLens — Material Setup Guide

## Overview
This material implements gravitational lensing as a post-process effect using the
custom HLSL shader in `/Shaders/GravitationalLens.usf`.

## How to Create in UE5 Editor

1. **Create a new Material** in `Content/Materials/`
   - Name: `M_BlackHoleLens`
   - Material Domain: `Post Process`
   - Blendable Location: `Before Tonemapping`

2. **Add a Custom node** (right-click → Custom):
   - Code: `#include "/BlackHoleGame/Shaders/GravitationalLens.usf"`
     Then call: `return GravitationalLensMain(...);`
   - Output Type: `CMOT Float4`

3. **Add SceneTexture node**:
   - Scene Texture Id: `PostProcessInput0`
   - Connect to Custom node input `SceneTexture`

4. **Add Material Parameter Collection** (`MPC_BlackHole`):
   - `BlackHoleScreenPos` (Vector2)
   - `SchwarzschildRadiusUV` (Scalar, default 0.02)
   - `PhotonSphereRadiusUV` (Scalar, default 0.03)
   - `LensingStrength` (Scalar, default 1.0)
   - `RedshiftZ` (Scalar, default 0.0)
   - `AspectRatio` (Scalar, default 1.777)

5. **In BlackHoleActor.cpp / Blueprint**, update the MPC each frame:
   ```
   UKismetMaterialLibrary::SetVectorParameterValue(World, MPC_BlackHole,
       "BlackHoleScreenPos", ProjectWorldToScreen(BHLocation));
   ```

## Performance Notes
- The lensing distortion is entirely GPU-side via screen-space UV distortion
- No hardware ray tracing required — works on DX11/DX12/Vulkan (AMD and NVIDIA)
- For low-end GPUs, reduce LensingStrength or disable via `r.PostProcessing.PropagateAlpha`
