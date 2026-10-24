from __future__ import annotations

import os
from pathlib import Path


def load_env(path: Path) -> dict[str, str]:
    values = dict(os.environ)
    if path.exists():
        for raw in path.read_text(encoding="utf-8").splitlines():
            line = raw.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, value = line.split("=", 1)
            values.setdefault(key.strip(), os.path.expandvars(os.path.expanduser(value.strip())))
    return values


def require(env: dict[str, str], *names: str) -> None:
    missing = [name for name in names if not env.get(name)]
    if missing:
        raise SystemExit("Missing settings in .env: " + ", ".join(missing))


def render(source: str, env: dict[str, str]) -> str:
    for key, value in env.items():
        source = source.replace(f"@{key}@", value)
    if "@" in source:
        unresolved = sorted({part.split("@", 1)[0] for part in source.split("@")[1::2]})
        if unresolved:
            raise ValueError("Unresolved template values: " + ", ".join(unresolved))
    return source
