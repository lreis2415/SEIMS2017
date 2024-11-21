# coding: utf-8
"""User defined functions.

    @author   : Liangjun Zhu

    @changelog:
    - 18-01-22  - lj - initial implementation.
    - 18-02-09  - lj - compatible with Python3.
    - 19-01-07  - lj - incorporated with PlotConfig
"""
from __future__ import absolute_import, unicode_literals

import json
import os
import pickle
import sys
from io import open
import numpy as np

from preprocess.text import ModelParamFields

if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
    sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))

import numpy
import matplotlib as mpl

if os.name != 'nt':  # Force matplotlib to not use any Xwindows backend.
    try:  # The 'warn' parameter of use() is deprecated since Matplotlib 3.1 and will be removed in 3.3.
        mpl.use('Agg', warn=False)
    except TypeError:
        mpl.use('Agg')
import matplotlib.dates as mdates
import matplotlib.pyplot as plt

from typing import Optional
from pygeoc.utils import StringClass, is_string

from utility import save_png_eps, PlotConfig
# import global_mongoclient as MongoDBObj
from preprocess.db_mongodb import MongoClient, ConnectMongoDB
from parameters_sensitivity.sensitivity import SpecialJsonEncoder

def write_param_values_to_mongodb(host, port, spatial_db, param_names, pop_genes, impact_subbasins):
    # update Parameters collection in MongoDB
    # conn = MongoDBObj.client  # type: MongoClient
    conn = ConnectMongoDB(host, port).get_conn()
    db = conn[spatial_db]
    collection = db['PARAMETERS']
    collection.update_many({}, {'$unset': {ModelParamFields.cali_values: '', ModelParamFields.impact_subbasins: ''}})
    param_names = np.array(param_names)
    # impact_subbasins = np.array(impact_subbasins)
    param_names_unique = np.unique(param_names)
    for name in param_names_unique:
        # find indices of unique names
        idx = np.where(param_names == name)[0]
        gene_values = pop_genes[:, idx]
        gene_values = gene_values.tolist()
        subbasins = []
        for i in idx:
            subbasins.append(impact_subbasins[i])
        # subbasins = impact_subbasins[idx]
        collection.find_one_and_update(
            {'NAME': name},
            {'$set': {
                ModelParamFields.cali_values: gene_values,
                ModelParamFields.impact_subbasins: subbasins
            }}
        )



    # for i in range(len(param_names)):
    #     name = param_names[i]
    #     if name not in cali_values:
    #         cali_values[name] = dict()
    #         cali_values[name]['values'] = list()
    #         cali_values[name]['subbasins'] = list()
    #     cali_values[name]['values'].append(pop_genes[:, i])
    #     cali_values[name]['subbasins'].append(impact_subbasins[i])
    # for name, v in cali_values.items():
    #     values = [list(x) for x in v['values']]
    #     subbasins = [list(x) for x in v['subbasins']]
    #     collection.find_one_and_update(
    #         {'NAME': name},
    #         {'$set': {
    #             ModelParamFields.cali_values: values,
    #             ModelParamFields.impact_subbasins: subbasins
    #         }}
    #     )


