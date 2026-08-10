#!/usr/bin/env python3
"""Prepare external data and fixed references for the benchmark suite."""

from __future__ import annotations

import argparse
import shutil
import sys
import tarfile
import urllib.error
import urllib.request
from pathlib import Path
from typing import Any, Dict

from utils import (
    display_project_path,
    EvaluationError,
    load_json,
    prepare_build,
    read_pfm,
    read_rgbe,
    render_command,
    resolve_project_path,
    run_logged,
    sha256,
    utc_text,
    validate_render_settings,
    write_json,
    write_pfm,
)


CONFIG_PATH = resolve_project_path("benchmark/datasets.json")
LOGS_DIRECTORY = resolve_project_path("benchmark/data/logs")
BUNNY_SCENE = resolve_project_path("benchmark/scenes/stanford_bunny.json")


# ==================================================
# External Data
# ==================================================

def download(dataset: Dict[str, Any], force: bool) -> Path:
    '''Verified, cached, and atomically published external file.'''
    destination = resolve_project_path(dataset["download_path"])
    expected_hash = dataset["download_sha256"]
    if destination.is_file() and not force:
        if sha256(destination) != expected_hash:
            raise EvaluationError(
                f"Cached download has the wrong SHA-256: "
                f"'{display_project_path(destination)}'"
            )
        return destination

    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = destination.with_name(destination.name + ".tmp")
    request = urllib.request.Request(
        dataset["source_url"], headers={"User-Agent": "Nearlighter/0.1"}
    )
    print(f"Download: {dataset['source_url']}")
    try:
        with urllib.request.urlopen(request, timeout=60) as response:
            with temporary.open("wb") as output:
                shutil.copyfileobj(response, output)
    except (OSError, urllib.error.URLError) as error:
        temporary.unlink(missing_ok=True)
        raise EvaluationError(f"Benchmark download failed: {error}") from error
    if sha256(temporary) != expected_hash:
        temporary.unlink(missing_ok=True)
        raise EvaluationError("Downloaded benchmark file failed SHA-256 validation")
    temporary.replace(destination)
    return destination


def prepare_cornell(dataset: Dict[str, Any], force: bool) -> None:
    '''Official RGBE conversion to the project linear PFM format.'''
    source = download(dataset, force)
    reference_path = resolve_project_path(dataset["reference_path"])
    if reference_path.is_file() and not force:
        reference = read_pfm(reference_path)
    else:
        reference = read_rgbe(source)
        write_pfm(reference_path, reference)
    if (reference.width != dataset["expected_width"] or
            reference.height != dataset["expected_height"]):
        raise EvaluationError("Cornell reference dimensions are incorrect")

    write_json(
        reference_path.parent / "manifest.json",
        {
            "schema_version": 1,
            "reference_id": reference_path.parent.name,
            "reference_kind": "official_synthetic",
            "created_at": utc_text(),
            "source": {
                "url": dataset["source_url"],
                "download_sha256": dataset["download_sha256"],
                "attribution": dataset["attribution"],
                "terms": dataset["terms"],
            },
            "image": {
                "path": reference_path.name,
                "sha256": sha256(reference_path),
                "width": reference.width,
                "height": reference.height,
                "conversion": "Radiance RGBE to linear RGB PFM",
            },
        },
    )
    print(f"Cornell GT: {display_project_path(reference_path)}")


def read_ply_counts(path: Path) -> Dict[str, int]:
    '''Vertex and face counts from the ASCII PLY header.'''
    counts = {"vertices": 0, "triangles": 0}
    try:
        with path.open("rb") as source:
            while True:
                line = source.readline()
                if not line:
                    raise EvaluationError(
                        "Stanford Bunny PLY header is incomplete"
                    )
                text = line.decode("ascii", errors="strict").strip()
                if text.startswith("element vertex "):
                    counts["vertices"] = int(text.rsplit(" ", 1)[1])
                elif text.startswith("element face "):
                    counts["triangles"] = int(text.rsplit(" ", 1)[1])
                elif text == "end_header":
                    return counts
    except (OSError, UnicodeError, ValueError) as error:
        raise EvaluationError(
            f"Failed to read Stanford Bunny PLY header: {error}"
        ) from error


