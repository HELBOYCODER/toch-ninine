#!/usr/bin/env python3
"""Validates the factory presets against the shader contract (docs/SHADER_CONTRACT.md).

For every presets/factory/*.frag it checks:
  * a JSON sidecar with matching name, author, version, tags, adsr and params
  * the forbidden-token list (no #version, #include, precision, textures, to-do markers, ...)
  * every for-loop has a literal bound of at most 64 iterations
  * #pragma params are valid and every JSON param matches a declared pragma
  * the injected source compiles as GLSL 150 with glslangValidator

Usage:
  python3 scripts/validate_presets.py --inject build/shaderforge_inject \
      [--glslang glslangValidator] [--platform linux]
Exit status is non-zero if any preset fails. Warnings never pass silently.
"""

import argparse
import json
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
PRESET_DIR = ROOT / "presets" / "factory"
MIN_PRESETS = 12
MAX_LOOP = 64

FORBIDDEN = re.compile(
    r"(#\s*version|#\s*include|\bprecision\b|\btexture\w*\s*\(|\bsampler\w*\b|T[O]DO|Not[I]mplemented)"
)
LOOP_ANY = re.compile(r"\bfor\s*\(")
LOOP_BOUNDED = re.compile(r"for\s*\(\s*int\s+\w+\s*=\s*-?\d+\s*;\s*\w+\s*<=?\s*(\d+)\s*;")
PRAGMA = re.compile(r"^\s*#\s*pragma\s+param\s+(\S+)\s+(\S+)\s+(\S+)\s+(\S+)\s*$", re.M)


class Failure(Exception):
    pass


def check_json(frag: pathlib.Path, source: str) -> None:
    sidecar = frag.with_suffix(".json")
    if not sidecar.exists():
        raise Failure("missing JSON sidecar")
    doc = json.loads(sidecar.read_text(encoding="utf-8"))

    if doc.get("name") != frag.stem:
        raise Failure('"name" must equal the file stem')
    if doc.get("author") != "factory":
        raise Failure('"author" must be "factory"')
    if doc.get("version") != 1:
        raise Failure('"version" must be 1')
    if not isinstance(doc.get("tags"), list) or not doc["tags"]:
        raise Failure('"tags" must be a non-empty list')

    adsr = doc.get("adsr")
    if not isinstance(adsr, list) or len(adsr) not in (1, 8):
        raise Failure('"adsr" must hold one entry (all tracks) or eight (one per track)')
    for entry in adsr:
        if not (isinstance(entry, list) and len(entry) == 4):
            raise Failure('each "adsr" entry must be [attack, decay, sustain, release]')
        attack, decay, sustain, release = entry
        if min(attack, decay, release) < 0 or not (0.0 <= sustain <= 1.0):
            raise Failure('"adsr" values out of range')

    declared = {}
    for match in PRAGMA.finditer(source):
        name, lo, hi, default = match.group(1), float(match.group(2)), float(match.group(3)), float(match.group(4))
        declared[name] = (lo, hi, default)

    params = doc.get("params", {})
    if not isinstance(params, dict):
        raise Failure('"params" must be an object')
    for key, value in params.items():
        if key not in declared:
            raise Failure(f'param "{key}" is not declared with #pragma param')
        lo, hi, _ = declared[key]
        if not (lo <= float(value) <= hi):
            raise Failure(f'param "{key}" = {value} is outside [{lo}, {hi}]')


def check_contract(source: str) -> None:
    match = FORBIDDEN.search(source)
    if match:
        raise Failure(f"forbidden token in shader: {match.group(0)!r}")

    loops = len(LOOP_ANY.findall(source))
    bounded = 0
    for match in LOOP_BOUNDED.finditer(source):
        bounded += 1
        if int(match.group(1)) > MAX_LOOP:
            raise Failure(f"loop bound {match.group(1)} exceeds {MAX_LOOP}")
    if loops != bounded:
        raise Failure("every for-loop must be 'for (int i = <int>; i <[=] <literal>; ...)'")

    if "fragColor =" not in source:
        raise Failure("fragColor is never assigned")
    if "clamp(" not in source:
        raise Failure("output is not clamped to 0..1")

    pragmas = PRAGMA.findall(source)
    if len(pragmas) > 8:
        raise Failure("more than 8 #pragma params")


def check_compile(frag: pathlib.Path, inject: pathlib.Path, glslang: str, platform: str,
                  workdir: pathlib.Path) -> None:
    injected = subprocess.run(
        [str(inject), str(frag), "--platform", platform],
        capture_output=True, text=True,
    )
    if injected.returncode != 0:
        raise Failure(f"preprocessor rejected the shader: {injected.stderr.strip()}")

    final = workdir / (frag.stem + ".frag")
    final.write_text(injected.stdout, encoding="utf-8")
    result = subprocess.run(
        [glslang, "-S", "frag", str(final)],
        capture_output=True, text=True,
    )
    if result.returncode != 0:
        raise Failure("glslangValidator:\n" + (result.stdout + result.stderr).strip())


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("--inject", required=True, type=pathlib.Path,
                        help="path to the shaderforge_inject binary")
    parser.add_argument("--glslang", default="glslangValidator",
                        help="glslangValidator executable (default: from PATH)")
    parser.add_argument("--platform", default="linux", choices=["linux", "macos", "windows"])
    args = parser.parse_args()

    glslang = shutil.which(args.glslang) or args.glslang
    if shutil.which(glslang) is None and not pathlib.Path(glslang).exists():
        print(f"error: {args.glslang} not found", file=sys.stderr)
        return 1

    frags = sorted(PRESET_DIR.glob("*.frag"))
    if len(frags) < MIN_PRESETS:
        print(f"error: expected at least {MIN_PRESETS} presets, found {len(frags)}", file=sys.stderr)
        return 1

    failures = []
    with tempfile.TemporaryDirectory(prefix="sf_presets_") as tmp:
        workdir = pathlib.Path(tmp)
        for frag in frags:
            try:
                source = frag.read_text(encoding="utf-8")
                check_contract(source)
                check_json(frag, source)
                check_compile(frag, args.inject, glslang, args.platform, workdir)
                print(f"ok    {frag.name}")
            except Failure as error:
                failures.append((frag.name, str(error)))
                print(f"FAIL  {frag.name}: {error}")

    print(f"\n{len(frags) - len(failures)}/{len(frags)} presets passed")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
