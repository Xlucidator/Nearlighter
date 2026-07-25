"""Shared utilities for Nearlighter Python scripts."""

from __future__ import annotations

import hashlib
import json
import math
import shlex
import struct
import subprocess
import sys
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, List, Optional, Sequence, Tuple


ROOT = Path(__file__).resolve().parents[1]


class EvaluationError(RuntimeError):
    """Expected configuration or evaluation failure."""


@dataclass(frozen=True)
class PFMImage:
    """Top-to-bottom RGB float pixels decoded from a PFM file."""

    width: int
    height: int
    pixels: Tuple[float, ...]


# ==================================================
# Generic Utilities
# ==================================================

def utc_now() -> datetime:
    '''Current UTC timestamp.'''
    return datetime.now(timezone.utc)


def utc_text(value: Optional[datetime] = None) -> str:
    '''ISO 8601 UTC timestamp.'''
    return (value or utc_now()).isoformat().replace("+00:00", "Z")


def write_json(path: Path, value: Any) -> None:
    '''Atomic human-readable JSON output.'''
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    with temporary.open("w", encoding="utf-8") as output:
        json.dump(value, output, indent=2, sort_keys=True)
        output.write("\n")
    temporary.replace(path)


def load_json(path: Path) -> Dict[str, Any]:
    '''JSON object loading and validation.'''
    try:
        with path.open("r", encoding="utf-8") as source:
            value = json.load(source)
    except (OSError, json.JSONDecodeError) as error:
        raise EvaluationError(f"Failed to read JSON '{path}': {error}") from error
    if not isinstance(value, dict):
        raise EvaluationError(f"Expected a JSON object in '{path}'")
    return value


def sha256(path: Path) -> str:
    '''File SHA-256 digest.'''
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def resolve_project_path(value: str) -> Path:
    '''Project-root path resolution.

    Input: CLI or configuration path text. Output: absolute ``Path``.
    Relative input: "assets/scene.json" -> ``ROOT / assets/scene.json``.
    Absolute input: "/tmp/scene.json" -> unchanged.
    '''
    path = Path(value)
    return path if path.is_absolute() else ROOT / path


def display_project_path(path: Path) -> str:
    '''Stable path text for logs and reports.

    Input: filesystem ``Path``. Output: display-oriented ``str``.
    Project input: ``ROOT / assets/scene.json`` -> "assets/scene.json".
    External input: ``/tmp/scene.json`` -> resolved absolute path text.
    '''
    try:
        return str(path.resolve().relative_to(ROOT))
    except ValueError:
        return str(path.resolve())


def validate_render_settings(settings: Dict[str, int]) -> Dict[str, int]:
    '''Validated deterministic render settings.'''
    positive_fields = ("width", "height", "samples_per_pixel", "max_depth")
    if any(settings[field] <= 0 for field in positive_fields):
        raise EvaluationError("Render dimensions, SPP, and depth must be positive")
    if settings["seed"] < 0 or settings["seed"] > 0xFFFFFFFFFFFFFFFF:
        raise EvaluationError("Render seed must fit in uint64")
    return settings


# ==================================================
# Process Utilities
# ==================================================

