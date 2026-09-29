# Vehicle Audio Review (Codex, 2026-04-03)

## Scope

This review covers the current runtime audio models in:

- `Source/GRIT/VehicleFramework/Components/AudioComponent/VehicleAudioMixer.cpp`
- `Source/GRIT/VehicleFramework/Components/AudioComponent/EngineAudioModel.cpp`
- `Source/GRIT/VehicleFramework/Components/AudioComponent/ForcedInductionAudioModel.cpp`
- `Source/GRIT/VehicleFramework/Components/AudioComponent/TireAudioModel.cpp`
- `Source/GRIT/VehicleFramework/Components/AudioComponent/BrakeAudioModel.cpp`
- `Source/GRIT/VehicleFramework/Components/AudioComponent/VehicleAudioTypes.h`
- `Source/GRIT/VehicleFramework/VehicleSolver.cpp`

The review is grounded first in the current telemetry set captured on 2026-04-03:

- `Source/GRIT/VehicleFramework/Telemetry/PowertrainLog.csv`
- `Source/GRIT/VehicleFramework/Telemetry/FrictionStateMachineLog.csv`
- `Source/GRIT/VehicleFramework/Telemetry/PerformanceLog.csv`

Important limitation: there is no direct audio waveform or audio CPU telemetry in the current logger, so the validation below is based on the physical-state logs that feed the audio models, plus direct code inspection of the synthesis path.

## Findings

### 1. Constant forced-induction hiss is being synthesized at idle even when the physical logs are effectively off-boost

Severity: high

Evidence:

- `ForcedInductionAudioModel.cpp:90-92` always adds turbo broadband terms:
  - `TurboWhoosh = AirNoise * (0.04 + 0.16 * boost + 0.06 * wastegate)`
  - `BovHiss = (...) * (0.14 + 0.42 * BovEnvelope)`
  - `WastegateHiss = (...) * (0.06 + 0.24 * WastegateEnvelope)`
- `ForcedInductionAudioModel.cpp:120` always adds a supercharger bypass-noise floor:
  - `BypassNoise = (...) * (0.05 + 0.28 * bypass + 0.18 * envelope)`

Why this matters:

- Those terms do not decay to zero when the event envelopes are zero. The BOV term alone has a permanent `0.14` multiplier.
- In the 2026-04-03 idle slice from `PowertrainLog.csv` (`Speed_kmh < 1`, `Throttle < 0.05`, `Brake < 0.05`, 149 samples):
  - `EngineRPM avg = 700`
  - `TurboShaftRPM avg = 2373.8`
  - `BoostGauge_bar avg = 0.0`
  - `ManifoldPressure_kPa avg = 101.33`
- That is physically consistent with near-atmospheric idle operation, so a clearly audible constant white/brown hiss is not supported by the logs.

Conclusion:

- The idle broadband noise you described is a real model issue, not just subjective taste. The current forced-induction model has a built-in hiss floor that should be event-gated or pushed far lower near zero boost/zero valve activity.

### 2. The squeal you hear while turning is primarily coming from the tire model, not the brakes, in the supplied capture

Severity: high

Evidence in code:

- `VehicleSolver.cpp:848-850` feeds wheel slip ratio, slip angle, and slip-energy state directly into the audio state.
- `TireAudioModel.cpp:59-66` forms `SlipNorm` from slip ratio, slip angle, and slip energy, then directly turns that into scrub and squeal envelopes.
- `TireAudioModel.cpp:89` places the squeal tone in the `800-5200 Hz` band.

Evidence in logs:

- No-brake turning slice from `PowertrainLog.csv` (`abs(Steering) > 0.08`, `Brake < 0.05`, `Speed_kmh > 20`, 8733 samples):
  - wheel brake torque `p95 = 0` on all four wheels
  - front slip angles are still non-zero and often significant
- Tighter normal-cornering slice (`abs(Steering) > 0.08`, `Brake < 0.05`, `Handbrake < 0.05`, `20 < Speed_kmh < 140`, `0.1 < |LateralAccel_G| < 0.9`, front slip ratios < 0.15, 2449 samples):
  - `W0_SlipAngle avg = 1.73 deg`, `p95 = 5.30 deg`
  - `W1_SlipAngle avg = 1.39 deg`, `p95 = 4.34 deg`
- When the current tire formula is replayed against that normal-cornering slice, the front axle produces:
  - estimated scrub target `avg = 0.020`, `p95 = 0.077`
  - estimated squeal target `avg = 0.0056`, `p95 = 0.015`
