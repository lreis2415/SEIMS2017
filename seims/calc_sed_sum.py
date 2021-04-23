# coding:utf-8

from pygeoc.raster import RasterUtilClass
from pygeoc.utils import FileClass, StringClass, UtilClass, get_config_parser, is_string

MODEL_PATH = 'D:/Programs/SEIMS/data/youwuzhen/ss_youwuzhen10m_longterm_model'

def calc_sed_sum(sceid, impl_period):
    raster_file = '%s/OUTPUT%d/SED_OL_SUM.tif' % (MODEL_PATH,sceid)
    rr = RasterUtilClass.read_raster(raster_file)
    sed_sum = rr.get_sum() / impl_period
    print(sed_sum)


if __name__ == '__main__':
    sceid = 0
    impl_period = 3
    calc_sed_sum(sceid, impl_period)
