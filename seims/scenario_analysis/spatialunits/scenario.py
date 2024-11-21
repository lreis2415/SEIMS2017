"""Scenario for optimizing BMPs based on slope position units.

    @author   : Liangjun Zhu, Huiran Gao

    @changelog:
    - 16-10-29  - hr - initial implementation.
    - 17-08-18  - lj - redesign and rewrite.
    - 18-02-09  - lj - compatible with Python3.
"""
from __future__ import absolute_import, division, unicode_literals
from future.utils import viewitems

import array
from collections import OrderedDict
from copy import deepcopy
import os
import sys
import random
import time
from struct import unpack
import json

from typing import Union, Dict, List, Tuple, Optional, Any, AnyStr
import numpy
from gridfs import GridFS
from pygeoc.raster import RasterUtilClass
from pygeoc.utils import FileClass, StringClass, UtilClass, get_config_parser, is_string
from pymongo.errors import NetworkTimeout
from pymongo import MongoClient

if os.path.abspath(os.path.join(sys.path[0], '../..')) not in sys.path:
    sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '../..')))

# import global_mongoclient as MongoDBObj

from utility import read_simulation_from_txt, mask_rasterio
from preprocess.text import DBTableNames, RasterMetadata
from preprocess.db_mongodb import MongoClient, ConnectMongoDB
from preprocess.sd_slopeposition_units import DelinateSlopePositionByThreshold
from scenario_analysis import _DEBUG, BMPS_CFG_UNITS, BMPS_CFG_METHODS
from scenario_analysis.scenario import Scenario
from scenario_analysis.config import SAConfig
from scenario_analysis.spatialunits.config import SASlpPosConfig, SAConnFieldConfig, \
    SACommUnitConfig


