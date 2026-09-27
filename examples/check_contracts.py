#!/usr/bin/env python3
"""Check gallery parity and pedagogical logic budgets (standard library only)."""

from __future__ import annotations

import re
import sys
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parent
MANIFEST = ROOT / "Common" / "ExampleManifest.hpp"
OBJECTIVE = ROOT.parent / "OBJECTIF.md"

RECORD = re.compile(
    r'^\s*X\((\w+), "([^"]+)", "([^"]+)", "([^"]+)", '
    r'"([^"]+)", (\d+), "([^"]+)"\)\s*\\?\s*$'
)


@dataclass(frozen=True)
class Entry:
    type_name: str
    name: str
    target: str
    source: str
    former: str
    budget: int
    scope: str


def fail(problems: list[str], message: str) -> None:
    problems.append(message)


def entries() -> list[Entry]:
    result = []
    for line in MANIFEST.read_text(encoding="utf-8").splitlines():
        match = RECORD.match(line)
        if match:
            result.append(
                Entry(
                    match.group(1),
                    match.group(2),
                    match.group(3),
                    match.group(4),
                    match.group(5),
                    int(match.group(6)),
                    match.group(7),
                )
            )
    return result


def without_comments_and_strings(text: str) -> str:
    """Blank comments and literals while preserving newlines and punctuation."""
    pattern = re.compile(
        r'R"(?P<tag>[A-Za-z0-9_]*)\(.*?\)(?P=tag)"'
        r'|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\''
        r"|//[^\n]*|/\*.*?\*/",
        re.DOTALL,
    )

    def blank(match: re.Match[str]) -> str:
        return "\n" * match.group(0).count("\n")

    return pattern.sub(blank, text)


def function_body(text: str, qualified_name: str) -> str:
    start = text.find(qualified_name)
    if start < 0:
        raise ValueError(f"fonction budgétée introuvable: {qualified_name}")
    opening = text.find("{", start)
    if opening < 0:
        raise ValueError(f"corps introuvable: {qualified_name}")
    depth = 1
    cursor = opening + 1
    while cursor < len(text) and depth:
        depth += (text[cursor] == "{") - (text[cursor] == "}")
        cursor += 1
    if depth:
        raise ValueError(f"accolades non équilibrées: {qualified_name}")
    return text[opening + 1 : cursor - 1]


def logic_count(source: Path, scope: str) -> int:
    clean = without_comments_and_strings(source.read_text(encoding="utf-8"))
    count = 0
    controls = re.compile(r"^\s*(if|else\s+if|for|while|switch|case|catch)\b")
    for name in scope.split("|"):
        body = function_body(clean, name)
        for line in body.splitlines():
            stripped = line.strip()
            if not stripped or stripped.startswith("#"):
                continue
            if controls.match(stripped):
                count += 1
                if stripped.startswith("for"):
                    continue
            count += line.count(";")
    return count


def main() -> int:
    problems: list[str] = []
    manifest = entries()
    if not manifest:
        fail(problems, "le manifeste ne contient aucune entrée")

    names = [entry.name for entry in manifest]
    duplicates = sorted({name for name in names if names.count(name) > 1})
    if duplicates:
        fail(problems, f"noms dupliqués: {', '.join(duplicates)}")
    if names != sorted(names):
        fail(problems, "les noms de galerie ne sont pas dans l'ordre")

    declared_sources = set()
    for entry in manifest:
        source = ROOT / entry.source
        declared_sources.add(source.resolve())
        if not source.is_file():
            fail(problems, f"{entry.name}: source absent: {entry.source}")
        elif source.stem != entry.name:
            fail(
                problems,
                f"{entry.name}: le nom ne correspond pas au source {entry.source}",
            )

    active_sources = {
        path.resolve()
        for path in ROOT.glob("*/*.cpp")
        if path.parent.name not in {"Common", "legacy"}
    }
    for source in sorted(active_sources - declared_sources):
        fail(problems, f"source active absente du manifeste: {source.relative_to(ROOT)}")
    for source in sorted(declared_sources - active_sources):
        fail(problems, f"source manifeste non active: {source.relative_to(ROOT)}")

    objective = OBJECTIVE.read_text(encoding="utf-8")
    matrix = re.search(r"<!--\s*parity-matrix:\s*(.*?)\s*-->", objective, re.DOTALL)
    if matrix is None:
        fail(problems, "OBJECTIF.md ne contient pas de parity-matrix")
        required_former: set[str] = set()
    else:
        required_former = {
            item.strip() for item in matrix.group(1).split(",") if item.strip()
        }
    covered = {
        former
        for entry in manifest
        for former in entry.former.split("|")
        if former != "-"
    }
    missing = sorted(required_former - covered)
    if missing:
        fail(problems, f"anciennes démos sans cible: {', '.join(missing)}")

    measured: list[tuple[str, int, int, str]] = []
    for entry in manifest:
        if entry.budget == 0:
            continue
        try:
            count = logic_count(ROOT / entry.source, entry.scope)
        except ValueError as error:
            fail(problems, f"{entry.target}: {error}")
            continue
        measured.append((entry.target, count, entry.budget, entry.name))
        if count > entry.budget:
            fail(
                problems,
                f"{entry.target}: {count} lignes de logique, budget {entry.budget}",
            )

    for target, count, budget, name in measured:
        label = "MVP frame gate" if name == "52_MvpDemo" else target
        print(f"{label}: {count}/{budget} ({name})")

    if problems:
        print("Gallery contract check failed:", file=sys.stderr)
        for problem in problems:
            print(f"  - {problem}", file=sys.stderr)
        return 1
    print(
        f"Gallery parity: {len(manifest)} active sources, "
        f"{len(required_former)} former demos covered"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
