#!/usr/bin/env python3
"""Generate one fixed Nearlighter regression reference."""

from __future__ import annotations

import argparse
import sys
from typing import Dict

from utils import (
    display_project_path,
    EvaluationError,
    prepare_build,
    read_pfm,
    render_command,
    resolve_project_path,
    run_logged,
    validate_render_settings,
)


# ==================================================
# Configuration
# ==================================================

def render_settings(arguments: argparse.Namespace) -> Dict[str, int]:
    '''Validated reference render settings.'''
    settings = {
        "width": arguments.width,
        "height": arguments.height,
        "samples_per_pixel": arguments.samples_per_pixel,
        "max_depth": arguments.max_depth,
        "seed": arguments.seed,
    }
    return validate_render_settings(settings)


def parse_arguments() -> argparse.Namespace:
    '''Reference-generation command-line arguments.'''
    parser = argparse.ArgumentParser(
        description="Generate a fixed linear PFM regression reference."
    )
    parser.add_argument("--scene", required=True, help="source scene JSON path")
    parser.add_argument("--output", required=True, help="reference PFM path")
    parser.add_argument(
        "--preview",
        help="preview PPM path; defaults to the output path with .ppm suffix",
    )
    parser.add_argument("--width", required=True, type=int)
    parser.add_argument("--height", required=True, type=int)
    parser.add_argument("--spp", dest="samples_per_pixel", required=True, type=int)
    parser.add_argument("--max-depth", required=True, type=int)
    parser.add_argument("--seed", required=True, type=int)
    parser.add_argument(
        "--build-directory",
        default="build-release",
        help="CMake build directory",
    )
    parser.add_argument(
        "--configuration",
        default="Release",
        help="CMake build configuration",
    )
    parser.add_argument(
        "--skip-build",
        action="store_true",
        help="reuse the existing Nearlighter executable",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="replace existing reference and preview files",
    )
    return parser.parse_args()


# ==================================================
# Reference Generation
# ==================================================

def main() -> int:
    '''Explicit one-reference generation workflow.'''

    # ----- Paths and settings -----
    arguments = parse_arguments()
    scene_path = resolve_project_path(arguments.scene)
    output_path = resolve_project_path(arguments.output)
    preview_path = (
        resolve_project_path(arguments.preview)
        if arguments.preview
        else output_path.with_suffix(".ppm")
    )
    build_directory = resolve_project_path(arguments.build_directory)
    settings = render_settings(arguments)

    if not scene_path.is_file():
        raise EvaluationError(
            f"Scene file is missing: '{display_project_path(scene_path)}'"
        )
    if output_path.suffix.lower() != ".pfm":
        raise EvaluationError("Reference output must use the .pfm extension")
    if preview_path.suffix.lower() != ".ppm":
        raise EvaluationError("Reference preview must use the .ppm extension")
    if not arguments.force and (output_path.exists() or preview_path.exists()):
        raise EvaluationError(
            "Reference output already exists; use --force for an intentional replacement"
        )

    # ----- Build preparation -----
    logs_directory = output_path.parent / "logs"
    executable = prepare_build(
        build_directory,
        arguments.configuration,
        logs_directory,
        arguments.skip_build,
        True,
    )

    # ----- Temporary render -----
    output_path.parent.mkdir(parents=True, exist_ok=True)
    preview_path.parent.mkdir(parents=True, exist_ok=True)
    temporary_output = output_path.with_name(output_path.name + ".tmp")
    temporary_preview = preview_path.with_name(preview_path.name + ".tmp")
    run_logged(
        render_command(
            executable,
            scene_path,
            settings,
            temporary_preview,
            temporary_output,
            show_progress=True,
        ),
        logs_directory / "render.log",
    )

    # ----- Output validation and publication -----
    reference = read_pfm(temporary_output)
    if reference.width != settings["width"] or reference.height != settings["height"]:
        raise EvaluationError("Generated reference dimensions are incorrect")
    temporary_output.replace(output_path)
    temporary_preview.replace(preview_path)

    print(f"Reference: {display_project_path(output_path)}")
    print(f"Preview:   {display_project_path(preview_path)}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except EvaluationError as error:
        print(f"ERROR: {error}", file=sys.stderr)
        sys.exit(1)
