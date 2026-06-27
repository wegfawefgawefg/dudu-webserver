# Dudu Webserver

Dudu version of the tiny C++/Lua/FastAPI webserver comparison.

The server is implemented directly in Dudu with native POSIX imports for sockets, polling, time, and process environment access. There are a few `cpp(...)` boundary casts where C APIs want exact C++ pointer spellings, but there is no local C++ webserver shim.

## Build

```bash
./scripts/build.sh
```

## Run

```bash
./scripts/run.sh
```

The scripts default to `/home/vega/.local/bin/dudu`. Override with:

```bash
DUDU=/path/to/dudu ./scripts/run.sh
```

Routes:

```text
/
/health
/time
/echo?name=vega
/routes
```

## Editor

This repo includes `.vscode/settings.json` so VS Code associates `*.dd` with the Dudu language and uses:

```text
/home/vega/.local/bin/duc
```