class SUScenario(Scenario):
    """Scenario analysis using different spatial units as BMPs configuration units."""

    def __init__(self, cf):
        # type: (Union[SASlpPosConfig, SAConnFieldConfig, SACommUnitConfig]) -> None
        """Initialization."""
        Scenario.__init__(self, cf)
        self.cfg = cf  # type: Union[SASlpPosConfig, SAConnFieldConfig, SACommUnitConfig]
        self.gene_num = cf.genes_num  # type: int
        self.gene_values = [0] * self.gene_num  # type: List[int, float] # 0 means no BMP

        self.bmps_params = dict()  # type: Dict[int, Any] # {bmp_id: {...}}
        self.suit_bmps = dict()  # type: Dict[AnyStr, Dict[int, List[int]]] # {type:{id: [bmp_ids]}}
        self.bmps_grade = dict()  # type: Dict[int, int] # {slppos_id: effectiveness_grade}

        self.read_bmp_parameters()
        bmps_suit_type = ['SLPPOS', 'LANDUSE'] \
            if self.cfg.bmps_cfg_unit == BMPS_CFG_UNITS[3] else ['LANDUSE']
        self.get_suitable_bmps(bmps_suit_type)

    def read_bmp_parameters(self):
        """Read BMP configuration from MongoDB.
        Each BMP is stored in Collection as one item identified by 'SUBSCENARIO' field,
        so the `self.bmps_params` is dict with BMP_ID ('SUBSCENARIO') as key.
        """
        # conn = MongoDBObj.client  # type: MongoClient
        conn = ConnectMongoDB(self.model.host, self.model.port).get_conn()  # type: MongoClient
        scenariodb = conn[self.scenario_db]

        bmpcoll = scenariodb[self.cfg.bmps_coll]
        # UserWarning: use an explicit session with no_cursor_timeout=True,
        # otherwise the cursor may still timeout after 30 minutes,
        # for more info see https://jira.mongodb.org/browse/DOCS-11255
        with conn.start_session() as session:
            for fb in bmpcoll.find(no_cursor_timeout=True, session=session):
                fb = UtilClass.decode_strs_in_dict(fb)
                if 'SUBSCENARIO' not in fb:
                    continue
                curid = fb['SUBSCENARIO']
                if curid not in self.cfg.bmps_subids:
                    continue
                if curid not in self.bmps_params:
                    self.bmps_params[curid] = dict()
                for k, v in fb.items():
                    if k == 'SUBSCENARIO':
                        continue
                    elif k == 'LANDUSE':
                        if isinstance(v, int):
                            v = [v]
                        elif v == 'ALL' or v == '':
                            v = None
                        else:
                            v = StringClass.extract_numeric_values_from_string(v)
                            v = [int(abs(nv)) for nv in v]
                        self.bmps_params[curid][k] = v[:]
                    elif k == 'SLPPOS':
                        if isinstance(v, int):
                            v = [v]
                        elif v == 'ALL' or v == '':
                            v = list(self.cfg.slppos_tags.keys())
                        else:
                            v = StringClass.extract_numeric_values_from_string(v)
                            v = [int(abs(nv)) for nv in v]
                        self.bmps_params[curid][k] = v[:]
                    elif k == 'INCOME':
                        if isinstance(v, int):  # scenario analysis
                            self.bmps_params[curid][k] = v
                        elif isinstance(v, str):  # bmp order optimization
                            v = StringClass.extract_numeric_values_from_string(v)
                            self.bmps_params[curid][k] = v[:]
                        else:
                            self.bmps_params[curid][k] = v
                    else:
                        self.bmps_params[curid][k] = v

    def get_suitable_bmps(self, types='LANDUSE'):
        # type: (Union[AnyStr, List[AnyStr]]) -> None
        """Construct the suitable BMPs for each slope position."""
        if is_string(types):
            types = [types]
        for bid, bdict in self.bmps_params.items():
            for type in types:
                if type not in bdict:
                    continue
                if type not in self.suit_bmps:
                    self.suit_bmps.setdefault(type, dict())
                suitsp = bdict[type]
                for sp in suitsp:
                    if sp not in self.suit_bmps[type]:
                        self.suit_bmps[type][sp] = [bid]
                    elif bid not in self.suit_bmps[type][sp]:
                        self.suit_bmps[type][sp].append(bid)
            if 'EFFECTIVENESS' in bdict:
                self.bmps_grade[bid] = bdict['EFFECTIVENESS']

    def initialize(self, input_genes=None):
        # type: (Optional[List]) -> List
        """Initialize a scenario.

        Returns:
            A list contains BMPs identifier of each gene location.
        """
        # Create configuration rate for each location randomly, 0.4 ~ 0.6
        cr = random.randint(40, 60) / 100.

        if input_genes is not None:  # Using the input genes
            if len(input_genes) == self.gene_num:
                self.gene_values = input_genes[:]
            else:  # Only usable for slope position units when optimizing unit boundary
                typenum = self.cfg.slppos_types_num
                tnum = self.cfg.thresh_num
                for idx, gv in enumerate(input_genes):
                    gidx = idx // typenum * (typenum + tnum) + idx % typenum
                    self.gene_values[gidx] = gv
            return self.gene_values
        else:
            if self.rule_mtd == BMPS_CFG_METHODS[0]:
                self.random_based_config(cr)
            else:
                self.rule_based_config(self.rule_mtd, cr)
        if self.cfg.boundary_adaptive and self.gene_num > self.cfg.units_num:
            # Randomly select boundary adaptive threshold
            thresholds = self.cfg.boundary_adaptive_threshs[:]
            for ti in range(self.gene_num):
                if ti in self.cfg.gene_to_unit:
                    continue
                if random.random() >= cr:
                    continue
                self.gene_values[ti] = thresholds[random.randint(0, len(thresholds) - 1)]
        if len(self.gene_values) == self.gene_num > 0:
            return self.gene_values
        else:
            raise RuntimeError('Initialize Scenario failed, please check the inherited scenario'
                               ' class, especially the overwritten rule_based_config and'
                               ' random_based_config!')

    def initialize_with_bmps_order(self, opt_genes, input_genes=False):
        # type: (List, Optional[List]) -> List
        """Initialize a scenario with bmps order.

        Returns:
            A list contains BMPs identifier of each gene location.
        """

        def generate_gene_values(obj, genes):
            invests = numpy.array(obj.cfg.investment_each_period, dtype=float)
            pro_dist = invests / numpy.sum(invests)
            rand_range = range(1, obj.cfg.change_times + 1)

            # gene index and BMP type
            for idx, gene in enumerate(genes):
                if numpy.isclose(gene, 0.):
                    obj.gene_values[idx] = 0
                else:
                    # rand_bit = random.randint(1, obj.cfg.change_times)
                    rand_bit = numpy.random.choice(rand_range, p=pro_dist)
                    obj.gene_values[idx] = int(gene) * 1000 + rand_bit

        if input_genes:
            self.gene_values = opt_genes
        else:
            generate_gene_values(self, opt_genes)
            satisfied, _ = self.satisfy_investment_constraints()
            while not satisfied:
                generate_gene_values(self, opt_genes)
                satisfied, _ = self.satisfy_investment_constraints()

        return self.gene_values

    def rule_based_config(self, method, conf_rate=0.5):
        # type: (float, AnyStr) -> None
        """Config available BMPs on each spatial units by knowledge-based rule method.
        The looping methods vary from different spatial units, e.g., for slope position units,
        it is from the bottom slope position of each hillslope tracing upslope.

        The available rule methods are 'SUIT', 'UPDOWN', and 'HILLSLP'.

        See Also:
            :obj:`scenario_analysis.BMPS_CFG_METHODS`
        """
        if self.cfg.bmps_cfg_unit == BMPS_CFG_UNITS[2]:  # CONNFIELD
            out_id = -1  # the last downstream unit ID
            for k, v in viewitems(self.cfg.units_infos['units']):
                if v['downslope'] <= 0:
                    out_id = k
                    break
            if out_id < 0:
                raise ValueError('The last downstream unit ID is not found!')
            cur_bmps = select_potential_bmps(out_id, self.suit_bmps['LANDUSE'],
                                             self.cfg.units_infos,
                                             self.cfg.unit_to_gene, self.gene_values,
                                             unit=self.cfg.bmps_cfg_unit,
                                             method=self.cfg.bmps_cfg_method)
            gene_idx = self.cfg.unit_to_gene[out_id]
            if cur_bmps is None or len(cur_bmps) == 0:
                self.gene_values[gene_idx] = 0
            else:
                self.gene_values[gene_idx] = cur_bmps[random.randint(0, len(cur_bmps) - 1)]
            if _DEBUG:
                print('-- Config BMPs for CONNFIELD+%s' % self.cfg.bmps_cfg_method)
                print('-- The most downstream ID is %d(gene index:%d)' % (out_id, gene_idx))
                print('-- Field ID, Gene Index, BMPs ID')
                print('-- %s, %s, %s' % (repr(out_id), repr(gene_idx),
                                         repr(self.gene_values[gene_idx])))
            up_units = self.cfg.units_infos['units'][out_id]['upslope'][:]
            unproceed = up_units[:]
            while len(unproceed) > 0:
                if _DEBUG:
                    print('-- unpreceed Field IDs: %s' % ','.join(repr(vv) for vv in unproceed))
                cur_unpreceed = list()
                for up_unit in unproceed:
                    gene_idx = self.cfg.unit_to_gene[up_unit]
                    cur_bmps = select_potential_bmps(up_unit, self.suit_bmps['LANDUSE'],
                                                     self.cfg.units_infos,
                                                     self.cfg.unit_to_gene, self.gene_values,
                                                     unit=self.cfg.bmps_cfg_unit,
                                                     method=self.cfg.bmps_cfg_method)
                    if cur_bmps is None or len(cur_bmps) == 0:
                        self.gene_values[gene_idx] = 0
                    elif random.random() > conf_rate:
                        self.gene_values[gene_idx] = 0
                    else:
                        self.gene_values[gene_idx] = cur_bmps[random.randint(0, len(cur_bmps) - 1)]
                    cur_upunits = self.cfg.units_infos['units'][up_unit]['upslope']
                    if _DEBUG:
                        print('-- %s, %s, %s' % (repr(up_unit), repr(gene_idx),
                                                 repr(self.gene_values[gene_idx])))
                        if -1 not in cur_upunits:
                            print('-- Upslope Field IDs of'
                                  ' %s is: %s' % (repr(up_unit),
                                                  ','.join(repr(vvv) for vvv in cur_upunits)))
                    for tmp_unit in cur_upunits:
                        if tmp_unit > 0:
                            cur_unpreceed.append(tmp_unit)
                unproceed = cur_unpreceed[:]
        elif self.cfg.bmps_cfg_unit == BMPS_CFG_UNITS[3]:  # SLPPOS
            spname = self.cfg.slppos_tagnames[-1][1]  # bottom slope position name, e.g., 'valley'
            for unitid, spdict in viewitems(self.cfg.units_infos[spname]):
                spidx = len(self.cfg.slppos_tagnames) - 1
                while True:  # trace upslope units
                    sptag = self.cfg.slppos_tagnames[spidx][0]
                    sp = self.cfg.slppos_tagnames[spidx][1]
                    up_spid = self.cfg.units_infos[sp][unitid]['upslope']
                    gene_idx = self.cfg.unit_to_gene[unitid]
                    spidx -= 1
                    # Get the union set of multiple suitable bmps
                    cur_suit_bmps = deepcopy(self.suit_bmps['SLPPOS'])
                    unit_area = self.cfg.units_infos[sp][unitid]['area']
                    unit_luids = self.cfg.units_infos[sp][unitid]['landuse']
                    lu_suit_bmps = self.suit_bmps['LANDUSE']
                    sp_suit_bmps = self.suit_bmps['SLPPOS'][sptag][:]
                    new_sp_suit_bmps = list()
                    for unit_luid, unit_luarea in viewitems(unit_luids):
                        if unit_luarea / unit_area < 0.1:
                            continue
                        if unit_luid not in lu_suit_bmps:
                            continue
                        for lu_suit_bmp in lu_suit_bmps[unit_luid]:
                            if lu_suit_bmp in sp_suit_bmps and lu_suit_bmp not in new_sp_suit_bmps:
                                new_sp_suit_bmps.append(lu_suit_bmp)
                    cur_suit_bmps[sptag] = new_sp_suit_bmps[:]

                    cur_bmps = select_potential_bmps(unitid, cur_suit_bmps, self.cfg.units_infos,
                                                     self.cfg.unit_to_gene, self.gene_values,
                                                     unit=self.cfg.bmps_cfg_unit,
                                                     method=self.cfg.bmps_cfg_method,
                                                     bmpgrades=self.bmps_grade,
                                                     tagnames=self.cfg.slppos_tagnames)
                    if cur_bmps is None or len(cur_bmps) == 0:
                        self.gene_values[gene_idx] = 0
                    elif random.random() > conf_rate:
                        # Do not config BMP according to probability
                        self.gene_values[gene_idx] = 0
                    else:
                        # config BMP
                        self.gene_values[gene_idx] = cur_bmps[random.randint(0, len(cur_bmps) - 1)]
                    if up_spid < 0:
                        break
                    unitid = up_spid
        else:
            # Loop each gene to config one of the suitable BMP
            for gene_idx in range(self.gene_num):
                unitid = self.cfg.gene_to_unit[gene_idx]
                cur_bmps = select_potential_bmps(unitid, self.suit_bmps['LANDUSE'],
                                                 self.cfg.units_infos,
                                                 self.cfg.unit_to_gene, self.gene_values,
                                                 unit=self.cfg.bmps_cfg_unit,
                                                 method=self.cfg.bmps_cfg_method,
                                                 bmpgrades=self.bmps_grade)
                if cur_bmps is None or len(cur_bmps) == 0:
                    self.gene_values[gene_idx] = 0
                    continue
                # select one randomly
                self.gene_values[gene_idx] = cur_bmps[random.randint(0, len(cur_bmps) - 1)]

    def random_based_config(self, conf_rate=0.5):
        # type: (float) -> None
        """Config BMPs on each spatial unit randomly."""
        pot_bmps = self.cfg.bmps_subids[:]
        for uid, i in viewitems(self.cfg.unit_to_gene):
            if random.random() >= conf_rate:
                continue
            self.gene_values[i] = pot_bmps[random.randint(0, len(pot_bmps) - 1)]

    def boundary_adjustment(self):
        """
        Update BMP configuration units and related data according to gene_values,
          i.e., bmps_info and units_infos
        """
        if not self.cfg.boundary_adaptive:
            return
        if self.gene_num == self.cfg.units_num:
            return
        # 1. New filename of BMP configuration unit
        dist = '%s_%d' % (self.cfg.orignal_dist, self.ID)
        self.bmps_info[self.cfg.bmpid]['DISTRIBUTION'] = dist
        spfilename = StringClass.split_string(dist, '|')[1]
        # 2. Organize the slope position IDs and thresholds by hillslope ID
        #    Format: {HillslopeID: {rdgID, bksID, vlyID, T_bks2rdg, T_bks2vly}, ...}
        slppos_threshs = dict()  # type: Dict[int, List]
        upperslppos = self.cfg.slppos_tagnames[0][1]  # Most upper slope position name
        for subbsnid, subbsndict in viewitems(self.cfg.units_infos['hierarchy_units']):
            for hillslpid, hillslpdict in viewitems(subbsndict):
                slppos_threshs[hillslpid] = list()
                for slppostag, slpposname in self.cfg.slppos_tagnames:
                    slppos_threshs[hillslpid].append(hillslpdict[slpposname])
                upper_geneidx = self.cfg.unit_to_gene[hillslpdict[upperslppos]]
                thresh_idx = upper_geneidx + len(hillslpdict)
                thresh_idxend = thresh_idx + self.cfg.thresh_num
                slppos_threshs[hillslpid] += self.gene_values[thresh_idx: thresh_idxend]
        # 3. Delineate slope position and get the updated information (landuse area, etc.)
        # 3.1 Erase current data in units_info
        for itag, iname in self.cfg.slppos_tagnames:
            if iname not in self.cfg.units_infos:
                continue
            for sid, datadict in viewitems(self.cfg.units_infos[iname]):
                self.cfg.units_infos[iname][sid]['area'] = 0.
                for luid in self.cfg.units_infos[iname][sid]['landuse']:
                    self.cfg.units_infos[iname][sid]['landuse'][luid] = 0.
        # 3.2 Delineate slope position and get data by subbasin
        # The whole watershed will be generateed for both version
        hillslp_data = DelinateSlopePositionByThreshold(self.modelcfg, slppos_threshs,
                                                        self.cfg.slppos_tag_gfs,
                                                        spfilename, subbsn_id=0)
        # 3.3 Update units_infos
        for tagname, slpposdict in viewitems(hillslp_data):
            for sid, datadict in viewitems(slpposdict):
                self.cfg.units_infos[tagname][sid]['area'] += hillslp_data[tagname][sid]['area']
                for luid in hillslp_data[tagname][sid]['landuse']:
                    if luid not in self.cfg.units_infos[tagname][sid]['landuse']:
                        self.cfg.units_infos[tagname][sid]['landuse'][luid] = 0.
                    newlanduse_area = hillslp_data[tagname][sid]['landuse'][luid]
                    self.cfg.units_infos[tagname][sid]['landuse'][luid] += newlanduse_area
        if self.modelcfg.version.upper() == 'MPI':
            for tmp_subbsnid in range(1, self.model.SubbasinCount + 1):
                DelinateSlopePositionByThreshold(self.modelcfg, slppos_threshs,
                                                 self.cfg.slppos_tag_gfs,
                                                 spfilename, subbsn_id=tmp_subbsnid)
        # print(self.cfg.units_infos)

    def decoding(self):
        """Decode gene values to Scenario item, i.e., `self.bmp_items`."""
        if self.ID < 0:
            self.set_unique_id()
        if self.bmp_items:
            self.bmp_items.clear()
        bmp_units = dict()  # type: Dict[int, List[int]] # {BMPs_ID: [units list]}
        for unit_id, gene_idx in viewitems(self.cfg.unit_to_gene):
            gene_v = self.gene_values[gene_idx]
            if gene_v == 0:
                continue
            if gene_v not in bmp_units:
                bmp_units[gene_v] = list()
            bmp_units[gene_v].append(unit_id)
        sce_item_count = 0
        for k, v in viewitems(bmp_units):
            curd = dict()
            curd['BMPID'] = self.cfg.bmpid
            curd['NAME'] = 'S%d' % self.ID
            curd['COLLECTION'] = self.bmps_info[self.cfg.bmpid]['COLLECTION']
            curd['DISTRIBUTION'] = self.bmps_info[self.cfg.bmpid]['DISTRIBUTION']
            curd['LOCATION'] = '-'.join(repr(uid) for uid in v)
            curd['SUBSCENARIO'] = k
            curd['ID'] = self.ID
            self.bmp_items[sce_item_count] = curd
            sce_item_count += 1
        # if BMPs_retain is not empty, append it.
        if len(self.bmps_retain) > 0:
            for k, v in viewitems(self.bmps_retain):
                curd = deepcopy(v)
                curd['BMPID'] = k
                curd['NAME'] = 'S%d' % self.ID
                curd['ID'] = self.ID
                self.bmp_items[sce_item_count] = curd
                sce_item_count += 1

    def decoding_with_bmps_order(self):
        """Decode gene values to Scenario item, i.e., `self.bmp_items`."""
        if self.ID < 0:
            self.set_unique_id()
        if self.bmp_items:
            self.bmp_items.clear()
        bmp_units = dict()  # type: Dict[int, List[str]] # {BMPs_ID: [units list]}
        for unit_id, gene_idx in viewitems(self.cfg.unit_to_gene):
            gene_v = self.gene_values[gene_idx]
            if gene_v == 0:
                continue
            # subscenario, year = [int(x) for x in str(int(gene_v))]
            subscenario, year = divmod(int(gene_v), 1000)
            if subscenario not in bmp_units:
                bmp_units[subscenario] = list()
            bmp_units[subscenario].append('{0}|{1}'.format(unit_id, year))

        sce_item_count = 0
        for k, v in viewitems(bmp_units):
            curd = dict()
            curd['BMPID'] = self.cfg.bmpid
            curd['NAME'] = 'S%d' % self.ID
            curd['COLLECTION'] = self.bmps_info[self.cfg.bmpid]['COLLECTION']
            curd['DISTRIBUTION'] = self.bmps_info[self.cfg.bmpid]['DISTRIBUTION']
            curd['LOCATION'] = '-'.join(v)
            curd['SUBSCENARIO'] = k
            curd['ID'] = self.ID
            curd['EFFECTIVENESSVARIABLE'] = 1 if self.cfg.effectiveness_changeable else 0
            curd['CHANGEFREQUENCY'] = self.cfg.change_frequency * 365 * 24 * 60 * 60  # convert to seconds
            self.bmp_items[sce_item_count] = curd
            sce_item_count += 1
        # if BMPs_retain is not empty, append it.
        if len(self.bmps_retain) > 0:
            for k, v in viewitems(self.bmps_retain):
                curd = deepcopy(v)
                curd['BMPID'] = k
                curd['NAME'] = 'S%d' % self.ID
                curd['ID'] = self.ID
                curd['EFFECTIVENESSVARIABLE'] = 0
                curd['CHANGEFREQUENCY'] = -1
                self.bmp_items[sce_item_count] = curd
                sce_item_count += 1

    def import_from_mongodb(self, sid):
        pass

    def import_from_txt(self, sid):
        pass

    def calculate_economy(self):
        """Calculate economic benefit by simple cost-benefit model, see Qin et al. (2018)."""
        self.economy = 0.
        capex = 0.
        opex = 0.
        income = 0.
        actual_years = self.cfg.runtime_years
        for unit_id, gene_idx in viewitems(self.cfg.unit_to_gene):
            gene_v = self.gene_values[gene_idx]
            if gene_v == 0:
                continue
            unit_lu = dict()
            for spname, spunits in self.cfg.units_infos.items():
                if unit_id in spunits:
                    unit_lu = spunits[unit_id]['landuse']
                    break
            bmpparam = self.bmps_params[gene_v]
            for luid, luarea in unit_lu.items():
                if luid in bmpparam['LANDUSE'] or bmpparam['LANDUSE'] is None:
                    capex += luarea * bmpparam['CAPEX']
                    opex += luarea * bmpparam['OPEX'] * actual_years
                    income += luarea * bmpparam['INCOME'][-1] * actual_years

        # self.economy = capex
        # self.economy = capex + opex
        self.economy = capex + opex - income
        # print('economy: capex {}, income {}, opex {}'.format(capex, income, opex))
        return self.economy

    def calculate_economy_bmps_order(self, costs, maintains, incomes):
        """Calculate economic benefit by simple cost-benefit model, see Qin et al. (2018)."""
        self.net_costs_per_period = (costs + maintains - incomes).tolist()
        self.costs_per_period = (costs + maintains).tolist()
        self.incomes_per_period = incomes.tolist()

        # use net present value
        net_present_value = 0.
        for index, net_cost in enumerate(self.net_costs_per_period):
            net_present_value += net_cost / numpy.power(1.0 + self.cfg.discount_rate, index + 1)
        self.economy = net_present_value
        print('economy:{}, capex {}, maintain {}, income {}'.format(self.economy, costs, maintains, incomes))
        return self.economy

    def calculate_environment(self):
        """Calculate environment benefit based on the output and base values predefined in
        configuration file.
        """
        if not self.modelrun:  # no evaluate done
            self.economy = self.worst_econ
            self.environment = self.worst_env
            return
        rfile = self.modelout_dir + os.path.sep + self.eval_info['ENVEVAL']

        if not FileClass.is_file_exists(rfile):
            time.sleep(0.1)  # Wait a moment in case of unpredictable file system error
        if not FileClass.is_file_exists(rfile):
            print('WARNING: Although SEIMS model has been executed, the desired output: %s'
                  ' cannot be found!' % rfile)
            self.economy = self.worst_econ
            self.environment = self.worst_env
            # model clean
            # self.model.SetMongoClient()
            # self.model.clean(delete_scenario=True)
            # self.model.UnsetMongoClient()
            return

        base_amount = self.eval_info['BASE_ENV']
        if StringClass.string_match(rfile.split('.')[-1], 'tif'):  # Raster data
            rr = RasterUtilClass.read_raster(rfile)
            sed_sum = rr.get_sum() / self.eval_timerange  # unit: year
        elif StringClass.string_match(rfile.split('.')[-1], 'txt'):  # Time series data
            sed_sum = read_simulation_from_txt(self.modelout_dir,
                                               ['SED'], self.model.OutletID,
                                               self.cfg.eval_stime, self.cfg.eval_etime)
        else:
            raise ValueError('The file format of ENVEVAL MUST be tif or txt!')

        if base_amount < 0:  # indicates a base scenario
            self.environment = sed_sum
            self.sed_sum = sed_sum
        else:
            # reduction rate of soil erosion
            self.environment = (base_amount - sed_sum) / base_amount
            self.sed_sum = sed_sum
            # print exception values
            if self.environment > 1. or self.environment < 0. or self.environment is numpy.nan:
                print('Exception Information: Scenario ID: %d, '
                      'SUM(%s): %s' % (self.ID, rfile, repr(sed_sum)))
                self.environment = self.worst_env

    def calculate_environment_bmps_order(self):
        """Calculate environment benefit based on the output and base values predefined in
                configuration file.
                """
        if not self.modelrun:  # no evaluate done
            self.economy = self.worst_econ
            self.environment = self.worst_env
            return
        rfile = self.modelout_dir + os.path.sep + self.eval_info['ENVEVAL']

        if not FileClass.is_file_exists(rfile):
            time.sleep(0.1)  # Wait a moment in case of unpredictable file system error
        if not FileClass.is_file_exists(rfile):
            print('WARNING: Although SEIMS model has been executed, the desired output: %s'
                  ' cannot be found!' % rfile)
            self.economy = self.worst_econ
            self.environment = self.worst_env
            # model clean
            # self.model.SetMongoClient()
            # self.model.clean(delete_scenario=True)
            # self.model.UnsetMongoClient()
            return

        base_amount = self.eval_info['BASE_ENV']
        sed_per_period = list()
        if StringClass.string_match(rfile.split('.')[-1], 'tif'):  # Raster data
            # sum of 2013-2017
            rr = RasterUtilClass.read_raster(rfile)
            sed_sum = rr.get_sum() / self.cfg.implementation_period  # Annual average of sediment 13-17
            for i in range(self.cfg.change_times):
                # 2013-2017
                filename = self.modelout_dir + os.path.sep + str(i + 3) + '_' + self.eval_info['ENVEVAL']
                sed_per_period.append(RasterUtilClass.read_raster(filename).get_sum())
            # sed_sum = sed_per_period[-1]  # 2017 sed sum
        elif StringClass.string_match(rfile.split('.')[-1], 'txt'):  # Time series data
            sed_sum = read_simulation_from_txt(self.modelout_dir,
                                               ['SED'], self.model.OutletID,
                                               self.cfg.eval_stime, self.cfg.eval_etime)
        else:
            raise ValueError('The file format of ENVEVAL MUST be tif or txt!')

        if base_amount < 0:  # indicates a base scenario
            self.environment = sed_sum
            self.sed_sum = sed_sum
            self.sed_per_period = sed_per_period
        else:
            # reduction rate of soil erosion (in percent)
            self.environment = (base_amount - sed_sum) * 100 / base_amount
            self.sed_sum = sed_sum
            self.sed_per_period = sed_per_period
            # print exception values
            if self.environment > 100. or self.environment is numpy.nan:
                print('Exception Information: Scenario ID: %d, SUM(%s): %s, per period: %s'
                      % (self.ID, rfile, repr(sed_sum), sed_per_period))
                self.environment = self.worst_env

    def export_scenario_to_gtiff(self, outpath=None):
        # type: (Optional[str]) -> None
        """Export scenario to GTiff.

        Read Raster from MongoDB should be extracted to pygeoc. -- Done using mask_rasterio!
        By ZhuLJ, 2023-03-25
        """
        if not self.export_sce_tif:
            return
        dist = self.bmps_info[self.cfg.bmpid]['DISTRIBUTION']
        dist_list = StringClass.split_string(dist, '|')
        if len(dist_list) >= 2 and dist_list[0] == 'RASTER':
            dist_name = '0_' + dist_list[1]  # prefix 0_ means the whole basin
            v_dict = dict()
            for unitidx, geneidx in viewitems(self.cfg.unit_to_gene):
                v_dict[unitidx] = self.gene_values[geneidx]
            if outpath is None:
                outpath = self.scenario_dir + os.path.sep + 'Scenario_%d.tif' % self.ID
            unit2bmpsstr = ','.join('%s:%s' % (repr(k), repr(v)) for k, v in v_dict.items())
            # print(unit2bmpsstr)
            mongoargs = [self.cfg.model.host, self.cfg.model.port,
                         self.cfg.model.db_name, 'SPATIAL']
            mask_rasterio(self.cfg.model.bin_dir,
                          [[dist_name, outpath, 0, -9999, 'INT32', unit2bmpsstr]],
                          mongoargs=mongoargs, maskfile='0_SUBBASIN', include_nodata=False)

    def calculate_profits_by_period(self):
        bmp_costs_by_period = [0.] * self.cfg.change_times
        bmp_maintain_by_period = [0.] * self.cfg.change_times
        bmp_income_by_period = [0.] * self.cfg.change_times
        for unit_id, gene_idx in viewitems(self.cfg.unit_to_gene):
            gene_v = self.gene_values[gene_idx]
            if gene_v == 0:
                continue
            unit_lu = dict()
            for spname, spunits in self.cfg.units_infos.items():
                if unit_id in spunits:
                    unit_lu = spunits[unit_id]['landuse']
                    break
            subscenario, impl_period = divmod(int(gene_v), 1000)
            bmpparam = self.bmps_params[subscenario]
            for luid, luarea in unit_lu.items():
                if luid in bmpparam['LANDUSE'] or bmpparam['LANDUSE'] is None:
                    capex = luarea * bmpparam['CAPEX']
                    opex = bmpparam['OPEX']
                    income = bmpparam['INCOME']
                    bmp_costs_by_period[impl_period - 1] += capex
                    # every period has income after impl
                    for prd in range(impl_period, self.cfg.change_times + 1):  # closed interval
                        bmp_maintain_by_period[prd - 1] += luarea * opex
                        bmp_income_by_period[prd - 1] += luarea * income[
                            prd - impl_period]  # each year has different benefit
        return bmp_costs_by_period, bmp_maintain_by_period, bmp_income_by_period

    def satisfy_investment_constraints(self):
        # compute economy
        bmp_costs_by_period, bmp_maintain_by_period, bmp_income_by_period = self.calculate_profits_by_period()
        invest = numpy.array(self.cfg.investment_each_period)
        costs = numpy.array(bmp_costs_by_period)
        maintain = numpy.array(bmp_maintain_by_period)
        income = numpy.array(bmp_income_by_period)
        diff = invest - (costs + maintain - income)
        print('investment constraints: ', invest)
        print('costs: ', costs)
        print('maintain: ', maintain)
        print('income: ', income)
        print('diff: ', diff)

        # not consider investment quota
        if not self.cfg.enable_investment_quota:
            return True, [costs, maintain, income]
        else:
            if self.cfg.investment_each_period is None:
                return False, [None, None, None]

            # satisfy economic constraint
            if numpy.all(numpy.greater(invest, costs + maintain - income)):
                self.net_costs_per_period = (costs + maintain - income).tolist()
                self.costs_per_period = (costs + maintain).tolist()
                self.incomes_per_period = income.tolist()
                return True, [costs, maintain, income]
            else:
                return False, [None, None, None]

    def statistics_by_period_bmp(self):
        periods = list()
        for _ in range(self.cfg.change_times):
            bmps = dict()
            for bmpparam in self.bmps_params.values():
                temp_dict = dict()
                temp_dict['AREA'] = 0.
                temp_dict['CAPEX'] = 0.
                temp_dict['OPEX'] = 0.
                temp_dict['INCOME'] = 0.
                bmps[bmpparam['NAME']] = temp_dict
            periods.append({'SUMMARY': {}, 'BMPS': bmps})

        for unit_id, gene_idx in viewitems(self.cfg.unit_to_gene):
            gene_v = self.gene_values[gene_idx]
            if gene_v == 0:
                continue
            unit_lu = dict()
            for spname, spunits in self.cfg.units_infos.items():
                if unit_id in spunits:
                    unit_lu = spunits[unit_id]['landuse']
                    break
            subscenario, impl_period = divmod(int(gene_v), 1000)
            bmpparam = self.bmps_params[subscenario]
            for luid, luarea in unit_lu.items():
                if luid in bmpparam['LANDUSE'] or bmpparam['LANDUSE'] is None:
                    bmpname = bmpparam['NAME']
                    # every period has opex,income after impl, only one capex
                    capex = luarea * bmpparam['CAPEX']
                    periods[impl_period - 1]['BMPS'][bmpname]['CAPEX'] += capex
                    periods[impl_period - 1]['BMPS'][bmpname]['AREA'] += luarea
                    for prd in range(impl_period, self.cfg.change_times + 1):  # closed interval
                        bmp_year_index = prd - impl_period
                        opex = luarea * bmpparam['OPEX']  # only 1 number
                        income = luarea * bmpparam['INCOME'][bmp_year_index]  # sequence numbers
                        periods[prd - 1]['BMPS'][bmpname]['OPEX'] += opex
                        periods[prd - 1]['BMPS'][bmpname]['INCOME'] += income

        for period in periods:
            total_capex = 0.
            total_opex = 0.
            total_income = 0.
            total_area = 0.
            for bmp_detail in period['BMPS'].values():
                total_capex += bmp_detail['CAPEX']
                total_opex += bmp_detail['OPEX']
                total_income += bmp_detail['INCOME']
                total_area += bmp_detail['AREA']
            period['SUMMARY']['CAPEX'] = total_capex
            period['SUMMARY']['OPEX'] = total_opex
            period['SUMMARY']['INCOME'] = total_income
            period['SUMMARY']['NETCOST'] = total_capex + total_opex - total_income
            period['SUMMARY']['AREA'] = total_area

        return periods

    def statistics_by_bmp(self):
        bmps = dict()
        for bmpparam in self.bmps_params.values():
            temp_dict = dict()
            temp_dict['AREA'] = 0.
            temp_dict['CAPEX'] = 0.
            temp_dict['OPEX'] = 0.
            temp_dict['INCOME'] = 0.
            bmps[bmpparam['NAME']] = temp_dict
        stats = dict({'SUMMARY': {}, 'BMPS': bmps})

        for unit_id, gene_idx in viewitems(self.cfg.unit_to_gene):
            gene_v = self.gene_values[gene_idx]
            if gene_v == 0:
                continue
            unit_lu = dict()
            for spname, spunits in self.cfg.units_infos.items():
                if unit_id in spunits:
                    unit_lu = spunits[unit_id]['landuse']
                    break
            subscenario, impl_period = divmod(int(gene_v), 1000)  # impl_period == 1 in this function
            bmpparam = self.bmps_params[subscenario]
            for luid, luarea in unit_lu.items():
                if luid in bmpparam['LANDUSE'] or bmpparam['LANDUSE'] is None:
                    bmpname = bmpparam['NAME']
                    # every period has opex,income after impl, only one capex
                    capex = luarea * bmpparam['CAPEX']
                    stats['BMPS'][bmpname]['CAPEX'] += capex
                    stats['BMPS'][bmpname]['AREA'] += luarea
                    for prd in range(impl_period, self.cfg.change_times + 1):  # closed interval
                        bmp_year_index = prd - impl_period
                        opex = luarea * bmpparam['OPEX']  # only 1 number
                        income = luarea * bmpparam['INCOME'][bmp_year_index]  # sequence numbers
                        stats['BMPS'][bmpname]['OPEX'] += opex
                        stats['BMPS'][bmpname]['INCOME'] += income
        return stats


