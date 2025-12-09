#!/usr/bin/env bash
set -euo pipefail

# scripts/show_coverage.sh
# Usage:
#   ./scripts/show_coverage.sh            # show report for runTests or webserv (whichever exists)
#   ./scripts/show_coverage.sh webserv    # show report for build/webserv
#   ./scripts/show_coverage.sh runTests utils.cpp  # show report for runTests and annotated view of utils.cpp

BUILD_DIR="build"
# Prefer build/coverage.profdata, but fall back to coverage.profdata at repo root
if [ -f "$BUILD_DIR/coverage.profdata" ]; then
  PROFDATA="$BUILD_DIR/coverage.profdata"
elif [ -f "coverage.profdata" ]; then
  PROFDATA="coverage.profdata"
else
  PROFDATA="$BUILD_DIR/coverage.profdata"
fi

if [ ! -f "$PROFDATA" ]; then
  echo "Error: $PROFDATA not found. Run the coverage pipeline first (build/tests to produce .profraw then llvm-profdata merge)."
  exit 1
fi

BINARY_ARG="${1:-}"
FILE_ARG="${2:-}"

show_report() {
  local binpath="$1"
  echo "\n== Coverage report for: $binpath =="
  llvm-cov report "$binpath" -instr-profile="$PROFDATA" || return 0
  if [ -n "$FILE_ARG" ]; then
    echo "\n== Annotated source: $FILE_ARG =="
    llvm-cov show "$binpath" -instr-profile="$PROFDATA" "$FILE_ARG" -format=text || true
  fi
}

if [ -n "$BINARY_ARG" ]; then
  BINPATH="$BUILD_DIR/$BINARY_ARG"
  if [ ! -f "$BINPATH" ]; then
    echo "Error: binary '$BINPATH' not found."
    exit 1
  fi
  show_report "$BINPATH"
  exit 0
fi

# No binary specified: prefer runTests, then webserv
if [ -f "$BUILD_DIR/runTests" ]; then
  show_report "$BUILD_DIR/runTests"
  exit 0
fi

if [ -f "$BUILD_DIR/webserv" ]; then
  show_report "$BUILD_DIR/webserv"
  exit 0
fi

# Also accept binaries at the repository root (some builds produce top-level binaries)
if [ -f "./runTests" ]; then
  show_report "./runTests"
  exit 0
fi

if [ -f "./webserv" ]; then
  show_report "./webserv"
  exit 0
fi

echo "No binaries found in $BUILD_DIR. Provide a binary name as the first argument."
exit 1
