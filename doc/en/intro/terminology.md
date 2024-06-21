Terminology {#terminology}
==================================

[TOC]

**SEIMS Main Program** – The executable program (`seims_mpi` for MPI&OpenMP version, `seims_omp` or `seims` for OpenMP version or seims for serial version) which would read all configuration files, load all configured modules and input data (includes climate, spatial data, and management data, etc.), and outputs specified outputs.

**SEIMS Module** – A dynamic link library file (i.e., `.dll` on Windows, `.so` on Unix-like systems, or `.dylib` on macOS) file which follows SEIMS module APIs and could be loaded by SEIMS main program. A SEIMS module is corresponding to one watershed subprocess. 

**SEIMS-based watershed model** – A SEIMS-based watershed model consists of one version of SEIMS main program (i.e., OpenMP version or MPI&OpenMP version), several customized SEIMS modules, and the watershed database. A SEIMS-based watershed model is prepared as a folder (e.g., `demo_youwuzhen30m_longterm_model`) which consists of several SEIMS configuration files, e.g., the `config.fig` file designed to define the selected SEIMS modules and the execution orders during the simulation of the watershed model.
