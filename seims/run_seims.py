"""@package run_seims
Configure and run SEIMS model.

    In order to avoid thread lock problems may be caused by pymongo (MongoClient),
      several functions should be called by following format:

      model.SetMongoClient()  # the singleton MongoDB client object will be used
      model.ReadOutletObservations()
      model.UnsetMongoClient()

      These functions are: `run()`, `clean()`, `ResetSimulationPeriod()`, `ResetOutputsPeriod()`,
                           `ReadMongoDBData()`, `ReadTimeseriesSimulations()`,
                           `ReadOutletObservations()`.

    @author   : Liangjun Zhu

    @changelog:
    - 2017-12-07 - lj - Initial implementation.
    - 2018-07-04 - lj - Support MPI version.
    - 2018-07-07 - lj - Add the outputs of single model run.
    - 2018-07-10 - lj - Add ParseSEIMSConfig for all SEIMS tools.
    - 2018-08-28 - lj - Add GetTimespan function and timespan counted by time.time().
    - 2018-11-15 - lj - Add model clean function.
    - 2019-01-08 - lj - Add output time period setting.
    - 2020-07-20 - lj - Read data from MongoDB once for all currently used properties.
    - 2020-08-11 - lj - Separate actually execution from run() and add CommandString property.
    - 2020-09-22 - lj - Add workload (slurm, mpi, etc.) mode. Functions improved.
    - 2023-05-22 - lj - Add cfg_name and fdir_mtd arguments.
"""
from __future__ import absolute_import, unicode_literals

import bisect
from copy import deepcopy
from collections import OrderedDict
from configparser import ConfigParser
from datetime import datetime
from io import open
import math
import os
import sys
from shutil import rmtree
import time
from typing import Optional, Union, Dict, List, AnyStr
from subprocess import CalledProcessError

if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
    sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))

# import global_mongoclient as MongoDBObj

from pygeoc.utils import UtilClass, FileClass, StringClass, \
    sysstr, is_string, get_config_parser

from preprocess.text import DBTableNames
from preprocess.db_mongodb import MongoClient, ConnectMongoDB
from preprocess.db_read_model import ReadModelData
from utility import read_simulation_from_txt, get_option_value, parse_datetime_from_ini
from utility import match_simulation_observation, calculate_statistics


