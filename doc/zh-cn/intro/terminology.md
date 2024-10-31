术语 {#terminology}
==================================

[TOC]

计划中...
**SEIMS主程序** – 可执行程序（MPI&OpenMP版本为 `seims_mpi`，OpenMP版本为 `seims_omp` 或 `seims`，串行版本为seims）将读取所有配置文件，加载所有配置模块和输入数据（包括气候、空间数据和管理数据等），并输出指定的输出结果。

**SEIMS模块** – 动态链接库文件（即Windows上的 `.dll`, Unix-like系统上的`.so`，或macOS上的 `.dylib`），该文件遵循SEIMS模块API，可由SEIMS主程序加载。一个SEIMS模块对应一个流域子过程。 

**基于SEIMS的流域模型** – 基于SEIMS的流域模型由一个版本的SEIMS主程序（即OpenMP版本或MPI&OpenMP版本），多个定制的SEIMS模块和流域数据库组成。基于SEIMS的流域模型以一个文件夹（如 `demo_youwuzhen30m_longterm_model`）的形式编制，该文件夹由多个SEIMS配置文件组成，如`config.fig`文件，其用于定义所选的SEIMS模块和流域模型模拟过程中的执行命令。
