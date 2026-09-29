from __future__ import annotations

from dataclasses import dataclass
from typing import Iterable, Optional


@dataclass(frozen=True)
class Metrics:
    radius_cm: float = 45.0
    rest_cm: float = 50.0
    max_drop_cm: float = 8.0
    max_raise_cm: float = 2.0


@dataclass(frozen=True)
class Surface:
    z_cm: float
    normal_z: float
    label: str


@dataclass(frozen=True)
class TraceResult:
    hit: bool
    displacement_cm: Optional[float]
    surface: Optional[str]


GROUND = Surface(0.0, 1.0, "ground")
CEILING_UNDERSIDE = Surface(60.0, -1.0, "ceiling_underside")
VERTICAL_WALL = Surface(0.0, 0.0, "vertical_wall")
STEEP_DRIVEABLE_SLOPE = Surface(0.0, 0.35, "steep_driveable_slope")


def first_blocking_hit(start_z: float, end_z: float, surfaces: Iterable[Surface]) -> Optional[Surface]:
    best_t = None
    best_surface = None
    denom = end_z - start_z
    if abs(denom) < 1.0e-6:
        return None

    for surface in surfaces:
        t = (surface.z_cm - start_z) / denom
        if 0.0 <= t <= 1.0 and (best_t is None or t < best_t):
            best_t = t
            best_surface = surface

    return best_surface


def displacement_from_hit(mount_z: float, hit_z: float, metrics: Metrics) -> float:
    wheel_center_z = hit_z + metrics.radius_cm
    current_distance = mount_z - wheel_center_z
    return metrics.rest_cm - current_distance


def contact_from_surface(mount_z: float, surface: Surface, metrics: Metrics, filter_normals: bool) -> TraceResult:
    if filter_normals and surface.normal_z <= 0.10:
        return TraceResult(False, None, surface.label)
    return TraceResult(True, displacement_from_hit(mount_z, surface.z_cm, metrics), surface.label)


def january_trace(mount_z: float, surfaces: Iterable[Surface], metrics: Metrics) -> TraceResult:
    start_z = mount_z - metrics.radius_cm + metrics.max_raise_cm
    end_z = mount_z - metrics.radius_cm - (metrics.rest_cm + metrics.max_drop_cm)
    surface = first_blocking_hit(start_z, end_z, surfaces)
    if surface is None:
        return TraceResult(False, None, None)
    return contact_from_surface(mount_z, surface, metrics, filter_normals=False)


def current_rescue_trace(mount_z: float, surfaces: Iterable[Surface], metrics: Metrics) -> TraceResult:
    surfaces = tuple(surfaces)
    start_lift = metrics.max_raise_cm + metrics.radius_cm + 50.0
    start_z = mount_z - metrics.radius_cm + start_lift
    end_z = mount_z - metrics.radius_cm - (metrics.rest_cm + metrics.max_drop_cm)
    surface = first_blocking_hit(start_z, end_z, surfaces)

    if surface is None:
        surface = first_blocking_hit(mount_z + 1000.0, mount_z - 5000.0, surfaces)

    if surface is None:
        return TraceResult(False, None, None)
    return contact_from_surface(mount_z, surface, metrics, filter_normals=False)


def fixed_trace(mount_z: float, surfaces: Iterable[Surface], metrics: Metrics) -> TraceResult:
    surfaces = tuple(surfaces)
    end_z = mount_z - metrics.radius_cm - (metrics.rest_cm + metrics.max_drop_cm)

    baseline_start_z = mount_z - metrics.radius_cm + metrics.max_raise_cm
    surface = first_blocking_hit(baseline_start_z, end_z, surfaces)
    if surface is not None:
        result = contact_from_surface(mount_z, surface, metrics, filter_normals=True)
        if result.hit:
            return result

    recovery_lift = metrics.max_raise_cm + metrics.rest_cm + metrics.max_drop_cm + (2.0 * metrics.radius_cm)
    recovery_start_z = mount_z - metrics.radius_cm + recovery_lift
    surface = first_blocking_hit(recovery_start_z, end_z, surfaces)
    if surface is None:
        return TraceResult(False, None, None)
    return contact_from_surface(mount_z, surface, metrics, filter_normals=True)


def fmt(result: TraceResult) -> str:
    if not result.hit:
        return "miss"
    return f"{result.displacement_cm:7.1f}@{result.surface}"


def run() -> None:
    metrics = Metrics()
    cases = [
        ("normal rest", 95.0, [GROUND], True),
        ("within max droop", 102.0, [GROUND], True),
        ("wheel center below ground", 30.0, [GROUND], True),
        ("deep sunken mount", -80.0, [GROUND], True),
        ("beyond diameter recovery", -160.0, [GROUND], False),
        ("airborne beyond max droop", 160.0, [GROUND], False),
        ("ceiling underside rejected", 30.0, [CEILING_UNDERSIDE, GROUND], False),
        ("vertical wall rejected", 30.0, [VERTICAL_WALL], False),
        ("steep driveable slope", 30.0, [STEEP_DRIVEABLE_SLOPE], True),
    ]

    failures: list[str] = []
    print(f"{'case':30} {'january':18} {'current':18} {'fixed':18} expected")
    for name, mount_z, surfaces, expected_contact in cases:
        jan = january_trace(mount_z, surfaces, metrics)
        cur = current_rescue_trace(mount_z, surfaces, metrics)
        fix = fixed_trace(mount_z, surfaces, metrics)
        print(f"{name:30} {fmt(jan):18} {fmt(cur):18} {fmt(fix):18} {'contact' if expected_contact else 'miss'}")

        if fix.hit != expected_contact:
            failures.append(f"{name}: fixed hit={fix.hit}, expected={expected_contact}")

    if january_trace(30.0, [GROUND], metrics).hit:
        failures.append("january trace unexpectedly hit the sunken wheel-center case")
    if not fixed_trace(30.0, [GROUND], metrics).hit:
        failures.append("fixed trace failed the sunken wheel-center case")
    if not current_rescue_trace(160.0, [GROUND], metrics).hit:
        failures.append("current rescue did not reproduce the airborne false-contact risk")
    if fixed_trace(160.0, [GROUND], metrics).hit:
        failures.append("fixed trace falsely contacted while beyond max droop")

    for mount_z in range(-180, 181, 5):
        fixed = fixed_trace(float(mount_z), [GROUND], metrics)
        end_z = mount_z - metrics.radius_cm - (metrics.rest_cm + metrics.max_drop_cm)
        recovery_start_z = mount_z - metrics.radius_cm + (
            metrics.max_raise_cm + metrics.rest_cm + metrics.max_drop_cm + (2.0 * metrics.radius_cm)
        )
        expected = end_z <= 0.0 and recovery_start_z >= 0.0
        if fixed.hit != expected:
            failures.append(
                f"sweep mount_z={mount_z}: fixed hit={fixed.hit}, expected={expected}, "
                f"end_z={end_z:.1f}, recovery_start_z={recovery_start_z:.1f}"
            )

    if failures:
        print("\nFAIL")
        for failure in failures:
            print(f" - {failure}")
        raise SystemExit(1)

    print("\nPASS: fixed trace recovers sunken wheels, rejects non-ground normals, and misses when airborne beyond max droop.")


if __name__ == "__main__":
    run()
