# Blueprint Setup Guide

## Required Blueprints

Create these Blueprints in UE5 Editor (`Content/Blueprints/`):

---

### BP_BlackHole
- Parent Class: `ABlackHoleActor`
- Add component: `UGraphicsQualityManager` (so it auto-detects GPU on level load)
- Assign accretion disk mesh: `SM_AccretionDisk` with `M_AccretionDisk`
- Assign post-process material: `M_BlackHoleLens` (blendable)
- Assign relativistic jet Niagara systems (see Niagara_README.md)
- Set `MassInSolarMasses = 10`, `SpinParameter = 0.7`
- Tick: project world position to screen space, update MPC_BlackHole

### BP_SpaceshipPawn
- Parent Class: `ASpaceshipPawn`
- Assign ship mesh: `SM_Spaceship` with `M_ShipHull` instance
- Assign engine Niagara systems to `EngineTrailLeft` / `EngineTrailRight`
- Override `OnHullDamageThreshold`: spawn crack decals, play damage sounds
- Override `OnShipDestroyed`: play explosion Niagara, show game over widget
- Override `OnPhotonSphereEntered`: play warning SFX, flash HUD

### BP_SpaceEnvironment
- Parent Class: `ASpaceEnvironmentActor`
- Assign skybox mesh: `SM_Skybox` with `M_SpaceBackground`
- Set `SpaceshipClass = BP_SpaceshipPawn`
- Tick: pass ship velocity to MPC_Ship

### BP_GameMode
- Parent Class: `ABlackHoleGameMode`
- Override `OnGameOver(bWon)`: show appropriate UI widget
- Wire `BP_BlackHole -> OnShipDestroyed` to `RestartGame()` with delay

---

## Level Setup

1. Place `BP_BlackHole` at world origin (0, 0, 0)
2. Place `BP_SpaceshipPawn` at (0, 1e9, 0) — 1 million km away on Y axis
3. Place `BP_SpaceEnvironment` at origin (large skybox)
4. Add a `PostProcessVolume` (Infinite Extent) with `M_BlackHoleLens` in blendables
5. Set World Settings `GameMode = BP_GameMode`
6. Set World Settings gravity to 0 (all gravity is handled in C++)
7. Save level as `Content/Maps/BlackHoleLevel.umap`
