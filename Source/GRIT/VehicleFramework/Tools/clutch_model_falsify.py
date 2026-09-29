#!/usr/bin/env python3
"""
Focused falsifier for FClutchSpecifications.

This mirrors the C++ formulas in Components/ClutchSpecifications.h. It is not a
full vehicle sim; it only proves the clutch target, engagement ramp, and torque
transfer invariants that caused the slow-launch/dead-pedal behavior.
"""

from __future__ import annotations

from dataclasses import dataclass
import math


PI = math.pi
RAD_S_TO_RPM = 60.0 / (2.0 * PI)
RPM_TO_RAD_S = (2.0 * PI) / 60.0


def clamp(value: float, lo: float, hi: float) -> float:
    return max(lo, min(hi, value))


def smooth01(value: float) -> float:
    t = clamp(value, 0.0, 1.0)
    return t * t * (3.0 - (2.0 * t))


def interp_to(current: float, target: float, dt: float, interp_speed: float) -> float:
    if interp_speed <= 0.0:
        return target
    alpha = clamp(dt * interp_speed, 0.0, 1.0)
    return current + ((target - current) * alpha)


@dataclass(frozen=True)
class ClutchSpec:
    max_torque_capacity: float = 1500.0
    engagement_time: float = 0.02
    disengagement_time: float = 0.02
    breakaway_slip_rpm: float = 300.0
    lock_slip_rpm: float = 50.0
    launch_bite_engagement: float = 0.22
    launch_lock_rpm_delta: float = 1500.0
    launch_speed_threshold_ms: float = 5.0
    launch_engagement_rate: float = 18.0
    cruise_engagement_rate: float = 25.0

    def is_launch_window(self, gear_ratio_abs: float, throttle: float, speed_ms: float) -> bool:
        return gear_ratio_abs > 0.001 and throttle > 0.01 and abs(speed_ms) < self.launch_speed_threshold_ms

    def target_engagement(
        self,
        gear_ratio_abs: float,
        is_shifting: bool,
        shift_clutch_position: float,
        engine_rpm: float,
        idle_rpm: float,
        throttle: float,
        brake: float,
        speed_ms: float,
    ) -> float:
        throttle_clamped = clamp(throttle, 0.0, 1.0)
        speed_abs = abs(speed_ms)

        if is_shifting:
            return clamp(shift_clutch_position, 0.0, 1.0)

        if gear_ratio_abs < 0.001:
            return 0.0

        if self.is_launch_window(gear_ratio_abs, throttle_clamped, speed_ms):
            rpm_alpha = smooth01((engine_rpm - idle_rpm) / max(self.launch_lock_rpm_delta, 1.0))
            throttle_alpha = smooth01((throttle_clamped - 0.02) / 0.30)
            launch_target = self.launch_bite_engagement + ((1.0 - self.launch_bite_engagement) * rpm_alpha)
            return clamp(launch_target * throttle_alpha, 0.0, 1.0)

        if brake > 0.1 and speed_abs < self.launch_speed_threshold_ms and throttle_clamped < 0.05:
            return 0.0

        if engine_rpm < idle_rpm + 200.0 and throttle_clamped < 0.05:
            return 0.0

        return 1.0

    def engagement_rate(
        self,
        current_engagement: float,
        target_engagement: float,
        launch_window: bool,
        is_shifting: bool,
    ) -> float:
        if target_engagement < current_engagement:
            return 1.0 / max(self.disengagement_time, 0.005)
        if launch_window:
            return self.launch_engagement_rate
        if is_shifting:
            return 1.0 / max(self.engagement_time, 0.005)
        return self.cruise_engagement_rate

    def torque_transfer(self, engagement: float, slip_omega_rad_s: float) -> tuple[float, float, bool]:
        engagement_clamped = clamp(engagement, 0.0, 1.0)
        capacity_nm = self.max_torque_capacity * engagement_clamped
        slip_rpm = abs(slip_omega_rad_s) * RAD_S_TO_RPM
        locked = engagement_clamped > 0.98 and slip_rpm < self.lock_slip_rpm

        if capacity_nm <= 1.0e-8:
            return 0.0, capacity_nm, locked

        breakaway_rad_s = max(self.breakaway_slip_rpm * RPM_TO_RAD_S, 1.0)
        denom = math.sqrt((slip_omega_rad_s * slip_omega_rad_s) + (breakaway_rad_s * breakaway_rad_s))
        saturation = slip_omega_rad_s / denom if denom > 1.0e-8 else 0.0
        return capacity_nm * saturation, capacity_nm, locked


