# Dudu Webserver

Dudu version of the tiny C++/Lua/FastAPI webserver comparison.

The server is implemented directly in Dudu with native POSIX imports for sockets, polling, time, and process environment access. There are a few `cpp(...)` boundary casts where C APIs want exact C++ pointer spellings, but there is no local C++ webserver shim.

Source layout mirrors the C++ version's shape:

```text
src/main.dd      entry point
src/server.dd    socket setup, accept loop, request read/write
src/app.dd       route handlers
src/request.dd   HTTP request parsing
src/response.dd  HTTP response serialization
```

## Build

```bash
./scripts/build.sh
```

## Run

```bash
./scripts/run.sh
```

The scripts use `dudu` from `PATH`.

Routes:

```text
/
/health
/time
/echo?name=vega
/routes
```

## Editor

This repo includes `.vscode/settings.json` so VS Code associates `*.dd` with
the Dudu language. The extension resolves `dudu` and `dudu-lsp` from `PATH`.
