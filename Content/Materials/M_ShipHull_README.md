# M_ShipHull — Spaceship Hull Material Guide

## Overview
The ship hull material supports:
- PBR base color, metallic, roughness, normal maps
- A `DamageLevel` scalar parameter (0..1) driven by TidalForceComponent
- Dynamic cracking effect via vertex shader displacement
- Engine glow ports (emissive with pulsing)

## Setup

1. **Create Material** `M_ShipHull` (Master Material)
   - Blend Mode: Opaque
   - Shading Model: Default Lit

2. **Texture slots** (assign in Material Instance):
   - `T_Hull_Albedo` (BaseColor)
   - `T_Hull_ORM` (Occlusion/Roughness/Metallic packed)
   - `T_Hull_Normal` (Normal, Tangent Space)
   - `T_Hull_Damage_Cracks` (BaseColor overlay for damage)
   - `T_Hull_Emissive` (engine ports, warning lights)

3. **Damage system** via Material Parameter:
   - `DamageLevel` (Scalar 0..1): lerps between clean hull and cracked/burnt hull
   - `CrackPatternIntensity` (Scalar): controls crack visibility
   - Connect to lerp between base albedo and damage texture

4. **Dynamic Instance**:
   ```cpp
   // In SpaceshipPawn.cpp, UpdateHullDamageVisuals():
   ShipMesh->SetScalarParameterValueOnMaterials("DamageLevel", 1.0f - StructuralIntegrity);
   ```

5. **Engine Glow**:
   - `EngineGlowIntensity` (Scalar): 0 when idle, 1..3 when thrusting, 5 when boosting
   - Driven by SpaceshipPawn::UpdateEngineParticles() via:
   ```cpp
   ShipMesh->SetScalarParameterValueOnMaterials("EngineGlowIntensity", Intensity);
   ```

## GPU Compatibility
Standard UE5 PBR — no special requirements.
Works on all DX11/DX12/Vulkan-capable GPUs.