def old_target_engagement(
    spec: ClutchSpec,
    current_gear_index: int,
    gear_ratio_abs: float,
    is_shifting: bool,
    engine_rpm: float,
    idle_rpm: float,
    throttle: float,
    brake: float,
    speed_ms: float,
) -> float:
    if is_shifting:
        return 0.0
    if gear_ratio_abs < 0.001:
        return 0.0

    low_rpm = engine_rpm < idle_rpm + 200.0
    if (brake > 0.1 and abs(speed_ms) < 5.0) or low_rpm:
        return 0.0

    if current_gear_index >= 3 and throttle > 0.01 and abs(speed_ms) < 5.0:
        alpha = clamp((engine_rpm - idle_rpm) / 1500.0, 0.0, 1.0)
        return 0.1 + (0.9 * alpha)

    return 1.0


def simulate_launch(spec: ClutchSpec, use_old_logic: bool, fps: int) -> dict[str, float]:
    dt = 1.0 / fps
    idle_rpm = 1000.0
    ratio = 4.2 * 3.55
    wheel_radius_m = 0.45
    mass_kg = 1750.0
    speed_ms = 0.0
    engagement = 0.0
    first_move_s = math.inf
    peak_torque_nm = 0.0

    for step in range(int(1.2 / dt)):
        t = step * dt
        throttle = clamp(t / 0.10, 0.0, 1.0)
        engine_rpm = idle_rpm + (1800.0 * smooth01(t / 0.75))
        trans_input_rpm = (speed_ms / wheel_radius_m) * ratio * RAD_S_TO_RPM

        if use_old_logic:
            target = old_target_engagement(spec, 3, ratio, False, engine_rpm, idle_rpm, throttle, 0.0, speed_ms)
            rate = 15.0 if target < engagement else 5.0
        else:
            launch = spec.is_launch_window(ratio, throttle, speed_ms)
            target = spec.target_engagement(ratio, False, 1.0, engine_rpm, idle_rpm, throttle, 0.0, speed_ms)
            rate = spec.engagement_rate(engagement, target, launch, False)

        engagement = clamp(interp_to(engagement, target, dt, rate), 0.0, 1.0)
        slip_omega = ((engine_rpm - trans_input_rpm) * RPM_TO_RAD_S)
        clutch_torque, capacity_nm, _locked = spec.torque_transfer(engagement, slip_omega)
        assert abs(clutch_torque) <= capacity_nm + 1.0e-6
        peak_torque_nm = max(peak_torque_nm, abs(clutch_torque))

        wheel_force_n = (clutch_torque * ratio * 0.92) / wheel_radius_m
        rolling_n = 220.0 if speed_ms > 0.02 else 0.0
        accel_ms2 = max(0.0, (wheel_force_n - rolling_n) / mass_kg)
        speed_ms += accel_ms2 * dt

        if speed_ms > 0.05 and first_move_s == math.inf:
            first_move_s = t

    return {
        "first_move_s": first_move_s,
        "speed_ms": speed_ms,
        "engagement": engagement,
        "peak_torque_nm": peak_torque_nm,
    }


def assert_close(actual: float, expected: float, tolerance: float, label: str) -> None:
    if abs(actual - expected) > tolerance:
        raise AssertionError(f"{label}: expected {expected:.6f}, got {actual:.6f}")


