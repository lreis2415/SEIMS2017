Postprocessing {#getstart_runmodel_postprocessing}
======================================================

[TOC]

After running a SEIMS-based watershed model, the postprocessing tool may be used for the calculation of model performance statistics such as Nash-Sutcliffe Efficiency (NSE), and the plotting of hydrographs of time-series data at the reach outlet such as streamflow and sediment.
# Simple usage

For simple usage, open a CMD window, enter the following commands to execute the postprocessing of the Youwuzhen watershed model.

```
cd D:\demo\SEIMS\seims\test
D:
python demo_postprocess.py -name youwuzhen
```

The runtime logs of postprocessing showed input paths and output model performance statistics (Figure 2:5 1).

By default, the hydrographs of streamflow (e.g., `Q.txt`) and sediment (e.g., `SED.txt`) at the watershed outlet were plotted and saved as figure files with the formats of PNG and PDF, e.g., `SEIMS\data\youwuzhen\demo_youwuzhen30m_longterm_model\OUTPUT0\png\Q-2013-01-01-2015-12-31.png` (Figure 2:5 2).

# Configuration file of postprocessing
Same to the data preprocessing, the simple usage of postprocessing also includes generating the configuration `INI` file according to the local paths of SEIMS such as Figure 2:5 3, and executing postprocessing by the advanced usage (See Section 2:5.3).

The configuration file of postprocessing, such as that of the Youwuzhen watershed model shown in Figure 2:5 3, includes four sections, i.e., `SEIMS_Model`, `PARAMETERS`, `OPTIONAL_PARAMETERS`, and `OPTIONAL_MATPLOT_SETTING`. Among them, `OPTIONAL_PARAMETERS` and `OPTIONAL_MATPLOT_SETTING` are optional. The names of sections and options should not be changed. 
+ `SEIMS_Model`: Basic settings of the SEIMS-based watershed model, including `HOSTNAME`, `PORT`, `MODEL_DIR`, `BIN_DIR`, `scenarioID`, and `calibrationID`, etc. The meanings of these options have been introduced in Section 2:4.3.2.
+ `PARAMETERS`
    + `PLOT_SUBBASINID`: The subbasin ID to be plotted.
    + `PLOT_VARIABLES`: The variables to be plotted, such as `Q`, `SED`, and `CH_TN`. Multiple variables can be separated by comma. Note that, the specified subbasin ID and the corresponding variables MUST be set as time-series output data in file.out (Section 2:4.2.2) and/or `FILE_OUT` collection of the main MongoDB database. If there is no matched observed data for the specified variable, the model performance statistics will not able to be calculated.
+ `OPTIONAL_PARAMETERS`: (Optional) Time period of model calibration and validation. The starting and ending date times of either calibration or validation MUST be presented or omitted simultaneously.
    + `Cali_Time_start`: Starting date time of model calibration with the format of `YYYY-MM-DD HH:MM:SS`.
    + `Cali_Time_end`: Ending date time of model calibration.
    + `Vali_Time_start`: Starting date time of model validation.
    + `Vali_Time_end`: Ending date time of model validation.
+ `OPTIONAL_MATPLOT_SETTINGS`: (Optional) Plot settings for matplotlib.
    + `FIGURE_FORMATS`: The format(s) of output figure(s) which can be one or multiple of PNG, JPG, TIF, PDF, EPS, SVG, and PS. Multiple formats are separated by comma, e.g., PNG,JPG,EPS.
    + `LANG_CN`: Whether Chinese are included (True) or not (False) in labels and legend texts, etc.
    + `FONT_TITLE`: Font title (font name) for the output figures. Below are tips to check the available fonts or use new ones.
        + Get the currently available font titles by the following Python code.
            @code
            ```py
            from matplotlib import font_manager
            flist = font_manager.get_fontconfig_fonts()
            font_names = list()
            for fname in flist:
                try:
                    font_names.append(font_manager.FontProperties(fname=fname).get_name())
                except IOError or Exception:
                    continue
            print(font_names)
            ```
            @endcode

        + To add and use a new font:
            + Download a font with `.ttf` format such as YaHei Mono which supports a hybrid of Chinese and English. If you have only .ttc formatted font file, you may want to convert it to .ttf file using transfonter.
            + Copy the font file to the font directory of matplotlib, e.g., `D:\demo\python27\Lib\site-packages\matplotlib\mpl-data\fonts\ttf`
            + Delete the cache files of matplotlib font located in the Windows user’s directory, e.g., `C:\Users\ZhuLJ\.matplotlib`. The cache files may be fontList.cache and `fontList.json` for Python 2, and `fontList.py3k.cache` and `fontList-v300.json` for Python 3.
            + Now the new font should be available for plotting. If the new font is not rendered, it may be because matplotlib needs to refresh its font cache. Run the following Python code to rebuild it.
    + Font sizes of title (`TITLE_FONTSIZE`), legend (`LEGEND_FONTSIZE`), axis-tick label (`TICKLABEL_FONTSIZE`), axis label (`AXISLABEL_FONTSIZE`), and universal label (`LABEL_FONTSIZE`) in points.
    + `DPI`: The resolution in dots (Integer) per inch, which is applied to non-vector formats.

```
[SEIMS_Model]
HOSTNAME = 127.0.0.1
PORT = 27017
MODEL_DIR = D:\demo\SEIMS\data\youwuzhen\demo_youwuzhen30m_longterm_model
BIN_DIR = D:\demo\SEIMS\bin
scenarioID = 0
calibrationID = -1

[PARAMETERS]
PLOT_SUBBASINID = 4
PLOT_VARIABLES = Q SED

[OPTIONAL_PARAMETERS]
# Calibration period (UTCTIME)
Cali_Time_start = 2013-01-01 00:00:00
Cali_Time_end = 2015-12-31 23:59:59
# Validation period (UTCTIME)
#Vali_Time_start = 2013-01-01 00:00:00
#Vali_Time_end = 2013-12-31 23:59:59

[OPTIONAL_MATPLOT_SETTINGS]
FIGURE_FORMATS = PDF,PNG
LANG_CN = False
FONT_TITLE = Times New Roman
TITLE_FONTSIZE = 14
LEGEND_FONTSIZE = 12
TICKLABEL_FONTSIZE = 12
AXISLABEL_FONTSIZE = 12
LABEL_FONTSIZE = 14
DPI = 300
```

# Advanced usage
The Python scripts of postprocessing are in `SEIMS/seims/postprocess`. The `main.py` is the entrance of postprocessing which can be executed though the unified format of running SEIMS Python scripts, e.g.,

```python
cd D:\demo\SEIMS\seims\postprocess
python main.py -ini D:\demo\SEIMS\data\youwuzhen\workspace\postprocess.ini
```

To illustrate the functionality of postprocessing, Figure 4 showed the calibration and validation of the simulated streamflow (Q, m<sup>3</sup>/s) at the watershed outlet derived from the demo Youwuzhen watershed model with default model parameters with Chinese labels and legend texts based on `YaHei Mono font` (see below for changes of the configuration). 

```
[OPTIONAL_PARAMETERS]
# Calibration period (UTCTIME)
Cali_Time_start = 2014-01-01 00:00:00
Cali_Time_end = 2015-12-31 23:59:59
# Validation period (UTCTIME)
Vali_Time_start = 2013-01-01 00:00:00
Vali_Time_end = 2013-12-31 23:59:59

[OPTIONAL_MATPLOT_SETTINGS]
FIGURE_FORMATS = PDF,PNG
LANG_CN = True
FONT_TITLE = YaHei Mono
```

# See more...
Also see introduction of the Python package @subpage intro_postprocess_pkg
