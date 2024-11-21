"""@package scenario
Base class of Scenario for coupling NSGA-II.

    @author   : Liangjun Zhu, Huiran Gao

    @changelog:
    - 16-10-29  - hr - initial implementation.
    - 17-08-18  - lj - redesign and rewrite.
    - 18-02-09  - lj - compatible with Python3.
    - 18-10-30  - lj - Update according to new config parser structure.
"""
from __future__ import absolute_import, unicode_literals

from datetime import timedelta
from io import open
import os
import sys
import uuid

from future.utils import viewitems

if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
    sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))

# from pymongo import MongoClient
# import global_mongoclient as MongoDBObj

from bson.objectid import ObjectId
from pygeoc.utils import get_config_parser
from pymongo.errors import NetworkTimeout
from typing import List, Iterator, Optional

from scenario_analysis.config import SAConfig
from preprocess.text import DBTableNames, ModelCfgFields
from preprocess.db_mongodb import MongoClient, ConnectMongoDB
from run_seims import MainSEIMS
from utility.scoop_func import scoop_log


def generate_uniqueid():
    # type: () -> Iterator[int]
    """Generate unique integer ID for Scenario using uuid.

    Usage:
        uniqueid = next(generate_uniqueid())
    """
    uid = int(str(uuid.uuid4().fields[-1])[:9])
    while True:
        yield uid
        uid += 1


