#coding:utf-8
"""Base class of calibration.

    @author   : Liangjun Zhu

    @changelog:
    - 18-01-22  - lj - design and implement.
    - 18-01-25  - lj - redesign the individual class, add 95PPU, etc.
    - 18-02-09  - lj - compatible with Python3.
    - 20-07-22  - lj - update to use global MongoClient object.
"""
from __future__ import absolute_import, unicode_literals

import time
from collections import OrderedDict
import os
import sys
from copy import deepcopy

if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
    sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))

from typing import Optional
from pygeoc.utils import FileClass

from utility import read_data_items_from_txt
# import global_mongoclient as MongoDBObj
from preprocess.db_mongodb import MongoClient, ConnectMongoDB
from preprocess.text import DBTableNames
from run_seims import MainSEIMS
from calibration.config import CaliConfig, get_optimization_config
from calibration.sample_lhs import lhs


class TimeseriesData(object):
    """Time series data, for observation and simulation data."""

    def __init__(self):
        self.vars = list()
        self.data = OrderedDict()


class ObsSimData(object):
    """Paired time series data of observation and simulation, associated with statistics."""

    def __init__(self):
        self.vars = list()
        self.data = OrderedDict()
        self.sim_obs_data = OrderedDict()
        self.objnames = list()
        self.objvalues = list()
        self.valid = False

    def efficiency_values(self, varname, effnames):
        values = list()
        tmpvars = list()
        for name in effnames:
            tmpvar = '%s-%s' % (varname, name)
            if tmpvar not in self.objnames:
                values.append(-9999.)
            else:
                if name.upper() == 'PBIAS':
                    tmpvars.append('%s-abs(PBIAS)' % varname)
                else:
                    tmpvars.append(tmpvar)
                values.append(self.objvalues[self.objnames.index(tmpvar)])
        return values, tmpvars

    def output_header(self, varname, effnames, prefix=''):
        concate = ''
        for name in effnames:
            tmpvar = '%s-%s' % (varname, name)
            if tmpvar not in self.objnames:
                concate += '\t'
            else:
                if name.upper() == 'PBIAS':
                    tmpvar = '%s-abs(PBIAS)' % varname
                if prefix != '':
                    concate += '%s-%s\t' % (prefix, tmpvar)
                else:
                    concate += '%s\t' % tmpvar
        return concate

    def output_efficiency(self, varname, effnames):
        concate = ''
        for name in effnames:
            tmpvar = '%s-%s' % (varname, name)
            if tmpvar not in self.objnames:
                concate += '\t'
            else:
                concate += '%.3f\t' % self.objvalues[self.objnames.index(tmpvar)]
        return concate


class Calibration(object):
    """Base class of automatic calibration.

    Attributes:
        ID(integer): Calibration ID in current generation, range from 0 to N-1(individuals).
        modelrun(boolean): Has SEIMS model run successfully?
    """

    def __init__(self, cali_cfg, id=-1):
        # type: (CaliConfig, Optional[int]) -> None
        """Initialize."""
        self.cfg = cali_cfg
        self.model = cali_cfg.model
        self.ID = id
        self.param_defs = dict()
        # run seims related
        self.modelrun = False
        self.reset_simulation_timerange()

    @property
    def ParamDefs(self):
        """Read cali_param_rng.def file

           name,lower_bound,upper_bound

            e.g.,
             Param1,0,1
             Param2,0.5,1.2
             Param3,-1.0,1.0

        Returns:
            a dictionary containing:
            - names - the names of the parameters
            - bounds - a list of lists of lower and upper bounds
            - num_vars - a scalar indicating the number of variables
                         (the length of names)
        """
        # read param_defs.json if already existed
        if self.param_defs:
            return self.param_defs
        # read param_range_def file and output to json file
        # conn = MongoDBObj.client  # type: MongoClient
        conn = ConnectMongoDB(self.cfg.model.host, self.cfg.model.port).get_conn()
        db = conn[self.cfg.model.db_name]
        collection = db['PARAMETERS']

        names = list()
        bounds = list()
        num_vars = 0
        if not FileClass.is_file_exists(self.cfg.param_range_def):
            raise ValueError('Parameters definition file: %s is not'
                             ' existed!' % self.cfg.param_range_def)
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
        self.param_defs = {'names': names, 'bounds': bounds, 'num_vars': num_vars}
        return self.param_defs

    def reset_simulation_timerange(self):
        """Update simulation time range in MongoDB [FILE_IN]."""
        # conn = MongoDBObj.client  # type: MongoClient
        conn = ConnectMongoDB(self.cfg.model.host, self.cfg.model.port).get_conn()
        db = conn[self.cfg.model.db_name]
        stime_str = self.cfg.model.simu_stime.strftime('%Y-%m-%d %H:%M:%S')
        etime_str = self.cfg.model.simu_etime.strftime('%Y-%m-%d %H:%M:%S')
        db[DBTableNames.main_filein].find_one_and_update({'TAG': 'STARTTIME'},
                                                         {'$set': {'VALUE': stime_str}})
        db[DBTableNames.main_filein].find_one_and_update({'TAG': 'ENDTIME'},
                                                         {'$set': {'VALUE': etime_str}})

    def initialize(self, n=1):
        """Initialize parameters samples by Latin-Hypercube sampling method.

        Returns:
            A list contains parameter value at each gene location.
        """
        param_num = self.ParamDefs['num_vars']
        lhs_samples = lhs(param_num, n)
        all = list()
        for idx in range(n):
            gene_values = list()
            for i, param_bound in enumerate(self.ParamDefs['bounds']):
                gene_values.append(lhs_samples[idx][i] * (param_bound[1] - param_bound[0]) +
                                   param_bound[0])
            all.append(gene_values)
        return all


