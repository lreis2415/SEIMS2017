#!/usr/bin/env bash
# cmake
git fetch cmake master
git subtree pull --prefix=cmake cmake master --squash
# taudem
git fetch taudem master
git subtree pull --prefix=seims/src/taudem taudem master --squash
# metis
git fetch metis master
git subtree pull --prefix=seims/src/metis metis master --squash
# ccgl
git fetch ccgl master
git subtree pull --prefix=seims/src/ccgl ccgl master --squash
