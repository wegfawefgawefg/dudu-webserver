# Dudu Webserver

Dudu version of the tiny C++/Lua/FastAPI webserver comparison.

It uses a local header-only C++ shim in `include/dudu_http.hpp` for the socket server and keeps the app code in Dudu.

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