def select_potential_bmps(unitid,  # type: int
                          suitbmps,  # type: Dict[int, List[int]] # key could be SLPPOS or LANDUSE
                          unitsinfo,  # type: Dict[Union[AnyStr, int], Any]
                          unit2gene,  # type: OrderedDict[int, int]
                          ind,  # type: Union[array.array, List[int], Tuple[int]] # gene values
                          unit='SLPPOS',  # type: AnyStr
                          method='SUIT',  # type: AnyStr
                          bmpgrades=None,  # type: Optional[Dict[int, int]]
                          tagnames=None  # type: Optional[List[Tuple[int, AnyStr]]] # for SLPPOS
                          ):
    # type: (...) -> Optional[List[int]]
    """Select potential BMPs for specific spatial unit."""
    suit_bmps_tag = -1
    down_unit = -1  # type: Optional[int]
    up_units = list()  # type: Optional[List[int]]
    if unit == BMPS_CFG_UNITS[3]:  # SLPPOS
        for spid, spdict in viewitems(unitsinfo):
            if unitid not in spdict:
                continue
            down_unit = spdict[unitid].get('downslope')
            up_units.append(spdict[unitid].get('upslope'))
            for t, n in tagnames:
                if spid == n:
                    suit_bmps_tag = t
                    break
    else:  # other spatial units only take `LANDUSE` to suit BMPs
        # ValueError checks should be done in other place
        suit_bmps_tag = unitsinfo['units'][unitid]['primarylanduse']
        down_unit = unitsinfo['units'][unitid].get('downslope')  # may be None
        up_units = unitsinfo['units'][unitid].get('upslope')  # may be None

    if suit_bmps_tag not in suitbmps:
        return None

    bmps = suitbmps[suit_bmps_tag][:]
    bmps = list(set(bmps))  # ascending
    # Config or not is controlled by Random probability outside this function,
    #  thus, there is no need to append 0 (i.e., no BMP)!
    # if 0 not in bmps:
    #     bmps.append(0)

    if method == BMPS_CFG_METHODS[0] or method == BMPS_CFG_METHODS[1]:  # RDM or SUIT
        return bmps

    down_position = False
    down_gvalue = -1
    if down_unit is not None and down_unit > 0:
        down_gvalue = ind[unit2gene[down_unit]]
    else:
        down_position = True

    if method == BMPS_CFG_METHODS[2]:  # UPDOWN
        if down_unit <= 0:  # If downslope unit does not exists
            return bmps
        upslope_configured = False
        for upslope_id in up_units:
            if upslope_id < 0:
                continue
            if ind[unit2gene[upslope_id]] > 0:
                upslope_configured = True
        if down_gvalue > 0:
            if _DEBUG:
                print('  Mutate on unit: %d, the downslope unit has been configured BMP.' % unitid)
            # If downslope unit is configured BMP, then this unit should not configure BMP
            bmps = [0]
        elif upslope_configured:
            if _DEBUG:
                print('  Mutate on unit: %d, at least one of the upslope units '
                      'has been configured BMP.' % unitid)
            # If downslope unit is configured BMP, then this unit should not configure BMP
            bmps = [0]
        else:
            # If downslope unit is not configured BMP and the upslope units are all not configured
            #  BMPs, then this unit will be configured one BMP
            if 0 in bmps:
                bmps.remove(0)
        return bmps

    if method == BMPS_CFG_METHODS[3]:  # SLPPOS
        if bmpgrades is None:  # By default, the effectiveness grade should be equal for all BMPs.
            bmpgrades = {bid: 1 for bid in bmps}
        if 0 not in bmpgrades:
            bmpgrades[0] = 0

        top_position = False
        up_gvalue = -1
        if up_units is not None and up_units[0] > 0:
            up_gvalue = ind[unit2gene[up_units[0]]]
        else:
            top_position = True

        up_grade = bmpgrades[up_gvalue] if up_gvalue in bmpgrades else 0
        down_grade = bmpgrades[down_gvalue] if down_gvalue in bmpgrades else 0
        new_bmps = list()
        if top_position and down_gvalue > 0:  # 1. the top slppos, and downslope with BMP
            for _bid, _bgrade in viewitems(bmpgrades):
                if _bgrade <= down_grade and _bid in bmps:
                    new_bmps.append(_bid)
        elif down_position and up_gvalue > 0:  # 2. the bottom slppos, and upslope with BMP
            for _bid, _bgrade in viewitems(bmpgrades):
                if up_grade <= _bgrade and _bid in bmps:
                    new_bmps.append(_bid)
        elif not top_position and not down_position:  # 3. middle slppos
            for _bid, _bgrade in viewitems(bmpgrades):
                if down_gvalue == 0 and up_gvalue <= _bgrade and _bid in bmps:
                    # 3.1. downslope no BMP
                    new_bmps.append(_bid)
                elif up_grade <= _bgrade <= down_grade and _bid in bmps:
                    # 3.2. downslope with BMP
                    new_bmps.append(_bid)
        else:  # Do nothing
            pass
        if len(new_bmps) > 0:
            bmps = list(set(new_bmps))
        return bmps

    return bmps  # for other BMP configuration methods, return without modification


