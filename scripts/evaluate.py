#!/usr/bin/env python3
"""Build Nearlighter and run reproducible image-quality evaluations."""

from __future__ import annotations

import argparse
import math
import os
import platform
import re
import statistics
import sys
from pathlib import Path
from typing import Any, Dict

from utils import (
    display_project_path,
    EvaluationError,
    PFMImage,
    ROOT,
    compiler_information,
    git_information,
    load_json,
    prepare_build,
    read_pfm,
    render_command,
    resolve_project_path,
    run_logged,
    sha256,
    utc_now,
    utc_text,
    validate_render_settings,
    write_json,
    write_pfm,
    write_ppm_preview,
)


CONFIG_PATH = ROOT / "scripts" / "evaluation.json"
EVALUATION_ROOT = ROOT / "build" / "evaluation"
DEFAULT_SUITE = "quick"
STATS_PATTERN = re.compile(
    r"Render:\s+([0-9.eE+-]+)\s+s,\s+([0-9.eE+-]+)\s+samples/s"
)
SCENE_LOAD_PATTERN = re.compile(r"Scene load:\s+([0-9.eE+-]+)\s+s")
PREPARATION_PATTERN = re.compile(r"Render prepare:\s+([0-9.eE+-]+)\s+s")
TIMING_SCHEMA_PATTERN = re.compile(r"Timing schema:\s+(\d+)")


# ==================================================
# Configuration
# ==================================================

def normalize_render(render: Dict[str, Any]) -> Dict[str, int]:
    '''Flat validated render settings.'''
    try:
        resolution = render["resolution"]
        settings = {
            "width": int(resolution["width"]),
            "height": int(resolution["height"]),
            "samples_per_pixel": int(render["samples_per_pixel"]),
            "max_depth": int(render["max_depth"]),
            "seed": int(render["seed"]),
        }
    except (KeyError, TypeError, ValueError) as error:
        raise EvaluationError(f"Invalid render configuration: {error}") from error
    return validate_render_settings(settings)


# ==================================================
# Build and Run
# ==================================================

def create_run_id(suite: str, git_commit: str) -> str:
    '''Unique timestamped evaluation run identifier.'''
    timestamp = utc_now().strftime("%Y%m%dT%H%M%SZ")
    short_commit = git_commit[:8] if git_commit else "nogit"
    base = f"{timestamp}-{short_commit}-{suite}"
    candidate = base
    suffix = 2
    while (EVALUATION_ROOT / "runs" / candidate).exists():
        candidate = f"{base}-{suffix}"
        suffix += 1
    return candidate


# ==================================================
# Reference and Metrics
# ==================================================

def parse_render_stats(output: str, settings: Dict[str, int]) -> Dict[str, Any]:
    '''Render timing and primary-sample statistics.'''
    matches = STATS_PATTERN.findall(output)
    scene_load_matches = SCENE_LOAD_PATTERN.findall(output)
    preparation_matches = PREPARATION_PATTERN.findall(output)
    timing_schema_matches = TIMING_SCHEMA_PATTERN.findall(output)
    if not matches or not scene_load_matches or not preparation_matches:
        raise EvaluationError("Nearlighter did not report parseable render statistics")
    scene_load_seconds = float(scene_load_matches[-1])
    preparation_seconds = float(preparation_matches[-1])
    integration_seconds = float(matches[-1][0])
    reported_throughput = float(matches[-1][1])
    timing_values = (scene_load_seconds, preparation_seconds, integration_seconds)
    if any(value < 0.0 or not math.isfinite(value) for value in timing_values):
        raise EvaluationError("Nearlighter reported invalid timing data")

    sample_count = (
        settings["width"]
        * settings["height"]
        * settings["samples_per_pixel"]
    )
    return {
        "timing_schema_version": int(timing_schema_matches[-1]) if timing_schema_matches else 1,
        "scene_load_seconds": scene_load_seconds,
        "preparation_seconds": preparation_seconds,
        "integration_seconds": integration_seconds,
        "sample_count": sample_count,
        "samples_per_second": reported_throughput,
    }


def load_reference(reference_path: Path) -> PFMImage:
    '''Fixed reference image from the configured path.'''
    if not reference_path.is_file():
        raise EvaluationError(
            f"Reference image is missing: '{display_project_path(reference_path)}'"
        )
    return read_pfm(reference_path)


