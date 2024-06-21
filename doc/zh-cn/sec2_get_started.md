快速入门 {#GET_STARTED}
===============================

SEIMS is mainly written by C++ with the support of GDAL (Geospatial Data Abstraction Library, https://www.gdal.org/), mongo-c-driver (https://github.com/mongodb/mongo-c-driver), OpenMP (Open Multi-Processing) and MPI (Message Passing Interface), while Python is used for organizing the utility tools such as data preprocessing, postprocessing, parameter sensitivity analysis, auto-calibration, and BMP (Best Management Practices) scenarios analysis.

SEIMS is designed to be an open-source, cross-platform, and high-performance integrated watershed modeling framework. Theoretically, SEIMS can be compiled by common used C/C++ compiler (e.g. Microsoft Visual C++ 2010+, GCC 4.6+, and Intel C++ 12.0+) as 32-bit or 64-bit programs and run on mainstream Operation Systems (e.g. Windows, Linux, and macOS).

In order to save the length of this manual, the software environments with Windows 10 64bit, Microsoft Visual C++ 2013 (MSVC 2013 for short), and Python 2.7.15 are selected for example. For the demo of parallel computing, a Linux cluster with IBM Platform LSF for workload management is adopted.

Users are encouraged to follow this manual step by step to get started with SEIMS, including download and installation, understanding the data preparation of the demo watershed, preprocessing and running the user-configured SEIMS-based watershed model, postprocessing, parameter sensitivity analysis, auto-calibration, and BMPs scenario analysis, etc.


- @subpage getstart_download_installation
- @subpage getstart_data_preparation
- @subpage getstart_data_preprocessing
- @subpage getstart_run_seims_model
- @subpage getstart_runmodel_postprocessing
- @subpage getstart_parameters_sensitivity
- @subpage getstart_autocalibration
- @subpage getstart_bmp_scenario_analysis
