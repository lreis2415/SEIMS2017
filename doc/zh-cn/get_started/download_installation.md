下载与编译安装 {#getstart_download_installation}
======================================================

[TOC]

SEIMS托管于[Github](https://github.com/lreis2415/SEIMS)。SEIMS依赖于多个开源软件，如 [GDAL](https://www.gdal.org/) 和[mongo-c-driver](https://github.com/mongodb/mongo-c-driver)。目前，还没有编译好的二进制文件可分发，只能通过源代码分发。

我们鼓励用户使用docker方式运行或者手动下载并安装（即从源代码编译）SEIMS。一般来说，SEIMS版本库(https://github.com/lreis2415/SEIMS/tree/master)中的 `master` 分支是相对稳定的版本，而 `dev` 分支反映了最新的开发变更，现在是默认分支。

我们强烈建议用户通过GitHub的actions查看SEIMS在Windows、Linux和macOS上的自动安装和测试的工作流：

[![Build on Windows using MSVC](https://github.com/lreis2415/SEIMS/actions/workflows/Build_on_Windows.yml/badge.svg)](https://github.com/lreis2415/SEIMS/actions/workflows/Build_on_Windows.yml)

[![Build on Ubuntu using GCC](https://github.com/lreis2415/SEIMS/actions/workflows/Build_on_Linux.yml/badge.svg)](https://github.com/lreis2415/SEIMS/actions/workflows/Build_on_Linux.yml)

[![Build on macOS using AppleClang](https://github.com/lreis2415/SEIMS/actions/workflows/Build_on_macOS.yml/badge.svg)](https://github.com/lreis2415/SEIMS/actions/workflows/Build_on_macOS.yml)

配置的yml脚本位于 `SEIMS/.github/workflows`. 
# docker方式


# 手动编译运行方式
## 下载

用户可以使用git来克隆仓库或直接下载压缩包zip文件。
+ 使用git克隆单独的 `master` 或 `dev` 分支。
  1. 下载并安装git，例如Windows用户可以访问 https://gitforwindows.org/. 
  2. 在文件资源管理器中打开或创建一个本地目录，该目录名中不要有空格，例如 `D:\demo`，在该目录的空白处右键单击，然后选择 “Git Bash Here”。然后，输入以下两个命令来配置 Git 的基本设置，也就是你的 GitHub 账户的用户名和邮箱。`git config --global user.name "YourUsername"``git config --global user.email "youremail@example.com"`
`
  3. 如果这是你第一次在计算机中使用git，需要创建一个SSH密钥并加入到你的Github账户中。

    首先，输入以下命令以生成一个 SSH 密钥，并将其存储到你的用户目录，如 `C:/Users/XXX/.ssh/id_rsa.pub`

    `ssh-keygen -t rsa -C "email@example.com"`

    然后，复制该文件的内容并添加到你的Github账户中（路径为：“Settings” -> “SSH and GPG keys” -> “New SSH key”）。 
  4. 克隆一个单独的分支，如 `master` 分支：
    `git clone git@github.com:lreis2415/SEIMS.git --branch master --single-branch`

+ 或者，用户可以直接下载压缩包zip文件（例如：`master` 分支的zip文件为 https://github.com/lreis2415/SEIMS/archive/master.zip ）并将其解压到一个本地无空格路径下，如 `D:\demo`。

## 必备的软件和库

要成功编译和运行SEIMS，需要几个软件和库。

1. 用于编译SEIMS的C/C++部分的软件和库
    + CMake 3.1+
    + C/C++ 编译器，部分支持 C++11，如 MSVC 2010+、 GCC 4.6+、 Intel C++ 12.0+ 和 Clang 8.0+
    + 用于C/C++的MPI实现，如 MS-MPI v6+、 MPICH、 OpenMPI 或 Intel MPI
    + 用于C/C++的GDAL
    + 用于C/C++的mongo-c-driver 1.16+
2. 支持运行SEIMS的软件
    + MongoDB (community) server 4.2
    + MongoDB GUI
3. 运行SEIMS的Python环境
    + Python 2.7.x 或 3.x （强烈推荐Anaconda 或 miniconda）
    + Python的第三方包（GDAL、 pymongo、 scoop、 DEAP、 PyGeoC等）可以通过 `conda` 命令下载

### 用于编译SEIMS的C/C++部分的软件和库

#### CMake
CMake 3.1+ 用于构建SEIMS的C/C++工程项目，例如Microsoft Visual Studio解决方案（`*.sln`）。CMake可以从官方网站 https://cmake.org/download/ 下载。适用于Windows 64位的安装程序类似于 `cmake-3.x.x-win64-x64.msi` 。
安装CMake之后，请将CMake可执行文件的路径（如 `C:\Program Files (x86)\CMake\bin` ）添加到系统变量 `PATH` 中。这样，就可以在CMD（命令提示符）中直接通过输入 `cmake` 运行cmake而无需输入绝对路径。

#### C/C++编译器
##### Microsoft Visual C++ (MSVC)
SEIMS使用了C++11的一些特性，例如 `nullptr` 和 `auto` 关键字。因此， MSVC 2010是最低支持版本。Microsoft Visual Studio是基于Microsoft Visual C++的强大的集成开发环境（IDE）。如果你不想安装 Microsoft Visual Studio，可以使用Visual C++构建工具来构建针对Windows桌面的C++库和应用程序，这些工具与Microsoft Visual Studio中提供的工具相同。不过，仍然强烈推荐使用Visual Studio。
如果你想开发基于MPI的并行应用程序，MSVC 2010是最佳选择，因为其是最后一个集成MPI集群调试器的MSVC版本。MPI集群调试器是Windows上用于调试基于MPI并行应用程序的最便捷和强大的工具。关于MPI集群调试器的更多详细信息可以在https://docs.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2010/dd560809(v=vs.100) 中找到。此外，MSVC 2013、2015和2019均支持并推荐。所有这些MSVC版本可以从 https://visualstudio.microsoft.com/vs/older-downloads/ 下载

##### GCC
在 Linux 或 macOS 系统上推荐使用 GCC。

##### Clang
macOS 默认使用Clang编译器的变体，即 AppleClang，它不支持OpenMP。
在Mac上，强烈推荐通过 homebrew 安装的Clang编译器。

##### Intel C++
Intel C++编译器可以作为附加组件安装在Windows的Microsoft Visual Studio中。

#### 用于C/C++的MPI
##### MS-MPI
Microsoft MPI (MS-MPI)是Microsoft实现的 MPI （Message Passing Interface, https://www.mpi-forum.org/）标准，用于在Windows平台上开发和运行并行应用程序。推荐使用MS-MPI v6及更高版本。
从Microsoft下载中心下载MS-MPI（需要两个安装文件，即`msmpisdk.msi` 和 `MSMpiSetup.exe`），例如MS-MPI v6的下载链接是 https://www.microsoft.com/en-us/download/details.aspx?id=47259，并将其安装到默认位置。安装后，MS-MPI的环境变量会自动设置。打开`系统->高级系统设置->环境变量->系统变量`，确保以下变量-值对已存在。如果没有，请手动添加，其路径与你的计算机上的路径一致。
```batch
MSMPI_BIN=C:\Program Files\Microsoft MPI\Bin\
MSMPI_INC=C:\Program Files (x86)\Microsoft SDKs\MPI\Include\
MSMPI_LIB32=C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x86\
MSMPI_LIB64=C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64\
```

##### MPICH
在Mac上，MPICH可以通过homebrew安装。

##### OpenMPI
在Mac上，OpenMPI可以通过homebrew安装。

#### 用于 C/C++的GDAL

##### 用于Windows的GISInternals
由Tamas Szekeres维护的[GISInternals支持网站](http://www.gisinternals.com/index.html)是在Windows平台上由MSVC编译的GDAL库最著名的网站。用户应根据所使用的编译器版本和操作系统架构选择确切的版本。例如，我在Windows 10-64位系统上安装了MSVC2013 (_MSC_VER=1800)，并且希望编译64位应用系统的GDAL-1.11.4，因此我应该从归档版本 (http://www.gisinternals.com/archive.php)下载`release-1800-x64-gdal-1-11-4-mapserver-6-4-3.zip` 和 `release-1800-x64-gdal-1-11-4-mapserver-6-4-3-libs.zip`

+ 将这两个zip文件解压到同一个没有空格的目录路径下，例如 `C:/GDAL`。
+ 打开高级系统属性->环境变量，在系统变量窗格下创建几个新的系统变量：
    ```batch
    GDAL_DIR=C:\GDAL
    GDAL_DATA=C:\GDAL\bin\gdal-data
    GDAL_PATHS=C:\GDAL;C:\GDAL\bin;C:\GDAL\bin\proj\apps;C:\GDAL\bin\gdal\apps;C:\GDAL\bin\ms\apps;C:\GDAL\bin\curl;
    ```
+ 然后，将`%%GDAL_PATHS%` 添加到系统变量 `PATH` 的末尾。
+ 此时，用于C/C++的GDAL库以及可执行的GDAL工具（例如 `gdalinfo` ）已安装。打开一个新的CMD窗口，输入以下命令
`gdalinfo --version`，屏幕上应打印类似于 `GDAL 1.11.4, released 2016/01/25` 的信息。

##### 在Ubuntu上使用apt-get
在Ubuntu上，可以通过 `apt-get` 命令轻松地安装GDAL：
```shell
apt-get update && sudo apt-get install -qq gdal-bin libgdal-dev
```
默认情况下，将会安装最新版本的GDAL。

##### 在macOS上使用homebrew
在macOS上，可以通过 `brew` 命令轻松地安装GDAL：
```shell
brew install gdal
```

#### C/C++的mongo-c-driver驱动程序
SEIMS采用了MongoDB的mongo-c-driver库（https://github.com/mongodb/mongo-c-driver）来处理MongoDB的数据输入/输出（IO）。由于 mongo-c-driver仍在积极持续开发中，不同版本的构建方法和API（应用程序接口）函数可能不同。推荐使用VS2019支持的最后一个版本，即 `1.16.2` 。欢迎报告由版本兼容性引起的任何错误！

##### 在Windows上使用MSVC
+ 下载release文件并解压到本地目录，例如 `D:\demo\mongo-c-driver-1.16.2`
+ 打开 `VS201x Developer Command Prompt` ，依次执行以下命令。假设编译后生成的mongo-c-driver库和头文件的目标路径为  `C:\mongo-c-driver-vs201x` 。
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

    注意事项：
    1. 如果CMD找不到 `msbuild.exe` ，请在.NetFramework的路径中查找它，例如 `C:\Windows\Microsoft.NET\Framework64\v4.0.30319\msbuild.exe`。
    2. 如果你想编译32位的mongo-c-driver库，请将生成器"Visual Studio 12 2013 Win64"替换为"Visual Studio 12 2013"。

+ 现在，mongo-c-driver已经构建并安装好了，bin、include和lib的目录可以在mongo-c-dirver的根目录下找到，即 `C:\mongo-c-driver-vs201x` 。
+ 创建两个新的系统变量：
    ```batch
    MONGOC_ROOT=C:\mongo-c-driver-vs201x
    MONGOC_LIB=C:\mongo-c-driver-vs201x\bin
    ```
+ 然后，将 `%%MONGOC_LIB%` 添加到系统变量 `PATH` 的最后。

##### 在Ubuntu上使用apt-get
在Ubuntu上，可以通过 `apt-get` 命令轻松地安装GDAL：
```shell
apt-get update && sudo apt-get install -qq libmongoc-1.0-0 libmongoc-dev
```
默认情况下，将会安装最新版本的GDAL。

##### 在macOS上使用homebrew
在macOS上，可以通过 `brew` 命令轻松地安装GDAL：
```shell
brew install mongo-c-driver
```

### 支持运行SEIMS的软件

#### MongoDB （社区） 服务器
> 注意：请参见[官方文档](https://www.mongodb.com/docs/drivers/c/) 中的mongo-c-driver 和 MongoDB服务器之间的兼容性表格说明。

鉴于对灵活的数据结构、弹性的可扩展性和高性能的要求，SEIM采用了一种广泛的使用NoSQL数据库MongoDB（https://www.mongodb.com）来管理各种数据。
MongoDB服务器的免费版本是MongoDB社区服务器，可以从官方网站下载，或使用包管理工具（如 apt-get 和 homebrew）进行安装。

##### Windows
下载之前的稳定发行版本3.6.9 https://fastdl.mongodb.org/win32/mongodb-win32-x86_64-2008plus-ssl-3.6.9-signed.msi 或其他合适的版本。
+ 1. 安装并简单测试
    - 安装MongoDB社区服务器（以 `mongodb-win32-x86_64-2008plus-ssl-3.2.3-signed.msi` 为例）到一个没有空格的本地目录，例如 `D:\MongoDB`。
    - 创建一个名为 logs 的新目录（如 `D:\MongoDB\logs`）来存储日志文件。
    - 创建一个名为 `db`（如 `D:\MongoDB\db` ）来存储数据。
    - 以管理员身份打开CMD并运行以下命令：
    `D:\MongoDB\bin\mongod.exe --dbpath=D:\mongodb\db`
    - CMD窗口打印的最后一行应类似于：
   ` Tue Oct 09 11:50:55 [websvr] admin web console waiting for connections on port 27017`
    这表示MongoDB服务器已经成功启动，并且端口 27017 已被MongoDB占用。
    - 不要关闭当前CMD窗口，以管理员身份打开一个新的CMD窗口，并运行以下命令：
    `D:\MongoDB\bin\mongo.exe`
    如果最后一行显示为 connecting to: test，表示MongoDB的链接成功。默认连接名为 test 的数据库。
    - 继续测试：

    ```
    use test
    db.foo.save({hello:0})
    db.foo.find()
    ```

        如果最后一行显示类似于 `{ "_id" : ObjectId("5c222fdff492e1436718e00b"), "Hello" : 0 }` ，表示键值数据（即键为 `hello` ，值为 `0` ）已成功插入到 test 数据库中新创建的集合 `foo` 中。
    - 输入 `exit` 退出MongoDB.
+ 2. 将MongoDB注册为系统服务
    如前一节所示，一个CMD窗口必须保持打开以保持MongoDB服务器的运行，这较为繁琐。因此，希望将MongoDB设置为系统服务，使其在Windows系统启动时自动启动，这样在每次重新启动Windows系统后就不需要手动启动它。
    - 以管理员身份打开CMD并输入以下命令：
    `D:\MongoDB\bin\mongod.exe --dbpath=D:\MongoDB\db --logpath=D:\MongoDB\logs\mongodb.log --install --serviceName "MongoDB"`
    - 如果控制台窗口中显示类似于 “Tue Oct 09 12:05:15 Service can be started from the command line with 'net start MongoDB'” 的信息（或可能没有显示内容），表示服务已成功注册。
    - 现在，只需一个简单的命令 net start MongoDB 即可启动MongoDB服务器。此命令会将MongoDB服务器设置为自动启动服务。
        - 或者，我们也可以手动将MongoDB服务设置为随Windows系统自动启动：
        `D:\MongoDB\bin\mongod --install --serviceName "MongoDB" --serviceDisplayName "MongoDB" --logpath D:\MongoDB\logs\mongodb.log --logappend --dbpath D:\MongoDB\db --directoryperdb`
        - 如果未发生错误，自动启动服务设置已成功！

#### MongoDB图形用户界面（GUI）
为了查看、查询和更新存储在MongoDB中的数据，迫切需要一个用户友好且高效的GUI。
Robo 3T 正是我们所需要的工具 (https://robomongo.org/download)。下载适合你系统的版本并安装。
打开 Robo 3T 并连接到 `localhost:27017` （即 `127.0.0.1:27017`）。可以看到我们刚刚插入的键值数据。

### Python环境
安装运行SEIMS所需的Python环境，
包括预处理、后处理、参数敏感性分析，
率定和情景分析

我们推荐使用Python3.x，尽管仍然支持Python2.7。
强烈推荐使用 `Conda` 来运行pySEIMS。
请在Anaconda或Miniconda shell中使用以下命令来创建一个用于SEIMS的新的Python环境。

```shell
cd SEIMS/seims
conda env create -f pyseims_env.yml
conda activate pyseims
```

> 查看 [this commit](https://github.com/lreis2415/SEIMS/commit/8767acc76a87473524ab8e9fa01943ac37cfcb12) 获取更多关于GDAL版本与C++和Python环境兼容性的信息

## C/C++构建环境的测试
现在，我们已经为SEIMS设置了C/C++构建环境，例如MSVC 2015、MS-MPI v6、GDAL 1.11.4和mongo-c-driver 1.16.2。为了防止任何不可预见的遗漏或错误，强烈建议用户通过编译通用跨平台地理计算库（CCGL，https://github.com/crazyzlj/CCGL）并运行其单元测试来测试C/C++构建环境。CCGL已经集成到SEIMS中，无需额外下载。

编译CCGL并运行单元测试的常用命令：
```
cd <path-to-CCGL>
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DUNITTEST=1
make -j4
ctest -C Debug --rerun-failed --output-on-failure
```
对于在Windows上使用MSVC的用户，请按照一下步骤操作：
- 打开一个新的VS2015 Developer Command Prompt，导航到CCGL的目录，如 `D:\demo\SEIMS\seims\src\ccgl` ，假如用于构建CCGL的目录是 `D:\compile\CCGL_vs2015`。
- 执行以下命令：
    ```bat
    cd D:\demo\SEIMS\seims\src\ccgl
    cmake -G "Visual Studio 14 2015 Win64" -B D:\compile\CCGL_vs2015 -DUNITTEST=1
    cmake --build D:\compile\CCGL_vs2015 --config Debug -- /m:4
    cd D:\compile\CCGL_vs2015
    ctest -C Debug --rerun-failed --output-on-failure
    ```
 如果出现任何 `FAILED` 测试，你应仔细检查必备软件和库的设置。如果不确定错误的含义，请联系开发人员获取支持。

## SEIMS的安装
SEIMS主要由C++和Python编写。Python是一个解释型语言，源代码可以直接在Python环境中执行，无需手动编译。因此，SEIMS的安装主要是C++应用程序的编译和安装。
SEIMS的C++应用程序不仅包括用于流域建模的主程序和模块，还包括用于预处理的集成程序，例如TauDEM进行的流域划分（http://hydrology.usu.edu/taudem/taudem5/index.html）和METIS进行的子流域图的静态任务调度（http://glaros.dtc.umn.edu/gkhome/metis/metis/overview）。所有的C++应用程序都由CMake组织，可以一次性构建、编译和安装。

编译SEIMS的常用命令：
```shell
cd <path-to-SEIMS>
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
make install
```

对于在Windows上使用MSVC的用户，请按照以下步骤操作：
+ 打开一个新的VS2015 Developer Command Prompt，导航到SEIMS目录，例如 `D:\demo\SEIMS`。
+ 使用CMake构建Visual Studio解决方案，并使用 `msbuild.exe` 编译整个SEIMS解决方案。
    ```bat
    cd D:\demo\SEIMS
    d:
    mkdir build
    cd build
    cmake -G "Visual Studio 12 2015 Win64" ..
    msbuild.exe ALL_BUILD.vcxproj /p:Configuration=Release
    msbuild.exe INSTALL.vcxproj /p:Configuration=Release
    ```
+ 安装和编译完成后，所有可执行文件和库都会安装在默认位置，即 `D:\demo\SEIMS\build\bin`。