def calculate_metrics(result: PFMImage, reference: PFMImage) -> Dict[str, Any]:
    '''Linear RGB error metrics.'''

    # ----- Image compatibility -----
    if result.width != reference.width or result.height != reference.height:
        raise EvaluationError(
            "Result and reference image dimensions do not match: "
            f"{result.width}x{result.height} versus "
            f"{reference.width}x{reference.height}"
        )

    # ----- Shared reference values -----
    reference_energy = math.fsum(value * value for value in reference.pixels)
    channel_count = len(result.pixels)
    peak = max(reference.pixels)

    def derive(result_scale: float, prefix: str = "") -> Dict[str, Any]:
        '''Metric family for one uniform result-image scale.'''
        squared_error = math.fsum(
            (result_scale * actual - expected) ** 2
            for actual, expected in zip(result.pixels, reference.pixels)
        )
        mse = squared_error / channel_count
        relative_mse = (
            squared_error / reference_energy if reference_energy > 0.0 else None
        )
        psnr = (
            10.0 * math.log10((peak * peak) / mse)
            if mse > 0.0 and peak > 0.0
            else None
        )
        return {
            f"{prefix}mse": mse,
            f"{prefix}rmse": math.sqrt(mse),
            f"{prefix}relative_mse": relative_mse,
            f"{prefix}psnr": psnr,
        }

    # ----- Raw and exposure-aligned metrics -----
    result_energy = math.fsum(value * value for value in result.pixels)
    exposure_scale = (
        math.fsum(
            actual * expected
            for actual, expected in zip(result.pixels, reference.pixels)
        ) / result_energy
        if result_energy > 0.0
        else None
    )
    metrics = derive(1.0)
    metrics["psnr_peak"] = peak
    metrics["exposure_scale"] = exposure_scale
    if exposure_scale is not None:
        metrics.update(derive(exposure_scale, "exposure_aligned_"))
        metrics["exposure_aligned_psnr_peak"] = peak
    return metrics


def difference_image(
    result: PFMImage, reference: PFMImage, result_scale: float = 1.0
) -> PFMImage:
    '''Per-channel absolute-difference image.'''
    pixels = tuple(
        abs(result_scale * actual - expected)
        for actual, expected in zip(result.pixels, reference.pixels)
    )
    return PFMImage(result.width, result.height, pixels)


def scale_image(image: PFMImage, scale: float) -> PFMImage:
    '''Uniformly scaled linear RGB image.'''
    return PFMImage(
        image.width,
        image.height,
        tuple(scale * value for value in image.pixels),
    )


# ==================================================
# Evaluation Orchestration
# ==================================================