class Scenario(object):
    """Base class of Scenario Analysis.

    Attributes:
        ID(integer): Unique ID in BMPScenario database -> BMP_SCENARIOS collection
        eval_timerange(float): Simulation time range, read from MongoDB, the unit is year.
        economy(float): Economical effectiveness, e.g., income minus expenses
        environment(float): Environmental effectiveness, e.g., reduction rate of soil erosion
        gene_num(integer): The number of genes of one chromosome, i.e., an individual
        gene_values(list): BMP identifiers on each location of gene. The length is gen_num.
        bmp_items(dict): BMP configuration items that can be imported to MongoDB directly.
                         The key is `bson.objectid.ObjectId`, the value is scenario item dict.
        rules(boolean): Config BMPs randomly or rule-based.
        modelrun(boolean): Has SEIMS model run successfully?
    """

    def __init__(self, cfg):
        # type: (SAConfig) -> None
        """Initialize."""
        self.ID = -1
        self.eval_timerange = 1.  # unit: year
        self.economy = 0.
        self.environment = 0.
        self.net_costs_per_period = list()
        self.costs_per_period = list()
        self.incomes_per_period = list()
        self.sed_sum = 0.
        self.sed_per_period = list()
        self.worst_econ = cfg.worst_econ
        self.worst_env = cfg.worst_env

        self.gene_num = 0
        self.gene_values = list()  # type: List[int]
        self.bmp_items = dict()

        self.rule_mtd = cfg.bmps_cfg_method
        self.bmps_info = cfg.bmps_info
        self.bmps_retain = cfg.bmps_retain
        self.eval_info = cfg.eval_info
        self.export_sce_txt = cfg.export_sce_txt
        self.export_sce_tif = cfg.export_sce_tif
        self.scenario_dir = cfg.scenario_dir  # predefined directories to store scenarios related

        # SEIMS-based model related
        self.modelcfg = cfg.model
        self.modelcfg_dict = self.modelcfg.ConfigDict
        self.model = MainSEIMS(args_dict=self.modelcfg_dict)

        self.model.ReadMongoDBData()

        self.model.SetMongoClient()
        self.scenario_db = self.model.ScenarioDBName
        self.model.ResetSimulationPeriod()  # Reset the simulation period
        # Reset the starttime and endtime of the desired outputs according to evaluation period
        if ModelCfgFields.output_id in self.eval_info:
            self.model.ResetOutputsPeriod(self.eval_info[ModelCfgFields.output_id],
                                          cfg.eval_stime, cfg.eval_etime)
        else:
            print('Warning: No OUTPUTID is defined in BMPs_info. Please make sure the '
                  'STARTTIME and ENDTIME of ENVEVAL are consistent with Evaluation period!')

        self.model.UnsetMongoClient()  # Unset in time!

        # (Re)Calculate timerange in the unit of year
        dlt = cfg.eval_etime - cfg.eval_stime + timedelta(seconds=1)
        self.eval_timerange = (dlt.days * 86400. + dlt.seconds) / 86400. / 365.
        self.modelout_dir = None  # determined in `execute_seims_model` based on unique scenario ID
        self.modelrun = False  # indicate whether the model has been executed

    def set_unique_id(self, given_id=None):
        # type: (Optional[int]) -> int
        """Set unique ID."""
        if given_id is None:
            self.ID = next(generate_uniqueid())
        else:
            self.ID = given_id
        # Update scenario ID for self.modelcfg and self.model
        self.model.scenario_id = self.ID
        self.model.UpdateScenarioID()
        self.modelcfg.scenario_id = self.ID
        self.modelcfg_dict['scenario_id'] = self.ID if self.modelcfg_dict else 0

        return self.ID

    def rule_based_config(self, method, conf_rate):
        # type: (float, str) -> None
        """Config available BMPs to each gene of the chromosome by rule-based method.

        Virtual function that should be overridden in inherited Scenario class.
        """
        pass

    def random_based_config(self, conf_rate):
        # type: (float) -> None
        """Config available BMPs to each gene of the chromosome by random-based method.

        Virtual function that should be overridden in inherited Scenario class.
        """
        pass

    def decoding(self):
        """Decoding gene_values to bmp_items

        This function should be overridden.
        """
        pass

    def export_to_mongodb(self):
        """Export current scenario to MongoDB.
        Delete the same ScenarioID if existed.
        """
        # conn = MongoDBObj.client  # type: MongoClient
        conn = ConnectMongoDB(self.model.host, self.model.port).get_conn()  # type: MongoClient
        db = conn[self.scenario_db]
        collection = db[DBTableNames.scenarios]
        try:
            # find ScenarioID, remove if existed.
            if collection.count_documents({'ID': self.ID}) > 0:
                collection.remove({'ID': self.ID})
        except NetworkTimeout or Exception:
            # In case of unexpected raise
            pass
        for objid, bmp_item in viewitems(self.bmp_items):
            bmp_item['_id'] = ObjectId()
            collection.insert_one(bmp_item)

    def export_scenario_to_txt(self):
        """Export current scenario information to text file.

        This function is better be called after `calculate_environment` and `calculate_environment`
            or in static method, e.g., `scenario_effectiveness`.
        """
        if not self.export_sce_txt:
            return
        ofile = self.scenario_dir + os.path.sep + 'Scenario_%d.txt' % self.ID
        with open(ofile, 'w', encoding='utf-8') as outfile:
            outfile.write('Scenario ID: %d\n' % self.ID)
            outfile.write('Gene number: %d\n' % self.gene_num)
            outfile.write('Gene values: %s\n' % ', '.join((repr(v) for v in self.gene_values)))
            outfile.write('Scenario items:\n')
            if len(self.bmp_items) > 0:
                header = list()
                for obj, item in viewitems(self.bmp_items):
                    header = list(item.keys())
                    break
                outfile.write('\t'.join(header))
                outfile.write('\n')
                for obj, item in viewitems(self.bmp_items):
                    outfile.write('\t'.join(str(v) for v in list(item.values())))
                    outfile.write('\n')
            outfile.write('Effectiveness:\n\teconomy: %f\n\tenvironment: %f\n\tsed_sum: %f\n\tsed_per_period: %s\n' % (
                self.economy, self.environment, self.sed_sum, self.sed_per_period))

    def export_scenario_to_gtiff(self):
        """Export the areal BMPs to gtiff for further analysis.

        This function should be overridden in inherited class.
        """
        pass

    def import_from_mongodb(self, sid):
        """Import a specified Scenario (`sid`) from MongoDB.

        This function should be overridden in inherited class.
        Returns:
            True if succeed, otherwise False.
        """
        pass

    def import_from_txt(self, sid):
        """Import a specified Scenario (`sid`) from text file.

        This function should be overridden in inherited class.
        Returns:
            True if succeed, otherwise False.
        """
        pass

    def calculate_economy(self):
        """Calculate economical effectiveness, which is application specified."""
        pass

    def calculate_environment(self):
        """Calculate environment effectiveness, which is application specified."""
        pass

    def clean(self, scenario_id=None, calibration_id=None, delete_scenario=False,
              delete_spatial_gfs=False):
        """Clean the intermediate data."""
        # model clean
        self.model.SetMongoClient()
        self.model.clean(scenario_id=scenario_id, calibration_id=calibration_id,
                         delete_scenario=delete_scenario,
                         delete_spatial_gfs=delete_spatial_gfs)
        self.model.UnsetMongoClient()

    def execute_seims_model(self):
        """Run SEIMS for evaluating environmental effectiveness.
        If execution fails, the `self.economy` and `self.environment` will be set the worst values.
        """
        scoop_log('Scenario ID: %d, running SEIMS model...' % self.ID)
        self.model.scenario_id = self.ID
        self.modelout_dir = self.model.output_dir

        self.model.SetMongoClient()
        self.model.run()
        self.model.UnsetMongoClient()

        self.modelrun = True
        return self.model.run_success

    def initialize(self, input_genes=None):
        # type: (Optional[List]) -> List
        """Initialize a scenario.

        Returns:
            A list contains BMPs identifier of each gene location.
        """
        pass


if __name__ == '__main__':
    cf = get_config_parser()
    cfg = SAConfig(cf)  # type: SAConfig
    sceobj = Scenario(cfg)  # type: Scenario

    # test the picklable of Scenario class.
    import pickle

    s = pickle.dumps(sceobj)
    # print(s)
    new_cfg = pickle.loads(s)  # type: Scenario
    print(new_cfg.modelcfg.ConfigDict)
    print('Model time range: %s - %s' % (new_cfg.model.start_time.strftime('%Y-%m-%d %H:%M:%S'),
                                         new_cfg.model.end_time.strftime('%Y-%m-%d %H:%M:%S')))
    print('model scenario ID: %d, configured scenario ID: %d' % (new_cfg.model.scenario_id,
                                                                 new_cfg.ID))
    new_cfg.set_unique_id()
    print('model scenario ID: %d, configured scenario ID: %d' % (new_cfg.model.scenario_id,
                                                                 new_cfg.ID))