def initialize_scenario(cf, input_genes=None):
    # type: (Union[SASlpPosConfig, SAConnFieldConfig, SACommUnitConfig], Optional[List]) -> List[int]
    """Initialize gene values"""
    sce = SUScenario(cf)
    return sce.initialize(input_genes=input_genes)


def initialize_scenario_with_bmps_order(cf, opt_genes, input_genes=False):
    # type: (Union[SASlpPosConfig, SAConnFieldConfig, SACommUnitConfig], Optional[List]) -> List[int]
    """Initialize gene values"""
    sce = SUScenario(cf)
    return sce.initialize_with_bmps_order(opt_genes, input_genes=input_genes)


def scenario_effectiveness(cf, ind):
    # type: (Union[SASlpPosConfig, SAConnFieldConfig, SACommUnitConfig], array.array) -> (float, float, int)
    """Run SEIMS-based model and calculate economic and environmental effectiveness."""
    # 1. instantiate the inherited Scenario class.
    sce = SUScenario(cf)
    ind.id = sce.set_unique_id()
    setattr(sce, 'gene_values', ind)
    # 2. update BMP configuration units and related data according to gene_values,
    #      i.e., bmps_info and units_infos
    sce.boundary_adjustment()
    # 3. decode gene values to BMP items and exporting to MongoDB.
    sce.decoding()
    sce.export_to_mongodb()
    # 4. execute the SEIMS-based watershed model and get the timespan
    sce.execute_seims_model()
    ind.io_time, ind.comp_time, ind.simu_time, ind.runtime = sce.model.GetTimespan()
    # 5. calculate scenario effectiveness and delete intermediate data
    sce.calculate_economy()
    sce.calculate_environment()
    # 6. Export scenarios information
    sce.export_scenario_to_txt()
    sce.export_scenario_to_gtiff()
    # 7. Clean the intermediate data of current scenario
    sce.clean(scenario_id=sce.ID, delete_scenario=True, delete_spatial_gfs=True)
    # 8. Assign fitness values
    ind.fitness.values = [sce.economy, sce.environment]

    return ind