def output_population_details(pops, outdir, gen_num,
                              plot_cfg=None  # type: Optional[PlotConfig]
                              ):
    """Output population details, i.e., the simulation data, etc."""

    # Save as json, which can be loaded by json.load()
    # 1. Save the time series simulation data of the entire simulation period
    all_sim_data = list()
    pickle_file = outdir + os.path.sep + 'gen%d_allSimData.pickle' % gen_num
    with open(pickle_file, 'wb') as f:
        for ind in pops:
            all_sim_data.append(ind.sim.data)
        pickle.dump(all_sim_data, f)

    # 2. Save the matched observation-simulation data of validation period,
    #      and the simulation data separately.
    cali_sim_obs_data = list()
    cali_sim_data = list()
    json_file = outdir + os.path.sep + 'gen%d_caliObsData.json' % gen_num
    with open(json_file, 'w', encoding='utf-8') as f:
        for ind in pops:
            ind.cali.sim_obs_data['Gen'] = ind.gen
            ind.cali.sim_obs_data['ID'] = ind.id
            ind.cali.sim_obs_data['var_name'] = ind.cali.vars
            cali_sim_obs_data.append(ind.cali.sim_obs_data)
        json_data = json.dumps(cali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
        f.write('%s' % json_data)
    pickle_file = outdir + os.path.sep + 'gen%d_caliSimData.pickle' % gen_num
    with open(pickle_file, 'wb') as f:
        for ind in pops:
            cali_sim_data.append(ind.cali.data)
        pickle.dump(cali_sim_data, f)
    # 3. Save the matched observation-simulation data of calibration period,
    #      and the simulation data separately.
    vali_sim_obs_data = list()
    vali_sim_data = list()
    if pops[0].vali.valid:
        json_file = outdir + os.path.sep + 'gen%d_valiObsData.json' % gen_num
        with open(json_file, 'w', encoding='utf-8') as f:
            for ind in pops:
                ind.vali.sim_obs_data['Gen'] = ind.gen
                ind.vali.sim_obs_data['ID'] = ind.id
                ind.vali.sim_obs_data['var_name'] = ind.vali.vars
                vali_sim_obs_data.append(ind.vali.sim_obs_data)
            json_data = json.dumps(vali_sim_obs_data, indent=4, cls=SpecialJsonEncoder)
            f.write('%s' % json_data)
        pickle_file = outdir + os.path.sep + 'gen%d_valiSimData.pickle' % gen_num
        with open(pickle_file, 'wb') as f:
            for ind in pops:
                vali_sim_data.append(ind.vali.data)
            pickle.dump(vali_sim_data, f)
    # 4. Try to plot.
    if plot_cfg is None:
        plot_cfg = PlotConfig()
    try:
        # Calculate 95PPU for current generation, and plot the desired variables, e.g., Q and SED
        calculate_95ppu(cali_sim_obs_data, cali_sim_data, outdir, gen_num,
                        vali_sim_obs_data, vali_sim_data,
                        plot_cfg=plot_cfg)
    except Exception:
        pass


def calculate_95ppu(sim_obs_data, sim_data, outdir, gen_num,
                    vali_sim_obs_data=None, vali_sim_data=None,
                    plot_cfg=None  # type: Optional[PlotConfig]
                    ):
    """Calculate 95% prediction uncertainty and plot the hydrographs."""
    if plot_cfg is None:
        plot_cfg = PlotConfig()
    plt.rcParams['xtick.direction'] = 'out'
    plt.rcParams['ytick.direction'] = 'out'
    plt.rcParams['font.family'] = plot_cfg.font_name
    plt.rcParams['timezone'] = 'UTC'
    plt.rcParams['mathtext.fontset'] = 'custom'
    plt.rcParams['mathtext.it'] = 'STIXGeneral:italic'
    plt.rcParams['mathtext.bf'] = 'STIXGeneral:italic:bold'
    if len(sim_data) < 2:
        return
    var_name = sim_obs_data[0]['var_name']
    for idx, var in enumerate(var_name):
        plot_validation = False
        if vali_sim_obs_data and vali_sim_data and var in vali_sim_obs_data[0]['var_name']:
            plot_validation = True
        ylabel_str = var
        if var in ['Q', 'QI', 'QG', 'QS']:
            ylabel_str += ' (m$^3$/s)'
        elif 'CONC' in var.upper():  # Concentrate
            if 'SED' in var.upper():
                ylabel_str += ' (g/L)'
            else:
                ylabel_str += ' (mg/L)'
        elif 'SED' in var.upper():  # amount
            ylabel_str += ' (kg)'
        cali_obs_dates = sim_obs_data[0][var]['UTCDATETIME'][:]
        if is_string(cali_obs_dates[0]):
            cali_obs_dates = [StringClass.get_datetime(s) for s in cali_obs_dates]
        obs_dates = cali_obs_dates[:]
        order = 1  # By default, the calibration period is before the validation period.
        if plot_validation:
            vali_obs_dates = vali_sim_obs_data[0][var]['UTCDATETIME']
            if is_string(vali_obs_dates[0]):
                vali_obs_dates = [StringClass.get_datetime(s) for s in vali_obs_dates]
            if vali_obs_dates[-1] <= cali_obs_dates[0]:
                order = 0
                obs_dates = vali_obs_dates + obs_dates
            else:
                obs_dates += vali_obs_dates
        obs_data = sim_obs_data[0][var]['Obs'][:]
        if plot_validation:
            if order:
                obs_data += vali_sim_obs_data[0][var]['Obs'][:]
            else:
                obs_data = vali_sim_obs_data[0][var]['Obs'][:] + obs_data

        cali_sim_dates = list(sim_data[0].keys())
        if is_string(cali_sim_dates[0]):
            cali_sim_dates = [StringClass.get_datetime(s) for s in cali_sim_dates]
        sim_dates = cali_sim_dates[:]
        if plot_validation:
            vali_sim_dates = list(vali_sim_data[0].keys())
            if is_string(vali_sim_dates[0]):
                vali_sim_dates = [StringClass.get_datetime(s) for s in vali_sim_dates]
            if order:
                sim_dates += vali_sim_dates
            else:
                sim_dates = vali_sim_dates + sim_dates
        sim_data_list = list()
        caliBestIdx = -1
        caliBestNSE = -9999.
        for idx2, ind in enumerate(sim_data):
            tmp = numpy.array(list(ind.values()))
            tmp = tmp[:, idx]
            if sim_obs_data[idx2][var]['NSE'] > caliBestNSE:
                caliBestNSE = sim_obs_data[idx2][var]['NSE']
                caliBestIdx = idx2
            tmpsim = tmp.tolist()
            if plot_validation:
                tmp_data = numpy.array(list(vali_sim_data[idx2].values()))[:, idx].tolist()
                if order:
                    tmpsim += tmp_data
                else:
                    tmpsim = tmp_data + tmpsim
            sim_data_list.append(tmpsim)

        sim_best = numpy.array(list(sim_data[caliBestIdx].values()))[:, idx]
        sim_best = sim_best.tolist()
        if plot_validation:
            tmp_data = numpy.array(list(vali_sim_data[caliBestIdx].values()))[:, idx].tolist()
            if order:
                sim_best += tmp_data
            else:
                sim_best = tmp_data + sim_best
        sim_data_list = numpy.array(sim_data_list)
        ylows = numpy.percentile(sim_data_list, 2.5, 0, interpolation='nearest')
        yups = numpy.percentile(sim_data_list, 97.5, 0, interpolation='nearest')

        def calculate_95ppu_efficiency(obs_data_list, obs_dates_list, sim_dates_list):
            # type: (...) -> (float, float)
            count = 0
            ylows_obs = list()
            yups_obs = list()
            for oi, ov in enumerate(obs_data_list):
                try:
                    si = sim_dates_list.index(obs_dates_list[oi])
                    ylows_obs.append(ylows[si])
                    yups_obs.append(yups[si])
                    if ylows[si] <= ov <= yups[si]:
                        count += 1
                except Exception:
                    continue
            p = float(count) / len(obs_data_list)
            ylows_obs = numpy.array(ylows_obs)
            yups_obs = numpy.array(yups_obs)
            r = numpy.mean(yups_obs - ylows_obs) / numpy.std(numpy.array(obs_data_list))
            return p, r

        # concatenate text
        p_value, r_value = calculate_95ppu_efficiency(sim_obs_data[0][var]['Obs'],
                                                      cali_obs_dates,
                                                      list(sim_data[0].keys()))
        txt = 'P-factor: %.2f\nR-factor: %.2f\n' % (p_value, r_value)
        txt += u'某一最优模拟\n' if plot_cfg.plot_cn else 'One best simulation:\n'
        txt += '    $\mathit{NSE}$: %.2f\n' \
               '    $\mathit{RSR}$: %.2f\n' \
               '    $\mathit{PBIAS}$: %.2f%%\n' \
               '    $\mathit{R^2}$: %.2f' % (sim_obs_data[caliBestIdx][var]['NSE'],
                                             sim_obs_data[caliBestIdx][var]['RSR'],
                                             sim_obs_data[caliBestIdx][var]['PBIAS'],
                                             sim_obs_data[caliBestIdx][var]['R-square'])
        # concatenate text of validation if needed
        vali_txt = ''
        if plot_validation:
            p_value, r_value = calculate_95ppu_efficiency(vali_sim_obs_data[0][var]['Obs'],
                                                          vali_obs_dates,
                                                          list(vali_sim_data[0].keys()))
            vali_txt = 'P-factor: %.2f\nR-factor: %.2f\n\n' % (p_value, r_value)
            vali_txt += '    $\mathit{NSE}$: %.2f\n' \
                        '    $\mathit{RSR}$: %.2f\n' \
                        '    $\mathit{PBIAS}$: %.2f%%\n' \
                        '    $\mathit{R^2}$: %.2f' % (vali_sim_obs_data[caliBestIdx][var]['NSE'],
                                                      vali_sim_obs_data[caliBestIdx][var]['RSR'],
                                                      vali_sim_obs_data[caliBestIdx][var]['PBIAS'],
                                                      vali_sim_obs_data[caliBestIdx][var]['R-square'])
        # plot
        fig, ax = plt.subplots(figsize=(12, 4))
        ax.fill_between(sim_dates, ylows.tolist(), yups.tolist(),
                        color=(0.8, 0.8, 0.8), label='95PPU')
        observed_label = u'实测值' if plot_cfg.plot_cn else 'Observed points'
        ax.scatter(obs_dates, obs_data, marker='.', s=20,
                   color='g', label=observed_label)
        besesim_label = u'最优模拟' if plot_cfg.plot_cn else 'Best simulation'
        ax.plot(sim_dates, sim_best, linestyle='--', color='red', linewidth=1,
                label=besesim_label)
        ax.set_xlim(left=min(sim_dates), right=max(sim_dates))
        ax.set_ylim(bottom=0.)
        date_fmt = mdates.DateFormatter('%m-%d-%y')
        ax.xaxis.set_major_formatter(date_fmt)
        ax.tick_params(axis='x', bottom=True, top=False, length=5, width=2, which='major',
                       labelsize=plot_cfg.tick_fsize)
        ax.tick_params(axis='y', left=True, right=False, length=5, width=2, which='major',
                       labelsize=plot_cfg.tick_fsize)
        plt.xlabel(u'时间' if plot_cfg.plot_cn else 'Date time',
                   fontsize=plot_cfg.axislabel_fsize)
        plt.ylabel(ylabel_str, fontsize=plot_cfg.axislabel_fsize)
        # plot separate dash line
        delta_dt = (sim_dates[-1] - sim_dates[0]) // 9
        delta_dt2 = (sim_dates[-1] - sim_dates[0]) // 35
        sep_time = sim_dates[-1]
        time_pos = [sep_time - delta_dt]
        time_pos2 = [sep_time - 2 * delta_dt]
        ymax, ymin = ax.get_ylim()
        yc = abs(ymax - ymin) * 0.9
        if plot_validation:
            sep_time = vali_sim_dates[0] if vali_sim_dates[0] >= cali_sim_dates[-1] \
                else cali_sim_dates[0]
            cali_vali_labels = [(u'验证期' if plot_cfg.plot_cn else 'Calibration'),
                                (u'率定期' if plot_cfg.plot_cn else 'Validation')]
            if not order:
                cali_vali_labels.reverse()
            time_pos = [sep_time - delta_dt, sep_time + delta_dt2]
            time_pos2 = [sep_time - 2 * delta_dt, sep_time + delta_dt2]
            ax.axvline(sep_time, color='black', linestyle='dashed', linewidth=2)
            plt.text(time_pos[0], yc, cali_vali_labels[0],
                     fontdict={'style': 'italic', 'weight': 'bold',
                               'size': plot_cfg.label_fsize},
                     color='black')
            plt.text(time_pos[1], yc, cali_vali_labels[1],
                     fontdict={'style': 'italic', 'weight': 'bold',
                               'size': plot_cfg.label_fsize},
                     color='black')

        # add legend
        handles, labels = ax.get_legend_handles_labels()
        figorders = [labels.index('95PPU'), labels.index(observed_label),
                     labels.index(besesim_label)]
        ax.legend([handles[idx] for idx in figorders], [labels[idx] for idx in figorders],
                  fontsize=plot_cfg.legend_fsize, loc=2, framealpha=0.8)
        # add text
        cali_pos = time_pos[0] if order else time_pos[1]
        plt.text(cali_pos, yc * 0.5, txt, color='red', fontsize=plot_cfg.label_fsize - 1)
        if plot_validation:
            vali_pos = time_pos[1] if order else time_pos[0]
            plt.text(vali_pos, yc * 0.5, vali_txt, color='red', fontsize=plot_cfg.label_fsize - 1)
        # fig.autofmt_xdate(rotation=0, ha='center')
        plt.tight_layout()
        save_png_eps(plt, outdir, 'Gen%d_95ppu_%s' % (gen_num, var), plot_cfg)
        # close current plot in case of 'figure.max_open_warning'
        plt.cla()
        plt.clf()
        plt.close()


if __name__ == '__main__':
    wp = r'C:\z_data\ChangTing\seims_models_phd\youwuzhen10m_longterm_model\Cali_NSGAII_Gen_3_Pop_4'
    simdir = wp + os.path.sep + 'simulated_data'
    all_cali_obs_data = list()
    all_cali_data = list()
    gen_cali_id = list()
    with open(simdir + os.path.sep + 'gen0_caliObsData.json', 'r', encoding='utf-8') as f:
        cali_obs_data = json.load(f)
    with open(simdir + os.path.sep + 'gen0_caliSimData.pickle', 'r', encoding='utf-8') as f:
        cali_data = pickle.load(f)
    # validation data
    with open(simdir + os.path.sep + 'gen0_valiObsData.json', 'r', encoding='utf-8') as f:
        vali_obs_data = json.load(f)
    with open(simdir + os.path.sep + 'gen0_valiSimData.pickle', 'r', encoding='utf-8') as f:
        vali_data = pickle.load(f)

    for a, b in zip(cali_obs_data, cali_data):
        if [a['Gen'], a['ID']] not in gen_cali_id:
            gen_cali_id.append([a['Gen'], a['ID']])
            all_cali_obs_data.append(a)
            all_cali_data.append(b)
    calculate_95ppu(cali_obs_data, cali_data, simdir, 0, vali_obs_data, vali_data)
    print('Total simulations %d' % len(all_cali_obs_data))