def prepare_bunny_mesh(dataset: Dict[str, Any], force: bool) -> Path:
    '''Official Stanford archive extraction and geometry validation.'''
    archive_path = download(dataset, force)
    mesh_path = resolve_project_path(dataset["mesh_path"])
    if not mesh_path.is_file() or force:
        mesh_path.parent.mkdir(parents=True, exist_ok=True)
        temporary = mesh_path.with_name(mesh_path.name + ".tmp")
        try:
            with tarfile.open(archive_path, "r:gz") as archive:
                member = archive.getmember(dataset["archive_member"])
                source = archive.extractfile(member)
                if source is None:
                    raise EvaluationError(
                        "Stanford Bunny archive member is not a file"
                    )
                with temporary.open("wb") as output:
                    shutil.copyfileobj(source, output)
        except (OSError, KeyError, tarfile.TarError) as error:
            temporary.unlink(missing_ok=True)
            raise EvaluationError(f"Failed to extract Stanford Bunny: {error}") from error
        if sha256(temporary) != dataset["mesh_sha256"]:
            temporary.unlink(missing_ok=True)
            raise EvaluationError("Extracted Stanford Bunny failed SHA-256 validation")
        temporary.replace(mesh_path)

    counts = read_ply_counts(mesh_path)
    if (counts["vertices"] != dataset["expected_vertices"] or
            counts["triangles"] != dataset["expected_triangles"]):
        raise EvaluationError("Stanford Bunny geometry counts are incorrect")
    print(
        f"Stanford Bunny: {display_project_path(mesh_path)} "
        f"({counts['vertices']} vertices, {counts['triangles']} triangles)"
    )
    return mesh_path


# ==================================================
# Bunny Regression Reference
# ==================================================

def prepare_bunny_reference(
    dataset: Dict[str, Any], mesh_path: Path, force: bool,
    skip_build: bool
) -> None:
    '''High-SPP Nearlighter regression baseline for the official Mesh.'''
    reference_path = resolve_project_path(dataset["reference_path"])
    preview_path = resolve_project_path(dataset["preview_path"])
    settings = validate_render_settings(dataset["reference_render"])
    if reference_path.is_file() and preview_path.is_file() and not force:
        reference = read_pfm(reference_path)
        if (reference.width != settings["width"] or
                reference.height != settings["height"]):
            raise EvaluationError("Stanford Bunny reference dimensions are incorrect")
        print(f"Bunny reference: {display_project_path(reference_path)}")
        return

    executable = prepare_build(
        resolve_project_path("build/release"), "Release", LOGS_DIRECTORY,
        skip_build, True,
    )
    reference_path.parent.mkdir(parents=True, exist_ok=True)
    temporary_reference = reference_path.with_name(reference_path.name + ".tmp")
    temporary_preview = preview_path.with_name(preview_path.name + ".tmp")
    run_logged(
        render_command(
            executable, BUNNY_SCENE, settings,
            temporary_preview, temporary_reference, show_progress=True,
        ),
        LOGS_DIRECTORY / "stanford_bunny_reference.log",
    )
    reference = read_pfm(temporary_reference)
    if (reference.width != settings["width"] or
            reference.height != settings["height"]):
        raise EvaluationError(
            "Generated Stanford Bunny reference dimensions are incorrect"
        )
    temporary_reference.replace(reference_path)
    temporary_preview.replace(preview_path)
    write_json(
        reference_path.parent / "manifest.json",
        {
            "schema_version": 1,
            "reference_id": reference_path.parent.name,
            "reference_kind": "nearlighter_regression_baseline",
            "created_at": utc_text(),
            "scene": display_project_path(BUNNY_SCENE),
            "source_mesh": {
                "path": display_project_path(mesh_path),
                "sha256": sha256(mesh_path),
                "attribution": dataset["attribution"],
                "terms": dataset["terms"],
            },
            "render_settings": settings,
            "image": {
                "path": reference_path.name,
                "sha256": sha256(reference_path),
            },
        },
    )
    print(f"Bunny reference: {display_project_path(reference_path)}")


# ==================================================
# Command-line Workflow
# ==================================================

def parse_arguments() -> argparse.Namespace:
    '''Benchmark preparation options.'''
    parser = argparse.ArgumentParser(
        description="Download official benchmark data and prepare references."
    )
    parser.add_argument(
        "--skip-bunny-reference", action="store_true",
        help="download data without building the Nearlighter Bunny baseline",
    )
    parser.add_argument(
        "--skip-build", action="store_true",
        help="reuse the existing Release executable for the Bunny baseline",
    )
    parser.add_argument(
        "--force", action="store_true",
        help="redownload data and replace prepared references",
    )
    return parser.parse_args()


def main() -> int:
    '''Complete explicit benchmark preparation workflow.'''
    arguments = parse_arguments()
    datasets = load_json(CONFIG_PATH)["datasets"]
    prepare_cornell(datasets["cornell_box_synthetic"], arguments.force)
    bunny_dataset = datasets["stanford_bunny"]
    mesh_path = prepare_bunny_mesh(bunny_dataset, arguments.force)
    if not arguments.skip_bunny_reference:
        prepare_bunny_reference(
            bunny_dataset, mesh_path, arguments.force, arguments.skip_build
        )
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except EvaluationError as error:
        print(f"ERROR: {error}", file=sys.stderr)
        sys.exit(1)
