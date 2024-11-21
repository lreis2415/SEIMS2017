What is SEIMS? {#whatis_seims}
==================================

[TOC]

SEIMS (Spatially Explicit Integrated Modeling System) is an integrated, modular, parallelized, distributed watershed modeling system,
which is also designed for scenario analysis, especially for evaluating and optimizing BMP scenarios.
SEIMS can be used to build a specific watershed model of a single watershed for long-term or storm-event simulation.
Multiple watershed processes could be considered, e.g., hydrological processes, soil erosion, nutrient cycling, and plant growth.

+ SEIMS is a distributed watershed modeling system, in which grid cells with hydrological connection are used as basic simulation unitswithin each subbasin. Support for using irregularly shaped fields, such as hydrologic response units (*[Arnold et al., 1998][arnold_1998_jawra]*) and patches (*[Tague and Band, 2004][tague_band_2004_earthinteractions]*), as basic simulation units is under development.
+ SEIMS is a multiple watershed processes integrated modular system, to which model developers could easily add their own modules (algorithms) of hydrology, soil erosion, nutrient cycling, plant growth, and BMP management, etc.
+ SEIMS is a parallelized modeling system, which takes fully use of the parallelizability at both the subbasin level and the basic simulation unit level (e.g., grid cell) simultaneously. Developers can easily implement parallelized watershed model in a nearly serial programming manner.
+ SEIMS is a configurable watershed modeling system, in which users can easily specify their modules, and outputs.
+ SEIMS is developed by C++ with the support of GDAL, mongo-c-driver, OpenMP and MPI libraries. Python is used to organize various workflows as utility tools, e.g., preprocessing, post-processing, parameter sensitivity analysis, auto-calibration, and scenario analysis. SEIMS can be compiled by commonly used C++ compiler (e.g., MSVC 2010+, GCC4.6+, Intel C++ 12+, and Clang 8.0+) and run on multiple parallel computing platforms (e.g., multi-core computer, and symmetric multiprocessing (SMP) clusters).
+ SEIMS, which uses several open-source technologies (e.g., GDAL, mongo-c-driver), is also open-sourced at Github (https://github.com/lreis2415/SEIMS).

## Key procedures

The following points are commonly used procedures to build a watershed model and perform scenario analysis based on SEIMS. The details of each procedure will be elaborated in [2 Get started](@ref GET_STARTED).

+ [Install the prerequisite software and libraries, and then install SEIMS](@ref getstart_download_installation)
+ [Prepare data (climate, spatial, and observations, etc.) of the study area](@ref getstart_data_preparation)
+ [Build database for SEIMS-based model by data preprocessing scripts](@ref getstart_data_preprocessing)
+ [Set up a SEIMS-based model according to model objectives and run model](@ref getstart_run_seims_model)
+ [Postprocessing, e.g., analyze, plot and graph outputs](@ref getstart_runmodel_postprocessing)
+ (Optional) [Parameters sensitivity analysis](@ref getstart_parameters_sensitivity)
+ (Optional) [Automatic calibration](@ref getstart_autocalibration)
+ (Optional) [Scenario analysis, e.g., BMP scenarios optimization for reducing soil erosion](@ref getstart_bmp_scenario_analysis)

[arnold_1998_jawra]: https://doi.org/10.1111/j.1752-1688.1998.tb05961.x "SWAT Paper"
[tague_band_2004_earthinteractions]: https://doi.org/10.1175/1087-3562(2004)8%3C1:RRHSSO%3E2.0.CO;2 "RHESSys Paper"