def run_capture(command: Sequence[str]) -> str:
    '''Captured command output; empty text on failure.'''
    try:
        completed = subprocess.run(
            command,
            cwd=ROOT,
            check=True,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
    except (OSError, subprocess.CalledProcessError):
        return ""
    return completed.stdout.strip()


def run_logged(command: Sequence[str], log_path: Path) -> str:
    '''Mirrored command output and persistent log.'''

    # ----- Command presentation -----
    log_path.parent.mkdir(parents=True, exist_ok=True)
    command_text = shlex.join(str(part) for part in command)
    print(f"$ {command_text}")

    # ----- Process execution -----
    lines: List[str] = []
    with log_path.open("a", encoding="utf-8") as log:
        log.write(f"$ {command_text}\n")
        try:
            process = subprocess.Popen(
                [str(part) for part in command],
                cwd=ROOT,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
            )
        except OSError as error:
            raise EvaluationError(f"Failed to start '{command[0]}': {error}") from error

        assert process.stdout is not None
        for line in process.stdout:
            sys.stdout.write(line)
            log.write(line)
            lines.append(line)
        return_code = process.wait()
        log.write(f"[exit code: {return_code}]\n")

    # ----- Exit validation -----
    if return_code != 0:
        raise EvaluationError(
            f"Command failed with exit code {return_code}: {command_text}"
        )
    return "".join(lines)


# ==================================================
# PFM Utilities
# ==================================================

def read_pfm(path: Path) -> PFMImage:
    '''RGB PFM decoding in top-to-bottom row order.'''

    # ----- Header and payload -----
    try:
        with path.open("rb") as source:
            magic = source.readline().strip()
            if magic != b"PF":
                raise EvaluationError(f"'{path}' is not an RGB PFM file")

            dimensions = source.readline().split()
            if len(dimensions) != 2:
                raise EvaluationError(f"Invalid PFM dimensions in '{path}'")
            width, height = (int(value) for value in dimensions)
            scale = float(source.readline())
            payload = source.read()
    except (OSError, ValueError) as error:
        raise EvaluationError(f"Failed to read PFM '{path}': {error}") from error

    if width <= 0 or height <= 0 or scale == 0.0 or not math.isfinite(scale):
        raise EvaluationError(f"Invalid PFM header in '{path}'")

    # ----- Payload validation -----
    channel_count = width * height * 3
    expected_bytes = channel_count * 4
    if len(payload) != expected_bytes:
        raise EvaluationError(
            f"PFM payload size mismatch in '{path}': "
            f"expected {expected_bytes}, got {len(payload)}"
        )

    # ----- Row and scale normalization -----
    endian = "<" if scale < 0.0 else ">"
    decoded = struct.unpack(f"{endian}{channel_count}f", payload)
    scale_factor = abs(scale)
    row_channels = width * 3
    pixels = [0.0] * channel_count
    for source_row in range(height):
        target_row = height - 1 - source_row
        source_offset = source_row * row_channels
        target_offset = target_row * row_channels
        for channel in range(row_channels):
            pixels[target_offset + channel] = (
                decoded[source_offset + channel] * scale_factor
            )

    # ----- Pixel validation -----
    if not all(math.isfinite(value) for value in pixels):
        raise EvaluationError(f"PFM image '{path}' contains NaN or infinity")
    return PFMImage(width, height, tuple(pixels))


def write_pfm(path: Path, image: PFMImage) -> None:
    '''Little-endian RGB PFM output.'''
    row_channels = image.width * 3
    path.parent.mkdir(parents=True, exist_ok=True)
    try:
        with path.open("wb") as output:
            output.write(f"PF\n{image.width} {image.height}\n-1.0\n".encode("ascii"))
            for y in range(image.height - 1, -1, -1):
                offset = y * row_channels
                row = image.pixels[offset : offset + row_channels]
                output.write(struct.pack(f"<{row_channels}f", *row))
    except OSError as error:
        raise EvaluationError(f"Failed to write PFM '{path}': {error}") from error


# ==================================================
# Nearlighter Utilities
# ==================================================

def render_command(
    executable: Path,
    scene_path: Path,
    settings: Dict[str, int],
    preview_path: Path,
    linear_path: Path,
    show_progress: bool = False,
) -> List[str]:
    '''Nearlighter CLI command for one deterministic render.'''
    command = [
        str(executable),
        "--scene",
        str(scene_path),
        "--output",
        str(preview_path),
        "--linear-output",
        str(linear_path),
        "--width",
        str(settings["width"]),
        "--height",
        str(settings["height"]),
        "--spp",
        str(settings["samples_per_pixel"]),
        "--max-depth",
        str(settings["max_depth"]),
        "--seed",
        str(settings["seed"]),
    ]
    if not show_progress:
        command.append("--no-progress")
    return command


def git_information() -> Dict[str, Any]:
    '''Git commit and working-tree state.'''
    commit = run_capture(["git", "rev-parse", "HEAD"])
    status = run_capture(["git", "status", "--porcelain"])
    return {
        "commit": commit or None,
        "dirty": bool(status),
    }


def compiler_information(build_directory: Path) -> Dict[str, Optional[str]]:
    '''Compiler metadata from the CMake build tree.'''

    # ----- Default metadata -----
    values: Dict[str, Optional[str]] = {
        "path": None,
        "id": None,
        "version": None,
        "banner": None,
    }
    cache = build_directory / "CMakeCache.txt"
    if not cache.exists():
        return values

    # ----- CMake cache metadata -----
    patterns = {
        "path": "CMAKE_CXX_COMPILER:FILEPATH=",
        "id": "CMAKE_CXX_COMPILER_ID:STRING=",
        "version": "CMAKE_CXX_COMPILER_VERSION:STRING=",
    }
    for line in cache.read_text(encoding="utf-8", errors="replace").splitlines():
        for field, prefix in patterns.items():
            if line.startswith(prefix):
                values[field] = line[len(prefix) :]

    # ----- Compiler banner -----
    if values["path"]:
        banner = run_capture([values["path"], "--version"])
        values["banner"] = banner.splitlines()[0] if banner else None
    return values


def find_executable(build_directory: Path, configuration: str) -> Path:
    '''Nearlighter executable in single- or multi-config builds.'''
    names = ("Nearlighter", "Nearlighter.exe")
    candidates = [build_directory / name for name in names]
    candidates.extend(build_directory / configuration / name for name in names)
    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()
    raise EvaluationError(
        f"Nearlighter executable was not found under '{build_directory}'"
    )


def prepare_build(
    build_directory: Path,
    configuration: str,
    logs_directory: Path,
    skip_build: bool,
    skip_tests: bool,
) -> Path:
    '''Nearlighter build preparation and optional CTest execution.'''

    # ----- Configure and build -----
    if not skip_build:
        run_logged(
            [
                "cmake",
                "-S",
                str(ROOT),
                "-B",
                str(build_directory),
                f"-DCMAKE_BUILD_TYPE={configuration}",
                "-DBUILD_TESTING=ON",
            ],
            logs_directory / "configure.log",
        )
        run_logged(
            [
                "cmake",
                "--build",
                str(build_directory),
                "--config",
                configuration,
                "--parallel",
            ],
            logs_directory / "build.log",
        )

    # ----- Executable resolution -----
    executable = find_executable(build_directory, configuration)

    # ----- Test execution -----
    if not skip_tests:
        run_logged(
            [
                "ctest",
                "--test-dir",
                str(build_directory),
                "-C",
                configuration,
                "--output-on-failure",
            ],
            logs_directory / "ctest.log",
        )
    return executable