class ParseSEIMSConfig(object):
    """Parse SEIMS model related configurations from `ConfigParser` object.

    Attributes:
        host (str): MongoDB IP address
        port (int): MongoDB port number
        bin_dir (str): Executable dir of SEIMS. Spaces and unicode characters are not allowed
        model_dir (str): Model dir which contains essential data and configs for running a model
        db_name (str): Name of main database stored in MongoDB, default is dirname of `model_dir`
        version (str): Version of SEIMS main program, can be `MPI` or `OMP` (default)
        mpi_bin (str): Path of MPI executable file, e.g., /usr/bin/mpirun
        hosts_opt (str): (Optional) Key to specify hostfile for MPI version,
                          e.g., `-f`, `-hostfile`, `-machine`, `-machinefile`
        hostfile (str): File containing hostnames or file mapping process numbers to computing nodes
        nprocess (int): Process number of MPI, i.e., how many MPI tasks will be executed
        npernode (int): Launch num processes per node on all allocated computing nodes
        flag_npernode (string): Flag to specify NPERNODE, e.g., -ppn for common MPI implementation
        nthread (int): Thread number of OpenMP, i.e., how many threads of each processes
        fdirmtd (int): Flow direction method for flow routing, default is 0 (D8),
                        can also be 1 (DINF) and 2 (MFDMD)
        lyrmtd (int): Method of creating routing layers of simulation units,
                        can be 0 (UP_DOWN) and 1 (DOWN_UP)
        scenario_id (int): Scenario ID
        calibration_id (int): Calibration ID
        subbasin_id (int): Subbasin ID, 0 for the entire basin, 1-N for subbasin,
                             9999 for SEIMS-field version
        simu_stime (datetime): Start time of simulation (UTCTIME), the format is YYYY-MM-DD HH:MM:SS
        simu_etime (datetime): End time of simulation (UTCTIME), the format is YYYY-MM-DD HH:MM:SS
        out_stime (datetime): Start time of output data (UTCTIME)
        out_etime (datetime): End time of output data (UTCTIME)
        workload (str): Use workload manager to run multiple models simultaneously, e.g., Slurm
        config_dict (dict): Dict of all configurations
        runtime (float): Running time counted by `time.time()` of Python,
                           in case of `GetTimespan()` failed
    """

    def __init__(self, cf=None):
        # type: (Optional[ConfigParser]) -> None
        self.host = '127.0.0.1'  # type: AnyStr
        self.port = 27017  # type: float
        self.bin_dir = ''  # type: AnyStr
        self.model_dir = ''  # type: AnyStr
        self.cfg_name = ''  # type: AnyStr
        self.db_name = ''  # type: AnyStr
        self.version = 'OMP'  # type: AnyStr
        self.mpi_bin = ''  # type: AnyStr
        self.hosts_opt = ''  # type: AnyStr
        self.hostfile = ''  # type: AnyStr
        self.nprocess = 1  # type: int
        self.npernode = 1  # type: int
        self.flag_npernode = ''  # type: AnyStr
        self.nthread = 2  # type: int
        self.fdirmtd = 0  # type: int
        self.lyrmtd = 1  # type: int
        self.scenario_id = 0  # type: int
        self.calibration_id = -1  # type: int
        self.subbasin_id = 0  # type: int
        self.simu_stime = None  # type: Optional[datetime]
        self.simu_etime = None  # type: Optional[datetime]
        self.out_stime = None  # type: Optional[datetime]
        self.out_etime = None  # type: Optional[datetime]
        self.workload = ''  # type: AnyStr

        self.config_dict = dict()  # type: Dict[AnyStr, Optional[AnyStr, datetime, int, float]]
        self.runtime = 0.  # type: float

        if cf is None:
            return

        sec_name = 'SEIMS_Model'

        # Don't raise exception, since DEFAULT session may be used.
        # if sec_name not in cf.sections():
        #     raise ValueError('[%s] section MUST be existed in *.ini file.' % sec_name)

        self.host = get_option_value(cf, sec_name, ['hostname', 'host', 'ip'])
        if not StringClass.is_valid_ip_addr(self.host):
            raise ValueError('HOSTNAME (%s) defined is illegal!' % self.host)
        self.port = get_option_value(cf, sec_name, 'port', valtyp=int)

        self.bin_dir = get_option_value(cf, sec_name, 'bin_dir')
        if not FileClass.is_dir_exists(self.bin_dir):
            raise IOError('Please Check the existence of BIN_DIR!')

        self.model_dir = get_option_value(cf, sec_name, 'model_dir')
        if not FileClass.is_dir_exists(self.model_dir):
            raise IOError('Please Check the existence of MODEL_DIR!')
        self.db_name = get_option_value(cf, sec_name, ['db_name', 'spatial_dbname'])
        if not self.db_name:  # If not specified, by default, equals to dir name of self.model_dir
            self.db_name = os.path.split(self.model_dir)[1]

        self.cfg_name = get_option_value(cf, sec_name, 'cfg_name')
        if self.cfg_name and not FileClass.is_dir_exists(self.model_dir + os.sep + self.cfg_name):
            print('WARNING: the specified cfg_name: %s is not existed!' % self.cfg_name)
            self.cfg_name = ''

        self.version = get_option_value(cf, sec_name, 'version')
        if not self.version or self.version not in ['MPI', 'mpi']:
            self.version = 'OMP'
        self.mpi_bin = get_option_value(cf, sec_name, 'mpi_bin')
        self.hosts_opt = get_option_value(cf, sec_name, ['hosts_opt', 'hostopt'])
        self.hostfile = get_option_value(cf, sec_name, 'hostfile')
        self.nprocess = get_option_value(cf, sec_name, ['nprocess', 'processnum'], valtyp=int)
        self.npernode = get_option_value(cf, sec_name, 'npernode', valtyp=int)
        if self.version.lower() == 'omp':
            self.nprocess = 1
            self.npernode = 1
        # Nodes required for each SEIMS-based model
        self.nnodes = self.nprocess // self.npernode + self.nprocess % self.npernode

        self.flag_npernode = get_option_value(cf, sec_name, 'flag_npernode')
        self.nthread = get_option_value(cf, sec_name, ['nthread', 'threadsnum'], valtyp=int)
        self.fdirmtd = get_option_value(cf, sec_name, 'fdirmtd', valtyp=int)
        self.lyrmtd = get_option_value(cf, sec_name, ['lyrmtd', 'layeringmethod'], valtyp=int)
        self.scenario_id = get_option_value(cf, sec_name, ['scenario_id', 'scenarioid'], valtyp=int)
        self.calibration_id = get_option_value(cf, sec_name,
                                               ['calibration_id', 'calibrationid'], valtyp=int)
        self.subbasin_id = get_option_value(cf, sec_name, ['subbasin_id', 'subbasinid'], valtyp=int)
        self.simu_stime = parse_datetime_from_ini(cf, sec_name, ['simu_stime', 'sim_time_start'],
                                                  required=False)
        self.simu_etime = parse_datetime_from_ini(cf, sec_name, ['simu_etime', 'sim_time_end'],
                                                  required=False)
        if self.simu_stime and self.simu_etime and self.simu_stime >= self.simu_etime:
            raise ValueError('Wrong simulation time settings in [%s]!' % sec_name)

        self.out_stime = parse_datetime_from_ini(cf, sec_name, ['out_stime', 'output_time_start'],
                                                 required=False)
        self.out_etime = parse_datetime_from_ini(cf, sec_name, ['out_etime', 'output_time_end'],
                                                 required=False)
        if self.out_stime and self.out_etime and self.out_stime >= self.out_etime:
            raise ValueError('Wrong output time settings in [%s]!' % sec_name)

        self.workload = get_option_value(cf, sec_name, 'workload')

    @property
    def ConfigDict(self):  # type: () -> Dict[AnyStr, Optional[AnyStr, datetime, int, float]]
        if not self.config_dict:
            self.config_dict = {'host': self.host, 'port': self.port,
                                'bin_dir': self.bin_dir, 'model_dir': self.model_dir,
                                'cfg_name': self.cfg_name, 'db_name': self.db_name,
                                'version': self.version, 'mpi_bin': self.mpi_bin,
                                'hosts_opt': self.hosts_opt, 'hostfile': self.hostfile,
                                'nprocess': self.nprocess, 'npernode': self.npernode,
                                'nnodes': self.nnodes, 'flag_npernode': self.flag_npernode,
                                'nthread': self.nthread,
                                'fdirmtd': self.fdirmtd, 'lyrmtd': self.lyrmtd,
                                'scenario_id': self.scenario_id,
                                'calibration_id': self.calibration_id,
                                'subbasin_id': self.subbasin_id,
                                'simu_stime': self.simu_stime, 'simu_etime': self.simu_etime,
                                'out_stime': self.out_stime, 'out_etime': self.out_etime,
                                'workload': self.workload
                                }
        print(self.config_dict)
        return self.config_dict


