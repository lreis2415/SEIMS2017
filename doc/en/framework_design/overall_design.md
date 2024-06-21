Overall design of SEIMS {#overall_design}
======================================================

[TOC]

The overall architecture of SEIMS was designed as shown in Figure 1, and consists mainly of the SEIMS module library based on the modular structure, the SEIMS main programs (i.e., OpenMP version and MPI&OpenMP version), the watershed database, and utility tools for watershed model applications (such as parameter sensitivity analysis tool, and auto-calibration tool). The parallel computing middleware at multiple levels is implemented at the basic-unit level in SEIMS modules, the subbasin level in the MPI&OpenMP version of SEIMS main program, and the model level in watershed model applications, respectively. A SEIMS-based model consists of one SEIMS main program, several customized SEIMS modules, and the watershed database. Each SEIMS module inherits from the base module class (i.e., `SimulationModule`) with standard and concise interfaces (e.g., `SetData`, `GetData`, and `Execute` functions) and dependents on base modules such as `I/O` module (Figure 1). The basic-unit level parallelization is achieved using OpenMP in the execution function of each SEIMS module. The basic version of SEIMS main program, which is the OpenMP version, is responsible for loading a set of user-configured modules to build and execute a simulation workflow. The MPI&OpenMP version of SEIMS main program uses the subbasin level domain decomposition and task scheduling to create instances of the OpenMP versioned SEIMS main program for each individual subbasins and distribute them among different computing nodes with MPI-based communication, so to achieve subbasin level parallel computing. Watershed model applications that requires numerous model runs (e.g., sensitivity analysis of a watershed model) are parallelized at model level based on job management.

With the support of parallel computing, SEIMS is compatible with common operating systems (such as Windows and Linux) and parallel computing platforms such as personal computers with multi-core CPU (Central Processing Unit) and SMP clusters..