def scenario_effectiveness_with_bmps_order(cf, ind):
    # type: (Union[SASlpPosConfig, SAConnFieldConfig, SACommUnitConfig], array.array) -> (float, float, int)
    """Run SEIMS-based model and calculate time extended economic and environmental effectiveness."""
    # 1. instantiate the inherited Scenario class.
    sce = SUScenario(cf)
    ind.id = sce.set_unique_id()
    setattr(sce, 'gene_values', ind)

    # 2. decode gene values to BMP items and exporting to MongoDB.
    sce.decoding_with_bmps_order()
    sce.export_to_mongodb()

    # 3. first evaluate economic investment to exclude scenarios that don't satisfy the constraints
    # if that don't satisfy the constraints, don't execute the time-consuming simulation process
    satisfied, [costs, maintains, incomes] = sce.satisfy_investment_constraints()  # sce.check_custom_constraints():
    if satisfied:
        # 4. execute the SEIMS-based watershed model and get the timespan
        sce.execute_seims_model()
        ind.io_time, ind.comp_time, ind.simu_time, ind.runtime = sce.model.GetTimespan()
        # 5. calculate scenario effectiveness and delete intermediate data
        sce.calculate_economy_bmps_order(costs, maintains, incomes)
        sce.calculate_environment_bmps_order()
    else:
        # worst conditions
        ind.io_time, ind.comp_time, ind.simu_time, ind.runtime = [0.] * 4
        sce.economy = sce.worst_econ
        sce.environment = sce.worst_env
    # 6. Export scenarios information
    sce.export_scenario_to_txt()
    sce.export_scenario_to_gtiff()
    # 7. Clean the intermediate data of current scenario
    # sce.clean(delete_scenario=True, delete_spatial_gfs=True)
    # 8. Assign fitness values
    ind.fitness.values = [sce.economy, sce.environment]
    ind.sed_sum = sce.sed_sum
    ind.sed_per_period = sce.sed_per_period
    ind.net_costs_per_period = sce.net_costs_per_period
    ind.costs_per_period = sce.costs_per_period
    ind.incomes_per_period = sce.incomes_per_period

    return ind


def main_multiple(eval_num):
    # type: (int) -> None
    """Test of multiple evaluations of scenarios."""
    cf = get_config_parser()
    base_cfg = SAConfig(cf)  # type: SAConfig
    if base_cfg.bmps_cfg_unit == BMPS_CFG_UNITS[3]:  # SLPPOS
        cfg = SASlpPosConfig(cf)
    elif base_cfg.bmps_cfg_unit == BMPS_CFG_UNITS[2]:  # CONNFIELD
        cfg = SAConnFieldConfig(cf)
    else:  # Common spatial units, e.g., HRU and EXPLICITHRU
        cfg = SACommUnitConfig(cf)
    cfg.construct_indexes_units_gene()

    cost = list()
    for _ in range(eval_num):
        sce = SUScenario(cfg)
        sce.initialize()
        sceid = sce.set_unique_id()
        print(sceid, sce.gene_values.__str__())
        sce.calculate_economy()
        cost.append(sce.economy)
    print(max(cost), min(cost), sum(cost) / len(cost))


def main_single(sceid, gene_values):
    """Test of single evaluation of scenario."""
    cf = get_config_parser()
    base_cfg = SAConfig(cf)  # type: SAConfig
    if base_cfg.bmps_cfg_unit == BMPS_CFG_UNITS[3]:  # SLPPOS
        cfg = SASlpPosConfig(cf)
    elif base_cfg.bmps_cfg_unit == BMPS_CFG_UNITS[2]:  # CONNFIELD
        cfg = SAConnFieldConfig(cf)
    else:  # Common spatial units, e.g., HRU and EXPLICITHRU
        cfg = SACommUnitConfig(cf)
    cfg.construct_indexes_units_gene()

    sce = SUScenario(cfg)
    sce.initialize(input_genes=gene_values)
    sce.boundary_adjustment()
    sce.set_unique_id(sceid)
    # print(sceid, sce.gene_values.__str__())
    sce.decoding()
    sce.export_to_mongodb()
    sce.execute_seims_model()
    sce.export_scenario_to_gtiff(sce.model.output_dir + os.sep + 'scenario_%d.tif' % sceid)
    sce.calculate_economy()
    sce.calculate_environment()

    print('Scenario %d: %s\n' % (sceid, ', '.join(repr(v) for v in sce.gene_values)))
    print('Effectiveness:\n\teconomy: %f\n\tenvironment: %f\n' % (sce.economy, sce.environment))


def main_manual(sceid, gene_values):
    """Test of set scenario manually."""
    cf = get_config_parser()
    base_cfg = SAConfig(cf)  # type: SAConfig
    if base_cfg.bmps_cfg_unit == BMPS_CFG_UNITS[3]:  # SLPPOS
        cfg = SASlpPosConfig(cf)
    elif base_cfg.bmps_cfg_unit == BMPS_CFG_UNITS[2]:  # CONNFIELD
        cfg = SAConnFieldConfig(cf)
    else:  # Common spatial units, e.g., HRU and EXPLICITHRU
        cfg = SACommUnitConfig(cf)
    cfg.construct_indexes_units_gene()
    sce = SUScenario(cfg)

    sce.set_unique_id(sceid)
    sce.initialize(input_genes=gene_values)
    sce.boundary_adjustment()

    sce.decoding()
    sce.export_to_mongodb()
    sce.execute_seims_model()
    sce.export_sce_tif = True
    sce.export_scenario_to_gtiff(sce.model.output_dir + os.sep + 'scenario_%d.tif' % sceid)
    sce.calculate_economy()
    sce.calculate_environment()
    sce.export_sce_txt = True
    sce.export_scenario_to_txt()

    print('Scenario %d: %s\n' % (sceid, ', '.join(repr(v) for v in sce.gene_values)))
    print('Effectiveness:\n\teconomy: %f\n\tenvironment: %f\n\tsed_sum: %f\n' % (
        sce.economy, sce.environment, sce.sed_sum))
    return sceid, sce.economy, sce.environment, sce.sed_sum

    # sce.clean(delete_scenario=True, delete_spatial_gfs=True)


def main_manual_bmps_order(sceid, gene_values):
    """Test of set scenario manually."""
    cf = get_config_parser()
    base_cfg = SAConfig(cf)  # type: SAConfig
    if base_cfg.bmps_cfg_unit == BMPS_CFG_UNITS[3]:  # SLPPOS
        cfg = SASlpPosConfig(cf)
    elif base_cfg.bmps_cfg_unit == BMPS_CFG_UNITS[2]:  # CONNFIELD
        cfg = SAConnFieldConfig(cf)
    else:  # Common spatial units, e.g., HRU and EXPLICITHRU
        cfg = SACommUnitConfig(cf)
    cfg.construct_indexes_units_gene()
    sce = SUScenario(cfg)

    sce.set_unique_id(sceid)
    sce.initialize(input_genes=gene_values)
    sce.decoding_with_bmps_order()
    sce.export_to_mongodb()
    satisfied, [costs, maintains, incomes] = sce.satisfy_investment_constraints()
    print('investments: ', costs + maintains)
    if satisfied:
        sce.execute_seims_model()
        sce.calculate_economy_bmps_order(costs, maintains, incomes)
        sce.calculate_environment_bmps_order()
        sce.export_sce_tif = True
        sce.export_scenario_to_gtiff(sce.model.output_dir + os.sep + 'scenario_%d.tif' % sceid)
        sce.export_sce_txt = True
        sce.export_scenario_to_txt()

        print('Scenario %d: %s\n' % (sceid, ', '.join(repr(v) for v in sce.gene_values)))
        print('Effectiveness:\n\teconomy: %f\n\tenvironment: %f\n\tsed_sum: %f\n\t'
              'sed_per_period: %s\n\tnet_costs_per_period: %s\n\tcosts_per_period: %s\n\t'
              'incomes_per_period: %s'
              % (sce.economy, sce.environment, sce.sed_sum, str(sce.sed_per_period),
                 str(sce.net_costs_per_period), str(sce.costs_per_period),
                 str(sce.incomes_per_period)))

    # sce.clean(delete_scenario=True, delete_spatial_gfs=True)