def main() -> None:
    spec = ClutchSpec()
    idle_rpm = 1000.0
    first_ratio = 4.2 * 3.55

    target = spec.target_engagement(first_ratio, False, 1.0, idle_rpm, idle_rpm, 1.0, 0.0, 0.0)
    if target < spec.launch_bite_engagement * 0.999:
        raise AssertionError(f"launch target stayed too low at idle/full throttle: {target:.3f}")

    old_target = old_target_engagement(spec, 3, first_ratio, False, idle_rpm, idle_rpm, 1.0, 0.0, 0.0)
    assert_close(old_target, 0.0, 1.0e-8, "old launch target at idle")

    engagement = 0.0
    for _ in range(12):
        launch = spec.is_launch_window(first_ratio, 1.0, 0.0)
        rate = spec.engagement_rate(engagement, target, launch, False)
        engagement = interp_to(engagement, target, 1.0 / 120.0, rate)
    if engagement < 0.18:
        raise AssertionError(f"launch engagement rose too slowly: {engagement:.3f}")

    no_throttle_low_rpm = spec.target_engagement(first_ratio, False, 1.0, idle_rpm, idle_rpm, 0.0, 0.0, 8.0)
    assert_close(no_throttle_low_rpm, 0.0, 1.0e-8, "no-throttle anti-stall target")

    braking_target = spec.target_engagement(first_ratio, False, 1.0, 1800.0, idle_rpm, 0.0, 1.0, 1.0)
    assert_close(braking_target, 0.0, 1.0e-8, "braking-to-stop target")

    for shift_position in (0.0, 0.25, 0.5, 1.0):
        shifted = spec.target_engagement(first_ratio, True, shift_position, 5000.0, idle_rpm, 1.0, 0.0, 30.0)
        assert_close(shifted, shift_position, 1.0e-8, f"shift clutch position {shift_position}")

    for engagement_test in (0.0, 0.1, 0.5, 1.0):
        for slip_rpm in range(-3000, 3001, 75):
            slip_omega = slip_rpm * RPM_TO_RAD_S
            torque, capacity, locked = spec.torque_transfer(engagement_test, slip_omega)
            if abs(torque) > capacity + 1.0e-6:
                raise AssertionError(f"torque exceeded capacity: tq={torque:.3f} cap={capacity:.3f}")
            if slip_rpm > 0 and torque < -1.0e-8:
                raise AssertionError("positive slip produced negative transfer torque")
            if slip_rpm < 0 and torque > 1.0e-8:
                raise AssertionError("negative slip produced positive transfer torque")
            if locked and not (engagement_test > 0.98 and abs(slip_rpm) < spec.lock_slip_rpm):
                raise AssertionError("lockup flag fired outside lock window")

    launch_results = {}
    for fps in (30, 60, 120, 240):
        new_result = simulate_launch(spec, use_old_logic=False, fps=fps)
        old_result = simulate_launch(spec, use_old_logic=True, fps=fps)
        launch_results[fps] = (new_result, old_result)
        if not math.isfinite(new_result["first_move_s"]):
            raise AssertionError(f"new clutch never moved at {fps} FPS")
        if new_result["first_move_s"] > old_result["first_move_s"] + (1.0 / fps):
            raise AssertionError(f"new clutch launched later than old at {fps} FPS")
        if not (0.0 <= new_result["engagement"] <= 1.0):
            raise AssertionError(f"engagement left [0,1] at {fps} FPS")

    print("ALL CLUTCH MODEL TESTS PASSED")
    for fps, (new_result, old_result) in launch_results.items():
        print(
            f"{fps:>3} FPS | new first_move={new_result['first_move_s']:.3f}s "
            f"old first_move={old_result['first_move_s']:.3f}s "
            f"new speed={new_result['speed_ms']:.2f}m/s old speed={old_result['speed_ms']:.2f}m/s "
            f"peak clutch tq={new_result['peak_torque_nm']:.1f}Nm"
        )


if __name__ == "__main__":
    main()