def initialize_calibrations(cf):
    """Initial individual of population.
    """
    cali = Calibration(cf)
    return cali.initialize()


def calibration_objectives(cali_obj, ind):
    """Evaluate the objectives of given individual.
    """
    cali_obj.ID = ind.id
    model_args = cali_obj.model.ConfigDict
    model_args.setdefault('calibration_id', -1)
    model_args['calibration_id'] = ind.id
    model_obj = MainSEIMS(args_dict=model_args)

    # Set observation data to model_obj, no need to query database
    model_obj.SetOutletObservations(ind.obs.vars, ind.obs.data)

    # Execute model
    model_obj.SetMongoClient()
    model_obj.run()
    time.sleep(0.1)  # Wait a moment in case of unpredictable file system error

    # read simulation data of the entire simulation period (include calibration and validation)
    if model_obj.ReadTimeseriesSimulations():
        ind.sim.vars = model_obj.sim_vars[:]
        ind.sim.data = deepcopy(model_obj.sim_value)
    else:
        model_obj.clean(calibration_id=ind.id)
        model_obj.UnsetMongoClient()
        return ind
    # Calculate NSE, R2, RMSE, PBIAS, and RSR, etc. of calibration period
    ind.cali.vars, ind.cali.data = model_obj.ExtractSimData(cali_obj.cfg.cali_stime,
                                                            cali_obj.cfg.cali_etime)
    ind.cali.sim_obs_data = model_obj.ExtractSimObsData(cali_obj.cfg.cali_stime,
                                                        cali_obj.cfg.cali_etime)

    ind.cali.objnames, \
    ind.cali.objvalues = model_obj.CalcTimeseriesStatistics(ind.cali.sim_obs_data,
                                                            cali_obj.cfg.cali_stime,
                                                            cali_obj.cfg.cali_etime)
    if ind.cali.objnames and ind.cali.objvalues:
        ind.cali.valid = True

    # Calculate NSE, R2, RMSE, PBIAS, and RSR, etc. of validation period
    if cali_obj.cfg.calc_validation:
        ind.vali.vars, ind.vali.data = model_obj.ExtractSimData(cali_obj.cfg.vali_stime,
                                                                cali_obj.cfg.vali_etime)
        ind.vali.sim_obs_data = model_obj.ExtractSimObsData(cali_obj.cfg.vali_stime,
                                                            cali_obj.cfg.vali_etime)

        ind.vali.objnames, \
        ind.vali.objvalues = model_obj.CalcTimeseriesStatistics(ind.vali.sim_obs_data,
                                                                cali_obj.cfg.vali_stime,
                                                                cali_obj.cfg.vali_etime)
        if ind.vali.objnames and ind.vali.objvalues:
            ind.vali.valid = True

    # Get timespan
    ind.io_time, ind.comp_time, ind.simu_time, ind.runtime = model_obj.GetTimespan()

    # delete model output directory for saving storage
    model_obj.clean(calibration_id=ind.id)
    model_obj.UnsetMongoClient()
    return ind


if __name__ == '__main__':
    cf, method = get_optimization_config()
    cfg = CaliConfig(cf, method=method)

    caliobj = Calibration(cfg)

    # test the picklable of Scenario class.
    import pickle

    s = pickle.dumps(caliobj)
    # print(s)
    new_cali = pickle.loads(s)
    print(new_cali.bin_dir)