def generate_tiff_txt(sceid, gene_values):
    cf = get_config_parser()
    base_cfg = SAConfig(cf)  # type: SAConfig
    if base_cfg.bmps_cfg_unit == BMPS_CFG_UNITS[3]:  # SLPPOS
        cfg = SASlpPosConfig(cf)
    elif base_cfg.bmps_cfg_unit == BMPS_CFG_UNITS[2]:  # CONNFIELD
        cfg = SAConnFieldConfig(cf)
    else:  # Common spatial units, e.g., HRU and EXPLICITHRU
        cfg = SACommUnitConfig(cf)
    cfg.construct_indexes_units_gene()
    sce = SUScenario(cfg)

    sce.set_unique_id(sceid)
    sce.initialize(input_genes=gene_values)
    sce.decoding()
    sce.export_to_mongodb()
    # indicate the model has run
    sce.modelrun = True
    sce.modelout_dir = sce.model.output_dir
    # sce.calculate_economy()
    # sce.calculate_environment()
    sce.export_sce_tif = True
    sce.export_scenario_to_gtiff(sce.model.output_dir + os.sep + 'scenario_%d.tif' % sceid)
    sce.export_sce_txt = True
    sce.export_scenario_to_txt()

    print('Scenario %d: %s\n' % (sceid, ', '.join(repr(v) for v in sce.gene_values)))
    print('Effectiveness:\n\teconomy: %f\n\tenvironment: %f\n' % (sce.economy, sce.environment))

    # Not responsible for deleting
    # sce.clean(delete_scenario=True, delete_spatial_gfs=True)


def extra_process_for_last_generation(cf, log_filename, last_gen, output_path, export_tif=False):
    base_cfg = SAConfig(cf)  # type: SAConfig
    if base_cfg.bmps_cfg_unit == BMPS_CFG_UNITS[3]:  # SLPPOS
        cfg = SASlpPosConfig(cf)
    elif base_cfg.bmps_cfg_unit == BMPS_CFG_UNITS[2]:  # CONNFIELD
        cfg = SAConnFieldConfig(cf)
    else:  # Common spatial units, e.g., HRU and EXPLICITHRU
        cfg = SACommUnitConfig(cf)
    cfg.construct_indexes_units_gene()

    def cumulative(lists):
        cu_list = []
        length = len(lists)
        cu_list = [sum(lists[0:x:1]) for x in range(0, length + 1)]
        return cu_list[1:]

    import ast
    # read runtime.log
    last_gen_str = 'Generation: {}'.format(last_gen)
    with open(log_filename) as logfp:
        for line in logfp:
            if last_gen_str not in line:
                continue
            next(logfp)  # title line
            for target_line in logfp:
                items = target_line.split('\t')
                sceid = int(items[1])
                print('output %d' % (sceid,))
                gene_values = ast.literal_eval(items[9][11:-2])
                output_tif = '{}/Scenario_{}.tif'.format(output_path, sceid)

                sce = SUScenario(cfg)
                sce.set_unique_id(sceid)
                sce.initialize(input_genes=gene_values)
                sce.decoding()
                sce.export_to_mongodb()
                # indicate the model has run
                sce.modelrun = True
                sce.modelout_dir = sce.model.output_dir

                # generate tif and txt for the scenario
                if export_tif:
                    sce.export_sce_tif = True
                    sce.export_scenario_to_gtiff(output_tif)

                # statistics by period for the scenario
                sce_dict = dict()
                sce_dict['sceid'] = sceid
                sce_dict['economy'] = float(items[2])
                sce_dict['environment'] = float(items[3])
                sce_dict['sed_sum'] = float(items[4])
                sce_dict['sed_pp'] = ast.literal_eval(items[5])
                sce_dict['net_cost_pp'] = ast.literal_eval(items[6])
                sce_dict['cost_pp'] = ast.literal_eval(items[7])
                sce_dict['income_pp'] = ast.literal_eval(items[8])
                sce_dict['gene_values'] = gene_values
                sce_dict['periods'] = sce.statistics_by_period_bmp()
                sce_dict['net_cost_cumul'] = cumulative(sce_dict['net_cost_pp'])
                sce_dict['redunction_rates'] = []

                # calc cumulative reduction rates
                cumul_sed = 0.
                base_sed_amounts = sce.eval_info['BASE_SED_PERIODS']
                cumul_sed_amounts = cumulative(base_sed_amounts)
                for index, sed in enumerate(sce_dict['sed_pp']):
                    environment = (base_sed_amounts[index] - sed) * 100 / base_sed_amounts[index]
                    sce_dict['periods'][index]['SUMMARY']['environment'] = environment

                    cumul_sed += sed
                    cumul_env = (cumul_sed_amounts[index] - cumul_sed) * 100 / cumul_sed_amounts[index]
                    sce_dict['redunction_rates'].append(cumul_env)

                output_json = '{}/Scenario_{}.json'.format(output_path, sceid)
                with open(output_json, 'w') as fp:
                    json.dump(sce_dict, fp)


