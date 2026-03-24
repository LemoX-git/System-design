from __future__ import annotations

from pathlib import Path

import yaml

from app.main import app


if __name__ == "__main__":
    schema = app.openapi()
    output_path = Path(__file__).resolve().parents[1] / "openapi.yaml"
    output_path.write_text(yaml.safe_dump(schema, sort_keys=False, allow_unicode=True), encoding="utf-8")
    print(f"OpenAPI schema written to {output_path}")