def evaluate_case(
    case_name: str,
    case_config: Dict[str, Any],
    executable: Path,
    run_directory: Path,
) -> Dict[str, Any]:
    '''One configured render, comparison, and case report.'''

    # ----- Case configuration -----
    scene_path = resolve_project_path(case_config["scene"])
    if not scene_path.is_file():
        raise EvaluationError(
            f"Scene file is missing for '{case_name}': "
            f"'{display_project_path(scene_path)}'"
        )
    settings = normalize_render(case_config["render"])
    reference_path = resolve_project_path(case_config["reference"])

    requested_metrics = case_config.get("metrics", [])
    supported_metrics = {
        "mse", "rmse", "relative_mse", "psnr", "exposure_scale",
        "exposure_aligned_mse", "exposure_aligned_rmse",
        "exposure_aligned_relative_mse", "exposure_aligned_psnr",
    }
    unsupported_metrics = set(requested_metrics) - supported_metrics
    if unsupported_metrics:
        names = ", ".join(sorted(unsupported_metrics))
        raise EvaluationError(f"Unsupported metrics for '{case_name}': {names}")
    reference = load_reference(reference_path)
    if reference.width != settings["width"] or reference.height != settings["height"]:
        raise EvaluationError(
            f"Reference dimensions for '{case_name}' do not match its render settings"
        )
    reference_hash = sha256(reference_path)

    # ----- Case artifacts -----
    case_directory = run_directory / "cases" / case_name
    case_directory.mkdir(parents=True, exist_ok=True)
    preview_path = case_directory / "preview.ppm"
    reference_preview_path = case_directory / "reference.ppm"
    result_path = case_directory / "result.pfm"
    write_ppm_preview(reference_preview_path, reference)
    resolved_config = {
        "case": case_name,
        "scene": case_config["scene"],
        "render_settings": settings,
        "reference": case_config["reference"],
        "reference_sha256": reference_hash,
        "repetitions": int(case_config.get("repetitions", 1)),
    }
    write_json(case_directory / "resolved-config.json", resolved_config)

    # ----- Reproducible render repetitions -----
    repetitions = resolved_config["repetitions"]
    if repetitions <= 0:
        raise EvaluationError(f"Repetitions for '{case_name}' must be positive")
    measurements = []
    result_hash = None
    for repetition in range(1, repetitions + 1):
        current_preview = (
            preview_path if repetition == 1
            else case_directory / f"repeat-{repetition}.ppm"
        )
        current_result = (
            result_path if repetition == 1
            else case_directory / f"repeat-{repetition}.pfm"
        )
        output = run_logged(
            render_command(
                executable, scene_path, settings, current_preview, current_result
            ),
            run_directory / "logs" / f"{case_name}-repeat-{repetition}.log",
        )
        current_hash = sha256(current_result)
        if result_hash is None:
            result_hash = current_hash
        elif current_hash != result_hash:
            raise EvaluationError(
                f"Repeated deterministic render changed for '{case_name}'"
            )
        measurements.append(parse_render_stats(output, settings))
        if repetition > 1:
            current_preview.unlink()
            current_result.unlink()

    # ----- Image comparison -----
    result = read_pfm(result_path)
    calculated_metrics = calculate_metrics(result, reference)
    if (
        any(name.startswith("exposure_aligned_") for name in requested_metrics)
        and calculated_metrics["exposure_scale"] is None
    ):
        raise EvaluationError(
            f"Cannot exposure-align the zero-energy result for '{case_name}'"
        )
    metrics = {name: calculated_metrics[name] for name in requested_metrics}
    if "psnr" in metrics:
        metrics["psnr_peak"] = calculated_metrics["psnr_peak"]
    if "exposure_aligned_psnr" in metrics:
        metrics["exposure_aligned_psnr_peak"] = calculated_metrics[
            "exposure_aligned_psnr_peak"
        ]
    write_pfm(
        case_directory / "difference.pfm",
        difference_image(result, reference),
    )
    if "exposure_scale" in requested_metrics:
        exposure_scale = calculated_metrics["exposure_scale"]
        if exposure_scale is not None:
            write_ppm_preview(
                case_directory / "preview-exposure-aligned.ppm",
                scale_image(result, exposure_scale),
            )
            write_pfm(
                case_directory / "difference-exposure-aligned.pfm",
                difference_image(result, reference, exposure_scale),
            )

    # ----- Timing aggregation -----
    aggregate_fields = (
        "scene_load_seconds", "preparation_seconds",
        "integration_seconds", "samples_per_second",
    )
    aggregates = {}
    for field in aggregate_fields:
        values = [measurement[field] for measurement in measurements]
        aggregates[field] = {
            "minimum": min(values),
            "median": statistics.median(values),
            "maximum": max(values),
        }
    stats = {
        "timing_schema_version": measurements[0]["timing_schema_version"],
        "repetitions": repetitions,
        "scene_load_seconds": aggregates["scene_load_seconds"]["median"],
        "preparation_seconds": aggregates["preparation_seconds"]["median"],
        "integration_seconds": aggregates["integration_seconds"]["median"],
        "sample_count": measurements[0]["sample_count"],
        "samples_per_second": aggregates["samples_per_second"]["median"],
        "aggregates": aggregates,
        "measurements": measurements,
    }

    # ----- Case report -----
    report = {
        "case": case_name,
        "scene": case_config["scene"],
        "render_settings": settings,
        "reference": case_config["reference"],
        "reference_sha256": reference_hash,
        "metrics": metrics,
        "render_stats": stats,
        "result_sha256": result_hash,
    }
    write_json(case_directory / "metrics.json", report)
    return report


def parse_arguments() -> argparse.Namespace:
    '''Evaluation command-line arguments.'''
    parser = argparse.ArgumentParser(
        description="Build Nearlighter and compare deterministic renders."
    )
    parser.add_argument(
        "--suite",
        default=DEFAULT_SUITE,
        help="evaluation suite defined in scripts/evaluation.json",
    )
    parser.add_argument(
        "--skip-build",
        action="store_true",
        help="reuse the existing configured executable",
    )
    parser.add_argument(
        "--skip-tests",
        action="store_true",
        help="skip the CTest prerequisite",
    )
    return parser.parse_args()