class MainSEIMS(object):
    """Main entrance to SEIMS model."""
    def __init__(self,
                 args_dict=None,  # type: Dict[AnyStr, Optional[AnyStr, datetime, int]]
                 host='127.0.0.1',  # type: AnyStr # MongoDB host address, default is `localhost`
                 port=27017,  # type: int # MongoDB port, default is 27017
                 bin_dir='',  # type: AnyStr # The directory of SEIMS binary
                 model_dir='',  # type: AnyStr # The directory of SEIMS model
                 cfg_name='',  # type: AnyStr # The specific model config name
                 db_name='',  # type: AnyStr  # Main spatial dbname which can diff from dirname
                 version='OMP',  # type: AnyStr # SEIMS version, can be `MPI` or `OMP` (default)
                 mpi_bin='',  # type: AnyStr # Full path of MPI executable file, e.g., './mpirun`
                 hosts_opt='-f',  # type: AnyStr # Option for assigning hostfile for MPI version
                 hostfile='',  # type: AnyStr # File containing computing nodes
                 nprocess=1,  # type: int # Process number of MPI
                 npernode=1,  # type: int # Process number per computing node
                 nnodes=1,  # type: int # Nodes required to execute
                 flag_npernode='',  # type: AnyStr # Flag to specify npernode
                 nthread=2,  # type: int # Thread number of OpenMP
                 fdirmtd=0,  # type: int # Flow direction, can be 0 (d8), 1 (dinf), or 2 (mfdmd)
                 lyrmtd=1,  # type: int # Layering method, can be 0 (UP_DOWN) or 1 (DOWN_UP)
                 scenario_id=-1,  # type: int # Scenario ID defined in `<model>_Scenario` database
                 calibration_id=-1,  # type: int # Calibration ID used for model auto-calibration
                 subbasin_id=0,  # type: int # Subbasin ID
                 simu_stime=None,  # type: Optional[datetime, AnyStr] # Start time of simulation
                 simu_etime=None,  # type: Optional[datetime, AnyStr] # End time of simulation
                 out_stime=None,  # type: Optional[datetime, AnyStr] # Start time of outputs
                 out_etime=None,  # type: Optional[datetime, AnyStr] # End time of outputs
                 workload=''  # type: AnyStr # Type of workload manager
                 ):
        # type: (...) -> None
        #  Derived from input arguments
        if not args_dict:  # Preferred to use 'args_dict' if existed, otherwise create it.
            args_dict = dict()

        self.host = args_dict['host'] if 'host' in args_dict else host
        self.port = args_dict['port'] if 'port' in args_dict else port

        self.version = args_dict['version'] if 'version' in args_dict else version
        bin_dir = args_dict['bin_dir'] if 'bin_dir' in args_dict else bin_dir
        model_dir = args_dict['model_dir'] if 'model_dir' in args_dict else model_dir
        suffix = '.exe' if sysstr == 'Windows' else ''
        if self.version.upper() == 'MPI':
            self.seims_exec = '%s/seims_mpi%s' % (bin_dir, suffix)
        else:
            self.seims_exec = '%s/seims_omp%s' % (bin_dir, suffix)
            if not FileClass.is_file_exists(self.seims_exec):  # If not support OpenMP, use `seims`!
                self.seims_exec = '%s/seims%s' % (bin_dir, suffix)
        self.seims_exec = os.path.abspath(self.seims_exec)

        self.model_dir = os.path.abspath(model_dir)
        self.cfg_name = args_dict['cfg_name'] if 'cfg_name' in args_dict else cfg_name
        self.db_name = args_dict['db_name'] if 'db_name' in args_dict else db_name

        self.mpi_bin = args_dict['mpi_bin'] if 'mpi_bin' in args_dict else mpi_bin
        self.hosts_opt = args_dict['hosts_opt'] if 'hosts_opt' in args_dict else hosts_opt
        self.hostfile = args_dict['hostfile'] if 'hostfile' in args_dict else hostfile

        self.nprocess = args_dict['nprocess'] if 'nprocess' in args_dict else nprocess
        self.npernode = args_dict['npernode'] if 'npernode' in args_dict else npernode

        self.nnodes = args_dict['nnodes'] if 'nnodes' in args_dict else nnodes
        self.flag_npernode = args_dict['flag_npernode'] if 'flag_npernode' in args_dict \
            else flag_npernode
        self.nthread = args_dict['nthread'] if 'nthread' in args_dict else nthread
        self.fdirmtd = args_dict['fdirmtd'] if 'fdirmtd' in args_dict else fdirmtd
        self.lyrmtd = args_dict['lyrmtd'] if 'lyrmtd' in args_dict else lyrmtd
        self.scenario_id = args_dict['scenario_id'] if 'scenario_id' in args_dict else scenario_id
        self.calibration_id = args_dict['calibration_id'] \
            if 'calibration_id' in args_dict else calibration_id
        self.subbasin_id = args_dict['subbasin_id'] if 'subbasin_id' in args_dict else subbasin_id

        self.simu_stime = args_dict['simu_stime'] if 'simu_stime' in args_dict else simu_stime
        self.simu_etime = args_dict['simu_etime'] if 'simu_etime' in args_dict else simu_etime
        self.out_stime = args_dict['out_stime'] if 'out_stime' in args_dict else out_stime
        self.out_etime = args_dict['out_etime'] if 'out_etime' in args_dict else out_etime
        if is_string(self.simu_stime) and not isinstance(self.simu_stime, datetime):
            self.simu_stime = StringClass.get_datetime(self.simu_stime)
        if is_string(self.simu_etime) and not isinstance(self.simu_etime, datetime):
            self.simu_etime = StringClass.get_datetime(self.simu_etime)
        if is_string(self.out_stime) and not isinstance(self.out_stime, datetime):
            self.out_stime = StringClass.get_datetime(self.out_stime)
        if is_string(self.out_etime) and not isinstance(self.out_etime, datetime):
            self.out_etime = StringClass.get_datetime(self.out_etime)

        self.workload = args_dict['workload'] if 'workload' in args_dict else workload  # type: AnyStr

        # Concatenate output directory name, which is also the name of runtime log
        # The format of OUTPUT directory is: OUTPUT_<FDIR>_<LYR>-<ScenarioID>-<CalibrationID>
        # - OUTPUT_<FDIR>_<LYR>--1 means no scenario, calibration ID is 1
        # - OUTPUT_<FDIR>_<LYR>-100- means scenario ID is 100, no calibration
        # - OUTPUT_<FDIR>_<LYR>-100-2 means scenario ID is 100, calibration ID is 2
        self.UpdateScenarioID()

        # Concatenate executable command
        self.cmd = list()
        self.executed = False  # The model has been executed or not, no matter success.
        self.run_success = False  # The model executed successfully or not.

        # Model data read from MongoDB
        self.outlet_id = -1
        self.subbasin_count = -1
        self.scenario_dbname = ''
        self.start_time = None
        self.end_time = None
        self.output_ids = list()  # type: List[AnyStr]
        self.output_items = dict()  # type: Dict[AnyStr, Union[List[AnyStr]]]

        # Data maybe used after model run
        self.timespan = dict()  # type: Dict[AnyStr, Dict[AnyStr, Union[float, Dict[AnyStr, float]]]]
        self.obs_vars = list()  # type: List[AnyStr]  # Observation types at the outlet
        self.obs_value = dict()  # type: Dict[datetime, List[float]] # Observation value
        self.sim_vars = list()  # type: List[AnyStr]  # Simulation types, part of `obs_vars`
        self.sim_value = dict()  # type: Dict[datetime, List[float]] # Simulation value
        # The format of sim_obs_dict:
        #         {VarName: {'UTCDATETIME': [t1, t2, ..., tn],
        #                    'Obs': [o1, o2, ..., on],
        #                    'Sim': [s1, s2, ..., sn]},
        #         ...
        #         }
        self.sim_obs_dict = dict()  # type: Dict[AnyStr, Dict[AnyStr, Union[float, List[Union[datetime, float]]]]]
        self.runtime = 0.
        self.runlogs = list()  # type: List[AnyStr]

        self.mongoclient = None  # type: Union[MongoClient, None]  # Set to None after use

    @property
    def Command(self):
        # type: (...) -> List[AnyStr]
        """Concatenate command (as a list) to run SEIMS-based model."""
        if self.cmd:
            return self.cmd
        self.cmd = list()
        if self.version.lower() == 'mpi':
            if self.workload.lower() == 'slurm':
                # srun is for a parallel job on cluster managed by Slurm, replacing mpirun.
                self.cmd += ['srun']
                self.cmd += ['-N', str(self.nnodes)]
                if self.npernode >= 1:
                    self.cmd += ['--ntasks-per-node=%d' % self.npernode]
                self.cmd += ['--cpu_bind=cores --cpus-per-task=%d' % self.nthread]
                self.cmd += ['--exclusive']
            else:
                self.cmd += [self.mpi_bin]
                if self.hostfile and os.path.exists(self.hostfile):
                    self.cmd += [self.hosts_opt, self.hostfile]
                    if self.npernode > 1 and self.flag_npernode != '':
                        self.cmd += [self.flag_npernode, str(self.npernode)]

            self.cmd += ['-n', str(self.nprocess)]
        self.cmd += [self.seims_exec,
                     '-wp', self.model_dir, '-thread', str(self.nthread),
                     '-fdir', str(self.fdirmtd),
                     '-lyr', str(self.lyrmtd), '-host', self.host, '-port', self.port]
        if self.cfg_name:
            self.cmd += ['-cfg']
        if self.scenario_id >= 0:
            self.cmd += ['-sce', str(self.scenario_id)]
        if self.calibration_id >= 0:
            self.cmd += ['-cali', str(self.calibration_id)]
        if self.subbasin_id >= 0:
            self.cmd += ['-id', str(self.subbasin_id)]
        # self.cmd += ['-ll Debug'] # todo, should be set in ini file
        return self.cmd

    @property
    def CommandString(self):
        """Concatenate commands list to one string."""
        return ' '.join(repr(aa) if isinstance(aa, int) or isinstance(aa, float) else aa
                        for aa in self.Command)

    def SetMongoClient(self):
        """Should be invoked outset of this script and followed by `UnsetMongoClient`
        """
        if self.mongoclient is None:
            self.mongoclient = ConnectMongoDB(ip=self.host, port=self.port).get_conn()
            # self.mongoclient = MongoDBObj.client  # type: MongoClient

    def UnsetMongoClient(self):
        """Should be invoked together with `SetMongoClient`
        """
        self.mongoclient = None

    def ReadMongoDBData(self):
        """
        Examples:
            model.SetMongoClient()
            model.ReadMongoDBData()
            model.UnsetMongoClient()
        """
        if self.outlet_id >= 0:
            return

        self.SetMongoClient()
        read_model = ReadModelData(self.mongoclient, self.db_name)

        self.outlet_id = read_model.OutletID
        self.subbasin_count = read_model.SubbasinCount
        self.scenario_dbname = read_model.ScenarioDBName
        self.start_time, self.end_time = read_model.SimulationPeriod
        self.output_ids, self.output_items = read_model.OutputItems()
        self.UnsetMongoClient()

    @property
    def OutletID(self):  # type: (...) -> int
        self.ReadMongoDBData()
        return self.outlet_id

    @property
    def SubbasinCount(self):  # type: (...) -> int
        self.ReadMongoDBData()
        return self.subbasin_count

    @property
    def ScenarioDBName(self):  # type: (...) -> AnyStr
        self.ReadMongoDBData()
        return self.scenario_dbname

    @property
    def SimulatedPeriod(self):  # type: (...) -> (datetime, datetime)
        self.ReadMongoDBData()
        return self.start_time, self.end_time

    @property
    def OutputIDs(self):  # type: (...) -> List[AnyStr]
        """Read output items from database."""
        self.ReadMongoDBData()
        return self.output_ids

    @property
    def OutputItems(self):  # type: (...) -> Dict[AnyStr, Optional[List[AnyStr]]]
        """Read output items from database."""
        self.ReadMongoDBData()
        return self.output_items

    def ReadOutletObservations(self, vars_list):
        # type: (List[AnyStr]) -> (List[AnyStr], Dict[datetime, List[float]])
        """

        Examples:
            model.SetMongoClient()
            model.ReadOutletObservations()
            model.UnsetMongoClient()
        """
        self.ReadMongoDBData()
        self.SetMongoClient()
        read_model = ReadModelData(self.mongoclient, self.db_name)
        self.obs_vars, self.obs_value = read_model.Observation(self.outlet_id, vars_list,
                                                               self.start_time, self.end_time)
        self.UnsetMongoClient()
        return self.obs_vars, self.obs_value

    def SetOutletObservations(self, vars_list, vars_value):
        # type: (List[AnyStr], Dict[datetime, List[float]]) -> None
        """Set observation data from the inputs."""
        self.obs_vars = vars_list[:]
        self.obs_value = deepcopy(vars_value)

    def ReadTimeseriesSimulations(self, stime=None, etime=None):
        # type: (Optional[datetime], Optional[datetime]) -> bool
        """Read time series simulation results from OUTPUT directory.
        If no start time or end time are specified, the time ranges from `FILE_IN` will be used.
        """
        if not self.obs_vars:
            return False
        startt, endt = self.SimulatedPeriod
        if stime is None:
            stime = startt
        if etime is None:
            etime = endt
        self.sim_vars, self.sim_value = read_simulation_from_txt(self.output_dir,
                                                                 self.obs_vars,
                                                                 self.OutletID,
                                                                 stime, etime)
        if len(self.sim_vars) < 1:  # No match simulation results
            return False
        self.sim_obs_dict = match_simulation_observation(self.sim_vars, self.sim_value,
                                                         self.obs_vars, self.obs_value)
        return True

    def ExtractSimData(self, stime=None, etime=None):
        # type: (Optional[datetime], Optional[datetime]) -> (List[AnyStr], Dict[datetime, List[float]])
        if stime is None and etime is None:
            return self.sim_vars, self.sim_value

        sidx = bisect.bisect_left(list(self.sim_value.keys()), stime)
        eidx = bisect.bisect_right(list(self.sim_value.keys()), etime)

        def slice_odict(odict, start=None, end=None):
            return OrderedDict([(k, v) for (k, v) in odict.items()
                                if k in list(odict.keys())[start:end]])

        return self.sim_vars, slice_odict(self.sim_value, sidx, eidx)

    def ExtractSimObsData(self, stime=None, etime=None):
        # type: (Optional[datetime], Optional[datetime]) -> Dict[AnyStr, Dict[AnyStr, List[Union[datetime, float]]]]
        if stime is None and etime is None:
            return deepcopy(self.sim_obs_dict)
        ext_dict = dict()
        for param, values in self.sim_obs_dict.items():
            ext_dict[param] = dict()
            sidx = bisect.bisect_left(values['UTCDATETIME'], stime)
            eidx = bisect.bisect_right(values['UTCDATETIME'], etime)
            ext_dict[param]['UTCDATETIME'] = values['UTCDATETIME'][sidx:eidx]
            ext_dict[param]['Obs'] = values['Obs'][sidx:eidx]
            ext_dict[param]['Sim'] = values['Sim'][sidx:eidx]
        return ext_dict

    @staticmethod
    def CalcTimeseriesStatistics(sim_obs_dict,
                                 # type: Dict[AnyStr, Dict[AnyStr, Union[float, List[Union[datetime, float]]]]]
                                 stime=None,  # type: Optional[datetime]
                                 etime=None  # type: Optional[datetime]
                                 ):
        # type: (...) -> (List[AnyStr], List[float])
        objnames = calculate_statistics(sim_obs_dict, stime, etime)
        if objnames is None:
            return None, None
        comb_vars = list()
        obj_values = list()
        for var in sim_obs_dict.keys():
            for objn in objnames:
                comb_vars.append('%s-%s' % (var, objn))
                objv = sim_obs_dict[var][objn]
                if objn.upper() == 'PBIAS':
                    objv = math.fabs(objv)
                obj_values.append(objv)
        return comb_vars, obj_values

    def GetTimespan(self):
        # type: (...) -> List[float]
        """Get summarized timespan, format is [IO, COMP, SIMU, RUNTIME]."""
        time_list = [0., 0., 0., self.runtime]
        if not self.timespan and self.executed:
            parsed = self.ParseTimespan()
            if parsed is None:
                time_list[2] = self.runtime
                return time_list
        tmp_timespan = self.timespan
        if 'MAX' in self.timespan:
            tmp_timespan = self.timespan['MAX']
        if 'IO' in tmp_timespan:
            if 'ALL' in tmp_timespan['IO']:
                time_list[0] = tmp_timespan['IO']['ALL']
            else:
                for k, v in tmp_timespan['IO'].items():
                    time_list[0] += v
        if 'COMP' in tmp_timespan:
            if 'ALL' in tmp_timespan['COMP']:
                time_list[1] = tmp_timespan['COMP']['ALL']
            else:
                for k, v in tmp_timespan['COMP'].items():
                    time_list[1] += v
        if 'SIMU' in tmp_timespan:
            if 'ALL' in tmp_timespan['SIMU']:
                time_list[2] = tmp_timespan['SIMU']['ALL']
        if time_list[2] == 0.:
            time_list[2] = self.runtime
        return time_list

    def ParseTimespan(self):
        # type: (...) -> Optional[Dict[AnyStr, Dict[AnyStr, Union[float, Dict[AnyStr, float]]]]]
        """The format of self.timespan is different for OpenMP version and MPI&OpenMP version.
        For OpenMP version:
           {'IO': {'Input': 0.2,
                   'Output': 0.04
                  }
            'COMP': {'TSD_RD_P': 0.0001,  # All modules
                     'ALL': 12.3
                    }
            'SIMU': {'ALL': 14.1}
           }
        For MPI&OpenMP version:
           {'MAX': {'IO': {'Input': 0.1,
                           'Output': 0.02,
                           'ALL': 0.12
                          }
                    'COMP': {'Slope': 5,
                             'Channel': 0.5,
                             'Barrier': 0.1,
                             'ALL': 5.6
                            }
                    'SIMU': {'ALL': 10.1}
                   }
            'MIN': {...}
            'AVG': {...}
           }
        """
        if not self.runlogs:
            if os.path.exists(self.runlog_name):
                self.runlogs = list()
                with open(self.runlog_name, 'r', encoding='utf-8') as f:
                    for tmpl in f:
                        self.runlogs.append(tmpl.strip().split('\n')[0])
            else:  # The runtime log does not exist
                return None
        for item in self.runlogs:
            if 'TIMESPAN' not in item:
                continue
            item = item.split('[TIMESPAN]')[1]
            values = StringClass.extract_numeric_values_from_string(item)
            if values is None or len(values) != 1:
                continue
            curtime = values[0]
            titles = item.replace('[', '').split(']')[:-1]  # e.g., 'COMP', 'ALL'
            if len(titles) < 2:  # e.g., 'MAX', 'COMP', 'ALL'
                continue
            titles = [title.strip() for title in titles]
            self.timespan.setdefault(titles[0], dict())
            if len(titles) > 2:
                self.timespan[titles[0]].setdefault(titles[1], dict())
                self.timespan[titles[0]][titles[1]].setdefault(titles[2], curtime)
            else:
                self.timespan[titles[0]].setdefault(titles[1], curtime)
        if self.timespan:
            self.run_success = True
        else:
            self.run_success = False
        return self.timespan

    def ResetSimulationPeriod(self):
        """Update simulation time range in MongoDB [FILE_IN].

        Examples:
            model.SetMongoClient()
            model.ResetSimulationPeriod()
            model.UnsetMongoClient()
        """
        self.SetMongoClient()
        read_model = ReadModelData(self.mongoclient, self.db_name)
        if self.simu_stime and self.simu_etime:
            stime_str = self.simu_stime.strftime('%Y-%m-%d %H:%M:%S')
            etime_str = self.simu_etime.strftime('%Y-%m-%d %H:%M:%S')
            db = read_model.maindb
            db[DBTableNames.main_filein].find_one_and_update({'TAG': 'STARTTIME'},
                                                             {'$set': {'VALUE': stime_str}})
            db[DBTableNames.main_filein].find_one_and_update({'TAG': 'ENDTIME'},
                                                             {'$set': {'VALUE': etime_str}})
        self.UnsetMongoClient()
        self.start_time, self.end_time = read_model.SimulationPeriod

    def ResetOutputsPeriod(self, output_ids,  # type: Union[AnyStr, List[AnyStr]]
                           stime,  # type: Union[datetime, List[datetime]]
                           etime  # type: Union[datetime, List[datetime]]
                           ):
        # type: (...) -> None
        """Reset the STARTTIME and ENDTIME of OUTPUTID(s).

        Examples:
            model.SetMongoClient()
            model.clean()
            model.UnsetMongoClient()
        """
        if not isinstance(output_ids, list):
            output_ids = [output_ids]
        if not isinstance(stime, list):
            stime = [stime]
        if not isinstance(etime, list):
            etime = [etime]
        self.SetMongoClient()
        read_model = ReadModelData(self.mongoclient, self.db_name)
        for idx, outputid in enumerate(output_ids):
            cur_stime = stime[0]
            if idx < len(stime):
                cur_stime = stime[idx]
            cur_etime = etime[0]
            if idx < len(etime):
                cur_etime = etime[idx]
            cur_stime_str = cur_stime.strftime('%Y-%m-%d %H:%M:%S')
            cur_etime_str = cur_etime.strftime('%Y-%m-%d %H:%M:%S')
            db = read_model.maindb
            db[DBTableNames.main_fileout].find_one_and_update({'OUTPUTID': outputid},
                                                              {'$set': {'STARTTIME': cur_stime_str,
                                                                        'ENDTIME': cur_etime_str}})

    def run(self, do_execute=True):
        """Run SEIMS model

        Examples:
            model.SetMongoClient()
            model.run()
            model.UnsetMongoClient()
        """
        stime = time.time()
        if not os.path.isdir(self.output_dir) or not os.path.exists(self.output_dir):
            os.makedirs(self.output_dir)
        # If the input time period is not consistent with the predefined time period in FILE_IN.
        if self.simu_stime and self.simu_etime and self.simu_stime != self.start_time \
            and self.simu_etime != self.end_time:
            self.ResetSimulationPeriod()
        # If the output time period is specified, reset the time period of all output IDs
        # if self.out_stime and self.out_etime:
        #     self.ResetOutputsPeriod(self.OutputIDs, self.out_stime, self.out_etime)

        if not do_execute:
            self.executed = False
            self.run_success = False
            return self.executed

        try:
            self.runlogs = UtilClass.run_command(self.Command)
            self.ParseTimespan()
        except CalledProcessError or IOError or Exception as err:
            # 1. SEIMS-based model running failed
            # 2. The OUTPUT directory was not been created successfully by SEIMS-based model
            # 3. Other unpredictable errors
            print('Run SEIMS model failed! %s' % str(err))
            self.run_success = False
        self.runtime = time.time() - stime
        self.executed = True
        return self.executed

    def clean(self, scenario_id=None, calibration_id=None, delete_scenario=False,
              delete_spatial_gfs=False):
        """Clean model outputs in OUTPUT<ScenarioID>-<CalibrationID> directory and/or
        GridFS files in OUTPUT collection.

        Examples:
            model.SetMongoClient()
            model.clean()
            model.UnsetMongoClient()
        """
        rmtree(self.output_dir, ignore_errors=True)
        self.SetMongoClient()
        read_model = ReadModelData(self.mongoclient, self.db_name)
        if scenario_id is None:
            scenario_id = self.scenario_id
        if calibration_id is None:
            calibration_id = self.calibration_id
        read_model.CleanOutputGridFs(scenario_id, calibration_id)
        if delete_scenario:
            read_model.CleanScenariosConfiguration(scenario_id)
            if delete_spatial_gfs:
                read_model.CleanSpatialGridFs(scenario_id)
        self.UnsetMongoClient()

    def UpdateScenarioID(self):
        """
        This function should be simultaneously updated with `InputArgs` class in C++
        """
        self.output_name = 'OUTPUT'
        if self.version.upper() == 'MPI':
            self.output_name += '_MPI'
        fdirs = ['_D8', '_DINF', '_MFDMD']
        lyrs = ['_UP_DOWN', '_DOWN_UP']
        self.output_name += fdirs[self.fdirmtd]
        self.output_name += lyrs[self.lyrmtd]
        self.output_name += '-'
        if self.scenario_id >= 0:
            self.output_name += '%d' % self.scenario_id
        self.output_name += '-'
        if self.calibration_id >= 0:
            self.output_name += '%d' % self.calibration_id
        if self.cfg_name:
            self.output_dir = '%s/%s/%s' % (self.model_dir, self.cfg_name, self.output_name)
        else:
            self.output_dir = '%s/%s' % (self.model_dir, self.output_name)
        self.runlog_name = os.path.join(self.output_dir, '%s.log' % self.output_name)


