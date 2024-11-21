Download and Installation {#getstart_download_installation}
======================================================

[TOC]

SEIMS is hosted on [Github](https://github.com/lreis2415/SEIMS). SEIMS is dependent on several open-source software, e.g., [GDAL](https://www.gdal.org/) and[mongo-c-driver](https://github.com/mongodb/mongo-c-driver). Currently, there are no compiled binaries for distribution, but only through source code. 

Users are encouraged to download and install (means compile from the source code) SEIMS manually. Generally, the `master` branch of SEIMS repository (https://github.com/lreis2415/SEIMS/tree/master) is the relative stable version, while the `dev` branch reflects the latest development changes and now is the default branch. 

Users are highly recommended to take a look at the automatic workflow of installation and testing of SEIMS on Windows, Linux, and macOS through GitHub actions:

[![Build on Windows using MSVC](https://github.com/lreis2415/SEIMS/actions/workflows/Build_on_Windows.yml/badge.svg)](https://github.com/lreis2415/SEIMS/actions/workflows/Build_on_Windows.yml)

[![Build on Ubuntu using GCC](https://github.com/lreis2415/SEIMS/actions/workflows/Build_on_Linux.yml/badge.svg)](https://github.com/lreis2415/SEIMS/actions/workflows/Build_on_Linux.yml)

[![Build on macOS using AppleClang](https://github.com/lreis2415/SEIMS/actions/workflows/Build_on_macOS.yml/badge.svg)](https://github.com/lreis2415/SEIMS/actions/workflows/Build_on_macOS.yml)

The configuration yml scripts are located in `SEIMS/.github/workflows`. 

# Download

Users can use git to clone the repository or download the compressed zip file directly. 
+ Clone the single `master` or `dev` branch using git.
  1. Download and install git, e.g., Windows users can visit https://gitforwindows.org/. 
  2. Open or create a local directory without spaces in File Explorer, e.g., `D:\demo`, right-click in the space and select “Git Bash Here”. Then, enter the following two commands to configure your basic settings of git, i.e., the username and email of your Github account.
  3. If it is the first time to use git in your computer, a SSH key should be created and added to your Github account.

    First, type the following command to generate a SSH key and store to your user directory, e.g., `C:/Users/XXX/.ssh/id_rsa.pub`

    `ssh-keygen -t rsa -C "email@example.com"`

    Then, copy the content of this file and add to your Github account (“Settings” -> “SSH and GPG keys” -> “New SSH key”).
  4. Clone a single branch, e.g., the `master` branch:
    `git clone git@github.com:lreis2415/SEIMS.git --branch master --single-branch`

+ Alternatively, users can download the compressed zip file (e.g., the zip file of the `master` branch, https://github.com/lreis2415/SEIMS/archive/master.zip) directly and then decompress it to a local directory without spaces, e.g., `D:\demo`. 

# Prerequisite software and libraries

Several software and libraries are required to compile and run SEIMS successfully.

1. Software and libraries to compile C/C++ part of SEIMS
    + CMake 3.1+
    + C/C++ Compiler with partial support for C++11, e.g., MSVC 2010+, GCC 4.6+, Intel C++ 12.0+, and Clang 8.0+
    + MPI implementation for C/C++, e.g., MS-MPI v6+, MPICH, OpenMPI or Intel MPI
    + GDAL for C/C++
    + mongo-c-driver 1.16+ for C/C++
2. Software to support running SEIMS
    + MongoDB (community) server 4.2
    + MongoDB GUI
3. Python environment to run SEIMS
    + Python 2.7.x or 3.x (Anaconda or miniconda is highly recommended)
    + third-party Python packages (GDAL, pymongo, scoop, DEAP, PyGeoC, etc.), which can be installed through one-line `conda` command

## Software and libraries to compile C/C++ part of SEIMS

### CMake
CMake 3.1+ is used to build the C/C++ project of SEIMS such as Microsoft Visual Studio solution (`*.sln`). CMake can be downloaded from the official site https://cmake.org/download/. The installer is something like `cmake-3.x.x-win64-x64.msi` for Windows64-x64.
After the installation of CMake, please add the path of CMake executable, e.g., `C:\Program Files (x86)\CMake\bin`, to the system variable `PATH`. In such a way, we can run CMake in CMD (Command Prompt) directly by typing cmake rather than the absolute path.

### C/C++ compiler
#### Microsoft Visual C++ (MSVC)
SEIMS uses several features of C++11 such as `nullptr` and `auto` keywords. Therefore, the minimum support version is MSVC 2010. Microsoft Visual Studio is a powerful IDE based on Microsoft Visual C++. If you don’t want to install Microsoft Visual Studio, the Visual C++ build tools can allow you to build C++ libraries and applications targeting Windows desktop, which are the same tools that you find in Microsoft Visual Studio. Even though, Visual Studio is still highly recommended. 
If you want to develop parallel applications based on MPI, MSVC 2010 is the best choice since it is the last MSVC version that integrated the MPI cluster debugger. MPI cluster debugger is the most convenient and powerful tools on Windows to debug MPI-based parallel applications. More details about MPI cluster debugger can be found in https://docs.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2010/dd560809(v=vs.100). Otherwise, MSVC 2013, 2015, and 2019 are all supported and recommended. All these MSVC versions can be downloaded from https://visualstudio.microsoft.com/vs/older-downloads/. 

#### GCC
GCC is recommended on Linux or macOS.

#### Clang
The macOS has the default variant of Clang, i.e., AppleClang, which excludes OpenMP.
The Clang compiler installed through homebrew is highly recommended on Mac.

#### Intel C++
The Intel C++ compiler can be installed as an addon in Microsoft Visual Studio on Windows.

### MPI for C/C++
#### MS-MPI
Microsoft MPI (MS-MPI) is a Microsoft implementation of the MPI (Message Passing Interface, https://www.mpi-forum.org/) standard for developing and running parallel applications on the Windows platform. MS-MPI v6 and later versions are recommended.
Download MS-MPI (two installer files are required, i.e., `msmpisdk.msi` and `MSMpiSetup.exe`) from Microsoft download center, e.g., https://www.microsoft.com/en-us/download/details.aspx?id=47259 for MS-MPI v6, and install to the default locations. By default, the environment variables of MS-MPI will be set automatically. Open `System->Advanced system settings->Environment variables->System variables` to make sure the following variable-value pairs are existed. If not, please add them manually with the correct paths of your computer.
```batch
MSMPI_BIN=C:\Program Files\Microsoft MPI\Bin\
MSMPI_INC=C:\Program Files (x86)\Microsoft SDKs\MPI\Include\
MSMPI_LIB32=C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x86\
MSMPI_LIB64=C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64\
```

#### MPICH
MPICH can be install through homebrew on Mac.

#### OpenMPI
OpenMPI can be install through homebrew on Mac.

### GDAL for C/C++

#### GISInternals for Windows
The [GISInternals support site](http://www.gisinternals.com/index.html) maintained by Tamas Szekeres is the most famous site that provides compiled GDAL library by MSVC on Windows platform. Users should pick up the exact version according to your compiler and OS architecture, e.g., I have MSVC 2013 (_MSC_VER=1800) installed on Windows 10-64bit and I want GDAL-1.11.4 to compile 64-bit applications, so I should download the `release-1800-x64-gdal-1-11-4-mapserver-6-4-3.zip` and `release-1800-x64-gdal-1-11-4-mapserver-6-4-3-libs.zip` from archived releases (http://www.gisinternals.com/archive.php).

+ Unzip these two zip files to the same directory path without spaces, e.g., `C:/GDAL`.
+ Open Advanced System Properties->Environment variables, create several new system variables under the System variables pane:
    ```batch
    GDAL_DIR=C:\GDAL
    GDAL_DATA=C:\GDAL\bin\gdal-data
    GDAL_PATHS=C:\GDAL;C:\GDAL\bin;C:\GDAL\bin\proj\apps;C:\GDAL\bin\gdal\apps;C:\GDAL\bin\ms\apps;C:\GDAL\bin\curl;
    ```
+ Then, append `%%GDAL_PATHS%` to the end of the system variable `PATH`.
+ The GDAL library for C/C++ has been installed, as well as executable utility tools of GDAL, e.g., `gdalinfo`. Open a new CMD window, and enter 
`gdalinfo --version`, something like `GDAL 1.11.4, released 2016/01/25` should be printed.

#### apt-get for Ubuntu
On Ubuntu, the GDAL can be installed simply by `apt-get` command:
```shell
apt-get update && sudo apt-get install -qq gdal-bin libgdal-dev
```
Be default, the latest GDAL version will be installed.

#### homebrew for macOS
On macOS, the GDAL can be installed simply by `brew` command:
```shell
brew install gdal
```

### mongo-c-driver for C/C++
The mongo-c-driver library for MongoDB (https://github.com/mongodb/mongo-c-driver) is adopted by SEIMS to handle data Input/Output (IO) with MongoDB. Since mongo-c-driver is still under active and continuous development, the building method and API (Application Program Interface) functions may be different with versions. The last version supported by VS2010 is recommended, i.e., `1.16.2`. Any reported bugs caused by version compatible are welcome!

#### Use MSVC on Windows
+ Download the released file and decompress it to a local directory, e.g., `D:\demo\mongo-c-driver-1.16.2`
+ Open `VS201x Developer Command Prompt` and execute the following commands in order. Assume that the desired destination of compiled libraries and headers of mongo-c-driver is `C:\mongo-c-driver-vs201x`.
    ```batch
    REM VS2010:
    cmake -G "Visual Studio 10 2010 Win64" -DENABLE_ZLIB=BUNDLED "-DCMAKE_INSTALL_PREFIX=C:\mongo-c-driver-vs2010" "-DCMAKE_PREFIX_PATH=C:\mongo-c-driver-vs2010" ..
    REM VS2015:
    cmake -G "Visual Studio 14 2015 Win64" -DENABLE_ZLIB=BUNDLED "-DCMAKE_INSTALL_PREFIX=C:\mongo-c-driver-vs2015" "-DCMAKE_PREFIX_PATH=C:\mongo-c-driver-vs2015" ..
    REM VS2019:
    cmake -G "Visual Studio 16 2019" -A x64 -DENABLE_ZLIB=BUNDLED "-DCMAKE_INSTALL_PREFIX=C:\mongo-c-driver-vs2019" "-DCMAKE_PREFIX_PATH=C:\mongo-c-driver-vs2019" ..

    msbuild.exe /p:Configuration=RelWithDebInfo ALL_BUILD.vcxproj
    msbuild.exe INSTALL.vcxproj
    ```

    Notes:
    1. If `msbuild.exe` cannot be found by CMD, please find it in the path of .NetFramework, e.g., `C:\Windows\Microsoft.NET\Framework64\v4.0.30319\msbuild.exe`.
    2. If you want to build the 32-bit mongo-c-driver libraries, please replace the generator "Visual Studio 12 2013 Win64" to "Visual Studio 12 2013".

+ Now, mongo-c-driver has been built and installed, the directories of bin, include, and lib can be found in the root directory of mongo-c-driver, i.e., `C:\mongo-c-driver-vs201x`.
+ Create two new system variables:
    ```batch
    MONGOC_ROOT=C:\mongo-c-driver-vs201x
    MONGOC_LIB=C:\mongo-c-driver-vs201x\bin
    ```
+ Then, append `%%MONGOC_LIB%` to the end of the system variable `PATH`.

#### apt-get for Ubuntu
On Ubuntu, the GDAL can be installed simply by `apt-get` command:
```shell
apt-get update && sudo apt-get install -qq libmongoc-1.0-0 libmongoc-dev
```
Be default, the latest GDAL version will be installed.

#### homebrew for macOS
On macOS, the GDAL can be installed simply by `brew` command:
```shell
brew install mongo-c-driver
```

## Software to support running SEIMS

### MongoDB (community) server
> Note: See [the offical document](https://www.mongodb.com/docs/drivers/c/) for Compatibility Table Legend between mongo-c-driver and MongoDB server.

Given the requirements of flexible data structures, elastic scalability, and high performance, a widely used NoSQL database, MongoDB (https://www.mongodb.com), was adopted to manage all kinds of data in SEIMS.
The free available version of MongoDB server is MongoDB community server which can be downloaded from the official website or installed using package management tools, e.g., apt-get and homebrew.

#### Windows
Download the previous stable release version 3.6.9 https://fastdl.mongodb.org/win32/mongodb-win32-x86_64-2008plus-ssl-3.6.9-signed.msi, or other proper version.
+ 1. Installation and simple test
    - Install the MongoDB community server (take `mongodb-win32-x86_64-2008plus-ssl-3.2.3-signed.msi` as example) to a local directory without spaces, e.g., `D:\MongoDB`.
    - Create a new directory named logs (e.g., `D:\MongoDB\logs`) to store journal files.
    - Create a new directory named `db` (e.g., `D:\MongoDB\db`) to store data.
    - Open a CMD as administrator and run the following commands:
    `D:\MongoDB\bin\mongod.exe --dbpath=D:\mongodb\db`
    - The last line printed in the CMD windows should be like:
   ` Tue Oct 09 11:50:55 [websvr] admin web console waiting for connections on port 27017`
    This means MongoDB server has started successfully and the port 27017 is occupied by MongoDB.
    - DO NOT close the current CMD window and open a new one as administrator, and run the following commands:
    `D:\MongoDB\bin\mongo.exe`
    If the last line is connecting to: test, the connection to MongoDB is successful. The default database named test is connected.
    - Continue to test:

    ```
    use test
    db.foo.save({hello:0})
    db.foo.find()
    ```

        If something like `{ "_id" : ObjectId("5c222fdff492e1436718e00b"), "Hello" : 0 }` showed in the last line, the key-value data (i.e., key is `hello` and value is `0`) has been inserted into the newly created collection `foo` in `test` database successfully.
    - Enter `exit` to leave MongoDB.
+ 2. Register MongoDB as system service
    As is shown in previous section, a CMD window MUST be active to keep the running of MongoDB server, which is tedious. Therefore, we wish to set MongoDB as system service and start it along with Windows system, so that we do not need to start it manually each time after restarts of Windows system.
    - Open a CMD as administrator and enter the following commands:
    `D:\MongoDB\bin\mongod.exe --dbpath=D:\MongoDB\db --logpath=D:\MongoDB\logs\mongodb.log --install --serviceName "MongoDB"`
    - If something like “Tue Oct 09 12:05:15 Service can be started from the command line with 'net start MongoDB'” showed in the console window (or maybe nothing showed), the service has been successfully registered.
    - Now, a simple command net start MongoDB is enough to start MongoDB server. This command will set the MongoDB server as automatically start service simultaneously.
        - Alternatively, we can also manually set the MongoDB service as automatically start service along with Windows system.
        `D:\MongoDB\bin\mongod --install --serviceName "MongoDB" --serviceDisplayName "MongoDB" --logpath D:\MongoDB\logs\mongodb.log --logappend --dbpath D:\MongoDB\db --directoryperdb`
        - If no errors occur, the automatically start service has been set successfully!

### MongoDB GUI
To view, query, and update data stored in MongoDB, a user-friendly and efficient GUI is urgently needed. 
Robo 3T is what we need (https://robomongo.org/download). Download the proper version for your system and install it.
Open Robo 3T and connect to the `localhost:27017` (i.e., `127.0.0.1:27017`). The key-pair data we just inserted can be found.

## Python environment
Install Python environment for running SEIMS,
including preprocess, postprocess, parameters_sensitivity,
calibration, and scenario_analysis

We recommend to use Python3.x, although Python2.7 is still supported.
`Conda` is highly recommended to run pySEIMS.
Please use the following commands in Anaconda or Miniconda shell to create a new python environment for SEIMS.

```shell
cd SEIMS/seims
conda env create -f pyseims_env.yml
conda activate pyseims
```

> See [this commit](https://github.com/lreis2415/SEIMS/commit/8767acc76a87473524ab8e9fa01943ac37cfcb12) for more information about GDAL versions' compatibility for C++ and Python environments.

# Test of the C/C++ building environment
Now, we have set up the C/C++ building environment for SEIMS, e.g., MSVC 2015, MS-MPI v6, GDAL 1.11.4, and mongo-c-driver 1.16.2. In case of any unpredictable omissions or errors, users are highly recommended to test the C/C++ building environment by compiling the Common Cross-platform Geographic-computing Library (CCGL, https://github.com/crazyzlj/CCGL) and running its unit test. CCGL has been integrated into SEIMS and no additional download is required. 

Common commands to compile CCGL and run unit tests:
```
cd <path-to-CCGL>
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DUNITTEST=1
make -j4
ctest -C Debug --rerun-failed --output-on-failure
```
Specifically, for users using MSVC on Windows, please follow the steps below:
- Open a new VS2015 Developer Command Prompt, and navigate to the directory of CCGL, e.g., `D:\demo\SEIMS\seims\src\ccgl`, and assume the directory for building CCGL is `D:\compile\CCGL_vs2015`.
- Execute following commands:
    ```bat
    cd D:\demo\SEIMS\seims\src\ccgl
    cmake -G "Visual Studio 14 2015 Win64" -B D:\compile\CCGL_vs2015 -DUNITTEST=1
    cmake --build D:\compile\CCGL_vs2015 --config Debug -- /m:4
    cd D:\compile\CCGL_vs2015
    ctest -C Debug --rerun-failed --output-on-failure
    ```
 If any `FAILED` tests occurred, you should check the settings of prerequisite software and libraries carefully. If you do not sure what the errors mean, please contact the developers for supports.

# Installation of SEIMS
SEIMS is mainly written by C++ and Python. Python is an interpreted language which means the source code can be executed directly under the Python environment without any manual compilation. Therefore, the installation of SEIMS is the compilation and installation of C++ applications. 
The C++ applications of SEIMS not only include the main programs and modules for watershed modeling, but also the integrated programs for preprocessing such as watershed delineation by TauDEM (http://hydrology.usu.edu/taudem/taudem5/index.html) and static task scheduling with the graph of subbasins by METIS (http://glaros.dtc.umn.edu/gkhome/metis/metis/overview). All the C++ applications are organized by CMake and can be built, compiled, and installed at one time.

Common commands to compile SEIMS:
```shell
cd <path-to-SEIMS>
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
make install
```

Specifically, for users using MSVC on Windows, please follow the steps below:
+ Open a new VS2013 Developer Command Prompt, and navigate to the directory of SEIMS, e.g., `D:\demo\SEIMS`.
+ Build the Visual Studio solution by CMake and compile the whole SEIMS solution by `msbuild.exe`.
    ```bat
    cd D:\demo\SEIMS
    d:
    mkdir build
    cd build
    cmake -G "Visual Studio 12 2015 Win64" ..
    msbuild.exe ALL_BUILD.vcxproj /p:Configuration=Release
    msbuild.exe INSTALL.vcxproj /p:Configuration=Release
    ```
+ After the compilation and installation, all executables and libraries are installed at the default location, i.e., `D:\demo\SEIMS\build\bin`.
