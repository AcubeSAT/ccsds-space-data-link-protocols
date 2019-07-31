#!/usr/bin/env bash

#
# Static code analysis using cppcheck
#
# Usage:
# $ ci/cppcheck.sh
#
set -e # Exit if any of the commands fail to execute

echo -e "\u001b[34;1mStarting cppcheck...\u001b[0m"

echo -e "\u001b[34;1mRunning cppcheck with default checklist...\u001b[0m"

cd "$(dirname "$0")/.."
cppcheck --enable=all --suppress=unusedFunction --error-exitcode=1 -I inc src test
