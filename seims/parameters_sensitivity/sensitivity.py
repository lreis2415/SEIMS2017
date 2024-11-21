# coding:utf-8
"""Base class of parameters sensitivity analysis.

    @author   : Liangjun Zhu

    @changelog:
    - 17-12-22  - lj - initial implementation.
    - 18-01-11  - lj - integration of screening method and variant-based method.
    - 18-01-16  - lj - split tasks when the run_count is very very large.
    - 18-02-09  - lj - compatible with Python3.
    - 18-07-04  - lj - support MPI version of SEIMS, and bugs fixed.
    - 18-08-24  - lj - Gather the execute time of all model runs.
"""
from __future__ import absolute_import, unicode_literals

from builtins import map
from io import open
import os
import sys
import json
import time
import pickle
from copy import deepcopy

if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
    sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))

import matplotlib as mpl

if os.name != 'nt':  # Force matplotlib to not use any Xwindows backend.
    try:  # The 'warn' parameter of use() is deprecated since Matplotlib 3.1 and will be removed in 3.3.
        mpl.use('Agg', warn=False)
    except TypeError:
        mpl.use('Agg')
import matplotlib.pyplot as plt
import numpy
from typing import List
from pygeoc.utils import FileClass, UtilClass
# Morris screening method
from SALib.sample.morris import sample as morris_spl
from SALib.analyze.morris import analyze as morris_alz
from SALib.plotting.morris import horizontal_bar_plot, covariance_plot
# FAST variant-based method
from SALib.sample.fast_sampler import sample as fast_spl
from SALib.analyze.fast import analyze as fast_alz

from utility import read_data_items_from_txt
from utility import save_png_eps
from utility import SpecialJsonEncoder
# import global_mongoclient as MongoDBObj
from run_seims import MainSEIMS
from preprocess.text import DBTableNames
from preprocess.db_mongodb import ConnectMongoDB
from parameters_sensitivity.config import PSAConfig
from parameters_sensitivity.figure import sample_histograms, empirical_cdf
from run_seims import ParseSEIMSConfig, create_run_model


