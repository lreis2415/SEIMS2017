流域管理措施情景分析 {#getstart_bmp_scenario_analysis}
======================================================

[TOC]

不同的最佳管理措施（或高效管理措施，简称BMPs）情景（多个BMP的空间配置）在流域尺度上可能具有显著不同的环境效益、经济成本收益以及实际可操作性。通过评估流域BMP情景的环境效益和经济成本收益，并提出最优方案，为综合流域管理的决策提供了重要参考价值。在BMP情景分析中，通过在空间单元上自动选择和分配BMP（称为BMP配置单元），创建每个BMP方案。随后，利用流域模型模拟该场景对流域表现的影响。模拟结果是实现BMP情景自动空间优化的基础。

作为示例，本节引用了Zhu *et al.* (2019b)中的实验之一。实验选择了水文连接的地块作为BMP配置单元（准备工作参见章节 2:3.2）。考虑了四种空间显式BMPs，并采用了基于上下游关系的专家知识的BMP配置策略（Wu *et al.*, 2018）。通过集成NSGA-II算法实现BMP情景的空间优化。更多信息参见Zhu *et al.* (2019b)。

# 简单用法
打开一个CMD窗口，输入以下命令运行游屋圳流域模型的预定义BMP分析：

```
cd D:\demo\SEIMS\seims\test
D:
python –m scoop –n 2 demo_scenario_analysis.py -name youwuzhen
```
 
运行日志包括每次模型运行的命令、每一代的平均目标值（即环境效益和净成本）以及耗时，如图1所示。

与基于 NSGA-II 算法的自动率定类似，在BMP情景优化过程中，每一代自动生成的BMP情景将被导入到情景数据库的`BMP_SCENARIOS`集合中（如图2）。

BMP情景分析的结果可在以下路径找到`SEIMS\data\youwuzhen\demo_youwuzhen30m_longterm_model\SA_NSGA2_CONNFIELD_UPDOWN_Gen_2_Pop_4`：
+ `Pareto_Economy-Environment` 文件夹：近似最优Pareto图，例如，图3展示了第一代和第二代在最大化环境效益和最小化经济净成本目标下的近似最优Pareto解集。
+ `Scenarios` 文件夹：BMP情景信息的纯文本文件及对应的空间分布栅格文件。例如，第二代中的一个BMP情景信息如下所示：
    ```
    Scenario ID: 125931440
    Gene number: 72
    Gene values: 0, 0, 0, 4, 0, 4, 0, 0, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 2, 0, 0, 4, 0, 2, 4, 4, 0, 1, 1, 0, 0, 0, 0, 4, 0, 0, 2, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 3, 4, 0, 2, 0, 0
    Scenario items:
    _id	NAME	COLLECTION	BMPID	LOCATION	DISTRIBUTION	SUBSCENARIO	ID
    481	S125931440	AREAL_STRUCT_MANAGEMENT	17	44-45-64	RASTER|FIELDS_15	1	125931440
    482	S125931440	AREAL_STRUCT_MANAGEMENT	17	21-35-40-53-70	RASTER|FIELDS_15	2	125931440
    483	S125931440	AREAL_STRUCT_MANAGEMENT	17	60-67	RASTER|FIELDS_15	3	125931440
    484	S125931440	AREAL_STRUCT_MANAGEMENT	17	4-6-12-16-34-38-41-42-50-68	RASTER|FIELDS_15	4	125931440
    485	S125931440	PLANT_MANAGEMENT	12	33	RASTER|LANDUSE	0	125931440
    Effectiveness:
        economy: 57.712680
        environment: 0.48278
    ```

    gene number为72，与BMP配置单元的数量相等，即`FIELDS_15`的空间单元数量。gene values表示分配到BMP配置单元的BMP类型，其中值为0表示未配置BMP。每种BMP类型对应一个情景项。该BMP情景的空间分布如图4所示。
+ `runtime.log` 文件：记录每一代的近似最优Pareto解集。
+ `hypervolume.txt` 文件：记录每一代的hypervolume 索引。

# 情景分析的配置文件
BMP情景分析的配置文件，如一下代码块中游屋圳流域模型的配置文件所示，包括五个部分：`SEIMS_Model`、`Scenario_Common`、`BMPs`、`NSGA2`和`OPTIONAL_MATPLOT_SETTING`。其中，`OPTIONAL_MATPLOT_SETTING`是可选项。**节和选项的名称不可更改。**

```
[SEIMS_Model]
MODEL_DIR = C:\z_code\Hydro\SEIMS\data\youwuzhen\demo_youwuzhen30m_longterm_model
BIN_DIR = C:\z_code\Hydro\SEIMS\bin
HOSTNAME = 127.0.0.1
PORT = 27017
threadsNum = 2
layeringMethod = 1
scenarioID = 0
# Simulation period (UTCTIME)
Sim_Time_start = 2012-01-01 00:00:00
Sim_Time_end = 2012-05-30 23:59:59

[Scenario_Common]
Eval_Time_start = 2012-02-27 00:00:00
Eval_Time_end = 2012-04-30 23:59:59
worst_economy = 300.
worst_environment = 0.
runtime_years = 8
export_scenario_txt = True
export_scenario_tif = True

[BMPs]
BMPs_info = {"17":{"COLLECTION": "AREAL_STRUCT_MANAGEMENT", "SUBSCENARIO": [1, 2, 3, 4]}}
BMPs_retain = {"12":{"COLLECTION": "PLANT_MANAGEMENT", "DISTRIBUTION": "RASTER|LANDUSE", "LOCATION": "33", "SUBSCENARIO": 0}}
Eval_info = {"OUTPUTID": "SED_OL", "ENVEVAL": "SED_OL_SUM.tif", "BASE_ENV": -9999}
BMPs_cfg_units = {"CONNFIELD": {"DISTRIBUTION": "RASTER|FIELDS_15", "UNITJSON": "connected_field_units_updown_15.json"}}
BMPs_cfg_method = UPDOWN

[NSGA2]
GenerationsNum = 2
PopulationSize = 4
CrossoverRate = 1.0
MaxMutatePerc = 0.2
MutateRate = 1.0
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

+ SEIMS_Model: 基于 SEIMS 流域模型的基本设置，详见章节 2:4.3.2。注意默认使用SEIMS主程序的OpenMP版本。如果需要MPI&OpenMP版本，可以配置version、`MPI_BIN` 和 `processNum` 等选项。
+ Scenario_Common: 情景分析的通用设置。
    1. `Eval_Time_start`: 模型评估的起始时间，格式为 `YYYY-MM-DD HH:MM:SS`。
    2. `Eval_Time_end`: 模型评估的结束时间，格式为 `YYYY-MM-DD HH:MM:SS`。
    3. `worst_economy`: 最差情景下的经济净成本。
    4. `worst_environment`: 最差情景下的环境效益。worst_economy 和 `worst_environment`用于计算hypervolume索引。
    5. `runtime_years`: 对于基于BMP长期环境效益的BMP情景分析，SEIMS假设BMP在首次建立后经过若干年维护达到相对稳定状态。因此，`runtime_years` 用于评估BMP情景成本模型。
    6. `Export_scenario_txt`: （可选）是否将每个 BMP 场景导出为纯文本文件（`True`或`False`），默认值为`False`。
    7. `Export_scenario_tif`: （可选）是否将每个 BMP 场景的空间分布导出为 GeoTiff 栅格文件（`True`或 `False`），默认值为 `False`。
+ `BMPs`: Application specific BMPs settings.
    1. `BMPs_info`: 所选BMP类型的信息，使用JSON格式。`BMPID`作为键（参见章节 2:2.7.1），值是对应字段值的JSON字符串（可用字段参见Table 2:2 8）。注意，尽管`BMPID`是整型，键的数据类型必须为字符串。`DISTRIBUTION` 和 `LOCATION`字段可以省略，将在 `BMPs_cfg_units`中定义。
        ```
        BMPs_info = {"17":{
                            "COLLECTION": "AREAL_STRUCT_MANAGEMENT",
                            "SUBSCENARIO": [1, 2, 3, 4]
                            }
                    }
        ```
    2. `BMPs_retain`：在优化期间生成BMP情景中保留的BMP类型，格式与 `BMPs_info`相同。
        ```
         BMPs_retain = {"12":{
                        "COLLECTION": "PLANT_MANAGEMENT",
                        "DISTRIBUTION": "RASTER|LANDUSE",
                        "LOCATION": "33",
                        "SUBSCENARIO": 0
                       }
                       }
        ```
    3. `Eval_info`: 模型评估信息，包括`OUTPUTID`、对应输出文件名（`ENVEVAL`）以及环境变量基准值（`BASE_ENV`），例如，年土壤侵蚀总量。如果`BASE_ENV`设置为-9999，则会在BMP情景分析前首先评估基准场景。
        ```
         Eval_info = {
                "OUTPUTID": "SED_OL",
                "ENVEVAL": "SED_OL_SUM.tif",
                "BASE_ENV": -9999
               }
        ```
    4. `BMPs_cfg_units`: BMP 配置单元信息，使用 JSON 格式。`UNITJSON`是描述每个空间单元基本信息以及空间单元间基本关系（如上下游关系）的文件。在当前版本中，支持水文响应单元（`HRU`）、明确空间位置的HRU（`EXPLICITHRU`）、水文连接地块（`CONNFIELD`）和坡位单元（`SLPPOS`）。更多详情请参考Zhu *et al.* (2019b)。
        ```
         BMPs_cfg_units = {"CONNFIELD": {
                                    "DISTRIBUTION": "RASTER|FIELDS_15",
                                    "UNITJSON": "connected_field_units_updown_15.json"
                                   }
                    }
        ```
    
    5. `BMPs_cfg_method`: 根据BMP配置单元的特征选择BMP配置策略，例如：随机策略（`RAND`）、基于个别BMP的适宜土地类型/坡位的知识策略（`SUIT`）、基于上下游关系的专家知识策略（`UPDOWN`, Wu *et al.* [2018]）以及基于坡面上BMP和坡位空间关系的领域知识策略（`HILLSLP`, Qin *et al.* [2018]）。更多详情请参考Zhu *et al.* (2019b)。
+ 特点优化方法的部分。当前版本中已集成并测试了NSGA-II算法。未来版本中将集成更多优化方法。
    + `NSGA2`: NSGA-II算法的参数设置。
        + `GenerationsNum`: 最大代数。
        + `PopulationSize`: 初始种群规模，即个体数量。
        + `CrossoverRate`: 交叉概率，范围为0到1。较大的交叉率表示两个父个体之间发生交叉操作的可能性更高。
        + `MutateRate`: 变异概率。
        + `MaxMutatePerc`:  基因变异的最大百分比。
        + `SelectRate`: 每代经过评估和排序的个体中选出的比例，称为近似最优的Pareto解集。        
+ `OPTIONAL_MATPLOT_SETTINGS`: matplotlib的绘图设置，详见章节 2:5.2。

# Advanced usage
情景分析的 Python 脚本位于 `SEIMS/seims/scenario_analysis/spatialunits`文件夹中。`main_nsga2.py`是基于NSGA-II算法的情景分析入口脚本，可以通过SEIMS Python脚本的统一格式运行，例如，
```
cd D:\demo\SEIMS\seims\scenario_analysis\spatialunits
python main_nsga2.py -ini D:\demo\SEIMS\data\youwuzhen\workspace\scenario_analysis.ini
```

# 更多介绍...
Also see introduction of the Python package @subpage intro_scenario_analysis_pkg
and @subpage intro_scenario_analysis_spatialunits_pkg
