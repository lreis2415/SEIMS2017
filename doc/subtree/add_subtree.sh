#!/usr/bin/env bash
# cmake
git remote add -f cmake git@github.com:lreis2415/cmake.git
git subtree add --prefix=cmake cmake master --squash
# taudem
git remote add -f taudem git@github.com:lreis2415/TauDEM_ext.git -m master
git subtree add --prefix=seims/src/taudem taudem master --squash
# metis
git remote add -f metis git@github.com:lreis2415/metis.git -m master
git subtree add --prefix=seims/src/metis metis master --squash
# ccgl
git remote add -f ccgl git@github.com:crazyzlj/CCGL.git -m master
git subtree add --prefix=seims/src/ccgl ccgl master --squash
