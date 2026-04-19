# Maps — Level Setup Guide

## BlackHoleLevel.umap

This is the main (and currently only) level for the game.

### World Settings
- **GameMode**: `BP_GameMode` (inherits `ABlackHoleGameMode`)
- **Default Pawn**: `BP_SpaceshipPawn`
- **Enable World Composition**: false (single large level)
- **World Bounds**: 1e12 UU (1 trillion meters = ~6.7 AU)
- **Large World Coordinates**: ENABLED (required for 1 billion meter scale)
  - Set in `.uproject` or `DefaultEngine.ini`: `r.LargeWorldCoordinates=1`
- **Default Gravity**: 0.0 (gravity handled entirely by BlackHolePhysicsComponent)
- **Navigation System**: None (no navmesh needed)

### Actor Placement

| Actor | Class | Location | Notes |
|---|---|---|---|
| BH_Main | BP_BlackHole | (0, 0, 0) | 10 solar mass, spin=0.7 |
| Ship | BP_SpaceshipPawn | (0, 1e9, 0) | 1 million km from BH |
| SkyEnv | BP_SpaceEnvironment | (0, 0, 0) | Skybox radius = 1e10 |
| PostProcess | PostProcessVolume | (0, 0, 0) | Infinite extent, add M_BlackHoleLens |
| SunLight | DirectionalLight | (0, 0, 0) | Disabled (space = no sun here) |
| AmbientLight | SkyLight | (0, 0, 0) | Intensity 0.05, space black |

### Post Process Volume Settings
- Infinite Extent: true
- **Blendables**: Add `M_BlackHoleLens` with weight 1.0
- **Chromatic Aberration**: 0.5 (subtle lens distortion from extreme gravity)
- **Vignette**: 0.3
- **Lens Flare**: 0.2
- **Bloom**: enabled, Intensity 3.0 (accretion disk glow)
- **Auto Exposure**: enabled, Min EV100 = -4, Max EV100 = 20
  (wide range to handle both deep space darkness and accretion disk brilliance)
- **Depth of Field**: Bokeh, focal distance driven by Blueprint

### Large World Coordinate Note
UE5.1+ supports LWC (Large World Coordinates) natively.
The game uses 1 UU = 1 meter, placing the black hole at origin and
the player starting 1 billion UU away.
Verify `r.LargeWorldCoordinates=1` is set and that the project uses UE5.1+.
