Parameters sensitivity analysis {#getstart_parameters_sensitivity}
=============================================================

[TOC]

Parameter sensitivity analysis is useful for identifying the most important or influential parameters for a specified simulation target (e.g., streamflow at the watershed outlet) (Zhan et al., 2013). A typical sensitivity analysis procedure includes sampling from the value ranges of determined model inputs, evaluating the model with the generated samples and saving the interested outputs, and calculating sensitivity indices by various methods such as the Morris screening method (Morris, 1991) and FAST (Fourier Amplitude Sensitivity Test, Cukier et al., 1978). The sensitivity analysis tool in SEIMS is organized as separated functions according to these steps, which means it is easy to incorporate any sampling method and sensitivity analysis method. The most time-consuming step is repetitive model evaluations, which is parallelized at model level by parallel job management based on SCOOP (Hold-Geoffroy et al., 2014).

# Simple usage
For simple usage, open a CMD window, enter the following commands to execute the predefined parameter sensitivity analysis of the Youwuzhen watershed model.

```python
cd D:\demo\SEIMS\seims\test
D:
python –m scoop –n 2 demo_parameters_sensitivity.py -name youwuzhen
```

The runtime logs of the parameter sensitivity analysis including the commands of each model run and the results of sensitivity analysis were showed in Figure 2:6 1.

Firstly, the sampled calibration values of selected parameters were organized as float arrays and imported as `CALI_VALUES` field into the `PARAMETERS` collection of the main spatial database. The SEIMS-based watershed model under a combination of calibrated parameters can be evaluated with the `calibrationID` argument set to the corresponding array index (start from 0) (Figure 2:6 1a). The calibration values can be queried by the following command in the console window of Robo 3T (Figure 2:6 2).

```
db.getCollection('PARAMETERS').find({'CALI_VALUES':{$exists:true}})
```

Then, all combinations of calibrated parameters were evaluated in parallel according to the workers’ number of SCOOP, e.g., two workers were allocated in this simple usage (-m scoop –n 2). After the evaluation of each calibrated model, the interested model performance indicators will be calculated and saved. In current version of SEIMS, NSE (Nash-Sutcliffe efficiency), RSR (root mean square error-standard deviation ratio), and PBIAS (percent bias) (Moriasi et al., 2007) were used as model performance indicators.
Finally, the sensitivity analysis method (Morris screening method in this demo) will be executed according to the above introduced model outputs. For example, Figure 2:6 3 showed the parameter sensitivity analysis result with respect to the NSE of the simulated streamflow (m3s-1) at the watershed outlet. Each point in the screening plot represents one parameter of the Youwuzhen watershed model. The larger the modified means μ* (Campolongo et al., 2007), the more sensitive the watershed response is to variation in the parameter (Yang, 2011; Zhan et al., 2013). More results can be found in the output directory, e.g., `SEIMS\data\youwuzhen\demo_youwuzhen30m_longterm_model\PSA_Morris_N4L2`.

# Configuration file of parameter sensitivity analysis

Also see introduction of the Python package @subpage intro_parameters_sensitivity_pkg

The configuration file of parameter sensitivity analysis, such as that of the Youwuzhen watershed model shown in Figure 2:6 4, includes four sections, i.e., SEIMS_Model, PARAMETERS, OPTIONAL_PARAMETERS, and OPTIONAL_MATPLOT_SETTING. Among them, OPTIONAL_PARAMETERS and OPTIONAL_MATPLOT_SETTING are optional. The names of sections and options should not be changed. 

+ SEIMS_Model: Basic settings of the SEIMS-based watershed model, see Section 2:4.3.2 for details. Note that the OpenMP version of SEIMS main program is used by default. However, the options of version, MPI_BIN, and processNum are supported if the MPI&OpenMP version is wanted.
+ PSA_PARAMETERS: Basic settings for parameter sensitivity analysis.
    1.	evaluateParam: Simulation target(s) to be evaluated for sensitivity analysis, e.g., Q for streamflow, SED for sediment, and CH_TN for total nitrogen. This option is similar to PLOT_VARIABLES of postprocessing in Section 2:5.2. Multiple parameters should be separated by comma.
    2.	paramRngDef: Filename of the definition of parameters’ range. The file follows the basic plain text file format of SEIMS. Each line indicates one parameter to be analyzed with the format of NAME, lower_bound, upper_bound. For example, K_pet,-0.3,0.3 means that the impact value of K_pet ranges from -0.3 to 0.3. The type of CHANGE is the same with that defined in PARAMETERS collection. Relative information please refer to Section 2:4.2.3.
    3.	PSA_Time_start: Starting date time of the calculation of model performance for the sensitivity analysis with the format of YYYY-MM-DD HH:MM:SS.
    4.	PSA_Time_end: Ending date time of the calculation of model performance for the sensitivity analysis with the format of YYYY-MM-DD HH:MM:SS.
+ Sections of specific sensitivity analysis methods supported by SALib. Currently, the Morris screening method including groups and optimal trajectories (Morris_Method) and Fourier Amplitude Sensitivity Test (FAST_Method) have been integrated and tested. In the future version of SEIMS, more methods and not limited to the methods of SALib should be integrated.
    1.	Morris_Method:
    N: The number of trajectories to generate.
    num_levels: The number of grid levels. The resulting samples have a row number of (D+1)*N, and column number of D, where D is the number of parameters.
    optimal_trajectories: The number of optimal trajectories to sample (between 2 and N) which can also be None.
    local_optimization: Flag whether to use local optimization. Speeds up the process tremendously for bigger N and num_levels. If set to False brute force method is used.
    2.	FAST_Method:
    N: The number of samples to generate.
    M: The interference parameter, i.e., the number of harmonics to sum in the Fourier series decomposition, the default is 4.
+ OPTIONAL_MATPLOT_SETTINGS: Plot settings for matplotlib, see Section 2:5.2.

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

# Advanced usage
The Python scripts of parameter sensitivity analysis are in `SEIMS/seims/parameters_sensitivity`. The `main.py` is the entrance which can be executed though the unified format of running SEIMS Python scripts, e.g.,

```python
cd D:\demo\SEIMS\seims\parameters_sensitivity
python main.py -ini D:\demo\SEIMS\data\youwuzhen\workspace\sensitivity_analysis.ini
```

# See more...
Also see introduction of the Python package @subpage intro_parameters_sensitivity_pkg