- In a harder-turn slice (`abs(Steering) > 0.2`, `Brake < 0.1`, `Handbrake < 0.05`, `Speed_kmh > 20`, `|LateralAccel_G| > 0.5`, 6545 samples), the same replay shows:
  - front estimated squeal target `avg = 0.153`, `p95 = 0.336`
  - about `53.5%` of front-wheel samples exceed an estimated squeal target of `0.05`

Why this matters:

- The model does not distinguish clean lateral scrub from true tonal tire squeal. It effectively treats high slip angle as a direct route into a high-frequency tonal component.
- That explains the user report: ordinary turning mostly produces scrub, but once the logged turn state becomes moderately aggressive the model crosses into squeal quickly.

Conclusion:

- In this capture, the turning squeal is tire-driven. The brake model is not the main cause of squeal during no-brake turning.

### 3. Brake audio is calibrated too loud relative to the engine and will dominate the mix under realistic pedal use

Severity: high

Evidence in code:

- `BrakeAudioModel.cpp:50` normalizes brake torque by `3500 Nm` and allows saturation up to `1.5`.
- `BrakeAudioModel.cpp:54-58` builds drag and squeal almost entirely from brake torque, temperature, and low-slip gating.
- `VehicleAudioMixer.cpp:45-47` sums four brake channels after panning.
- `VehicleAudioTypes.h:100-104` uses `EngineGain = 0.95`, `BrakeGain = 0.45`, `TireGain = 0.72`, `TurboGain = 0.60`.

Evidence in logs:

- Braking slice from `PowertrainLog.csv` (`Brake > 0.2`, `Speed_kmh > 10`, 1514 samples):
  - `Brake avg = 0.820`
  - wheel brake torque averages `5956-6170 Nm`
  - wheel brake torque `p95 = 7649-7657 Nm`
- Replaying the current normalization against that slice gives:
  - `BrakeTorqueNorm avg = 1.358`
  - `BrakeTorqueNorm p95 = 1.5` (hard saturation)
  - estimated `DragTarget avg = 0.283`, `p95 = 0.473` per wheel before mixer gain

Why this matters:

- The brake model is spending much of real braking near its own cap, and then four channels are summed into the stereo mix.
- The engine layer is more physically anchored than the brake layer, but the mix has no loudness management beyond final soft clipping.

Conclusion:

- The user report that the brakes are louder than the engine is supported by both the logs and the current gain structure.

### 4. The turbo layer becomes piercing because the tonal model emphasizes upper harmonics without enough psychoacoustic control

Severity: medium

Evidence in code:

- `ForcedInductionAudioModel.cpp:76` sets the turbo fundamental from `260 + 4200 * sqrt(TurboNorm)`, clamped to `5800 Hz`.
- `ForcedInductionAudioModel.cpp:85-88` adds a strong `1.98x` harmonic and a subcomponent, then band-passes the result around the turbo fundamental.

Evidence in logs:

- Boosted slice from `PowertrainLog.csv` (`TurboShaftRPM > 30000` or `BoostGauge_bar > 0.1`, 6728 samples):
  - `TurboShaftRPM avg = 53694.6`
  - `TurboShaftRPM p95 = 71960.1`
  - `BoostGauge_bar avg = 0.337`
- Replaying the current turbo-frequency mapping gives:
  - estimated turbo fundamental `avg = 2745.5 Hz`
  - `p95 = 3169.0 Hz`
- With the code's `1.98x` harmonic, that places a strong secondary component near roughly `5.4-6.3 kHz` in the same operating region.

Why this matters:

- That upper band is exactly where whistle turns into an irritating squeal for many listeners.
- The user's comment that the turbo is good overall but becomes annoying when pitch climbs is consistent with the current harmonic design.

Conclusion:

- The problem is not that the turbo model is bad overall. It is that the high-frequency harmonic content is too exposed once shaft speed rises.

### 5. The mix has no source-aware loudness management, so the loudest synthetic layer simply wins

Severity: medium

Evidence:

- `VehicleAudioMixer.cpp:38-54` simply sums engine, forced induction, tires, brakes, and suspension, then applies one final soft clip.
- There is no source-dependent ducking, no band-limited masking logic, and no psychoacoustic loudness control.

Why this matters:

- When brake drag, tire squeal, or turbo whistle enters an energetically strong band, the mix cannot rebalance itself.
- This is one reason the brake layer can bury the engine and why high-frequency tones feel harsher than their raw scalar gain suggests.