def create_run_model(modelcfg_dict, scenario_id=-1, calibration_id=-1, subbasin_id=-1,
                     do_execute=True):
    """Create, Run, and return SEIMS-based watershed model object.

    Args:
        modelcfg_dict: Dict of arguments for SEIMS-based watershed model
        scenario_id: Scenario ID which can override the scenario_id in modelcfg_dict
        calibration_id: Calibration ID which can override the calibration_id in modelcfg_dict
        subbasin_id: Subbasin ID (0 for the whole watershed, 9999 for the field version) which
                     can override the subbasin_id in modelcfg_dict
        do_execute: Execute model or not.
    Returns:
        The instance of SEIMS-based watershed model.
    """
    if scenario_id > 0:
        modelcfg_dict['scenario_id'] = scenario_id
    if calibration_id >= 0:
        modelcfg_dict['calibration_id'] = calibration_id
    if subbasin_id >= 0:
        modelcfg_dict['subbasin_id'] = subbasin_id
    model_obj = MainSEIMS(args_dict=modelcfg_dict)

    model_obj.SetMongoClient()
    model_obj.run(do_execute=do_execute)
    model_obj.UnsetMongoClient()

    return model_obj


def main():
    """Run SEIMS-based watershed model with configuration file."""
    cf = get_config_parser()
    runmodel_cfg = ParseSEIMSConfig(cf)
    seims_obj = MainSEIMS(args_dict=runmodel_cfg.ConfigDict)
    seims_obj.run()

    for log in seims_obj.runlogs:
        print(log)


if __name__ == '__main__':
    main()