class Sensitivity(object):
    """Base class of Sensitivity Analysis."""

    def __init__(self, psa_cfg):
        """
        Initialization.
        Args:
            psa_cfg: PSAConfig object.
        """
        self.cfg = psa_cfg  # type: PSAConfig
        self.model = psa_cfg.model  # type: ParseSEIMSConfig
        self.param_defs = dict()
        self.param_values = None
        self.run_count = 0
        self.output_values = None
        self.objnames = list()  # Objective names, e.g., NSE-Q, RMSE-Q, PBIAS-SED
        self.psa_si = dict()

    def run(self):
        """PSA workflow."""
        self.reset_simulation_timerange()
        self.read_param_ranges()
        self.generate_samples()
        self.write_param_values_to_mongodb()
        self.evaluate_models()
        self.calculate_sensitivity()

    def plot(self):
        try:
            self.plot_samples_histogram()
            if self.cfg.method == 'morris':
                self.plot_morris()
                self.plot_cdf()
        except Exception:
            print('Plot failed, please run this function independently.')

    def reset_simulation_timerange(self):
        """Update simulation time range in MongoDB [FILE_IN]."""
        # conn = MongoDBObj.client  # type: MongoClient
        conn = ConnectMongoDB(self.cfg.model.host, self.cfg.model.port).get_conn()
        db = conn[self.model.db_name]
        stime_str = self.model.simu_stime.strftime('%Y-%m-%d %H:%M:%S')
        etime_str = self.model.simu_etime.strftime('%Y-%m-%d %H:%M:%S')
        db[DBTableNames.main_filein].find_one_and_update({'TAG': 'STARTTIME'},
                                                         {'$set': {'VALUE': stime_str}})
        db[DBTableNames.main_filein].find_one_and_update({'TAG': 'ENDTIME'},
                                                         {'$set': {'VALUE': etime_str}})

    def read_param_ranges(self):
        """Read param_rng.def file

           name,lower_bound,upper_bound,group,dist
           (group and dist are optional)

            e.g.,
             Param1,0,1[,Group1][,dist1]
             Param2,0,1[,Group2][,dist2]
             Param3,0,1[,Group3][,dist3]

        Returns:
            a dictionary containing:
            - names - the names of the parameters
            - bounds - a list of lists of lower and upper bounds
            - num_vars - a scalar indicating the number of variables
                         (the length of names)
            - groups - a list of group names (strings) for each variable
            - dists - a list of distributions for the problem,
                        None if not specified or all uniform
        """
        # read param_defs.json if already existed
        if not self.param_defs:
            if FileClass.is_file_exists(self.cfg.outfiles.param_defs_json):
                with open(self.cfg.outfiles.param_defs_json, 'r', encoding='utf-8') as f:
                    self.param_defs = UtilClass.decode_strs_in_dict(json.load(f))
                return
        # read param_range_def file and output to json file
        # conn = MongoDBObj.client  # type: MongoClient
        conn = ConnectMongoDB(self.cfg.model.host, self.cfg.model.port).get_conn()
        db = conn[self.model.db_name]
        collection = db['PARAMETERS']

        names = list()
        bounds = list()
        groups = list()
        dists = list()
        num_vars = 0
        items = read_data_items_from_txt(self.cfg.param_range_def)
        for item in items:
            if len(item) < 3:
                continue
            # find parameter name, print warning message if not existed
            if collection.count_documents({'NAME': item[0]}) <= 0:
                print('WARNING: parameter %s is not existed!' % item[0])
                continue
            num_vars += 1
            names.append(item[0])
            bounds.append([float(item[1]), float(item[2])])
            # If the fourth column does not contain a group name, use
            # the parameter name
            if len(item) >= 4:
                groups.append(item[3])
            else:
                groups.append(item[0])
            if len(item) >= 5:
                dists.append(item[4])
            else:
                dists.append('unif')
        if groups == names:
            groups = None
        elif len(set(groups)) == 1:
            raise ValueError('Only one group defined, results will not bemeaningful')

        # setting dists to none if all are uniform
        # because non-uniform scaling is not needed
        if all([d == 'unif' for d in dists]):
            dists = None

        self.param_defs = {'names': names, 'bounds': bounds,
                           'num_vars': num_vars, 'groups': groups, 'dists': dists}

        # Save as json, which can be loaded by json.load()
        json_data = json.dumps(self.param_defs, indent=4, cls=SpecialJsonEncoder)
        with open(self.cfg.outfiles.param_defs_json, 'w', encoding='utf-8') as f:
            f.write('%s' % json_data)

    def generate_samples(self):
        """Sampling and write to a single file and MongoDB 'PARAMETERS' collection"""
        if self.param_values is None or len(self.param_values) == 0:
            if FileClass.is_file_exists(self.cfg.outfiles.param_values_txt):
                self.param_values = numpy.loadtxt(self.cfg.outfiles.param_values_txt)
                self.run_count = len(self.param_values)
                return
        if not self.param_defs:
            self.read_param_ranges()
        if self.cfg.method == 'morris':
            self.param_values = morris_spl(self.param_defs, self.cfg.morris.N,
                                           self.cfg.morris.num_levels,
                                           optimal_trajectories=self.cfg.morris.optimal_t,
                                           local_optimization=self.cfg.morris.local_opt)
            print(self.param_values)
        elif self.cfg.method == 'fast':
            self.param_values = fast_spl(self.param_defs, self.cfg.fast.N, self.cfg.fast.M)
        else:
            raise ValueError('%s method is not supported now!' % self.cfg.method)
        self.run_count = len(self.param_values)
        # Save as txt file, which can be loaded by numpy.loadtxt()
        numpy.savetxt(self.cfg.outfiles.param_values_txt,
                      self.param_values, delimiter=str(' '), fmt=str('%.4f'))

    def write_param_values_to_mongodb(self):
        """Update Parameters collection in MongoDB.
        Notes:
            The field value of 'CALI_VALUES' of all parameters will be deleted first.
        """
        if not self.param_defs:
            self.read_param_ranges()
        if self.param_values is None or len(self.param_values) == 0:
            self.generate_samples()
        # conn = MongoDBObj.client  # type: MongoClient
        conn = ConnectMongoDB(self.cfg.model.host, self.cfg.model.port).get_conn()
        db = conn[self.model.db_name]
        collection = db['PARAMETERS']
        collection.update_many({}, {'$unset': {ModelParamFields.cali_values: ''}})
        for idx, pname in enumerate(self.param_defs['names']):
            v2str = ','.join(str(v) for v in self.param_values[:, idx])
            collection.find_one_and_update({'NAME': pname}, {'$set': {ModelParamFields.cali_values: v2str}})

    def evaluate_models(self):
        """Run SEIMS for objective output variables, and write out.
        """
        if self.output_values is None or len(self.output_values) == 0:
            if FileClass.is_file_exists(self.cfg.outfiles.output_values_txt):
                self.output_values = numpy.loadtxt(self.cfg.outfiles.output_values_txt)
                return
        assert (self.run_count > 0)

        # model configurations
        model_cfg_dict = self.model.ConfigDict
        # model_cfg_dict.setdefault('do_execute', True)  # By default, the model will be executed

        # Parameters to be evaluated
        input_eva_vars = self.cfg.evaluate_params

        # Computing resources
        # Recommend: n / N * c = cores of one node (taking the args of Slurm as an example)
        # For example, we want to submit the job on 6 nodes, each node has 36 cores,
        #              each SEIMS-based model require 4 processes and each process
        #              with 2 threads (i.e., mpiexec -n 4 seims_mpi -thread 2 ...),
        #              then N = 6, n = 108 (6*36//2), c = 2.
        #              This means, a total of 108//4=27 models can be executed simultaneously.
        arg_N = self.cfg.resource.nnodes
        arg_c = self.model.nthread
        arg_n = -1
        if arg_N >= 1 and self.cfg.resource.ncores_pernode >= 1:
            arg_n = arg_N * self.cfg.resource.ncores_pernode // arg_c

        # split tasks
        #   1. If workload of sensitivity analysis is slurm or scoop,
        #      pnum_task equals n // nprocess of SEIMS-model
        #   2. Otherwise, pnum_task equals 100
        pnum_task = 100
        if self.cfg.resource.workload.lower() == 'slurm' or \
            self.cfg.resource.workload.lower() == 'scoop':
            pnum_task = arg_n // self.cfg.model.nprocess
        task_num = self.run_count // pnum_task
        if task_num == 0:
            split_seqs = [range(self.run_count)]
        else:
            split_seqs = numpy.array_split(numpy.arange(self.run_count), task_num + 1)
            split_seqs = [a.tolist() for a in split_seqs]

        # Loop partitioned tasks
        run_model_stime = time.time()
        exec_times = list()  # execute time of all model runs
        for idx, cali_seqs in enumerate(split_seqs):
            cur_out_file = '%s/outputs_%d.txt' % (self.cfg.outfiles.output_values_dir, idx)
            if FileClass.is_file_exists(cur_out_file):
                continue
            model_cfg_dict_list = list()
            for i, caliid in enumerate(cali_seqs):
                tmpcfg = deepcopy(model_cfg_dict)
                tmpcfg['calibration_id'] = caliid
                model_cfg_dict_list.append(tmpcfg)

            if self.cfg.resource.workload.lower() == 'slurm' or \
                self.cfg.resource.workload.lower() == 'bash':
                from utility.slurmpy import Slurm
                output_models = list()
                model_cmd_list = list()
                for ii_model_cfg in model_cfg_dict_list:
                    output_models.append(create_run_model(ii_model_cfg, do_execute=False))
                for ii_model in output_models:
                    model_cmd_list.append(ii_model.CommandString)

                slurmjob = Slurm('sensitivity_%d' % idx,  # Job name
                                 {
                                     'W': '',  # -W, --wait. Do not exit until all jobs terminate
                                     'partition': self.cfg.resource.partition,
                                     'N': arg_N,  # -N, --nodes. Request N nodes to the job
                                 },
                                 scripts_dir=self.cfg.outfiles.psa_scripts_dir,
                                 log_dir=self.cfg.outfiles.psa_logs_dir,
                                 bash_strict=False if self.cfg.resource.workload.lower() == 'slurm'
                                 else True)
                slurmjob.run('%s &\nwait' % ' &\n'.join(model_cmd_list)
                             if self.cfg.resource.workload.lower() == 'slurm'
                             else '%s\nwait' % '\n'.join(model_cmd_list),
                             _cmd='sbatch' if self.cfg.resource.workload.lower() == 'slurm'
                             else 'bash',
                             name_addition='psa')
                print('Slurm job index %d done!' % idx)

                # Postprocess the models that executed by Slurm job
                for iii in range(len(output_models)):
                    output_models[iii].executed = True
                    output_models[iii].ParseTimespan()
            elif self.cfg.resource.workload.lower() == 'cmd' and (
                self.model.workload.lower() == 'mpi' or self.model.workload == ''):
                # For function testing on Windows
                output_models = list()
                model_cmd_list = list()
                for ii_model_cfg in model_cfg_dict_list:
                    output_models.append(create_run_model(ii_model_cfg, do_execute=False))
                for ii_model in output_models:
                    model_cmd_list.append(ii_model.CommandString)
                # Run model sequentially
                for iii in range(len(output_models)):
                    UtilClass.run_command(model_cmd_list[iii])
                    output_models[iii].executed = True
                    output_models[iii].ParseTimespan()
            else:
                try:  # parallel on multiprocessor or clusters using SCOOP
                    from scoop import futures
                    output_models = list(futures.map(create_run_model,
                                                     model_cfg_dict_list))  # type: List[MainSEIMS]
                except ImportError or ImportWarning:  # serial
                    output_models = list(map(create_run_model,
                                             model_cfg_dict_list))  # type: List[MainSEIMS]
            time.sleep(0.1)  # Wait a moment in case of unpredictable file system error
            # Read observation data from MongoDB only once
            if len(output_models) < 1:  # Although this is not gonna happen, just for insurance.
                continue

            output_models[0].SetMongoClient()
            obs_vars, obs_data_dict = output_models[0].ReadOutletObservations(input_eva_vars)
            output_models[0].UnsetMongoClient()

            if obs_vars is None or (len(obs_vars)) < 1:  # Make sure the observation data exists.
                continue
            # Loop the executed models
            eva_values = list()
            for imod, mod_obj in enumerate(output_models):
                mod_obj.SetMongoClient()
                # Read executable timespan of each model run
                exec_times.append(mod_obj.GetTimespan())
                # Set observation data since there is no need to read from MongoDB.
                if imod != 0:
                    mod_obj.SetOutletObservations(obs_vars, obs_data_dict)
                # Read simulation
                mod_obj.ReadTimeseriesSimulations(self.cfg.psa_stime, self.cfg.psa_etime)
                # Calculate NSE, R2, RMSE, PBIAS, RSR, ln(NSE), NSE1, and NSE3
                self.objnames, obj_values = mod_obj.CalcTimeseriesStatistics(mod_obj.sim_obs_dict)
                eva_values.append(obj_values)
                # delete model output directory and GridFS files for saving storage
                mod_obj.clean()
                mod_obj.UnsetMongoClient()
            if not isinstance(eva_values, numpy.ndarray):
                eva_values = numpy.array(eva_values)
            numpy.savetxt(cur_out_file, eva_values, delimiter=str(' '), fmt=str('%.4f'))
            # Save as pickle data for further usage. DO not save all models which maybe very large!
            cur_model_out_file = '%s/models_%d.pickle' % (self.cfg.outfiles.output_values_dir, idx)
            with open(cur_model_out_file, 'wb') as f:
                pickle.dump(output_models, f)
        exec_times = numpy.array(exec_times)
        numpy.savetxt('%s/exec_time_allmodelruns.txt' % self.cfg.psa_outpath,
                      exec_times, delimiter=str(' '), fmt=str('%.4f'))
        print('Running time of all SEIMS models:\n'
              '\tIO\tCOMP\tSIMU\tRUNTIME\n'
              'MAX\t%s\n'
              'MIN\t%s\n'
              'AVG\t%s\n'
              'SUM\t%s\n' % ('\t'.join('%.3f' % v for v in exec_times.max(0)),
                             '\t'.join('%.3f' % v for v in exec_times.min(0)),
                             '\t'.join('%.3f' % v for v in exec_times.mean(0)),
                             '\t'.join('%.3f' % v for v in exec_times.sum(0))))
        print('Running time of executing SEIMS models: %.2fs' % (time.time() - run_model_stime))
        # Save objective names as pickle data for further usgae
        with open('%s/objnames.pickle' % self.cfg.psa_outpath, 'wb') as f:
            pickle.dump(self.objnames, f)

        # load the first part of output values
        self.output_values = numpy.loadtxt('%s/outputs_0.txt' % self.cfg.outfiles.output_values_dir)
        if task_num == 0:
            import shutil
            shutil.move('%s/outputs_0.txt' % self.cfg.outfiles.output_values_dir,
                        self.cfg.outfiles.output_values_txt)
            shutil.rmtree(self.cfg.outfiles.output_values_dir)
            return
        for idx in range(1, task_num + 1):
            tmp_outputs = numpy.loadtxt('%s/outputs_%d.txt' % (self.cfg.outfiles.output_values_dir,
                                                               idx))
            self.output_values = numpy.concatenate((self.output_values, tmp_outputs))
        numpy.savetxt(self.cfg.outfiles.output_values_txt,
                      self.output_values, delimiter=str(' '), fmt=str('%.4f'))

    def calculate_sensitivity(self):
        """Calculate Morris elementary effects.
           It is worth to be noticed that evaluate_models() allows to return
           several output variables, hence we should calculate each of them separately.
        """
        if not self.objnames:
            if FileClass.is_file_exists('%s/objnames.pickle' % self.cfg.psa_outpath):
                with open('%s/objnames.pickle' % self.cfg.psa_outpath, 'rb') as f:
                    self.objnames = pickle.load(f)
        if not self.psa_si:
            if FileClass.is_file_exists(self.cfg.outfiles.psa_si_json):
                with open(self.cfg.outfiles.psa_si_json, 'rb') as f:
                    self.psa_si = UtilClass.decode_strs_in_dict(json.load(f))
                    return
        if self.output_values is None or len(self.output_values) == 0:
            self.evaluate_models()
        if self.param_values is None or len(self.param_values) == 0:
            self.generate_samples()
        if not self.param_defs:
            self.read_param_ranges()
        row, col = self.output_values.shape
        assert (row == self.run_count)
        for i in range(col):
            print(self.objnames[i])
            if self.cfg.method == 'morris':
                tmp_Si = morris_alz(self.param_defs,
                                    self.param_values,
                                    self.output_values[:, i],
                                    conf_level=0.95, print_to_console=True,
                                    num_levels=self.cfg.morris.num_levels)
            elif self.cfg.method == 'fast':
                tmp_Si = fast_alz(self.param_defs, self.output_values[:, i],
                                  print_to_console=True)
            else:
                raise ValueError('%s method is not supported now!' % self.cfg.method)
            self.psa_si[i] = tmp_Si
        # print(self.psa_si)
        # Save as json, which can be loaded by json.load()
        json_data = json.dumps(self.psa_si, indent=4, cls=SpecialJsonEncoder)
        with open(self.cfg.outfiles.psa_si_json, 'w', encoding='utf-8') as f:
            f.write('%s' % json_data)
        self.output_psa_si()

    def output_psa_si(self):
        psa_sort_txt = self.cfg.outfiles.psa_si_sort_txt
        psa_sort_dict = dict()
        param_names = self.param_defs.get('names')
        psa_sort_dict['names'] = param_names[:]
        for idx, si_dict in self.psa_si.items():
            if 'objnames' not in psa_sort_dict:
                psa_sort_dict['objnames'] = list()
            psa_sort_dict['objnames'].append(self.objnames[idx])
            for param, values in si_dict.items():
                if param == 'names':
                    continue
                if param not in psa_sort_dict:
                    psa_sort_dict[param] = list()
                psa_sort_dict[param].append(values[:])
        # print(psa_sort_dict)
        output_str = ''
        header = ' ,' + ','.join(psa_sort_dict['objnames']) + '\n'
        for outvar, values in psa_sort_dict.items():
            if outvar == 'names' or outvar == 'objnames':
                continue
            mtx = numpy.transpose(numpy.array(values))  # col is 'objnames' and row is 'names'
            sortidx = numpy.argsort(numpy.argsort(mtx, axis=0), axis=0)
            # concatenate output string
            output_str += outvar + '\n'
            output_str += header
            for i, v in enumerate(mtx):
                output_str += param_names[i] + ',' + ','.join('{}'.format(i) for i in v) + '\n'
            output_str += '\n'
            output_str += '%s-Sorted\n' % outvar
            output_str += header
            for i, v in enumerate(sortidx):
                output_str += param_names[i] + ',' + ','.join('{}'.format(i) for i in v) + '\n'
            output_str += '\n'
            print(output_str)
        with open(psa_sort_txt, 'w', encoding='utf-8') as f:
            f.write('%s' % output_str)

    def plot_samples_histogram(self):
        """Save plot as png(300 dpi) and eps (vector)."""
        # Plots histogram of all samples
        if not self.param_defs:
            self.read_param_ranges()
        if self.param_values is None or len(self.param_values) == 0:
            self.generate_samples()
        sample_histograms(self.param_values, self.param_defs.get('names'),
                          self.cfg.morris.num_levels, self.cfg.psa_outpath, 'samples_histgram',
                          {'color': 'black', 'histtype': 'step'}, self.cfg.plot_cfg)

    def plot_morris(self):
        """Save plot as png(300 dpi) and eps (vector)."""
        if not self.psa_si:
            self.calculate_sensitivity()
        for i, v in enumerate(self.objnames):
            fig, (ax1, ax2) = plt.subplots(1, 2)
            horizontal_bar_plot(ax1, self.psa_si.get(i), {}, sortby='mu_star')
            covariance_plot(ax2, self.psa_si.get(i), {})
            save_png_eps(plt, self.cfg.psa_outpath, 'mu_star_%s' % v)
            # close current plot in case of 'figure.max_open_warning'
            plt.cla()
            plt.clf()
            plt.close()

    def plot_cdf(self):
        if not self.param_defs:
            self.read_param_ranges()
        if self.param_values is None or len(self.param_values) == 0:
            self.generate_samples()
        if self.output_values is None or len(self.output_values) == 0:
            self.evaluate_models()
        param_names = self.param_defs.get('names')
        for i, objn in enumerate(self.objnames):
            values = self.output_values[:, i]
            if 'NSE' in objn:  # NSE series, i.e., NSE, lnNSE, NSE1, and NSE3
                empirical_cdf(values, [0], self.param_values, param_names,
                              self.cfg.morris.num_levels,
                              self.cfg.psa_outpath, 'cdf_%s' % self.objnames[i],
                              {'histtype': 'step'}, self.cfg.plot_cfg)
            elif 'R-square' in objn:  # R-square, equally divided as two classes
                empirical_cdf(values, 2, self.param_values, param_names,
                              self.cfg.morris.num_levels,
                              self.cfg.psa_outpath, 'cdf_%s' % self.objnames[i],
                              {'histtype': 'step'}, self.cfg.plot_cfg)
            elif 'RSR' in objn:  # RSR
                empirical_cdf(values, [1], self.param_values, param_names,
                              self.cfg.morris.num_levels,
                              self.cfg.psa_outpath, 'cdf_%s' % self.objnames[i],
                              {'histtype': 'step'}, self.cfg.plot_cfg)


if __name__ == '__main__':
    from parameters_sensitivity.config import get_psa_config

    cf, method = get_psa_config()
    cfg = PSAConfig(cf, method=method)

    print(cfg.param_range_def)

    saobj = Sensitivity(cfg)
    saobj.write_param_values_to_mongodb()
    # saobj.calculate_sensitivity()
    # saobj.plot_samples_histogram()
    # saobj.plot_morris()
