#!/bin/sh
exec mpiexec --allow-run-as-root -np $(nproc) "$@"
