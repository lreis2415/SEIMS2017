#!/usr/bin/env bash
git subtree push --prefix=cmake cmake master
git subtree push --prefix=seims/src/taudem taudem master
git subtree push --prefix=seims/src/metis metis master
git subtree push --prefix=seims/src/ccgl ccgl master
