Auto-Calibration {#getstart_autocalibration}
======================================================

[TOC]

After parameter sensitivity analysis, a small number of the most sensitive parameters can be selected for auto-calibration. The value ranges of parameters may be the same with those defined in the parameter sensitivity analysis, or they may be narrowed by excluding values that result in an unacceptable model performance, such as in cases where *NSE* is less than zero.

In the current version of SEIMS, the NSGA-II algorithm (Non-dominated Sorting Genetic Algorithm II) was used for auto-calibration. During NSGA-II execution, the Latin-hypercube sampling method (Iman and Shortencarier, 1984) was used to generate initial samples of calibrated parameters. The crossover and mutation operations were similar to the original implementations in NSGA-II by Deb *et al.* (2002).

# Simple usage
For simple usage, open a CMD window, enter the following commands to execute the auto-calibration of the streamflow modeling of the Youwuzhen watershed model.

```bat
cd D:\demo\SEIMS\seims\test
D:
python –m scoop –n 2 demo_calibration.py -name youwuzhen
```
 
The runtime logs of auto-calibration including commands of each model run, the average model performance indicators of each generation, and the time-consuming were showed in Figure 1.

Like the parameter sensitivity analysis, the calibration values of selected parameters will be generated and imported as `CALI_VALUES` field into the `PARAMETERS` collection of the main spatial database for each generation during the optimization of auto-calibration (Figure 2).

The results of the auto-calibration can be found in `SEIMS\data\youwuzhen\demo_youwuzhen30m_longterm_model\CALI_NSGA2_Gen_2_Pop_4`. The near-optimal Pareto solutions plotted as a scatter plot can give a simple and direct interpretation of the calibration optimization processes. For example, Figure 3 showed the near optimal Pareto solutions of the first and second generations with the objectives of maximize the *NSE* and minimize the *RSR* of the simulated streamflow at the watershed outlet, which indicated an evolutionary directory to maximum *NSE* and minimum *RSR*.

In the meantime, to have a better aware of the calibration results, one best solution with the maximum *NSE* of the calibration period was plotted for each generation (red dash line in Figure 4) together with the 2.5% and 97.5% levels of the cumulative distribution of the output variables (gray band in Figure 4), i.e., streamflow at the watershed outlet in this demo.

# Configuration file of auto-calibration
The configuration file of auto-calibration, such as that of the Youwuzhen watershed model shown in the following code block, includes four sections, i.e., `SEIMS_Model`, `CALI_Settings`, `NSGA2`, and `OPTIONAL_MATPLOT_SETTING`. Among them, `OPTIONAL_MATPLOT_SETTING` is optional. The names of sections and options should not be changed.

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

+ `SEIMS_Model`: Basic settings of the SEIMS-based watershed model, see Section 2:4.3.2 for details. Note that the OpenMP version of SEIMS main program is used by default. However, the options of version, `MPI_BIN`, and `processNum` are supported if the MPI&OpenMP version is wanted.
+ `CALI_Settings`: Basic settings for auto-calibration.
    1. `paramRngDef`: Filename of the definition of parameters’ range for calibration. The format is the same with `paramRngDef` of parameter sensitivity analysis (Section 2:6.2).
    2. `Cali_Time_start`: Starting date time of the calibration period for model calibration with the format of `YYYY-MM-DD HH:MM:SS`.
    3. `Cali_Time_end`: Ending date time of the calibration period for model calibration with the format of `YYYY-MM-DD HH:MM:SS`.
    4. `Vali_Time_start`: (Optional) Starting date time of the validation period with the format of `YYYY-MM-DD HH:MM:SS` for visualization only to explore the effective of the calibrated parameters, e.g., Figure 2:7 4.
    5. `Vali_Time_end`: (Optional) Ending date time of the validation period with the format of `YYYY-MM-DD HH:MM:SS`. The `Vali_Time_start` and `Vali_Time_end` should be specified simultaneously.
    6. Note that the simulation period minus the calibration and validation period is the warm-up period.
+ Sections of specific optimization methods for auto-calibration. Currently, the NSGA-II algorithm has been integrated and tested. In the future version of SEIMS, more methods should be integrated.
    + `NSGA2`: Parameter settings of NSGA-II algorithm.
        + `GenerationsNum`: Maximum number of generations.
        + `PopulationSize`: Initial population size, i.e., the number of individuals.
        + `CrossoverRate`: Crossover probability, ranges from 0 to 1. A larger crossover rate indicates a higher possibility to take place the crossover operation between two parent individuals. 
        + `MutateRate`: Mutate probability.
        + `MaxMutatePerc`:  Maximum percent of genes to be mutated.
        + `SelectRate`: Selection rate of the evaluated and sorted individuals for each generation which is known as near-optimal Pareto solutions.
+ `OPTIONAL_MATPLOT_SETTINGS`: Plot settings for matplotlib, see Section 2:5.2.

# Advanced usage
The Python scripts of auto-calibration are in `SEIMS/seims/calibration`. The `main_nsga2.py` is the entrance for the auto-calibration based on NSGA-II algorithm, which can be executed though the unified format of running SEIMS Python scripts, e.g.,

```shell
cd D:\demo\SEIMS\seims\calibration
python main_nsga2.py -ini D:\demo\SEIMS\data\youwuzhen\workspace\calibration.ini
```

# See more...
Also see introduction of the Python package @subpage intro_auto_calibration_pkg
