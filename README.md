About {#mainpage}
=====================
## SEIMS: A modular and parallelized watershed modeling framework

Copyright (C) 2013-2023 [LREIS](http://www.lreis.ac.cn), [NJNU](http://en.njnu.edu.cn), and [LZU](http://www.lzu.edu.cn). All rights reserved.

* [SEIMS GitHub](https://github.com/lreis2415/SEIMS)
* SEIMS Documentations
  * [Archieved PDF version in English](SEIMS-UserManual.pdf)
  * Online versions:
  [English version](https://lreis2415.github.io/SEIMS/),
  [简体中文版 (**尚未完成!**)](https://lreis2415.github.io/SEIMS/zh-cn/)

## Build Status

[![Build on Windows using MSVC](https://github.com/lreis2415/SEIMS/actions/workflows/Build_on_Windows.yml/badge.svg)](https://github.com/lreis2415/SEIMS/actions/workflows/Build_on_Windows.yml)
[![Build on Ubuntu using GCC](https://github.com/lreis2415/SEIMS/actions/workflows/Build_on_Linux.yml/badge.svg)](https://github.com/lreis2415/SEIMS/actions/workflows/Build_on_Linux.yml)
[![Build on macOS using AppleClang](https://github.com/lreis2415/SEIMS/actions/workflows/Build_on_macOS.yml/badge.svg)](https://github.com/lreis2415/SEIMS/actions/workflows/Build_on_macOS.yml)

## Brief Introduction

The **Spatially Explicit Integrated Modeling System** (**SEIMS**),
is a lightweight, modular, and parallelized watershed modeling framework,
that focusing on build and perform watershed process models in a plug-and-play way, and
conduct scenario optimization of watershed best management practices (BMPs).

SEIMS is implemented using standard C++ and Python to be cross-platform compatible.
SEIMS uses [CMake](https://cmake.org) to manage the entire project for compatibility
on mainstream compilation environments.
The compiled C++ programs include the SEIMS main programs (the OpenMP version and the
MPI&OpenMP version), SEIMS module library (i.e., dynamic/shared libraries), and
executable programs for data preprocessing.
Python is used for utility tools including data preprocessing, calibration,
sensitivity analysis, scenario analysis, and so on.

SEIMS contains several module categories, include
**Hydrology, Erosion, Nutrient, Plant Growth, BMP Management**, etc.
Algorithms are integrated from SWAT, LISEM, WetSpa Extension, DHSVM, CASC2D, etc.

SEIMS is still being developing and any constructive feedback
(issues or push requests) will be welcome and appreciated.

## Installation
Users are highly recommended to take a look at the automatic workflow of installation and testing of SEIMS on Windows, Linux, and macOS through GitHub actions. The configuration yml scripts are located in `SEIMS/.github/workflows`. 
The detailed instruction for installing SEIMS can be found [here](https://lreis2415.github.io/SEIMS/download_installation.html).

## Selected peer-reviewed papers

### Watershed modeling framework

+ Zhu, L.-J., Liu, J., Qin, C.-Z., Zhu, A-X., **2019**.
[A modular and parallelized watershed modeling framework](http://www.sciencedirect.com/science/article/pii/S1364815218309241).
_Environmental Modelling & Software_ 122, 104526.
+ Liu, J., Zhu, A-X., Qin, C.-Z., Wu, H., Jiang, J., **2016**.
[A two-level parallelization method for distributed hydrological models](http://dx.doi.org/10.1016/j.envsoft.2016.02.032).
_Environmental Modelling & Software_ 80: 175–184.
+ Liu, J., Zhu, A-X., Liu, Y., Zhu, T., Qin, C.-Z., **2014**.
[A layered approach to parallel computing for spatially distributed hydrological modeling](http://dx.doi.org/10.1016/j.envsoft.2013.10.005).
_Environmental Modelling & Software_ 51: 221–227.
+ Liu, J., Zhu, A-X., Qin, C.-Z., **2013**.
[Estimation of theoretical maximum speedup ratio for parallel computing of grid-based distributed hydrological models](https://doi.org/10.1016/j.cageo.2013.04.030).
_Computers & Geosciences_ 60: 58–62.

### Scenario optimization of BMPs

+ Shen, S., Qin, C.-Z., Zhu, L.-J., Zhu, A-X. **2023**. 
[Optimizing the implementation plan of watershed best management practices with time-varying effectiveness under stepwise investment](https://doi.org/10.1029/2022WR032986). 
_Water Resources Research_ 59(6), e2022WR032986.
+ Zhu, L.-J., Qin, C.-Z., and Zhu, A-X. **2021**.
[Spatial Optimization of Watershed Best Management Practice Scenarios Based on Boundary-Adaptive Configuration Units](https://doi.org/10.1177/0309133320939002).
_Progress in Physical Geography: Earth and Environment_ 45(2): 207–227.
+ Zhu, L.-J., Qin, C.-Z., Zhu, A-X., Liu, J., Wu, H., **2019**.
[Effects of different spatial configuration units for the spatial optimization of watershed best management practice scenarios](https://doi.org/10.3390/w11020262).
_Water_ 11(2), 262.
+ Qin, C.-Z., Gao, H.-R., Zhu, L.-J., Zhu, A-X., Liu, J.-Z., Wu, H., **2018**.
[Spatial optimization of watershed best management practices based on slope position units](https://doi.org/10.2489/jswc.73.5.504).
_Journal of Soil and Water Conservation_ 73(5): 504–517.
+ Wu, H., Zhu, A-X., Liu, J., Liu, Y., Jiang, J., **2018**.
[Best Management Practices Optimization at Watershed Scale: Incorporating Spatial Topology among Fields](https://doi.org/10.1007/s11269-017-1801-8).
_Water Resource Management_ 32: 155–177.


### Data sets

+ Liu, J., Fang, P., Que, Y., Zhu, L.-J., Duan, Z., Tang, G., Liu, P., Ji, M., Liu, Y., **2022**. [A dataset of lake-catchment characteristics for the Tibetan Plateau](https://doi.org/10.5194/essd-14-3791-2022). Earth Syst. Sci. Data, 14: 3791–3805.


## Support

SEIMS is an open source software. Support is provided through the Github issues and Email of present developers.

+ Issues: https://github.com/lreis2415/SEIMS/issues
+ Emails of present developers:
  + Dr. Liang-Jun Zhu (zlj@lreis.ac.cn)
  + Prof. Junzhi Liu (liujunzhi@lzu.edu.cn)