Conclusion:

- Even with better per-model calibration, the current mixer architecture will still exaggerate the most annoying source unless the mix policy is improved.

## What Works Well

- The engine layer is more physically grounded than the tire and brake layers. `EngineAudioModel.cpp:73-123` ties the sound to combustion-trigger timing, firing-rate progression, and resonator shaping instead of a purely cosmetic oscillator.
- The supercharger path is also better behaved than the turbo path. `ForcedInductionAudioModel.cpp:99-118` anchors supercharger pitch to shaft speed and drive ratio, which is one reason it reads as mechanically plausible.
- The overall runtime cost is likely modest. The code is lightweight per sample and per wheel, and `PerformanceLog.csv` shows no physics-side performance emergency. However, there is no direct audio timing metric in the current logger, so this is a code-inspection judgment, not a measured CPU claim.

## Research Notes

Recent primary sources line up with the issues above:

- Engine synthesis:
  - `Physics-Informed Neural Engine Sound Modeling with Differentiable Pulse-Train Synthesis` (arXiv, submitted 2026-03-12) reports that pulse-train plus resonator structure improves harmonic reconstruction and keeps parameters physically interpretable. That supports the current direction of the engine layer more than pure noise-based approaches.
- Tire-road noise:
  - `New road surface source correction factors for Nord2000 calculations in Sweden` (Euronoise 2025) summarizes tire-road noise mechanisms as road-induced structural vibration, stick-slip/stick-snap, air pumping, and horn amplification near the contact patch. That is a better physical framing than mapping steering slip angle directly to a squeal oscillator.
- Turbo compressor acoustics:
  - `Saw-Tooth Ramps for the Suppression of Flow-Acoustic Coupling in a High-Frequency Silencer for Turbocharger Compressors` (SAE, 2025) specifically treats turbocharger-compressor blade-pass-frequency narrowband noise as a high-frequency problem. That supports reducing the exposed high-order turbo harmonics in the current model.
  - `Effects of variable vaned diffuser on compressor performance and aerodynamic noise in a marine diesel engine turbocharger` (Proc IMechE Part C, first published online 2025-09-13) reinforces that compressor noise depends strongly on operating condition and aero geometry, not just shaft speed. That supports making the turbo hiss/tone more regime-dependent and less constant.
- Brake squeal:
  - `Predictive Assessment of Drum Brake Squeal Utilizing Advanced Time-Domain CAE Synthesis` (SAE, published 2025-04-07) reflects the current industry view that squeal is a structural/friction-instability problem, not just a direct function of brake torque. The present brake model is therefore too simple to sound realistic under load.
- Psychoacoustics:
  - `Psychoacoustic assessment of synthetic sounds for electric vehicles in a virtual reality experiment` (Euronoise 2025 / arXiv 2025) reports that annoyance is better predicted by psychoacoustic sound-quality metrics than by plain level metrics. That matches the current issue: the brake and turbo layers are not only loud, they are tonally irritating.

## Recommended Direction Before Any Code Change

1. Remove the permanent idle hiss floors from BOV, wastegate, and bypass-noise terms. These should be event-gated.
2. Split tire audio into separate rolling, scrub, and true squeal regimes. Ordinary steering slip should mostly live in scrub, not in a bright tonal squeal.
3. Re-normalize brake audio against the actual logged torque range and make pad/disc squeal conditional on a narrower instability window.
4. Keep the turbo character, but reduce high-harmonic exposure as shaft speed rises and shift more energy into a smoother broadband whoosh.
5. Add audio telemetry before the next pass:
   - per-layer RMS
   - per-layer peak
   - audio CPU time
   - tire/brake/turbo event counters

## Sources

- https://arxiv.org/abs/2603.09391
- https://dael.euracoustics.org/confs/fa2025/data/articles/000603.pdf
- https://www.mdpi.com/2071-1050/15/14/11300
- https://saemobilus.sae.org/papers/saw-tooth-ramps-suppression-flow-acoustic-coupling-a-high-frequency-silencer-turbocharger-compressors-2025-01-0023
- https://journals.sagepub.com/doi/10.1177/09544070251364923
- https://saemobilus.sae.org/papers/predictive-assessment-drum-brake-squeal-utilizing-advanced-time-domain-cae-synthesis-2026-01-0221
- https://arxiv.org/abs/2510.25593
