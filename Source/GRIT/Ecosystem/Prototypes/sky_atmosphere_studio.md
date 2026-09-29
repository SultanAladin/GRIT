# Sky Atmosphere Studio — Design Notes

Single-file WebGL2 prototype for a **multi-dome, localized sky/atmosphere system** designed for an Unreal Engine portal use case. Each dome is an independent sphere of atmosphere with its own sun, scattering, fog, and clouds. Outside any dome, the view is the void (black). The camera can fly from one dome to another to preview the portal transition.

## File
- `sky_atmosphere_studio.html` — open in any modern browser (Chrome/Edge/Firefox). No build step.

## Controls
- **WASD** — move
- **Mouse drag** — look
- **Space / Ctrl** — up / down
- **Shift** — boost
- **Scroll** — FOV
- Bottom bar: Free-fly · Orbit · Tour
- Right panel tabs: Domes · Sun · Atmosphere · Fog · Clouds · Camera

## Architecture

### 1. Atmosphere — single-scattering Rayleigh+Mie+Ozone
Per-fragment raymarch of the radiative transfer equation along the view ray, clipped to the dome's sphere. At each sample we evaluate:
- **Density** of Rayleigh, Mie, and Ozone particles as exponential / bell-curve falloff with height
- **Optical depth** from sample to sun (secondary 4–6 step march)
- **Transmittance** = exp(−(βR·odR + 1.11·βM·odM + βO·odO))
- **In-scattered light** = T_camera · T_light · sunColor · (βR·phaseR + βM·phaseM)

Coefficient values from Bruneton & Neyret 2008 (Hillaire 2020 production constants), then **rescaled** so a small ~5 km dome scatters as visibly as the real ~60 km atmosphere. The rescale is the key trick that makes a tiny localized atmosphere look like a real sky.

### 2. Locality (the whole point)
Each dome is `{center, radius, params}`. The shader:
1. Ray–sphere intersects against every dome
2. Sorts hits by entry distance
3. Walks segments front-to-back. Each segment contributes its own scattering using its own params
4. Ground inside a dome terminates the walk (opaque)
5. **Outside all domes = black** — exactly the portal-world behavior you want

Domes shouldn't overlap in normal use; if they do, the nearer one wins (no blending). That keeps the portal worlds clean and independent.

### 3. Volumetric fog
Exponential height fog integrated along the view ray inside the dome. Single-scattering from sun with Henyey-Greenstein phase. Same accumulate-and-transmit pattern as the atmosphere but on its own loop with its own anisotropy.

### 4. Clouds (separate layer, animated)
Raymarched in a **spherical shell** above each dome's ground:
- Base layer: 5-octave value-noise FBM (cumulus shape)
- Detail: inverted Worley + high-frequency Worley (puffy granularity)
- Coverage subtracts a constant; clamp ≥0 ⇒ classic Schneider/Häggström pattern
- Vertical envelope: smoothstep at bottom + 1−smoothstep at top
- Lighting: 4-step shadow march toward sun + ambient · Henyey-Greenstein phase
- Wind: animated 2D XZ offset, configurable speed/direction

### 5. HDR & tonemap
Linear scattering accumulation → ACES filmic (Narkowicz fit) → gamma 2.2. Exposure slider drives pre-tonemap multiplier.

### 6. Sun
Analytic disc using angular radius (cos threshold), sqrt-falloff limb darkening, attenuated by atmospheric transmittance. Color from Tanner Helland blackbody approximation of color temperature.

## Presets (parameter bundles)

| Preset | Sun el | Turb | Mie | Tint | Use |
|---|---|---|---|---|---|
| Clear | 45° | 2 | 1.0× | white | Mid-day blue sky |
| Hazy | 38° | 5 | 2.6× | warm | Polluted / humid day |
| Overcast | 32° | 8 | 4.5× | cool | Diffuse grey sky |
| Sunset | 4° | 4 | 1.6× | orange | Long path → red sky |
| Twilight | −3° | 2.5 | 1.0× | violet | After-sunset blue hour |
| Night | −25° | 2 | 0.7× | cold blue | Moonlight feel |

## Quality / perf knobs
- **Atmo steps** — view-ray samples (default 24)
- **Cloud steps** — cloud-shell samples (default 32)
- **Resolution scale** — 50% / 75% / 100% of device pixels (raymarch is per-pixel)

## Source references
- E. Bruneton & F. Neyret, *Precomputed Atmospheric Scattering*, EGSR 2008
- S. Hillaire, *A Scalable and Production Ready Sky and Atmosphere Rendering Technique*, EGSR 2020 — basis for UE5 `SkyAtmosphere.usf`
- A. Schneider & N. Vos, *The Real-Time Volumetric Cloudscapes of Horizon Zero Dawn*, SIGGRAPH 2015
- F. Häggström, *Real-time rendering of volumetric clouds*, master's thesis, 2018
- L. Hosek & A. Wilkie, *An Analytic Model for Full Spectral Sky-Dome Radiance*, SIGGRAPH 2012
- K. Narkowicz, *ACES Filmic Tone Mapping Curve*, 2015
- UE5.8 reference: `Engine/Shaders/Private/SkyAtmosphere.usf`, `VolumetricCloud.usf`, `VolumetricFog.usf`

## Known prototype limitations
- Single scattering only (no multi-scattering LUT — Hillaire 2020 §5). Sunset still works because Mie carries most of the warmth at low sun angles
- No transmittance LUT precompute — recomputes per-sample. Cheap because domes are small
- No aerial perspective LUT — every pixel marches. Fine at 75% res
- Clouds use 3-octave noise in real-time (no precomputed 3D texture). Cheaper but slightly busier
- No moon / stars in night preset yet
- Domes don't blend at overlaps (by design — would dilute portal worlds)

## C++/UE5 port plan (next step, only if HTML is approved)

### Components
- `USkyDomeComponent : USceneComponent` — one per dome, holds all parameters as a `FSkyDomeParams` struct
- `USkyDomeManagerSubsystem : UWorldSubsystem` — collects all active dome components, packs into a single uniform/SRV buffer (matches the HTML uniform layout)
- `USkyDomePostProcessMaterial` — post-process material function reading scene depth, computing the same ray-sphere walk per pixel

### Shader strategy
- Translate the GLSL fragment shader to HLSL → custom node inside a post-process material *or* a usf injected at `PostProcessInput0`
- Atmosphere/cloud constants → MaterialParameterCollection or a structured buffer
- Reuse UE's `SkyAtmosphereCommon.ush` helpers where the math matches Hillaire (transmittance LUT precompute would be a follow-up optimization)
- Disable UE's default `USkyAtmosphereComponent` while this system is active (or use it as the fallback when outside any dome)

### Portal integration
- Portal trigger volume swaps "active set" of domes — but since domes already exist independently in world space, the simpler model is: portal teleports the player position into the next dome's space, and the existing dome system handles the rest with no swap
- Dome transitions can be smoothed with a brief fade or matched-parameter morph if the portal isn't instant

### Lighting integration
- Each dome emits a `FLightParameters` struct that gets pushed into the active directional light when the player enters that dome
- Sky light cubemap can be baked offline per preset, or computed from the shader's irradiance integral (Hillaire §4)
