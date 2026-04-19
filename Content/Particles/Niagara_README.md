# Niagara Particle Systems Guide

## NS_EngineTrail
**Purpose**: Ship engine exhaust
- Emitter type: GPU Sprite
- Spawn rate: 200/s (idle) → 800/s (boost)
- Particle lifetime: 0.8s
- Color: gradient — white core → blue → transparent
- Size: 2..8 UU
- Velocity: cone backward from engine nozzle
- Exposed parameter: `ThrustIntensity` (float)
  - Drives spawn rate multiplier and initial speed
  - Set by SpaceshipPawn: `EngineTrailLeft->SetFloatParameter("ThrustIntensity", val)`

## NS_ShipExplosion
**Purpose**: One-shot ship destruction
- Emitter type: GPU Sprite + Mesh
- Burst: 2000 debris particles + 500 spark particles
- Debris: random ship fragment meshes, tumbling, fading over 3-5s
- Sparks: bright white-orange, short lifetime 0.3s
- Shockwave: expanding ring, scale 0→200 over 0.5s

## NS_AccretionJet
**Purpose**: Relativistic jets from black hole poles
- Two instances: one at +Z pole, one at -Z pole
- Emitter type: GPU Sprite (beam emitter)
- Length: extends to 5× Schwarzschild radius beyond each pole
- Color: bright blue-white with plasma knots
- Plasma knots: sub-emitters spawning randomly along beam axis
- Speed: extremely fast (0.3c visual) → particles move from inner base outward
- Exposed parameter: `JetIntensity` (float)
- Width varies with turbulence noise

## NS_HawkingParticles
**Purpose**: Hawking radiation near event horizon (visual metaphor)
- Very subtle — only visible within 1.5× Schwarzschild radius
- Spawn rate: 50/s
- Tiny bright specks (1 UU), lifetime 0.1s
- Color: virtual particle pairs — one red, one blue (particle/antiparticle)
- Opacity very low (0.2)

## Performance Notes
- All Niagara systems use GPU simulation — no CPU overhead
- NS_ShipExplosion auto-culls when off-screen
- NS_AccretionJet uses LOD: reduce spawn rate by 50% past 500km
- Compatible with AMD and NVIDIA — Niagara GPU simulation uses standard compute shaders
