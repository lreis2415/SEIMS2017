模拟后处理 {#getstart_runmodel_postprocessing}
======================================================

[TOC]

在运行基于SEIMS的流域模型后，可以使用后处理工具来计算模型性能统计指标，如Nash-Sutcliffe效率（NSE），并绘制河段出口的时间序列数据（如流量和泥沙）的水文图。
# 简单用法

打开一个CMD窗口，输入以下命令以对游屋圳流域模型执行后处理。

```
cd D:\demo\SEIMS\seims\test
D:
python demo_postprocess.py -name youwuzhen
```

运行时日志会显示输入路径和输出的模型性能统计指标（图 2:5 1）。

默认情况下，流域出口的流量（如 `Q.txt`）和泥沙（如`SED.txt`）的水文图被绘制并以PNG和PDF格式保存，如`SEIMS\data\youwuzhen\demo_youwuzhen30m_longterm_model\OUTPUT0\png\Q-2013-01-01-2015-12-31.png`（图 2:5 2）。

# 后处理配置文件
与数据预处理类似，后处理的简单用法也包括根据SEIMS的本地路径生成`INI`配置文件（如图 2:5 3），并通过高级用法执行后处理（见章节 2:5.3）。

后处理的配置文件，如游屋圳流域模型的配置文件，见图 2:5 3，包含四个部分，即`SEIMS_Model`、`PARAMETERS`、`OPTIONAL_PARAMETERS`和`OPTIONAL_MATPLOT_SETTING`。其中，`OPTIONAL_PARAMETERS`和`OPTIONAL_MATPLOT_SETTING`为可选部分。节和选项的名称不能更改。
+ `SEIMS_Model`： 基于 SEIMS 流域模型的基本设置，包括 `HOSTNAME`、`PORT`、`MODEL_DIR`、`BIN_DIR`、`scenarioID`和`calibrationID`等。这些选项的详细含义请参考章节 2:4.3.2。
+ `PARAMETERS`
    + `PLOT_SUBBASINID`: 要绘制图形的子流域ID。
    + `PLOT_VARIABLES`: 要绘制的变量，如`Q`、`SED`和`CH_TN`。多个变量用逗号分隔。注意，指定的子流域ID和对应的变量**必须**在file.out（章节 2:4.2.2）和/或 MongoDB 主数据库的`FILE_OUT` 集合中设置为时间序列输出数据。如果指定变量没有对应的观测数据，无法计算模型性能统计指标。
+ `OPTIONAL_PARAMETERS`： （可选）模型率定和严重的时间范围。率定和验证时间的起始和结束必须同时出现或同时省略。
    + `Cali_Time_start`： 模型率定的起始时间，格式为 `YYYY-MM-DD HH:MM:SS`。
    + `Cali_Time_end`： 模型率定的结束时间。
    + `Vali_Time_start`： 模型验证的起始时间。
    + `Vali_Time_end`： 模型验证的结束时间。
+ `OPTIONAL_MATPLOT_SETTINGS`： （可选）matplotlib绘图设置。
    + `FIGURE_FORMATS`： 输出图形的格式，可以是PNG、JPG、TIF、PDF、EPS、SVG和PS中的一个或多个。多个格式用逗号分隔，例如PNG、JPG、EPS。
    + `LANG_CN`： 是否在标签和图例等文本中包含中文，True（包含）或False（不包含）。
    + `FONT_TITLE`： 输出图形的字体标题（字体名称）。以下是检查可用字体或使用新字体的提示。
        + 通过运行Python代码获取当前可用字体。
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

        + 增加并使用新字体：
            + 下载支持中英文的 `.ttf`格式字体文件（例如YaHei Mono）。如果只有`.ttf`格式，可以使用transfonter工具将其转换为`.ttf`。
            + 将字体文件复制到matplotlib的字体目录，如`D:\demo\python27\Lib\site-packages\matplotlib\mpl-data\fonts\ttf`
            + 删除matplotlib字体缓存文件，其位于Windows用户目录下，如`C:\Users\ZhuLJ\.matplotlib`。对于Python 2，缓存文件可以是`fontList.cache`和`fontList.json`；对于Python 3，缓存文件可以是`fontList.py3k.cache` 和 `fontList-v300.json`。
            + 现在，新字体应该可以用于绘图了。如果没有渲染新字体，可能是因为matplotlib需要刷新字体缓存。运行以下Python代码重建字体缓存。
    + 标题字体（`TITLE_FONTSIZE`）、图例（`LEGEND_FONTSIZE`）、坐标轴刻度标签（`TICKLABEL_FONTSIZE`）、坐标轴标签（`AXISLABEL_FONTSIZE`）和通用标签（`LABEL_FONTSIZE`）的字体大小，单位为点。
    + `DPI`： 分辨率，单位为每英寸点（整型），适用于非矢量格式。

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

# 高级用法
后处理的Python脚本位于`SEIMS/seims/postprocess`文件夹中。`main.py`是后处理的入口脚本，可以通过统一的SEIMS Python脚本运行格式执行，例如：

```python
cd D:\demo\SEIMS\seims\postprocess
python main.py -ini D:\demo\SEIMS\data\youwuzhen\workspace\postprocess.ini
```

为了展示后处理的功能，图4展示了使用默认模型参数的游屋圳流域模型模拟的流域出口流量（Q, m<sup>3</sup>/s）的率定和验证结果。图中使用了中文标签和图例文本，基于 `YaHei Mono font` 字体（有关配置更改见下文）。

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

# 更多介绍...
Also see introduction of the Python package @subpage intro_postprocess_pkg