参数自动率定 {#getstart_autocalibration}
======================================================

[TOC]

在参数敏感性分析之后，可以选择少量最敏感的参数进行自动率定。这些参数的取值范围可以与敏感性分析中定义的范围相同，也可以通过排除导致模型性能不可接受的值来缩小范围，例如当*NSE*小于零时。

在当前版本的SEIMS中，自动率定使用NSGA-II算法（Non-dominated Sorting Genetic Algorithm II）。在NSGA-II执行过程中，采用Latin-hypercube采样方法（Iman and Shortencarier, 1984）来生成初始的率定参数样本。交叉和变异操作类似于Deb *等* (2002)在NSGA-II中的原始实现。

# 简单用法
打开一个CMD窗口，输入以下命令运行游屋圳流域模型的流量模拟自动率定。

```bat
cd D:\demo\SEIMS\seims\test
D:
python –m scoop –n 2 demo_calibration.py -name youwuzhen
```
 
运行日志包括每个模型运行的命令、每一代平均模型性能指标以及耗时（如图1所示）。

与参数敏感性分析类似，在自动率定优化过程中，每一代生成的选定参数的率定值将被导入到主空间数据库的 `PARAMETERS` 集合中，并作为`CALI_VALUES` 字段存储（如图2所示）。

自动率定的结果可以在`SEIMS\data\youwuzhen\demo_youwuzhen30m_longterm_model\CALI_NSGA2_Gen_2_Pop_4`中找到。近似最优的Pareto解集以散点图的形式显示，能直观解释率定优化的过程。例如，图3显示了第一代和第二代的近似最优Pareto解集，其目标是最大化 *NSE* 和最小化 *RSR* （流域出口模拟流量的*NSE*和*RSR*），展示了向最大 *NSE* 和最小*RSR*演化的方向。

此外，为更好地理解率定结果，每一代中 *NSE* 最大的率定期最优解（红色虚线，图4）与输出变量的累积分布2.5%和97.5%水平（灰色带，图4）一起绘制，即本示例中的流域出口流量。

# 自动率定的配置文件
自动率定的配置文件，如下面代码所示的游屋圳流域模型的配置文件，包括四个部分：`SEIMS_Model`、`CALI_Settings`、`NSGA2`和  `OPTIONAL_MATPLOT_SETTING`。其中，`OPTIONAL_MATPLOT_SETTING`为可选向。节和选项的名称不能更改。

```
[SEIMS_Model]
MODEL_DIR = D:\demo\SEIMS\data\youwuzhen\demo_youwuzhen30m_longterm_model
BIN_DIR = D:\demo\SEIMS\bin
HOSTNAME = 127.0.0.1
PORT = 27017
threadsNum = 2
layeringMethod = 1
scenarioID = 0
# Simulation period (UTCTIME)
Sim_Time_start = 2012-01-01 00:00:00
Sim_Time_end = 2012-05-30 23:59:59

[CALI_Settings]
# Parameters and ranges
paramRngDef = cali_param_rng-Q.def
# Calibration period (UTCTIME)
Cali_Time_start = 2012-02-27 00:00:00
Cali_Time_end = 2012-04-30 23:59:59
# Validation period (UTCTIME)
Vali_Time_start = 2012-05-01 00:00:00
Vali_Time_end = 2012-05-30 23:59:59

[NSGA2]
GenerationsNum = 2
PopulationSize = 4
CrossoverRate = 0.8
MaxMutatePerc = 0.2
MutateRate = 0.1
SelectRate = 1.0

[OPTIONAL_MATPLOT_SETTINGS]
FIGURE_FORMATS = PDF,PNG
LANG_CN = False
FONT_TITLE = Times New Roman
TITLE_FONTSIZE = 16
LEGEND_FONTSIZE = 12
TICKLABEL_FONTSIZE = 12
AXISLABEL_FONTSIZE = 12
LABEL_FONTSIZE = 14
DPI = 300
```

+ `SEIMS_Model`: 基于 SEIMS 流域模型的基本设置，详见章节 2:4.3.2。注意默认使用SEIMS主程序的OpenMP版本。如果需要MPI&OpenMP版本，可以配置version、`MPI_BIN` 和 `processNum` 等选项。
+ `CALI_Settings`: 自动率定的基本设置。
    1. `paramRngDef`: 定义率定参数范围的文件名，格式与参数敏感性分析的  `paramRngDef`相同（章节 2:6.2）。 
    2. `Cali_Time_start`: 率定期的起始时间，格式为 `YYYY-MM-DD HH:MM:SS`。
    3. `Cali_Time_end`: 率定期的结束时间，格式为 `YYYY-MM-DD HH:MM:SS`。
    4. `Vali_Time_start`: （可选）visualization验证期的起始时间，仅用于可视化以探索率定参数的效果，格式为 `YYYY-MM-DD HH:MM:SS` ，例如图 2:7 4。
    5. `Vali_Time_end`: （可选）visualization验证期的结束时间，格式为`YYYY-MM-DD HH:MM:SS`。  `Vali_Time_start` 和 `Vali_Time_end` 必须同时指定。
    6. 注意：模拟期减去率定期和验证器为预热期。
+ 特定优化方法的部分。当前版本中已集成并测试了NSGA-II算法。未来版本中将集成更多优化方法。
    + `NSGA2`: NSGA-II算法的参数设置。
        + `GenerationsNum`: 最大代数。
        + `PopulationSize`: 初始种群规模，即个体数量。
        + `CrossoverRate`: 交叉概率，范围为0到1。较大的交叉率表示两个父个体之间发生交叉操作的可能性更高。
        + `MutateRate`: 变异概率。
        + `MaxMutatePerc`:  基因变异的最大百分比。
        + `SelectRate`: 每代经过评估和排序的个体中选出的比例，称为近似最优的Pareto解集。
+ `OPTIONAL_MATPLOT_SETTINGS`: matplotlib的绘图设置，详见章节 2:5.2。

# 高级用法
自动率定的Python脚本位于`SEIMS/seims/calibration`文件夹中。`main_nsga2.py`是基于NSGA-II算法的自动率定入口脚本，可以通过SEIMS Python脚本的统一格式运行，例如，

```shell
cd D:\demo\SEIMS\seims\calibration
python main_nsga2.py -ini D:\demo\SEIMS\data\youwuzhen\workspace\calibration.ini
```

# 更多介绍...
Also see introduction of the Python package @subpage intro_auto_calibration_pkg
