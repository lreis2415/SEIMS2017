"""MongoDB utility

    @author   : Liangjun Zhu

    @changelog:
    - 16-12-07  - lj - rewrite for version 2.0
    - 17-06-27  - lj - reorganize as basic class other than Global variables
    - 18-02-08  - lj - compatible with Python3.
    - 20-07-20  - lj - no need to invoke close() of MongoClient after use
"""
from __future__ import absolute_import, unicode_literals

import os
import sys
if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
    sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))

from pymongo import MongoClient
from pymongo.errors import ConnectionFailure, InvalidOperation
from preprocess.text import DBTableNames, ModelParamFields


def singleton(cls):
    instances = dict()

    def wrapper(*args, **kwargs):
        if cls not in instances:
            instances[cls] = cls(*args, **kwargs)
        return instances[cls]

    return wrapper


@singleton
class ConnectMongoDB(object):
    """Connect to MongoDB, and close when finished."""
    client = None

    def __init__(self, ip, port, maxPoolSize=None):
        """initial mongodb client by hostname and port.

        Starting with version 3.0 the MongoClient constructor no longer blocks while connecting to
         the server or servers, and it no longer raises ConnectionFailure if they are unavailable,
         nor ConfigurationError if the user's credentials are wrong.
         Instead, the constructor returns immediately and launches the connection process on
          background threads.
        --https://api.mongodb.com/python/current/api/pymongo/mongo_client.html
        """
        self.conn = MongoClient(ip, port, maxPoolSize=maxPoolSize)
        try:
            self.conn.admin.command('ismaster')
        except ConnectionFailure as err:
            sys.stderr.write('Could not connect to MongoDB: %s' % err)
            sys.exit(1)

    def get_conn(self):  # type: (...) -> MongoClient
        """get MongoDB connection."""
        return self.conn

    def close(self):
        """Close collection.

        Create this client once for each process, and reuse it for all operations.
        It is a common mistake to create a new client for each request, which is very inefficient.
        --https://stackoverflow.com/questions/41015490/how-can-i-force-pymongo-to-close-sockets

        So, for now, I will comment the close operation. By lj.
        """
        # self.conn.close()
        pass


class MongoQuery(object):
    """
    Query data from MongoDB
    """

    @staticmethod
    def get_init_parameter_value(db_model, param_name, field=ModelParamFields.value):
        """Query initial parameter value, raise exception if error occurs."""
        coll_param = db_model[DBTableNames.main_parameter]
        param_dict = coll_param.find_one({ModelParamFields.name: param_name})
        if param_dict is None or param_dict.get(field) is None:
            raise RuntimeError('%s item is not existed in MongoDB!' % param_name)
        return param_dict.get(field)


class MongoUtil(object):
    """Some utility functions."""

    @staticmethod
    def run_bulk(bulk, errmsg=''):
        """Execute bulk operations, do not raise exception.
        Deprecated from v3.5
        """
        try:
            bulk.execute()
        except InvalidOperation:
            print('WARNING: %s' % errmsg)

    @staticmethod
    def run_bulk_write(coll, req_list):
        """Run bulk_write.

        Reference: https://pymongo.readthedocs.io/en/stable/api/pymongo/collection.html
                   #pymongo.collection.Collection.bulk_write
        """
        if len(req_list) == 0:
            return None
        return coll.bulk_write(req_list)
