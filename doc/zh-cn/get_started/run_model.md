运行基于SEIMS的流域过程模型 {#getstart_run_seims_model}
============================================================

[TOC]

# 简单使用
打开终端（如Windows的CMD），输入以下命令以执行预定义的基于SEIMS的游屋圳流域模型（以下简称游屋圳流域模型）：
```python
cd D:\demo\SEIMS\seims\test
D:
python demo_runmodel.py –name youwuzhen
```

模拟将在大约28秒内完成（如图1所示）。预定义的输出信息可以在模型文件夹（即`MODEL_DIR\OUTPUT0`）中找到，如图2所示。

![ywzrunmodelsimple](../../img/screenshot-runmodel-simple-usage.jpg) <img src="screenshot-runmodel-simple-usage.jpg" alt="ywzrunmodelsimple" width="400"/>

图1 游屋圳流域模型的OpenMP版本的简单使用

 ![ywzrunmodeloutput0](../../img/screenshot-runmodel-output0-simple-usage.jpg) <img src="screenshot-runmodel-output0-simple-usage.jpg" alt="ywzrunmodeloutput0" width="400"/>

图2 游屋圳流域模型的预定义输出

# 基于SEIMS模型文件结构
简单来说，基于SEIMS的流域模型是一个文件夹（如`SEIMS\data\youwuzhen\demo_youwuzhen30m_longterm_model`），其中包括若干SEIMS配置文件，如`file.in`、`file.out`、`param.cali` 和 `config.fig`。

`file.in` 和 `file.out`是基本模型配置所必需的，而 `param.cali` 是率定模型参数的可选文件。~~这三个纯文本文件会在数据预处理过程中被读取并导入到主空间数据库中（章节 2:3.2）。~~当这些文件发生变化（如在手动率定过程中）时，也可以仅通过运行单个预处理Python脚本（即 `SEIMS\seims\preprocess\db_import_model_parameters.py`）来更新MongoDB。

`config.fig` 文件是构建基于SEIMS的流域模型的最重要的配置文件，用于选择参与流域模拟的SEIMS模块以及指定这些模块的模拟顺序。

## file.in文件
`file.in` 文件用于定义不同时间步长的模拟时段的模拟模式。与[此处](@ref dataprep_basic_plaintest)描述的纯文本格式不同，其基本格式是 `[TAG]|[VALUE]`。目前该文件中必须包含以下四个 `TAG`。
+ `MODE`：字符串类型，用于指定长期模拟或暴雨事件模拟。值只能为 `Daily` 或 `Storm`（不区分大小写）。
+ `INTERVAL`：整型，根据 `MODE`定义时间步长（或所谓的时间分辨率）。对于长期模拟，单位是日，例如 `INTERVAL|1`表示时间步长是一日。对于暴雨事件模拟，单位为秒，例如 `INTERVAL|3600` 表示时间步长为一小时。
    特别地，对于暴雨事件模拟，可为坡面尺度和河道尺度这两个不同的过程指定不同的时间步长。例如，`INTERVAL|60,3600`表示坡面尺度过程的时间步长为1分钟，河道尺度过程的时间步长为1小时。
+ `STARTTIME`：模拟的起始时间，格式为 `YYYY-MM-DD HH:MM:SS`
+ `ENDTIME`： 模拟的结束时间，格式为 `YYYY-MM-DD HH:MM:SS`

对于游屋圳流域模型，其进行的是时间步长为一天的长期模拟，时间范围为2012年1月1日至2015年12月31， `file.in`的内容如下。

```
MODE|Daily
INTERVAL|1
STARTTIME|2012-01-01 00:00:00
ENDTIME|2015-12-31 23:59:59
```

## file.out文件
`file.out`文件用于定义基于SEIMS的流域模型的输出内容。格式与[此处](@ref dataprep_basic_plaintest)描述的纯文本格式相同。每行对应一个输出变量。表 1列出了各行的可用字段。

表 1 `file.out` 配置文件的可用字段

|      字段名称     |      数据类型     |      描述     |
|---|---|---|
|     OUTPUTID    |     字符串    |     唯一输出标识符，例如QRECH表示河道出口的流量。    |
|     TYPE    |     字符串    |     空间数据输出的聚合类型，包括SUM、AVE、MAX和MIN。多个类型用短横线连接，例如SUM-AVE表示同时输出总和和平均值。对于时间序列输出，TYPE可以为NONE。    |
|     STARTTIME    |     日期时间字符串    |     当前输出的起始时间，格式为YYYY-MM-DD HH:MM:SS    |
|     ENDTIME    |     日期时间字符串    |     当前输出的结束时间，格式为 YYYY-MM-DD HH:MM:SS    |
|     INTERVAL    |     整型    |     输出的时间步长，默认值为 -9999，表示与模拟的时间步长相同。    |
|     INTERVAL_UNIT    |     字符串    |     时间步长的单位，可以为DAY或SEC。默认值为 -9999，表示与模拟的单位相同。   |
|     FILENAME    |     字符串    |     输出文件名及后缀，例如 Q.txt 和 PET.tif。    |
|     SUBBASIN    |     字符串或整型    |     指定输出的子流域，可为ALL、OUTLET或用短横线连接的子流域编号，例如1-4-5。    |

在这些字段中，只有 `OUTPUTID` 是必填项。未指定字段的值将采用用户可扩展的输出数据库中的默认值（位于`SEIMS\seims\preprocess\database\AvailableOutputs.csv`）。因此，用户可以通过仅指定输出 ID 来定义`QRECH`（每个时间步长的河段出口流量，单位为m3/s）和 `PET` （潜在蒸散量，单位为mm）的输出，如：
```
OUTPUTID
QRECH
PET
```
或者可以指定更详细的信息，如：

```
OUTPUTID,TYPE,STARTTIME,ENDTIME,INTERVAL,INTERVAL_UNIT,SUBBASIN
QRECH,NONE,2012-01-01 00:00:00,2015-12-31 23:59:59,1,DAY,OUTLET
PET,SUM-AVE,2012-01-01 00:00:00,2015-12-31 23:59:59,-9999,-9999,ALL
```

## param.cali文件
`param.cali`文件用于定义率定参数。该文件可以不存在或为空，此时模型将使用默认的参数值。文件格式**必须**包含`NAME`、`IMPACT`和`CHANGE`。 `CHANGE`是可选的，不允许有标题行。 `NAME` **必须**是预定义参数列表中的一个，用户可以通过修改`SEIMS\seims\preprocess\database\model_param_ini.csv`文件进行拓展。 `CHANGE` 表示参数的影响类型，可为 `RC` （相对变化）或 `AC`（绝对变化）。`IMPACT`是浮点变化值，对于 `RC`，新参数值将是`initial_value * impact`，对于`AC`，则是`initial_value + impact`。

因此，`param.cali`文件支持以下三种格式：

```
Runoff_co,1.5
gw0,-50,
K_pet,-0.3,AC
```

注意，修改`param.cali`文件后需要重新执行`db_import_model_parameters.py`来更新数据库。


## config.fig文件
`config.fig` 文件用于定义基于SEIMS的流域模型中使用的SEIMS模块。**模块的排列顺序决定了模拟过程中模块的执行顺序。**SEIMS模块的基本执行顺序应为与驱动因子（如气候数据）相关的模块，坡面过程相关的模块，河道和地下水汇流过程。例如，游屋圳流域模型的`config.fig` 文件内容如图 2:4 3所示，详细信息请参见Qin等(2018)。

```
01 ### Driver factors, including meteorological data and precipitation
02 0 | TimeSeries | | TSD_RD
03 0 | Interpolation_0 | Thiessen | ITP
04 ### Surface processes
05 0 | Soil temperature | Finn Plauborg | STP_FP
06 0 | PET | PenmanMonteith | PET_PM
07 0 | Interception | Maximum Canopy Storage | PI_MCS
08 0 | Snow melt | Snowpeak Daily | SNO_SP
09 0 | Infiltration | Modified rational | SUR_MR
10 0 | Depression and Surface Runoff | Linsley | DEP_LINSLEY
11 0 | Hillslope erosion | MUSLE | SERO_MUSLE
12 0 | Plant Management Operation | SWAT | PLTMGT_SWAT
13 0 | Percolation | Storage routing | PER_STR
14 0 | Subsurface | Darcy and Kinematic | SSR_DA
15 0 | SET | Linearly Method from WetSpa | SET_LM
16 0 | PG | Simplified EPIC | PG_EPIC
17 0 | ATMDEP | Atomosphere deposition | ATMDEP
18 0 | NUTR_TF | Nutrient Transformation of C, N, and P | NUTR_TF
19 0 | Water overland routing | IUH | IUH_OL
20 0 | Sediment overland routing | IUH | IUH_SED_OL
21 0 | Nutrient | Attached nutrient loss | NUTRSED
22 0 | Nutrient | Soluble nutrient loss | NUTRMV
23 0 | Pothole | SWAT cone shape | IMP_SWAT
24 0 | Soil water | Water balance | SOL_WB
25 ### Route Modules, including water, sediment, and nutrient
26 0 | Groundwater | Linear reservoir | GWA_RE
27 0 | Nutrient | groundwater nutrient transport | NUTRGW
28 0 | Water channel routing | MUSK | MUSK_CH
29 0 | Sediment channel routing | Simplified Bagnold equation | SEDR_SBAGNOLD
30 0 | Nutrient | Channel routing | NutrCH_QUAL2E
```

如上所述，与 [此处](@ref dataprep_basic_plaintest) 描述的纯文本格式不同，每个选定的 SEIMS 模块的基本格式为： `[MODULE NO.]|[PROCESS NAME]|[METHOD NAME]|[MODULE ID]`，其中
+ `MODULE NO.` 可为任意数字
+ `PROCESS NAME` 是对应流域过程的名称
+ `METHOD NAME` 是用于模拟该流域过程的算法名称
+ `MODULE ID` 是SEIMS 模块的 ID，即对应动态链接库文件（`dll`、`so` 或 `dylib`文件）的名称。若模块无法找到活加载，SEIMS会退出并报告错误。

SEIMS主程序仅使用`PROCESS NAME` 和 `MODULE ID`，而`MODULE NO.` 和 `METHOD NAME`仅用于提高可读性。`MODULE ID`应与SEIMS模块的ID完全匹配。某些模块的`PROCESS NAME`可能包含额外的配置信息。

+ 对于插值模块，必须在`PROCESS NAME`的后缀中指定垂直插值方法（0表示不基于垂直递减率进行垂直插值，1表示进行垂直插值），例如：
`0 | Interpolation_1 | Thiessen | ITP`

    > TODO: 在当前版本的SEIMS中，降水和温度的垂直递减率被定义为常数，分别为0.03 mm/100 m（降水）和-0.65 degC/100（温度）。未来版本中，应允许用户根据研究区输入不同气象变量的垂直递减率。

# 高级使用
SEIMS主程序包括一个OpenMP版本（`seims_omp`）和一个OpenMP和MPI混合版本（简称 MPI&OpenMP 版本，`seims_mpi`）。在OpenMP版本中，基于OpenMP的每个模块的并行计算在基本单元层次（如网格单元或不规则多边形）进行。在MPI&OpenMP 版本中，首先将流域分解为子流域，在子流域层次上进行基于MPI的并行计算。然后再每个子流域内，与OpenMP版本相同，基于OpenMP的并行计算在基本单元层次进行。

> 注意：如果C/C++编译器不支持OpenMP，则OpenMP并行计算将不起作用，且SEIMS主程序的可执行文件名将为 `seims`。

## 在命令行界面运行
命令行界面（CLI）是运行基于SEIMS的流域模型的推荐方式。OpenMP版本和MPI&OpenMP版本的调用命令略有不同。
### OpenMP版本
OpenMP版本的主程序饿完整推荐用法如下。 

```shell
seims_omp -wp <modelPath> [-thread <threadsNum> -lyr <layeringMethod> -host <hostname> -port <port> -sce <scenarioID> -cali <calibrationID> -id <subbasinID>]
```

其中，
1.	`modelPath` 是基于SEIMS的流域模型的路径（参见数据预处理的配置文件中的`MODEL_DIR`，章节 2:3.2）。
2.	`threadsNum` （可选）用于OpenMP并行计算的线程数量，必须大于或等于1（默认值）。
3.	`layeringMethod` （可选）routing layering method，0表示the routing layering method based on flow direction algorithms of `UPDOWN`（默认，源头分层），1表示 `DOWNUP`（出口分层）
4.	`hostname` 是MongoDB 服务器的 IP 地址，默认值为`127.0.0.1` （即`localhost`）。
5.	`port` 是MongoDB 服务器的端口号，默认值为27017。
6.	`scenarioID` 是BMP情景的ID，在情景数据库的`BMP_SCENARIOS` 集合中定义。默认值为-1，表示不应用场景。
7.	`calibrationID` 率定数据的ID（即索引），在主数据库的 `PARAMETERS`集合中定义。默认值为-1，表示不应用率定。
8.	`subbasinID` 是要执行的子流域ID。0表示整个流域。9999为Field版本保留。
9.  todo获取更多可用参数。

对于游屋圳流域，一个完整的命令示例如下： 

```bat
 cd D:\demo\SEIMS\bin
 D:
 seims_omp -wp D:\demo\SEIMS\data\youwuzhen\demo_youwuzhen30m_longterm_model -thread 4 -lyr 1 -host 127.0.0.1 -port 27017 -sce 0
```

OpenMP版本的运行日志如图2:4 1所示，其中包括数据输入/输出（IO）、各模块计算和整个模拟的耗时。

输出文件夹将在模型目录中创建，其命名格式为`OUTPUT<_MPI>-<FlowDir>-<LayerMethod>-<scenarioID>-<calibrationID>`。

### MPI&OpenMP版本
SEIMS主程序的MPI&OpenMP版本与OpenMP版本的参数相同，但调用方式不同（如图 3:1 1）。运行MPI程序的基本格式为：

```shell
mpiexec -<hostsopt> <hostfile> -n <processNum> seims_mpi -wp <modelPath> [-thread <threadsNum> -lyr <layeringMethod> -host <hostname> -port <port> -sce <scenarioID> -cali <calibrationID> -id <subbasinID>]
```

其中，
1. `hostopt` （可选）是MPI的输入参数标识符，用于指定主机列表文件，其形式因MPI实现而异，例如，`-machinefile` 或 `-hostfile`。
2. `hostfile` （可选）是注记列表文件。其格式可能和MPI实现不同。例如，OpenMPI使用的格式为：
    ```
    b10n08.cluster.com slots=3 max-slots=3
    b08n01.cluster.com slots=4 max-slots=4
    b10n05.cluster.com slots=4 max-slots=4
    ```
    IntelMPI使用格式为：
    ```
    b10n08.cluster.com 3
    b08n01.cluster.com 4
    b10n05.cluster.com 4
    ```
3. `processNum` （可选）是进程数量。

对于游屋圳流域模型，在Windows平台上运行MPI&OpenMP版本的完整命令示例为:

```bat
 cd D:\demo\SEIMS\bin
 D:
 mpiexec -n 4 seims_mpi -wp D:\demo\SEIMS\data\youwuzhen\demo_youwuzhen30m_longterm_model -thread 1 -lyr 1 -host 127.0.0.1 -port 27017 -sce 0
```

对应的运行日志如图 2:4 5。与OpenMP版本的输出信息（如图 2:4 1）不同，这里给出了每个进程（即本利中MPI的4个进程）的数据IO、所有模块计算和整个模拟的最大耗时（`[TIMESPAN][MAX]`）、最小耗时（`[TIMESPAN][MIN]`）和平均耗时（`[TIMESPAN][AVG]`）。最大耗时是评估并行性能时最常用的统计数据。

## 使用Python运行
除了通过命令行界面（CLI）运行，SEIMS还提供了基于配置文件的方法使用Python执行模型。与数据预处理的简单方法（2:3.2）类似，运行游屋圳流域模型的简单使用方式会根据本地的SEIMS路径生成一个配置文件，例如 `SEIMS\data\youwuzhen\workspace\runmodel.ini` （如图2:4 7）。

基于SEIMS的流域模型的配置文件，例如图 2:4 7展示的游屋圳流域模型的配置文件，在一节（即 `SEIMS_Model`）中包含多个选项来指定前面章节介绍的SEIMS主程序参数。以下是完整参数列表。

+ `SEIMS_Model`： 
    1. `MODEL_DIR`： 研究区的模型路径，包含流域建模所需的多个配置文件。
    2. `BIN_DIR`： SEIMS模块和主程序路径，默认与数据预处理配置文件（`preprocess.ini`，参见章节 2:3.2）中的`CPP_PROGRAM_DIR`相同。
    3. `HOSTNAME`： MongoDB服务器的IP地址，例如 `127.0.0.1` （即 `localhost`）。
    4. `PORT`： MongoDB服务器端口号，默认值为27017。
    5. `VERSION`： （可选）SEIMS主程序版本，可为`OMP`（OpenMP版本）和 `MPI`（MPI&OpenMP版本），默认值为`OMP`。
    6. `MPI_BIN`： （可选）MPI可执行文件的完整路径，例如 `C:\Program Files\Microsoft MPI\Bin\mpiexec.exe`。
    7. `hostopt`： （可选）MPI输入参数标识符，用于指定主机列表文件，不同MPI实现有不同的标识符，例如`-machinefile` 或 `-hostfile`。
    `hostfile`： （可选）主机列表文件，其格式可能因MPI实现不同。
    8. `processNum`： （可选）进程数，默认值为1.
    9. `threadsNum`： （可选）每个进程的线程数，默认值为4。
    10. `layeringMethod`： （可选）基于流向算法的Routing layering method ，可为0或1。 
    11. `scenarioID`： （可选）BMP情景的ID。
    12. `calibrationID`： （可选）率定数据的ID（即索引）。 
    13. `subbasinID`： （可选）子流域的ID，0表示整个流域，9999为Field版本保留。
    14. `Sim_Time_start`： （可选）模拟起始时间，格式为  `YYYY-MM-DD HH:MM:SS`。
    15. `Sim_Time_end`： （可选）模拟结束时间，格式为 `YYYY-MM-DD HH:MM:SS`。如果未指定，将使用数据库中 `FILE_IN`集合准备的起始和结束时间。
    16. `Output_Time_start`： （可选）输出项的起始时间，格式为 `YYYY-MM-DD HH:MM:SS`。
    17. `Output_Time_end`： （可选）输出项的结束时间，格式为 `YYYY-MM-DD HH:MM:SS`。如果指定，将更新并应用于`FILE_OUT`集合中所有选定输出ID的时间段。

```
[SEIMS_Model]
MODEL_DIR = D:\demo\SEIMS\data\youwuzhen\demo_youwuzhen30m_longterm_model
BIN_DIR = D:\demo\SEIMS\bin
HOSTNAME = 127.0.0.1
PORT = 27017
version = OMP
mpi_bin = mpiexec
# hostopt = ...
# hostfile = ...
processNum = 2
threadsNum = 4
layeringMethod = 1
scenarioID = 0
calibrationID = -1
subbasinID = 0
# Simulation period (UTCTIME)
Sim_Time_start = 2012-01-01 00:00:00
Sim_Time_end = 2015-12-31 23:59:59
# Time period of ALL selected outputs
Output_Time_start = 2012-01-01 00:00:00
Output_Time_end = 2015-12-31 00:00:00
```

要运行上述配置文件定义的基于SEIMS的流域模型，请按照统一的SEIMS Python脚本格式，例如，

```python
cd D:\demo\SEIMS\seims
python run_seims.py -ini D:\demo\SEIMS\data\youwuzhen\workspace\runmodel.ini
```

## 自定义游屋圳流域模型
为了展示SEIMS构建不同流域模型的能力，我们对游屋圳流域模型进行了一个简单的修改：将用于潜在蒸散量模拟的Penman-Monteith method 方法（`PET_PM`）替换为Priestley-Taylor方法（`PET_PT`）。需要注意的是，除了潜在蒸散量的计算外， `PET_PM`模块还模拟了潜在植物蒸腾量，而`PET_PT` 没有。因此，我们添加了一个名为AET_PTH的模块，用于模拟潜在植物蒸腾量、潜在土壤蒸发量和实际土壤蒸发量。同时，原本的土壤蒸发模块（`SET_LM`）不再需要。以下是新的自定义游屋圳流域模型的`config.fig`文件内容：

```
01 ### Driver factors, including climate and precipitation
02 0 | TimeSeries | | TSD_RD
03 0 | Interpolation_0 | Thiessen | ITP
04 ### Surface processes
05 0 | Soil temperature | Finn Plauborg | STP_FP
06 0 | PET | PriestleyTaylor | PET_PT
07 #0 | PET | PenmanMonteith | PET_PM
08 0 | Interception | Maximum Canopy Storage | PI_MCS
09 0 | Snow melt | Snowpeak Daily | SNO_SP
10 0 | Infiltration | Modified rational | SUR_MR
11 0 | Depression and Surface Runoff | Linsley | DEP_LINSLEY
12 0 | Hillslope erosion | MUSLE | SERO_MUSLE
13 0 | Plant Management Operation | SWAT | PLTMGT_SWAT
14 0 | Percolation | Storage routing | PER_STR
15 0 | Subsurface | Darcy and Kinematic | SSR_DA
16 0 | AET | Hargreaves and PriestleyTaylor | AET_PTH
17 #0 | SET | Linearly Method from WetSpa | SET_LM
18 0 | PG | Simplified EPIC | PG_EPIC
19 0 | ATMDEP | Atomosphere deposition | ATMDEP
20 0 | NUTR_TF | Nutrient Transformation of C, N, and P | NUTR_TF
21 0 | Water overland routing | IUH | IUH_OL
22 0 | Sediment overland routing | IUH | IUH_SED_OL
23 0 | Nutrient | Attached nutrient loss | NUTRSED
24 0 | Nutrient | Soluble nutrient loss | NUTRMV
25 0 | Pothole | SWAT cone shape | IMP_SWAT
26 0 | Soil water | Water balance | SOL_WB
27 ### Route Modules, including water, sediment, and nutrient
28 0 | Groundwater | Linear reservoir | GWA_RE
29 0 | Nutrient | groundwater nutrient transport | NUTRGW
30 0 | Water channel routing | MUSK | MUSK_CH
31 0 | Sediment channel routing | Simplified Bagnold equation | SEDR_SBAGNOLD
32 0 | Nutrient | Channel routing | NutrCH_QUAL2E
```

在所有模型参数保持默认的情况下，分别运行了原始游屋圳流域模型和新的自定义模型。图 2:4 9显示了Penman-Monteith法（图 2:4 9a）和 Priestley-Taylor法（图 2:4 9b）模拟的平均潜在蒸散量的空间分布。两种方法模拟的空间模式非常相似，但具体数值有所不同。模拟潜在蒸散量的差异也反映在模拟的流域出口的流量上（图 2:4 10）。

