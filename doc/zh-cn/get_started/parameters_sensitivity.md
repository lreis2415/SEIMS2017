参数敏感性分析 {#getstart_parameters_sensitivity}
=============================================================

[TOC]

参数敏感性分析对于确定指定模拟目标（例如流域出口的流量）最重要或最有影响的参数非常有用（Zhan et al., 2013）。典型的敏感性分析流程包括从确定的模型输入值范围中取样、用生成的样本对模型进行评估并保存感兴趣的输出，以及通过各种方法计算敏感性指数，如Morris筛选方法（Morris, 1991）和FAST方法（Fourier Amplitude Sensitivity Test, Cukier et al., 1978）。SEIMS中的敏感性分析工具按照这些步骤组织为独立的功能模块，因此可以轻松集成任何采样方法和敏感性分析方法。最耗时的步骤是重复的模型评估，SEIMS使用基于SCOOP的并行工作管理（Hold-Geoffroy et al., 2014），在模型层次实现并行化。

# 简单用法
打开一个CMD窗口，输入以下命令执行游屋圳流域模型的预定义参数敏感性分析：

```python
cd D:\demo\SEIMS\seims\test
D:
python –m scoop –n 2 demo_parameters_sensitivity.py -name youwuzhen
```

运行日志包括每次模型运行的命令和敏感性分析结果，如图 2:6 1所示。

首先，选定参数的采样率定值组织为浮点数组，并作为 `CALI_VALUES`字段导入主空间数据库的`PARAMETERS` 集合中。通过将 `calibrationID`设置为对应数组索引（从0开始），可以评估基于SEIMS的流域模型在一组率定参数下的表现（图 2:6 1a）。可以通过以下命令在Robo 3T的控制台窗口中查询率定值（图 2:6 2）。

```
db.getCollection('PARAMETERS').find({'CALI_VALUES':{$exists:true}})
```

然后，基于SCOOP的工作线程数，对所有组合的率定参数进行并行评估。例如，在简单用法中分配了2个线程（-m scoop –n 2）。在每个率定模型评估完成后，将计算感兴趣的模型性能指标并保存。当前版本的SEIMS中，NSE（Nash-Sutcliffe 效率）、RSR（均方根误差-标准差比）、PBIAS（偏差百分比）（Moriasi et al., 2007）被用作模型性能指标。
最终，根据上述模型输出执行敏感性分析方法（此例中为Morris筛选方法）。例如，图 2:6 3展示了流域出口模拟流量（m3s-1）NSE指标对应的参数敏感性分析结果。筛选图中的每个点代表游屋圳流域模型的一个参数。修改后的均值μ*值越大（Campolongo et al., 2007），流域对参数变化的响应越敏感（Yang, 2011; Zhan et al., 2013）。更多结果可在输出目录中找到，例如：`SEIMS\data\youwuzhen\demo_youwuzhen30m_longterm_model\PSA_Morris_N4L2`。

# 参数敏感性分析的配置文件

另请参阅Python包的介绍 @subpage intro_parameters_sensitivity_pkg

参数敏感性分析的配置文件（如图 2:6 4所示的游屋圳流域模型配置文件）包括四个部分：SEIMS_Model、PARAMETERS、OPTIONAL_PARAMETERS 和 OPTIONAL_MATPLOT_SETTING。其中，OPTIONAL_PARAMETERS 和 OPTIONAL_MATPLOT_SETTING是可选的。section和选项的名称不能更改。

+ SEIMS_Model: 基于SEIMS流域模型的基本设置，详情参见章节 2:4.3.2。注意默认使用SEIMS主程序的OpenMP版本。如果需要MPI&OpenMP版本，可以配置version、MPI_BIN 和 processNum等选项。
+ PSA_PARAMETERS: 参数敏感性分析的基本设置。
    1.	evaluateParam: 待评估的模拟目标，如流量（Q）、泥沙（SED）和总氮（CH_TN）。此选项与章节 2:5.2后处理中的PLOT_VARIABLES类似。多个参数用逗号分隔。
    2.	paramRngDef: 定义参数范围的文件名。该文件遵循SEIMS的基本纯文本文件格式。每行表示一个待分析参数，格式为：NAME, lower_bound, upper_bound。例如：K_pet,-0.3,0.3表示参数K_pet的影响范围为-0.3 到 0.3，影响类型与PARAMETERS集合中的定义一致。相关信息请参见章节 2:4.2.3。
    3.	PSA_Time_start: 敏感性分析中模型性能计算的起始时间，格式为 YYYY-MM-DD HH:MM:SS。
    4.	PSA_Time_end: 敏感性分析中模型性能计算的结束时间，格式为 YYYY-MM-DD HH:MM:SS。
+ 支持 SALib 的特定敏感性分析方法部分。目前，已集成并测试了Morris筛选方法，包括分组和最有轨迹（Morris_Method）以及Fourier幅度敏感性测试（FAST_Method）。在SEIMS的未来版本中，将集成更多的方法，且不限于SALib提供的方法。
    1.	Morris_Method:
    N: 生成的轨迹数。
    num_levels: 网格层次的数目。生成的样本行数为(D+1)*N，列数为D，其中D是参数数目。
    optimal_trajectories: 采样的最优轨迹数（范围为2至N），也可以设置为None。
    local_optimization: 是否使用局部优化。对于较大的N和num_levels，开启此选项可以显著加速。如果设置为False，则使用暴力法。
    2.	FAST_Method:
    N: 生成的样本数。
    M: 干扰参数，即 Fourier 系列分解中求和的谐波数，默认值为 4。
+ OPTIONAL_MATPLOT_SETTINGS: matplotlib的绘图设置，参见章节 2:5.2。

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
Sim_Time_end = 2012-03-09 23:59:59

[PSA_Settings]
evaluateParam = Q
paramRngDef = morris_param_rng-Q.def
# Objective calculation period (UTCTIME)
PSA_Time_start = 2012-02-27 00:00:00
PSA_Time_end = 2012-03-09 23:59:59

[Morris_Method]
N = 4
num_levels = 2
optimal_trajectories = None
local_optimization = True

[OPTIONAL_MATPLOT_SETTINGS]
FIGURE_FORMATS = PDF,PNG
FONT_TITLE = Times New Roman
DPI = 300
```

# 高级用法
参数敏感性分析的Python脚本位于`SEIMS/seims/parameters_sensitivity`文件夹中，`main.py`是入口脚本，可以通过SEIMS Python脚本的统一格式运行，例如，

```python
cd D:\demo\SEIMS\seims\parameters_sensitivity
python main.py -ini D:\demo\SEIMS\data\youwuzhen\workspace\sensitivity_analysis.ini
```

# 更多介绍...
Also see introduction of the Python package @subpage intro_parameters_sensitivity_pkg
