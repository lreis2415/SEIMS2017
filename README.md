
# SEIMS

## 1.Brief introduction

**SEIMS**, shorted for **Spatially Explicit Integrated Modeling System**, is an integrated, parallized, distributed, and continuous **Wa**tershed modeling and **S**cenario **A**nalysis.

SEIMS is written by **C++** with support of OpenMP and MPI. **Python** is used for organizing the preprocessing and postprocessing workflow. The basic programming framework makes it possible for cross-platform, such as Windows, Linux, and macOS.

SEIMS contains several module classes, include **Hydrology, Erosion, Nutrient, Plant Growth, BMP Management**. Algorithms of watershed processes are adopted from SWAT, LISEM, WetSpa Extension, DHSVM, CASC2D, etc.

SEIMS is still under development and any constructive feedback will be welcome and appreciate.

## 2.Wiki

More information about SEIMS please refer to [SEIMS Wiki](https://github.com/lreis2415/SEIMS2017/wiki).

## 3.Get Started
### 3.1.Get source code

+ Download or clone from [Github](https://github.com/lreis2415/SEIMS2017/tree/master). A useful Github guidiance can be found [here](https://github.com/lreis2415/SEIMS2017/wiki/Git-guidance).

### 3.2.Compile and Install

+ [Windows](https://github.com/lreis2415/SEIMS2017/wiki/Windows)
+ [Linux](https://github.com/lreis2415/SEIMS2017/wiki/Linux)
+ [macOS](https://github.com/lreis2415/SEIMS2017/wiki/macOS)

### 3.3.Config MongoDB database
+ [Install MongoDB client and start mongo service automatically.](https://github.com/lreis2415/SEIMS2017/wiki/MongoDB-install-and-config)
+ You may need a MongoDB IDE to view and edit data. MongoVUE, Robomongo, or Mongo Explorer for JetBrains (e.g. PyCharm, CLion) are recommended.

### 3.4.Run the Demo data
Demo data is provided in `~/data`. SEIMS workflow can be summerized as five part.
+ a) [Data preparation](https://github.com/lreis2415/SEIMS2017/wiki/Data-preparation) and [Preprocessing for SEIMS](https://github.com/lreis2415/SEIMS2017/wiki/Data-preprocess)
+ b) [Run SEIMS model](https://github.com/lreis2415/SEIMS2017/wiki/Executation-and-calibration)
+ c) [Postprocess, e.g. export hydrograph](https://github.com/lreis2415/SEIMS2017/wiki/result-postprocess)
+ d) Model calibration
+ e) Scenario analysis

### 3.5.Build your own model
Now, you can build you own SEIMS model!


Contact Us
----------
Dr.Junzhi Liu (liujunzhi@njnu.edu.cn)

Liangjun Zhu (zlj@lreis.ac.cn)

*Updated: 2017-2-13*


