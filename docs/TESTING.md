# Testing

## FORGE

```bash
cmake -S forge -B build/forge -DCMAKE_BUILD_TYPE=Release
cmake --build build/forge --clean-first --parallel 2
ctest --test-dir build/forge --output-on-failure
bash forge/tests/api_smoke.sh
```

Tests are in `forge/tests/` and use temporary directories for storage scenarios.

## EMBER

```bash
cd backend
uv sync --group dev
uv run ruff check .
uv run pytest -q
```

## POSTO

```bash
cd ember/posto-—-elegant-mail-client
npm ci
npm run lint
npm run build
```

## Docker

```bash
docker compose config
docker compose up --build
```

## Verification discipline

An implementation or test file being committed is not evidence that it passes. Record the exact command, environment, and result after running it. The format-manifest, malformed-string, BCC-visibility, pagination, Docker, and full-stack CI additions in the current development branch have not yet been run in this environment.
