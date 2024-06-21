BMP scenario analysis {#getstart_bmp_scenario_analysis}
======================================================

[TOC]

Different best management practices (or beneficial management practices, BMPs for short) scenarios (i.e., spatial configurations of multiple BMPs) at the watershed scale may have significantly different environmental effectiveness, economic cost-benefit, and practicality. It is valuable for decision-making of integrated watershed management to assess the environmental effectiveness and economic cost-benefit of watershed BMP scenarios and then propose optimal ones. During the BMP scenarios analysis, each individual BMP scenario is created by automatically selecting and allocating BMPs on spatial units (known as BMP configuration units). Then the effects of the scenario on watershed behavior are simulated by watershed models. The simulation result is the basis of the automatic spatial optimization of BMP scenarios.

As a demo, one of the experiments conducted in Zhu *et al.* (2019b) is used in this section. The hydrologically connected fields were selected as the BMPs configuration units (see Section 2:3.2 for the preparation). Four spatially explicit BMPs were considered, and the BMP configuration strategy based on expert knowledge of upstream–downstream relationships (Wu *et al.*, 2018) was adopted. The NSGA-II algorithm was integrated to achieve the spatial optimization of BMP scenarios. More information please refers to Zhu *et al.* (2019b).

# Simple usage
For simple usage, open a CMD window, enter the following commands to execute the predefined BMP scenarios analysis of the Youwuzhen watershed model.

```
cd D:\demo\SEIMS\seims\test
D:
python –m scoop –n 2 demo_scenario_analysis.py -name youwuzhen
```
 
The runtime logs of scenario analysis including commands of each model run, the average objective values (i.e., environmental effectiveness and net-cost) of each generation, and the time-consuming were showed in Figure 1.

Like the auto-calibration based on NSGA-II algorithm, the automatically generated BMP scenarios were imported into the `BMP_SCENARIOS` collection of the scenario database for each generation during the BMP scenarios optimization (Figure 2).

The results of the BMP scenarios analysis can be found in `SEIMS\data\youwuzhen\demo_youwuzhen30m_longterm_model\SA_NSGA2_CONNFIELD_UPDOWN_Gen_2_Pop_4` including:
+ `Pareto_Economy-Environment` folder: Near optimal Pareto plots, e.g., Figure 3 showed the near optimal Pareto solutions of the first and second generations with the objectives of maximizing the environmental effectiveness and minimizing the economic net-cost.
+ `Scenarios` folder: The BMP scenario information in plain text file and the corresponding spatial distribution raster file. For example, one BMP scenario information of the second generation is as follows:
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

    The gene number is 72 which equals to the number of BMP configuration units, i.e., the spatial unit number of `FIELDS_15`. The gene values indicate the BMP types that configured on BMP configuration units. The value of 0 means no BMP configured. Each BMP type has one scenario item. The spatial distribution of this BMP scenario was shown in Figure 4.
+ `runtime.log` file: Recoding the near optimal Pareto solutions of each generation.
+ `hypervolume.txt` file: The hypervolume index of each generation.

# Configuration file of scenario analysis
The configuration file of BMP scenarios analysis, such as that of the Youwuzhen watershed model shown in the following code block, includes five sections, i.e., `SEIMS_Model`, `Scenario_Common`, `BMPs`, `NSGA2`, and `OPTIONAL_MATPLOT_SETTING`. Among them, `OPTIONAL_MATPLOT_SETTING` is optional. **The names of sections and options should not be changed.**

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

+ SEIMS_Model: Basic settings of the SEIMS-based watershed model, see Section 2:4.3.2 for details. Note that the OpenMP version of SEIMS main program is used by default. However, the options of version, `MPI_BIN`, and `processNum` are supported if the MPI&OpenMP version is wanted.
+ Scenario_Common: Common settings for scenarios analysis.
    1. `Eval_Time_start`: Starting date time of model evaluation with the format of `YYYY-MM-DD HH:MM:SS`.
    2. `Eval_Time_end`: Ending date time of model evaluation with the format of `YYYY-MM-DD HH:MM:SS`.
    3. `worst_economy`: Economic net-cost under the circumstance of worst scenario.
    4. `worst_environment`: Environmental effectiveness under the circumstance of worst scenario. The worst_economy and `worst_environment` are set for the calculation of hypervolume index. 
    5. `runtime_years`: For the BMP scenarios analysis based on the evaluation of long-term environmental effectiveness of BMPs, SEIMS assumed that BMPs can reach relative stability after several years of maintenance from their first establishment. Therefore, the `runtime_years` is specified for the evaluation of BMP scenario cost model.
    6. `Export_scenario_txt`: (Optional) Output each BMP scenario as plain text file (`True`) or not (`False`). The default is `False`.
    7. `Export_scenario_tif`: (Optional) Output the spatial distribution of each BMP scenario as GeoTiff raster file (`True`) or not (`False`). The default is `False`.
+ `BMPs`: Application specific BMPs settings.
    1. `BMPs_info`: Information of selected BMP types for BMP scenarios analysis. The JSON format is used. The key is `BMPID` (see Section 2:2.7.1) and the value is another JSON string of the corresponding field-values (available fields can be found in Table 2:2 8). Note that, the data type of the key MUST be string, although `BMPID` is integer. The `DISTRIBUTION` and `LOCATION` fields can be omitted which will be defined in `BMPs_cfg_units`.
        ```
        BMPs_info = {"17":{
                            "COLLECTION": "AREAL_STRUCT_MANAGEMENT",
                            "SUBSCENARIO": [1, 2, 3, 4]
                            }
                    }
        ```
    2. `BMPs_retain`: Retained BMP types for generated BMP scenarios during the optimization. The format is the same with `BMPs_info`.
        ```
         BMPs_retain = {"12":{
                        "COLLECTION": "PLANT_MANAGEMENT",
                        "DISTRIBUTION": "RASTER|LANDUSE",
                        "LOCATION": "33",
                        "SUBSCENARIO": 0
                       }
                       }
        ```
    3. `Eval_info`: Information of model evaluation, including the `OUTPUTID` and the corresponding output filename (`ENVEVAL`) and the base value of environmental variable (`BASE_ENV`), e.g., the annual total amount of soil erosion. If the `BASE_ENV` is set to -9999, the base scenario will be firstly evaluated before the BMP scenarios analysis.
        ```
         Eval_info = {
                "OUTPUTID": "SED_OL",
                "ENVEVAL": "SED_OL_SUM.tif",
                "BASE_ENV": -9999
               }
        ```
    4. `BMPs_cfg_units`: Information of BMP configuration units which also follows the JSON format. The `UNITJSON` is a file that describes the basic information of each spatial unit and spatial relationships between spatial units, e.g., the upstream and downstream relationships. In the current version, the hydrologic response units (`HRU`), spatially explicit HRUs (`EXPLICITHRU`), hydrologically connected fields (`CONNFIELD`), and slope position units (`SLPPOS`) are supported. More details please refer to Zhu *et al.* (2019b).
        ```
         BMPs_cfg_units = {"CONNFIELD": {
                                    "DISTRIBUTION": "RASTER|FIELDS_15",
                                    "UNITJSON": "connected_field_units_updown_15.json"
                                   }
                    }
        ```
    
    5. `BMPs_cfg_method`: BMP configuration strategy used according to the characteristics of BMP configuration units, such as randomly strategy (`RAND`), strategy with knowledge on the suitable landuse types/slope positions of individual BMPs (`SUIT`), strategy based on expert knowledge of upstream–downstream relationships (`UPDOWN`, Wu *et al.* [2018]), and strategy with domain knowledge on the spatial relationships between BMPs and slope positions along the hillslope (`HILLSLP`, Qin *et al.* [2018]). More details please refer to Zhu *et al.* (2019b).
+ Sections of specific optimization methods for scenario analysis. Currently, the NSGA-II algorithm has been integrated and tested. In the future version of SEIMS, more methods should be integrated.
    + `NSGA2`: Parameter settings of NSGA-II algorithm.
        + `GenerationsNum`: Maximum number of generations.
        PopulationSize: Initial population size, i.e., the number of individuals.
        + `CrossoverRate`: Crossover probability, ranges from 0 to 1. A larger crossover rate indicates a higher possibility to take place the crossover operation between two parent individuals. 
        + `MutateRate`: Mutate probability.
        + `MaxMutatePerc`:  Maximum percent of genes to be mutated.
        + `SelectRate`: Selection rate of the evaluated and sorted individuals for each generation which is known as near-optimal Pareto solutions.
+ `OPTIONAL_MATPLOT_SETTINGS`: Plot settings for matplotlib, see Section 2:5.2.

# Advanced usage
The Python scripts of scenarios analysis are in `SEIMS/seims/scenario_analysis/spatialunits`. The `main_nsga2.py` is the entrance for the scenarios analysis based on NSGA-II algorithm, which can be executed though the unified format of running SEIMS Python scripts, e.g.,
```
cd D:\demo\SEIMS\seims\scenario_analysis\spatialunits
python main_nsga2.py -ini D:\demo\SEIMS\data\youwuzhen\workspace\scenario_analysis.ini
```

# See more...

Also see introduction of the Python package @subpage intro_scenario_analysis_pkg
and @subpage intro_scenario_analysis_spatialunits_pkg
