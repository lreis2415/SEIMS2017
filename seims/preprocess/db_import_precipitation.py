#! /usr/bin/env python
# -*- coding: utf-8 -*-
"""Import precipitation data, daily or storm
    @author   : Liangjun Zhu, Junzhi Liu
    @changelog: 16-12-07  lj - rewrite for version 2.0
                17-07-04  lj - reorganize according to pylint and google style
"""
import datetime
import time

from pymongo import ASCENDING

from seims.preprocess.config import parse_ini_configuration
from seims.preprocess.db_mongodb import ConnectMongoDB
from seims.preprocess.text import DBTableNames, DataValueFields, DataType
from seims.preprocess.utility import read_data_items_from_txt
from seims.pygeoc.pygeoc.utils.utils import StringClass


class ImportPrecipitation(object):
    """Import precipitation data, daily or storm."""

    @staticmethod
    def regular_data_from_txt(db, data_file, is_first):
        """Regular precipitation data from text file."""
        clim_data_items = read_data_items_from_txt(data_file)
        clim_flds = clim_data_items[0]
        station_id = []
        for i in range(3, len(clim_flds)):
            station_id.append(clim_flds[i])
        for i, clim_data_item in enumerate(clim_data_items):
            if i == 0:
                continue
            dic = dict()
            precipitation = []
            cur_y = 0
            cur_m = 0
            cur_d = 0
            for j, clim_data_v in enumerate(clim_data_item):
                if StringClass.string_match(clim_flds[j], DataValueFields.y):
                    cur_y = int(clim_data_v)
                elif StringClass.string_match(clim_flds[j], DataValueFields.m):
                    cur_m = int(clim_data_v)
                elif StringClass.string_match(clim_flds[j], DataValueFields.d):
                    cur_d = int(clim_data_v)
                else:
                    for k, cur_id in enumerate(station_id):
                        if StringClass.string_match(clim_flds[j], cur_id):
                            precipitation.append(float(clim_data_v))

            dt = datetime.datetime(cur_y, cur_m, cur_d, 0, 0)
            sec = time.mktime(dt.timetuple())
            utc_time = time.gmtime(sec)
            dic[DataValueFields.local_time] = dt
            dic[DataValueFields.time_zone] = time.timezone / 3600.
            dic[DataValueFields.utc] = datetime.datetime(utc_time[0], utc_time[1],
                                                         utc_time[2], utc_time[3])

            for j, cur_id in enumerate(station_id):
                cur_dic = dict()
                cur_dic[DataValueFields.value] = precipitation[j]
                cur_dic[DataValueFields.id] = int(cur_id)
                cur_dic[DataValueFields.type] = DataType.p
                cur_dic[DataValueFields.time_zone] = dic[DataValueFields.time_zone]
                cur_dic[DataValueFields.local_time] = dic[DataValueFields.local_time]
                cur_dic[DataValueFields.utc] = dic[DataValueFields.utc]
                curfilter = {DataValueFields.id: cur_dic[DataValueFields.id],
                             DataValueFields.type: cur_dic[DataValueFields.type],
                             DataValueFields.utc: cur_dic[DataValueFields.utc]}
                if is_first:
                    db[DBTableNames.data_values].insert_one(cur_dic)
                else:
                    db[DBTableNames.data_values].find_one_and_replace(curfilter, cur_dic,
                                                                      upsert=True)
        # Create index
        db[DBTableNames.data_values].create_index([(DataValueFields.id, ASCENDING),
                                                   (DataValueFields.type, ASCENDING),
                                                   (DataValueFields.utc, ASCENDING)])

    @staticmethod
    def workflow(cfg, clim_db):
        """Workflow"""
        print ("Import Daily Precipitation Data... ")
        c_list = clim_db.collection_names()
        first_import = False
        if not StringClass.string_in_list(DBTableNames.data_values, c_list):
            clim_db.create_collection(DBTableNames.data_values)
            first_import = True
        ImportPrecipitation.regular_data_from_txt(clim_db, cfg.prec_data, first_import)


def main():
    """TEST CODE"""
    seims_cfg = parse_ini_configuration()
    client = ConnectMongoDB(seims_cfg.hostname, seims_cfg.port)
    conn = client.get_conn()
    hydroclim_db = conn[seims_cfg.climate_db]

    ImportPrecipitation.workflow(seims_cfg, hydroclim_db)

    client.close()


if __name__ == "__main__":
    main()