def main() -> int:
    '''Evaluation workflow and run-level reporting.'''

    # ===== Run Preparation =====

    # ----- Configuration -----
    arguments = parse_arguments()
    config = load_json(CONFIG_PATH)

    suites = config.get("suites", {})
    if arguments.suite not in suites:
        available = ", ".join(sorted(suites))
        raise EvaluationError(
            f"Unknown suite '{arguments.suite}'. Available: {available}"
        )
    suite = suites[arguments.suite]
    cases = config.get("cases", {})

    # ----- Run identity and directories -----
    git_info = git_information()
    run_id = create_run_id(arguments.suite, git_info.get("commit") or "")
    run_directory = EVALUATION_ROOT / "runs" / run_id
    logs_directory = run_directory / "logs"
    logs_directory.mkdir(parents=True, exist_ok=False)

    # ----- Initial manifest -----
    build_config = config.get("build", {})
    build_directory = resolve_project_path(
        build_config.get("directory", "build/release")
    )
    configuration = build_config.get("configuration", "Release")
    manifest: Dict[str, Any] = {
        "schema_version": 1,
        "run_id": run_id,
        "status": "running",
        "suite": arguments.suite,
        "started_at": utc_text(),
        "git": git_info,
        "evaluation_config_sha256": sha256(CONFIG_PATH),
        "build": {
            "directory": display_project_path(build_directory),
            "configuration": configuration,
        },
        "host": {
            "platform": platform.platform(),
            "machine": platform.machine(),
            "processor": platform.processor(),
            "python": platform.python_version(),
            "cpu_count": os.cpu_count(),
        },
        "cases": {},
    }
    write_json(run_directory / "manifest.json", manifest)

    try:
        # ===== Evaluation Execution =====

        # ----- Build and tests -----
        executable = prepare_build(
            build_directory,
            configuration,
            logs_directory,
            arguments.skip_build,
            arguments.skip_tests,
        )
        manifest["build"]["compiler"] = compiler_information(build_directory)
        manifest["build"]["executable"] = display_project_path(executable)
        write_json(run_directory / "manifest.json", manifest)

        # ----- Case evaluation -----
        summary_cases: Dict[str, Any] = {}
        for case_name in suite["cases"]:
            if case_name not in cases:
                raise EvaluationError(f"Unknown case '{case_name}' in suite")
            manifest["cases"][case_name] = {"status": "running"}
            write_json(run_directory / "manifest.json", manifest)
            report = evaluate_case(
                case_name,
                cases[case_name],
                executable,
                run_directory,
            )
            summary_cases[case_name] = report
            manifest["cases"][case_name] = {
                "status": "completed",
                "scene": report["scene"],
                "render_settings": report["render_settings"],
                "reference": report["reference"],
                "reference_sha256": report["reference_sha256"],
                "result_sha256": report["result_sha256"],
            }
            write_json(run_directory / "manifest.json", manifest)

            metrics = report["metrics"]
            stats = report["render_stats"]
            metric_parts = []
            for name in cases[case_name].get("metrics", []):
                value = metrics[name]
                suffix = (
                    " dB" if name.endswith("psnr") and value is not None else ""
                )
                text = f"{value:.6g}{suffix}" if value is not None else "infinite"
                metric_parts.append(f"{name.upper()} {text}")
            metric_text = ", ".join(metric_parts)
            print(f"{case_name}: {metric_text}")
            print(
                f"  Timing ({stats['repetitions']} run(s), median): "
                f"load {stats['scene_load_seconds']:.6g} s, "
                f"prepare {stats['preparation_seconds']:.6g} s, "
                f"render {stats['integration_seconds']:.6g} s, "
                f"{stats['samples_per_second']:.6g} samples/s"
            )

        # ===== Run Completion =====

        # ----- Summary and manifest -----
        completed_at = utc_text()
        summary = {
            "schema_version": 1,
            "run_id": run_id,
            "suite": arguments.suite,
            "completed_at": completed_at,
            "cases": summary_cases,
        }
        write_json(run_directory / "summary.json", summary)
        manifest["status"] = "completed"
        manifest["completed_at"] = completed_at
        write_json(run_directory / "manifest.json", manifest)

        # ----- Latest run pointer -----
        write_json(
            EVALUATION_ROOT / "latest.json",
            {
                "run_id": run_id,
                "suite": arguments.suite,
                "completed_at": completed_at,
            },
        )
        print(f"Evaluation completed: {display_project_path(run_directory)}")
        return 0
    except Exception as error:
        # ----- Failure manifest -----
        manifest["status"] = "failed"
        manifest["failed_at"] = utc_text()
        manifest["error"] = str(error)
        write_json(run_directory / "manifest.json", manifest)
        raise


if __name__ == "__main__":
    try:
        sys.exit(main())
    except EvaluationError as error:
        print(f"ERROR: {error}", file=sys.stderr)
        sys.exit(1)
