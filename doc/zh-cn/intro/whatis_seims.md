SEIMS是什么? {#whatis_seims}
==================================

[TOC]

SEIMS （Spatially Explicit Integrated Modeling System） 是一个集成式、模块化、并行化、分布式的流域建模系统，
同时设计用于情景分析，特别是用于评估和优化BMP情景。
SEIMS可用于建立单一流域的特定流域模型，以进行针对长期或大量事件的模拟。
可考虑多种流域过程，如水文过程、土壤侵蚀、养分循环和植物生长等。

+ SEIMS是一种分布式流域建模系统，在每个子流域内以具有水文连接的网格单元作为基本模拟单元。支持使用如水文响应单元（*[Arnold等, 1998][arnold_1998_jawra]*）和板块（*[Tague and Band, 2004][tague_band_2004_earthinteractions]*）的不规则形状作为基本模拟单元正在开发中。
+ SEIMS是一种多流域过程集成的模块化系统，模型开发者可以轻松地添加自己的水文、土壤侵蚀、养分循环、植物生长以及BMP管理等模块（算法）。
+ SEIMS是一种并行化建模系统，充分利用了子流域层面和基本模拟单元（如栅格单元）层面同时具有的并行性。开发者可以轻松地以近乎串行的编程方式实现并行化流域建模。
+ SEIMS是一种可配置的流域建模系统，用户可以轻松地指定模块和输出
+ SEIMS采用C++开发，支持GDAL、mongo-c-driver、OpenMP和MPI库。Python用于将各种工作流组织为实用工具，例如预处理、后处理、参数敏感性分析、自动率定和情景分析。SEIMS可以使用常用的C++编译器（如MSVC2010+、GCC4.6+、Intel C++ 12+、and Clang 8.0+）进行编译，并可在多个并行计算平台（如多核计算机和对称多处理（SMP）集群）上运行
+ SEIMS使用了多种开源技术（如GDAL、mongo-c-driver），在Github上也是开源的（ https://github.com/lreis2415/SEIMS ）。

## 关键过程

以下几点是基于SEIMS建立流域模型并进行情景分析的常用步骤。每个过程的细节将在[2 开始](@ref GET_STARTED)中详细阐述。

+ [ 安装必备软件和库，然后安装SEIMS ](@ref getstart_download_installation)
+ [ 准备研究区域的数据（气候数据、空间数据、实际观测数据等） ](@ref getstart_data_preparation)
+ [ 通过数据预处理脚本为基于SEIMS的模型建立数据库 ](@ref getstart_data_preprocessing)
+ [ 根据模型目标创建一个基于SEIMS的模型并运行模型 ](@ref getstart_run_seims_model)
+ [ 后处理，例如分析、绘图和图形输出 ](@ref getstart_runmodel_postprocessing)
+ (可选) [ 参数敏感性分析 ](@ref getstart_parameters_sensitivity)
+ (可选) [ 自动校准 ](@ref getstart_autocalibration)
+ (可选) [ 情景分析，如减少土壤侵蚀的BMP情景优化 ](@ref getstart_bmp_scenario_analysis)

[arnold_1998_jawra]: https://doi.org/10.1111/j.1752-1688.1998.tb05961.x "SWAT Paper"
[tague_band_2004_earthinteractions]: https://doi.org/10.1175/1087-3562(2004)8%3C1:RRHSSO%3E2.0.CO;2 "RHESSys Paper"