def test_func():
    # main_single()
    # main_multiple(4)

    # run base
    # sid = 0
    # gvalues = [0.0] * 105
    # main_manual(sid, gvalues)

    # selected scenario
    # sid = 10
    # gvalues = [0.0, 2.0, 2.0, 2.0, 2.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2.0, 0.0, 0.0, 2.0, 2.0, 0.0, 2.0, 0.0, 0.0,
    #            0.0, 0.0, 0.0, 2.0, 2.0, 0.0, 0.0, 2.0, 0.0, 1.0, 1.0, 0.0, 2.0, 2.0, 0.0, 2.0, 2.0, 0.0, 0.0, 2.0, 0.0,
    #            2.0, 2.0, 0.0, 2.0, 0.0, 0.0, 1.0, 3.0, 2.0, 0.0, 2.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 2.0, 2.0,
    #            0.0, 2.0, 2.0, 1.0, 0.0, 0.0, 2.0, 2.0, 0.0, 1.0, 2.0, 0.0, 0.0, 0.0, 0.0, 2.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    #            2.0, 2.0, 2.0, 0.0, 0.0, 0.0, 1.0, 3.0, 4.0, 1.0, 3.0, 0.0, 1.0, 3.0, 0.0, 2.0, 2.0, 0.0, 0.0, 0.0, 0.0]
    # main_manual(sid, gvalues)

    # STEP+FIXED+HH
    print('-----STEP+FIXED+HH-----')
    sid = 240815984
    gvalues = [0.0, 2002.0, 2003.0, 2003.0, 2002.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2004.0, 0.0, 0.0, 2003.0, 2001.0, 0.0, 2003.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2005.0, 2002.0, 0.0, 0.0, 2002.0, 0.0, 1004.0, 1001.0, 0.0, 2001.0, 2001.0, 0.0, 2003.0, 2004.0, 0.0, 0.0, 2003.0, 0.0, 2003.0, 2001.0, 0.0, 2004.0, 0.0, 0.0, 1001.0, 3001.0, 2002.0, 0.0, 2003.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1001.0, 2002.0, 2004.0, 0.0, 2002.0, 2003.0, 1001.0, 0.0, 0.0, 2001.0, 2001.0, 0.0, 1001.0, 2001.0, 0.0, 0.0, 0.0, 0.0, 2004.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2005.0, 2001.0, 2005.0, 0.0, 0.0, 0.0, 1004.0, 3001.0, 4002.0, 1004.0, 3003.0, 0.0, 1002.0, 3001.0, 0.0, 2005.0, 2002.0, 0.0, 0.0, 0.0, 0.0]
    main_manual_bmps_order(sid, gvalues)

    # STEP+FIXED+MM
    print('-----STEP+FIXED+MM-----')
    sid = 248886478
    gvalues = [0.0, 2002.0, 2003.0, 2005.0, 2002.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2003.0, 0.0, 0.0, 2005.0, 2001.0, 0.0, 2003.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2005.0, 2002.0, 0.0, 0.0, 2001.0, 0.0, 1003.0, 1001.0, 0.0, 2005.0, 2001.0, 0.0, 2003.0, 2004.0, 0.0, 0.0, 2003.0, 0.0, 2003.0, 2001.0, 0.0, 2003.0, 0.0, 0.0, 1001.0, 3001.0, 2001.0, 0.0, 2005.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1001.0, 2004.0, 2005.0, 0.0, 2005.0, 2003.0, 1002.0, 0.0, 0.0, 2002.0, 2001.0, 0.0, 1001.0, 2002.0, 0.0, 0.0, 0.0, 0.0, 2003.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2005.0, 2001.0, 2005.0, 0.0, 0.0, 0.0, 1003.0, 3003.0, 4003.0, 1002.0, 3004.0, 0.0, 1001.0, 3004.0, 0.0, 2005.0, 2004.0, 0.0, 0.0, 0.0, 0.0]
    main_manual_bmps_order(sid, gvalues)

    # STEP+FIXED+LL
    print('-----STEP+FIXED+LL-----')
    sid = 187742724
    gvalues = [0.0, 2004.0, 2005.0, 2005.0, 2002.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2004.0, 0.0, 0.0, 2002.0, 2003.0, 0.0, 2002.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2001.0, 2001.0, 0.0, 0.0, 2002.0, 0.0, 1001.0, 1002.0, 0.0, 2001.0, 2002.0, 0.0, 2002.0, 2005.0, 0.0, 0.0, 2005.0, 0.0, 2003.0, 2001.0, 0.0, 2003.0, 0.0, 0.0, 1005.0, 3002.0, 2003.0, 0.0, 2001.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1003.0, 2001.0, 2005.0, 0.0, 2002.0, 2002.0, 1004.0, 0.0, 0.0, 2005.0, 2001.0, 0.0, 1004.0, 2003.0, 0.0, 0.0, 0.0, 0.0, 2002.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2004.0, 2002.0, 2003.0, 0.0, 0.0, 0.0, 1003.0, 3002.0, 4003.0, 1003.0, 3003.0, 0.0, 1001.0, 3004.0, 0.0, 2002.0, 2005.0, 0.0, 0.0, 0.0, 0.0]
    main_manual_bmps_order(sid, gvalues)

    # # STEP+VARY+HH
    # print('-----STEP+VARY+HH-----')
    # sid = 573558828
    # gvalues = [0.0, 2002.0, 2002.0, 2005.0, 2002.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2002.0, 0.0, 0.0, 2004.0, 2003.0, 0.0, 2003.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2004.0, 2002.0, 0.0, 0.0, 2002.0, 0.0, 1003.0, 1001.0, 0.0, 2001.0, 2001.0, 0.0, 2001.0, 2005.0, 0.0, 0.0, 2002.0, 0.0, 2003.0, 2001.0, 0.0, 2003.0, 0.0, 0.0, 1001.0, 3001.0, 2005.0, 0.0, 2002.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1001.0, 2001.0, 2001.0, 0.0, 2003.0, 2004.0, 1004.0, 0.0, 0.0, 2004.0, 2001.0, 0.0, 1001.0, 2003.0, 0.0, 0.0, 0.0, 0.0, 2005.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2001.0, 2001.0, 2001.0, 0.0, 0.0, 0.0, 1001.0, 3002.0, 4003.0, 1004.0, 3001.0, 0.0, 1002.0, 3005.0, 0.0, 2003.0, 2002.0, 0.0, 0.0, 0.0, 0.0]
    # main_manual_bmps_order(sid, gvalues)
    #
    # # STEP+VARY+MM
    # print('-----STEP+VARY+MM-----')
    # sid = 248509513
    # gvalues = [0.0, 2002.0, 2002.0, 2004.0, 2002.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2004.0, 0.0, 0.0, 2005.0, 2001.0, 0.0, 2005.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2005.0, 2004.0, 0.0, 0.0, 2002.0, 0.0, 1001.0, 1001.0, 0.0, 2004.0, 2001.0, 0.0, 2005.0, 2004.0, 0.0, 0.0, 2005.0, 0.0, 2004.0, 2003.0, 0.0, 2001.0, 0.0, 0.0, 1002.0, 3002.0, 2001.0, 0.0, 2002.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1001.0, 2001.0, 2004.0, 0.0, 2002.0, 2004.0, 1001.0, 0.0, 0.0, 2004.0, 2001.0, 0.0, 1001.0, 2003.0, 0.0, 0.0, 0.0, 0.0, 2005.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2005.0, 2001.0, 2005.0, 0.0, 0.0, 0.0, 1002.0, 3002.0, 4004.0, 1001.0, 3001.0, 0.0, 1003.0, 3005.0, 0.0, 2003.0, 2003.0, 0.0, 0.0, 0.0, 0.0]
    # main_manual_bmps_order(sid, gvalues)
    #
    # # STEP+VARY+LL
    # print('-----STEP+VARY+LL-----')
    # sid = 120310505
    # gvalues = [0.0, 2004.0, 2003.0, 2002.0, 2002.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2003.0, 0.0, 0.0, 2005.0, 2002.0, 0.0, 2005.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2004.0, 2001.0, 0.0, 0.0, 2001.0, 0.0, 1001.0, 1002.0, 0.0, 2005.0, 2001.0, 0.0, 2002.0, 2002.0, 0.0, 0.0, 2003.0, 0.0, 2005.0, 2001.0, 0.0, 2003.0, 0.0, 0.0, 1001.0, 3003.0, 2002.0, 0.0, 2003.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1004.0, 2005.0, 2005.0, 0.0, 2003.0, 2004.0, 1003.0, 0.0, 0.0, 2001.0, 2001.0, 0.0, 1004.0, 2002.0, 0.0, 0.0, 0.0, 0.0, 2003.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2004.0, 2002.0, 2005.0, 0.0, 0.0, 0.0, 1004.0, 3002.0, 4005.0, 1003.0, 3002.0, 0.0, 1001.0, 3001.0, 0.0, 2001.0, 2003.0, 0.0, 0.0, 0.0, 0.0]
    # main_manual_bmps_order(sid, gvalues)

    # benchmark scenario: all BMPs are implemented in the 1st year
    # sid = 1051
    # gvalues = [0.0, 2001.0, 2001.0, 2001.0, 2001.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2001.0, 0.0, 0.0, 2001.0, 2001.0,
    #            0.0, 2001.0, 0.0, 0.0,
    #            0.0, 0.0, 0.0, 2001.0, 2001.0, 0.0, 0.0, 2001.0, 0.0, 1001.0, 1001.0, 0.0, 2001.0, 2001.0, 0.0, 2001.0,
    #            2001.0, 0.0, 0.0, 2001.0, 0.0,
    #            2001.0, 2001.0, 0.0, 2001.0, 0.0, 0.0, 1001.0, 3001.0, 2001.0, 0.0, 2001.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    #            0.0, 1001.0, 2001.0, 2001.0,
    #            0.0, 2001.0, 2001.0, 1001.0, 0.0, 0.0, 2001.0, 2001.0, 0.0, 1001.0, 2001.0, 0.0, 0.0, 0.0, 0.0, 2001.0,
    #            0.0,
    #            0.0, 0.0, 0.0, 0.0,
    #            2001.0, 2001.0, 2001.0, 0.0, 0.0, 0.0, 1001.0, 3001.0, 4001.0, 1001.0, 3001.0, 0.0, 1001.0, 3001.0, 0.0,
    #            2001.0, 2001.0, 0.0, 0.0, 0.0, 0.0]
    # main_manual_bmps_order(sid, gvalues)

    # benchmark scenario: all BMPs are implemented in the 2nd year
    # sid = 1052
    # gvalues = [0.0, 2002.0, 2002.0, 2002.0, 2002.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2002.0, 0.0, 0.0, 2002.0, 2002.0,
    #            0.0, 2002.0, 0.0, 0.0,
    #            0.0, 0.0, 0.0, 2002.0, 2002.0, 0.0, 0.0, 2002.0, 0.0, 1002.0, 1002.0, 0.0, 2002.0, 2002.0, 0.0, 2002.0,
    #            2002.0, 0.0, 0.0, 2002.0, 0.0,
    #            2002.0, 2002.0, 0.0, 2002.0, 0.0, 0.0, 1002.0, 3002.0, 2002.0, 0.0, 2002.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    #            0.0, 1002.0, 2002.0, 2002.0,
    #            0.0, 2002.0, 2002.0, 1002.0, 0.0, 0.0, 2002.0, 2002.0, 0.0, 1002.0, 2002.0, 0.0, 0.0, 0.0, 0.0, 2002.0, 0.0,
    #            0.0, 0.0, 0.0, 0.0,
    #            2002.0, 2002.0, 2002.0, 0.0, 0.0, 0.0, 1002.0, 3002.0, 4002.0, 1002.0, 3002.0, 0.0, 1002.0, 3002.0, 0.0,
    #            2002.0, 2002.0, 0.0, 0.0, 0.0, 0.0]
    # main_manual_bmps_order(sid, gvalues)

    # benchmark scenario: all BMPs are implemented in the 3rd year
    # sid = 1053
    # gvalues = [0.0, 2003.0, 2003.0, 2003.0, 2003.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2003.0, 0.0, 0.0, 2003.0, 2003.0,
    #            0.0, 2003.0, 0.0, 0.0,
    #            0.0, 0.0, 0.0, 2003.0, 2003.0, 0.0, 0.0, 2003.0, 0.0, 1003.0, 1003.0, 0.0, 2003.0, 2003.0, 0.0, 2003.0,
    #            2003.0, 0.0, 0.0, 2003.0, 0.0,
    #            2003.0, 2003.0, 0.0, 2003.0, 0.0, 0.0, 1003.0, 3003.0, 2003.0, 0.0, 2003.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    #            0.0, 1003.0, 2003.0, 2003.0,
    #            0.0, 2003.0, 2003.0, 1003.0, 0.0, 0.0, 2003.0, 2003.0, 0.0, 1003.0, 2003.0, 0.0, 0.0, 0.0, 0.0, 2003.0, 0.0,
    #            0.0, 0.0, 0.0, 0.0,
    #            2003.0, 2003.0, 2003.0, 0.0, 0.0, 0.0, 1003.0, 3003.0, 4003.0, 1003.0, 3003.0, 0.0, 1003.0, 3003.0, 0.0,
    #            2003.0, 2003.0, 0.0, 0.0, 0.0, 0.0]
    # main_manual_bmps_order(sid, gvalues)

    # benchmark scenario: all BMPs are implemented in the 4th year
    # sid = 1054
    # gvalues = [0.0, 2004.0, 2004.0, 2004.0, 2004.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2004.0, 0.0, 0.0, 2004.0, 2004.0,
    #            0.0, 2004.0, 0.0, 0.0,
    #            0.0, 0.0, 0.0, 2004.0, 2004.0, 0.0, 0.0, 2004.0, 0.0, 1004.0, 1004.0, 0.0, 2004.0, 2004.0, 0.0, 2004.0,
    #            2004.0, 0.0, 0.0, 2004.0, 0.0,
    #            2004.0, 2004.0, 0.0, 2004.0, 0.0, 0.0, 1004.0, 3004.0, 2004.0, 0.0, 2004.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    #            0.0, 1004.0, 2004.0, 2004.0,
    #            0.0, 2004.0, 2004.0, 1004.0, 0.0, 0.0, 2004.0, 2004.0, 0.0, 1004.0, 2004.0, 0.0, 0.0, 0.0, 0.0, 2004.0, 0.0,
    #            0.0, 0.0, 0.0, 0.0,
    #            2004.0, 2004.0, 2004.0, 0.0, 0.0, 0.0, 1004.0, 3004.0, 4004.0, 1004.0, 3004.0, 0.0, 1004.0, 3004.0, 0.0,
    #            2004.0, 2004.0, 0.0, 0.0, 0.0, 0.0]
    # main_manual_bmps_order(sid, gvalues)

    # benchmark scenario: all BMPs are implemented in the 5th year
    # sid = 1055
    # gvalues = [0.0, 2005.0, 2005.0, 2005.0, 2005.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2005.0, 0.0, 0.0, 2005.0, 2005.0,
    #            0.0, 2005.0, 0.0, 0.0,
    #            0.0, 0.0, 0.0, 2005.0, 2005.0, 0.0, 0.0, 2005.0, 0.0, 1005.0, 1005.0, 0.0, 2005.0, 2005.0, 0.0, 2005.0,
    #            2005.0, 0.0, 0.0, 2005.0, 0.0,
    #            2005.0, 2005.0, 0.0, 2005.0, 0.0, 0.0, 1005.0, 3005.0, 2005.0, 0.0, 2005.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    #            0.0, 1005.0, 2005.0, 2005.0,
    #            0.0, 2005.0, 2005.0, 1005.0, 0.0, 0.0, 2005.0, 2005.0, 0.0, 1005.0, 2005.0, 0.0, 0.0, 0.0, 0.0, 2005.0, 0.0,
    #            0.0, 0.0, 0.0, 0.0,
    #            2005.0, 2005.0, 2005.0, 0.0, 0.0, 0.0, 1005.0, 3005.0, 4005.0, 1005.0, 3005.0, 0.0, 1005.0, 3005.0, 0.0,
    #            2005.0, 2005.0, 0.0, 0.0, 0.0, 0.0]
    # main_manual_bmps_order(sid, gvalues)

    # BMP 1 are implemented in the first year
    # sid = 101
    # gvalues = [0.0, 2003.0, 0.0, 0.0, 0.0, 0.0, 2003.0, 0.0, 0.0, 0.0, 1001.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    #            2003.0,
    #            0.0, 0.0, 2003.0, 0.0, 2003.0, 0.0, 2003.0, 2003.0, 0.0, 2003.0, 0.0, 0.0, 2003.0, 2003.0, 0.0, 2003.0, 2003.0, 0.0, 1001.0,
    #            3003.0, 0.0,
    #            0.0, 2003.0, 2003.0, 0.0, 2003.0, 0.0, 2003.0, 2003.0, 0.0, 1001.0, 3003.0, 0.0, 0.0, 1001.0, 0.0, 2003.0, 0.0, 0.0, 0.0,
    #            2003.0, 0.0,
    #            0.0, 1001.0, 0.0, 2003.0, 0.0, 0.0, 2003.0, 2003.0, 0.0, 1001.0, 2003.0, 0.0, 1001.0, 3003.0, 4003.0, 2003.0, 0.0, 0.0, 0.0,
    #            0.0, 0.0,
    #            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1001.0, 2003.0, 0.0, 1001.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2003.0, 2003.0, 0.0, 0.0, 0.0,
    #            0.0]
    # main_manual_bmps_order(sid, gvalues)

    # # BMP 2 are implemented in the first year
    # sid = 102
    # gvalues = [0.0, 2001.0, 0.0, 0.0, 0.0, 0.0, 2001.0, 0.0, 0.0, 0.0, 1003.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    #            2001.0,
    #            0.0, 0.0, 2001.0, 0.0, 2001.0, 0.0, 2001.0, 2001.0, 0.0, 2001.0, 0.0, 0.0, 2001.0, 2001.0, 0.0, 2001.0, 2001.0, 0.0, 1003.0,
    #            3003.0, 0.0,
    #            0.0, 2001.0, 2001.0, 0.0, 2001.0, 0.0, 2001.0, 2001.0, 0.0, 1003.0, 3003.0, 0.0, 0.0, 1003.0, 0.0, 2001.0, 0.0, 0.0, 0.0,
    #            2001.0, 0.0,
    #            0.0, 1003.0, 0.0, 2001.0, 0.0, 0.0, 2001.0, 2001.0, 0.0, 1003.0, 2001.0, 0.0, 1003.0, 3003.0, 4003.0, 2001.0, 0.0, 0.0, 0.0,
    #            0.0, 0.0,
    #            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1003.0, 2001.0, 0.0, 1003.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2001.0, 2001.0, 0.0, 0.0, 0.0,
    #            0.0]
    # main_manual_bmps_order(sid, gvalues)

    # BMP 3 are implemented in the first year
    # sid = 103
    # gvalues = [0.0, 2003.0, 0.0, 0.0, 0.0, 0.0, 2003.0, 0.0, 0.0, 0.0, 1003.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    #            2003.0,
    #            0.0, 0.0, 2003.0, 0.0, 2003.0, 0.0, 2003.0, 2003.0, 0.0, 2003.0, 0.0, 0.0, 2003.0, 2003.0, 0.0, 2003.0, 2003.0, 0.0, 1003.0,
    #            3001.0, 0.0,
    #            0.0, 2003.0, 2003.0, 0.0, 2003.0, 0.0, 2003.0, 2003.0, 0.0, 1003.0, 3001.0, 0.0, 0.0, 1003.0, 0.0, 2003.0, 0.0, 0.0, 0.0,
    #            2003.0, 0.0,
    #            0.0, 1003.0, 0.0, 2003.0, 0.0, 0.0, 2003.0, 2003.0, 0.0, 1003.0, 2003.0, 0.0, 1003.0, 3001.0, 4003.0, 2003.0, 0.0, 0.0, 0.0,
    #            0.0, 0.0,
    #            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1003.0, 2003.0, 0.0, 1003.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2003.0, 2003.0, 0.0, 0.0, 0.0,
    #            0.0]
    # main_manual_bmps_order(sid, gvalues)
    #
    # # BMP 4 are implemented in the first year
    # sid = 104
    # gvalues = [0.0, 2003.0, 0.0, 0.0, 0.0, 0.0, 2003.0, 0.0, 0.0, 0.0, 1003.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    #            2003.0,
    #            0.0, 0.0, 2003.0, 0.0, 2003.0, 0.0, 2003.0, 2003.0, 0.0, 2003.0, 0.0, 0.0, 2003.0, 2003.0, 0.0, 2003.0, 2003.0, 0.0, 1003.0,
    #            3003.0, 0.0,
    #            0.0, 2003.0, 2003.0, 0.0, 2003.0, 0.0, 2003.0, 2003.0, 0.0, 1003.0, 3003.0, 0.0, 0.0, 1003.0, 0.0, 2003.0, 0.0, 0.0, 0.0,
    #            2003.0, 0.0,
    #            0.0, 1003.0, 0.0, 2003.0, 0.0, 0.0, 2003.0, 2003.0, 0.0, 1003.0, 2003.0, 0.0, 1003.0, 3003.0, 4001.0, 2003.0, 0.0, 0.0, 0.0,
    #            0.0, 0.0,
    #            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1003.0, 2003.0, 0.0, 1003.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2003.0, 2003.0, 0.0, 0.0, 0.0,
    #            0.0]
    # main_manual_bmps_order(sid, gvalues)

    # BMP 4 are implemented in the first year
    # sid = 11111
    # gvalues = [0.0, 2002.0, 2002.0, 2004.0, 2002.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2004.0, 0.0, 0.0, 2005.0, 2001.0,
    #            0.0, 2005.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2005.0, 2004.0, 0.0, 0.0, 2002.0, 0.0, 1001.0, 1001.0, 0.0, 2004.0,
    #            2001.0, 0.0, 2005.0, 2004.0, 0.0, 0.0, 2005.0, 0.0, 2004.0, 2003.0, 0.0, 2001.0, 0.0, 0.0, 1002.0,
    #            3002.0, 2001.0, 0.0, 2002.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1001.0, 2001.0, 2004.0, 0.0, 2002.0,
    #            2004.0, 1001.0, 0.0, 0.0, 2004.0, 2001.0, 0.0, 1001.0, 2003.0, 0.0, 0.0, 0.0, 0.0, 2005.0, 0.0, 0.0, 0.0,
    #            0.0, 0.0, 2005.0, 2001.0, 2005.0, 0.0, 0.0, 0.0, 1002.0, 3002.0, 4004.0, 1001.0, 3001.0, 0.0, 1003.0,
    #            3005.0, 0.0, 2003.0, 2003.0, 0.0, 0.0, 0.0, 0.0]
    # main_manual_bmps_order(sid, gvalues)

    # Generate TXT files and gif files for the models that have been run.
    # generate_giff_txt(sid, gvalues)
    # generate_giff_txt_with_bmps_order(sid, gvalues)


