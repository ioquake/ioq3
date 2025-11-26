#!/usr/bin/env python3
"""Compare Metal shader parser output against the legacy OpenGL2 renderer."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path
from typing import Dict, List, Sequence

DEFAULT_FILTER = "textures/test/*"

RGB_GEN_MAP = {
    0: "bad",
    1: "identitylighting",
    2: "identity",
    3: "entity",
    4: "oneminusentity",
    5: "vertex",
    6: "vertex",
    7: "vertex",
    8: "vertex",
    9: "oneminusvertex",
    10: "wave",
    11: "lightingdiffuse",
    12: "fog",
    13: "const",
}

ALPHA_GEN_MAP = {
    0: "identity",
    1: "skip",
    2: "entity",
    3: "oneminusentity",
    4: "vertex",
    5: "oneminusvertex",
    6: "lightingspecular",
    7: "wave",
    8: "portal",
    9: "const",
}

StageSummary = Dict[str, object]
ShaderSummary = Dict[str, object]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run shader_parser for GL2 and Metal backends and compare the results.")
    parser.add_argument("--shader-parser", required=True,
                        help="Path to the shader_parser executable")
    parser.add_argument("--scripts", action="append", required=True,
                        help="Script directory to scan (repeatable)")
    parser.add_argument("--filters", action="append", default=None,
                        help="Shader name glob filters (repeatable)")
    parser.add_argument("--verbose", action="store_true",
                        help="Print verbose diff output")
    args = parser.parse_args()
    if not args.filters:
        args.filters = [DEFAULT_FILTER]
    return args


def _normalize_path(path: str) -> str:
    if not path:
        return ""
    trimmed = path.strip()
    if not trimmed:
        return ""
    return trimmed.replace("\\", "/").lower()


def _first_json_block(payload: str) -> str:
    start = payload.find('{')
    if start == -1:
        raise ValueError("shader_parser output did not contain JSON data")
    return payload[start:]


def _run_shader_parser(binary: Path, scripts: Sequence[str], filters: Sequence[str], backend: str) -> Dict:
    args: List[str] = [str(binary), "--backend", backend, "--output", "-"]
    for directory in scripts:
        args.extend(["--scripts", directory])
    for pattern in filters:
        args.extend(["--filter", pattern])

    result = subprocess.run(args, check=False, capture_output=True, text=True)
    if result.returncode != 0:
        sys.stderr.write(result.stderr)
        raise RuntimeError(f"shader_parser ({backend}) failed with exit code {result.returncode}")

    json_payload = _first_json_block(result.stdout)
    return json.loads(json_payload)


def _canonicalize_metal(document: Dict) -> Dict[str, ShaderSummary]:
    canonical: Dict[str, ShaderSummary] = {}
    for shader in document.get("shaders", []):
        name = shader.get("name", "").lower()
        stages: List[StageSummary] = []
        for stage in shader.get("stages", []):
            stages.append({
                "images": tuple(_normalize_path(p) for p in stage.get("images", [])),
                "usesLightmap": bool(stage.get("usesLightmap", False)),
                "srcBlend": stage.get("srcBlend"),
                "dstBlend": stage.get("dstBlend"),
                "depthWrite": bool(stage.get("depthWrite", False)),
                "alphaFunc": int(stage.get("alphaFunc", 0)),
                "tcGen": stage.get("tcGen", "texture"),
                "tcModCount": int(stage.get("tcMods", 0)),
                "rgbGen": str(stage.get("rgbGen", "identity")).lower(),
                "alphaGen": str(stage.get("alphaGen", "identity")).lower(),
            })
        canonical[name] = {
            "stageCount": int(shader.get("stageCount", len(stages))),
            "stages": stages,
        }
    return canonical


def _map_rgb_gen(value: int) -> str:
    return RGB_GEN_MAP.get(value, f"unknown_{value}")


def _map_alpha_gen(value: int) -> str:
    return ALPHA_GEN_MAP.get(value, f"unknown_{value}")


def _canonicalize_gl(document: Dict) -> Dict[str, ShaderSummary]:
    canonical: Dict[str, ShaderSummary] = {}
    for shader in document.get("shaders", []):
        name = shader.get("name", "").lower()
        stages: List[StageSummary] = []
        for stage in shader.get("stages", []):
            images: List[str] = []
            uses_lightmap = bool(stage.get("usesLightmap", False))
            for bundle in stage.get("bundles", []):
                slot = bundle.get("slot")
                bundle_images = [
                    _normalize_path(img) for img in bundle.get("images", []) if img
                ]
                if slot == 0 and bundle_images:
                    images = bundle_images
                if slot == 1 and bundle_images:
                    uses_lightmap = True
            stages.append({
                "images": tuple(images),
                "usesLightmap": uses_lightmap,
                "srcBlend": stage.get("srcBlend", "GL_ONE"),
                "dstBlend": stage.get("dstBlend", "GL_ZERO"),
                "depthWrite": bool(stage.get("depthWrite", False)),
                "alphaFunc": int(stage.get("alphaFunc", 0)),
                "tcGen": stage.get("tcGen", "texture"),
                "tcModCount": int(stage.get("tcModCount", 0)),
                "rgbGen": _map_rgb_gen(int(stage.get("rgbGen", 0))),
                "alphaGen": _map_alpha_gen(int(stage.get("alphaGen", 0))),
            })
        canonical[name] = {
            "stageCount": int(shader.get("numStages", len(stages))),
            "stages": stages,
        }
    return canonical


def _compare_shaders(gl_shaders: Dict[str, ShaderSummary],
                     metal_shaders: Dict[str, ShaderSummary],
                     verbose: bool) -> int:
    failures = 0
    missing_in_gl = sorted(set(metal_shaders.keys()) - set(gl_shaders.keys()))
    missing_in_metal = sorted(set(gl_shaders.keys()) - set(metal_shaders.keys()))
    for name in missing_in_gl:
        failures += 1
        print(f"[FAIL] Missing GL2 shader: {name}")
    for name in missing_in_metal:
        failures += 1
        print(f"[FAIL] Missing Metal shader: {name}")

    for name in sorted(set(gl_shaders.keys()) & set(metal_shaders.keys())):
        gl_shader = gl_shaders[name]
        metal_shader = metal_shaders[name]
        if gl_shader["stageCount"] != metal_shader["stageCount"]:
            failures += 1
            print(f"[FAIL] {name}: stage count {gl_shader['stageCount']} != {metal_shader['stageCount']}")
            continue

        for idx, (gl_stage, metal_stage) in enumerate(zip(gl_shader["stages"], metal_shader["stages"])):
            for field in ("images", "usesLightmap", "srcBlend", "dstBlend", "depthWrite",
                          "alphaFunc", "tcGen", "tcModCount", "rgbGen", "alphaGen"):
                if gl_stage.get(field) != metal_stage.get(field):
                    failures += 1
                    if verbose:
                        print(f"[FAIL] {name} stage {idx} field '{field}':"
                              f" gl={gl_stage.get(field)} metal={metal_stage.get(field)}")
                    else:
                        print(f"[FAIL] {name} stage {idx} field '{field}' differs")
    if failures == 0:
        print("[PASS] Metal parser matches GL2 for selected shaders.")
    return failures


def main() -> int:
    args = parse_args()
    binary = Path(args.shader_parser).resolve()
    script_dirs = [str(Path(entry).resolve()) for entry in args.scripts]

    try:
        gl_doc = _run_shader_parser(binary, script_dirs, args.filters, "gl2")
        metal_doc = _run_shader_parser(binary, script_dirs, args.filters, "metal")
    except Exception as err:  # pylint: disable=broad-except
        print(f"Error while running shader_parser: {err}", file=sys.stderr)
        return 1

    gl_shaders = _canonicalize_gl(gl_doc)
    metal_shaders = _canonicalize_metal(metal_doc)
    failures = _compare_shaders(gl_shaders, metal_shaders, args.verbose)
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