def recalc_economy():
    # only for some custom functions
    # the operation list -> str only works in python 3
    log_file = 'D:/TempData/runtime.log.bk'
    output_file = 'D:/TempData/runtime.log'
    contents = []
    with open(log_file, 'r') as fp_in:
        for index, line in enumerate(fp_in.readlines()):
            if index in [0,1]:
                contents.append(line)
                continue
            items = line.split('\t')
            sce_id = int(items[1])
            gene_values_str = items[7][12:-3]
            # print(gene_values_str)
            gene_values = [float(g) for g in gene_values_str.split(',')]
            # print(gene_values)

            cf = get_config_parser()
            cfg = SASlpPosConfig(cf)
            cfg.construct_indexes_units_gene()
            sce = SUScenario(cfg)
            sce.set_unique_id(sce_id)
            sce.initialize(input_genes=gene_values)
            sce.decoding_with_bmps_order()
            # sce.export_to_mongodb()
            satisfied, [costs, maintains, incomes] = sce.satisfy_investment_constraints()
            new_economy = sce.calculate_economy_bmps_order(costs, maintains, incomes)
            items.insert(7,str(sce.costs_per_period))
            items.insert(8,str(sce.incomes_per_period))
            contents.append('\t'.join(items))

    with open(output_file, 'w') as fp_out:
        fp_out.writelines(contents)


if __name__ == '__main__':
    # output_tif = 'D:/Programs/SEIMS/data/youwuzhen/ss_youwuzhen10m_longterm_model/Scenario_220322012.tif'
    # gene_values = [0.0, 2.0, 2.0, 2.0, 2.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2.0, 0.0, 0.0, 2.0, 2.0, 0.0, 2.0, 0.0,
    #                0.0, 0.0, 0.0, 0.0, 2.0, 2.0, 0.0, 0.0, 2.0, 0.0, 1.0, 1.0, 0.0, 2.0, 2.0, 0.0, 2.0, 2.0, 0.0, 0.0,
    #                2.0, 0.0, 2.0, 2.0, 0.0, 2.0, 0.0, 0.0, 1.0, 3.0, 2.0, 0.0, 2.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    #                1.0, 2.0, 2.0, 0.0, 2.0, 2.0, 1.0, 0.0, 0.0, 2.0, 2.0, 0.0, 1.0, 2.0, 0.0, 0.0, 0.0, 0.0, 2.0, 0.0,
    #                0.0, 0.0, 0.0, 0.0, 2.0, 2.0, 2.0, 0.0, 0.0, 0.0, 1.0, 3.0, 4.0, 1.0, 3.0, 0.0, 1.0, 3.0, 0.0, 2.0,
    #                2.0, 0.0, 0.0, 0.0, 0.0]
    # generate_giff_txt_with_bmps_order(220322012, gene_values, True, True, output_tif)

    gvalues = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 2.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
               0.0, 0.0, 2.0, 0.0, 3.0, 0.0, 2.0, 2.0, 0.0, 2.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 2.0, 0.0, 0.0, 2.0, 0.0,
               0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 4.0, 1.0, 3.0, 0.0, 2.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0,
               1.0, 0.0, 4.0, 2.0, 0.0, 0.0, 2.0, 2.0, 0.0, 1.0, 2.0, 0.0, 1.0, 0.0, 0.0, 2.0, 0.0, 0.0, 0.0, 0.0, 0.0,
               0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 2.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2.0, 0.0, 0.0, 0.0, 0.0]
    generate_tiff_txt(111, gvalues)

    # recalc_economy()

    # test_func()

    # cf = get_config_parser()
    # extra_process_for_last_generation(cf,
    #     'D:/Programs/SEIMS/data/youwuzhen/ss_youwuzhen10m_longterm_model/SA_NSGA2_SLPPOS_HILLSLP_Gen_2_Pop_4/runtime.log',
    #     2,
    #     'D:/Programs/SEIMS/data/youwuzhen/ss_youwuzhen10m_longterm_model/group12_opt30/Scenarios/')

# cf = get_config_parser()
# # cfg = SAConfig(cf)  # type: SAConfig
# cfg = SAConnFieldConfig(cf)
# sceobj = SUScenario(cfg)  # type: Scenario
#
# # test the picklable of Scenario class.
# import pickle
#
# s = pickle.dumps(sceobj)
# # print(s)
# new_cfg = pickle.loads(s)  # type: Scenario
# print(new_cfg.modelcfg.ConfigDict)
# print('Model time range: %s - %s' % (new_cfg.model.start_time.strftime('%Y-%m-%d %H:%M:%S'),
#                                      new_cfg.model.end_time.strftime('%Y-%m-%d %H:%M:%S')))
# print('model scenario ID: %d, configured scenario ID: %d' % (new_cfg.model.scenario_id,
#                                                              new_cfg.ID))
# new_cfg.set_unique_id()
# print('model scenario ID: %d, configured scenario ID: %d' % (new_cfg.model.scenario_id,
#                                                              new_cfg.ID))
