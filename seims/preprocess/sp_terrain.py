# coding:utf-8
"""Terrain related spatial parameters extraction

    @author   : Liangjun Zhu, Junzhi Liu

    @changelog:
    - 16-07-06  lj - Code optimization by numpy
    - 16-12-07  lj - rewrite for version 2.0
    - 17-06-27  lj - reorganize as basic class other than Global variables
    - 18-02-08  lj - compatible with Python3.

    @TODO:
    - 1. for depression_capacity() function
      - 1.1. Add stream order modification, according to depression.ave of WetSpa.
      - 1.2. Add another depressional storage method according to SWAT, depstor.f

"""
from __future__ import absolute_import, unicode_literals

from math import exp, sqrt
import os
import sys

from preprocess.text import SpatialNamesUtils

if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
    sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))

import numpy
from osgeo.gdal import GDT_Float32
from osgeo.ogr import FieldDefn as ogr_FieldDefn
from osgeo.ogr import OFTReal
from osgeo.ogr import Open as ogr_Open
from pygeoc.hydro import FlowModelConst
from pygeoc.raster import RasterUtilClass

from utility import status_output, UTIL_ZERO, DEFAULT_NODATA
from utility import mask_rasterio
from preprocess.db_import_stream_parameters import ImportReaches2Mongo

sys.setrecursionlimit(10000)


class TerrainUtilClass(object):
    """Terrain related spatial parameters algorithms."""

    def __init__(self):
        """Empty"""
        pass

    @staticmethod
    def flow_length_cell(i, j, ysize, xsize, fdir, cellsize, weight, length,
                         flow_dir_code='TauDEM'):
        """Calculate flow length of cell."""
        celllen = FlowModelConst.get_cell_length(flow_dir_code)
        differ = FlowModelConst.get_cell_shift(flow_dir_code)
        # print(i,j, weight[i][j])
        if i < ysize and j < xsize:
            if length[i][j] == 0:
                if weight[i][j] > 0:
                    prei = i
                    prej = j
                    wt = weight[i][j]
                    fdir_v = fdir[i][j]
                    di = differ[fdir_v][0]
                    dj = differ[fdir_v][1]
                    i = i + di
                    j = j + dj
                    relen = TerrainUtilClass.flow_length_cell(i, j, ysize, xsize, fdir, cellsize,
                                                              weight, length,
                                                              flow_dir_code)
                    # print(i, j, fdir_v)
                    length[prei][prej] = cellsize * celllen[fdir_v] * wt + relen
                    return length[prei][prej]
                else:
                    return 0
            if length[i][j] > 0:
                return length[i][j]

            if length[i][j] < 0:
                print('Error in calculating flowlen_cell function! (%d, %d)' % (i, j))
                return -1
        return 0

    @staticmethod
    def calculate_flow_length(flow_dir_file, weight, flow_dir_code='TauDEM'):
        """Generate flow length with weight."""
        flow_dir_raster = RasterUtilClass.read_raster(flow_dir_file)
        fdir_data = flow_dir_raster.data
        xsize = flow_dir_raster.nCols
        ysize = flow_dir_raster.nRows
        nodata_value = flow_dir_raster.noDataValue
        # geotransform = flow_dir_raster.srs
        cellsize = flow_dir_raster.dx
        length = numpy.zeros((ysize, xsize))

        for i in range(0, ysize):
            for j in range(0, xsize):
                if abs(fdir_data[i][j] - nodata_value) < UTIL_ZERO:
                    length[i][j] = nodata_value
                    continue
                TerrainUtilClass.flow_length_cell(i, j, ysize, xsize, fdir_data, cellsize, weight,
                                                  length, flow_dir_code)
        return length

    @staticmethod
    def depression_capacity(maindb, landuse_file, slope_file, soil_texture_file,
                            depression_file, imper_perc=0.3):
        """Initialize depression capacity according to landuse, soil, and slope.
        Args:
            maindb: main MongoDatabase
            landuse_file: landuse raster file
            slope_file: slope raster file
            soil_texture_file: soil texture file
            depression_file: resulted depression raster file
            imper_perc: impervious percent in urban cell, 0.3 as default
        """
        # read landuselookup table from MongoDB
        st_fields = ['DSC_ST%d' % (i,) for i in range(1, 13)]
        query_result = maindb['LANDUSELOOKUP'].find()
        if query_result is None:
            raise RuntimeError('LanduseLoop Collection is not existed or empty!')
        dep_sd0 = dict()
        for row in query_result:
            tmpid = row.get('LANDUSE_ID')
            dep_sd0[tmpid] = [float(row.get(item)) for item in st_fields]

        landu_r = RasterUtilClass.read_raster(landuse_file)
        landu_data = landu_r.data
        geotrans = landu_r.geotrans
        srs = landu_r.srs
        xsize = landu_r.nCols
        ysize = landu_r.nRows
        landu_nodata = landu_r.noDataValue

        slo_data = RasterUtilClass.read_raster(slope_file).data
        soil_texture_array = RasterUtilClass.read_raster(soil_texture_file).data

        id_omited = []

        def cal_dep(landu, soil_texture, slp):
            """Calculate depression"""
            last_stid = 0
            if abs(landu - landu_nodata) < UTIL_ZERO:
                return DEFAULT_NODATA
            landu_id = int(landu)
            if landu_id not in dep_sd0:
                if landu_id not in id_omited:
                    print('The landuse ID: %d does not exist.' % (landu_id,))
                    id_omited.append(landu_id)
            stid = int(soil_texture) - 1
            try:
                depression_grid0 = dep_sd0[landu_id][stid]
                last_stid = stid
            except Exception:
                depression_grid0 = dep_sd0[landu_id][last_stid]

            depression_grid = exp(numpy.log(depression_grid0 + 0.0001) + slp * (-9.5))
            # TODO, check if it is  (landu_id >= 98)? By LJ
            if landu_id == 106 or landu_id == 107 or landu_id == 105:
                return 0.5 * imper_perc + (1. - imper_perc) * depression_grid
            else:
                return depression_grid

        cal_dep_numpy = numpy.frompyfunc(cal_dep, 3, 1)
        dep_storage_cap = cal_dep_numpy(landu_data, soil_texture_array, slo_data)

        RasterUtilClass.write_gtiff_file(depression_file, ysize, xsize, dep_storage_cap,
                                         geotrans, srs, DEFAULT_NODATA, GDT_Float32)

    @staticmethod
    def hydrological_radius(acc_file, radius_file, storm_probability='T2'):
        """Calculate hydrological radius."""
        acc_r = RasterUtilClass.read_raster(acc_file)
        xsize = acc_r.nCols
        ysize = acc_r.nRows
        nodata_value = acc_r.noDataValue
        cellsize = acc_r.dx
        data = acc_r.data
        coe_table = {"T2": [0.05, 0.48],
                     "T10": [0.12, 0.52],
                     "T100": [0.18, 0.55]}
        ap = coe_table[storm_probability][0]
        bp = coe_table[storm_probability][1]

        def radius_cal(acc):
            """Calculate hydrological radius"""
            if abs(acc - nodata_value) < UTIL_ZERO:
                return DEFAULT_NODATA
            return numpy.power(ap * ((acc + 1) * cellsize * cellsize / 1000000.), bp)

        radius_cal_numpy = numpy.frompyfunc(radius_cal, 1, 1)
        radius = radius_cal_numpy(data)

        RasterUtilClass.write_gtiff_file(radius_file, ysize, xsize, radius,
                                         acc_r.geotrans, acc_r.srs,
                                         DEFAULT_NODATA, GDT_Float32)

    @staticmethod
    def flow_velocity(slope_file, radius_file, manning_file, velocity_file):
        """velocity."""
        slp_r = RasterUtilClass.read_raster(slope_file)
        slp_data = slp_r.data
        xsize = slp_r.nCols
        ysize = slp_r.nRows
        nodata_value = slp_r.noDataValue

        rad_data = RasterUtilClass.read_raster(radius_file).data
        man_data = RasterUtilClass.read_raster(manning_file).data

        vel_max = 3.0
        vel_min = 0.0001

        def velocity_cal(rad, man, slp):
            """Calculate velocity"""
            if abs(slp - nodata_value) < UTIL_ZERO:
                return DEFAULT_NODATA
            # print(rad, man, slp)
            tmp = numpy.power(man, -1) * numpy.power(rad, 2. / 3.) * numpy.power(slp, 0.5)
            if tmp < vel_min:
                return vel_min
            if tmp > vel_max:
                return vel_max
            return tmp

        velocity_cal_numpy = numpy.frompyfunc(velocity_cal, 3, 1)
        velocity = velocity_cal_numpy(rad_data, man_data, slp_data)
        RasterUtilClass.write_gtiff_file(velocity_file, ysize, xsize, velocity, slp_r.geotrans,
                                         slp_r.srs, DEFAULT_NODATA, GDT_Float32)

    @staticmethod
    def flow_time_to_stream(streamlink, velocity, flow_dir_file, t0_s_file,
                            flow_dir_code='TauDEM'):
        """Calculate flow time to the workflow channel from each grid cell."""
        strlk_data = RasterUtilClass.read_raster(streamlink).data

        vel_r = RasterUtilClass.read_raster(velocity)
        vel_data = vel_r.data
        xsize = vel_r.nCols
        ysize = vel_r.nRows
        # noDataValue = vel_r.noDataValue

        weight = numpy.where(strlk_data <= 0, numpy.ones((ysize, xsize)),
                             numpy.zeros((ysize, xsize)))
        traveltime = numpy.where(vel_r.validZone, numpy.zeros((ysize, xsize)), vel_data)
        flowlen = TerrainUtilClass.calculate_flow_length(flow_dir_file, weight, flow_dir_code)
        traveltime = numpy.where(vel_r.validZone, flowlen / (vel_data * 5. / 3.) / 3600.,
                                 traveltime)
        RasterUtilClass.write_gtiff_file(t0_s_file, ysize, xsize, traveltime, vel_r.geotrans,
                                         vel_r.srs, DEFAULT_NODATA, GDT_Float32)

    @staticmethod
    def std_of_flow_time_to_stream(streamlink, flow_dir_file, slope, radius, velocity, delta_s_file,
                                   flow_dir_code='TauDEM'):
        """Generate standard deviation of t0_s (flow time to the workflow channel from each cell).
        """
        strlk_r = RasterUtilClass.read_raster(streamlink)
        strlk_data = strlk_r.data
        rad_data = RasterUtilClass.read_raster(radius).data
        slo_data = RasterUtilClass.read_raster(slope).data

        vel_r = RasterUtilClass.read_raster(velocity)
        vel_data = vel_r.data
        xsize = vel_r.nCols
        ysize = vel_r.nRows
        nodata_value = vel_r.noDataValue

        def initial_variables(vel, strlk, slp, rad):
            """initial variables"""
            if abs(vel - nodata_value) < UTIL_ZERO:
                return DEFAULT_NODATA
            if strlk <= 0:
                tmp_weight = 1
            else:
                tmp_weight = 0
            # 0 is river
            if slp < 0.0005:
                slp = 0.0005
            # dampGrid = vel * rad / (slp / 100. * 2.) # No need to divide 100
            # in my view. By LJ
            damp_grid = vel * rad / (slp * 2.)
            celerity = vel * 5. / 3.
            tmp_weight *= damp_grid * 2. / numpy.power(celerity, 3.)
            return tmp_weight

        initial_variables_numpy = numpy.frompyfunc(initial_variables, 4, 1)
        weight = initial_variables_numpy(vel_data, strlk_data, slo_data, rad_data)

        delta_s_sqr = TerrainUtilClass.calculate_flow_length(flow_dir_file, weight, flow_dir_code)

        def cal_delta_s(vel, sqr):
            """Calculate delta s"""
            if abs(vel - nodata_value) < UTIL_ZERO:
                return nodata_value
            else:
                return sqrt(sqr) / 3600.

        cal_delta_s_numpy = numpy.frompyfunc(cal_delta_s, 2, 1)
        delta_s = cal_delta_s_numpy(vel_data, delta_s_sqr)

        RasterUtilClass.write_gtiff_file(delta_s_file, ysize, xsize, delta_s, strlk_r.geotrans,
                                         strlk_r.srs, DEFAULT_NODATA, GDT_Float32)

    @staticmethod
    def calculate_latitude_dependent_parameters(lat_file, min_dayl_file, dormhr_file, dorm_hr):
        """
        Calculate latitude dependent parameters, include:
           1. minimum daylength (daylmn), 2. day length threshold for dormancy (dormhr)
        """
        # calculate minimum daylength, from readwgn.f of SWAT
        # daylength=2*acos(-tan(sd)*tan(lat))/omega
        # where solar declination, sd, = -23.5 degrees for minimum daylength in
        # northern hemisphere and -tan(sd) = .4348
        # absolute value is taken of tan(lat) to convert southern hemisphere
        # values to northern hemisphere
        # the angular velocity of the earth's rotation, omega, = 15 deg/hr or
        # 0.2618 rad/hr and 2/0.2618 = 7.6394
        cell_lat_r = RasterUtilClass.read_raster(lat_file)
        lat_data = cell_lat_r.data
        nodata = cell_lat_r.noDataValue
        nodatas = numpy.ones((cell_lat_r.nRows, cell_lat_r.nCols)) * nodata

        daylmn_data = numpy.where(cell_lat_r.validZone,
                                  0.4348 * numpy.abs(numpy.tan(lat_data / 57.296)), nodatas)
        # Caution: DO NOT use numpy.arccos directly in numpy.where, because
        #           np.arccos(in) will be computed before np.where, which will cause
        #           RuntimeWarning: invalid value encountered in arccos
        #   see https://stackoverflow.com/q/61197560/4837280
        daylmn_data = numpy.where(cell_lat_r.validZone,
                                  numpy.arccos(numpy.clip(daylmn_data, -1, 1)), nodatas)
        daylmn_data = numpy.where(cell_lat_r.validZone, daylmn_data * 7.6394, nodatas)
        RasterUtilClass.write_gtiff_file(min_dayl_file, cell_lat_r.nRows, cell_lat_r.nCols,
                                         daylmn_data, cell_lat_r.geotrans, cell_lat_r.srs,
                                         nodata, GDT_Float32)

        def cal_dorm_hr(lat):
            """calculate day length threshold for dormancy"""
            if lat == cell_lat_r.noDataValue:
                return cell_lat_r.noDataValue
            else:
                if 20. <= lat <= 40:
                    return (numpy.abs(lat - 20.)) / 20.
                elif lat > 40.:
                    return 1.
                elif lat < 20.:
                    return -1.

        cal_dorm_hr_numpy = numpy.frompyfunc(cal_dorm_hr, 1, 1)

        if dorm_hr < -UTIL_ZERO:
            dormhr_data = cal_dorm_hr_numpy(lat_data)
        else:
            dormhr_data = numpy.where(cell_lat_r.validZone,
                                      numpy.ones((cell_lat_r.nRows, cell_lat_r.nCols)) * dorm_hr,
                                      lat_data)
        RasterUtilClass.write_gtiff_file(dormhr_file, cell_lat_r.nRows, cell_lat_r.nCols,
                                         dormhr_data, cell_lat_r.geotrans, cell_lat_r.srs,
                                         cell_lat_r.noDataValue, GDT_Float32)

    @staticmethod
    def calculate_channel_width_depth(acc_file, chwidth_file, chdepth_file):
        """Calculate channel width and depth according to drainage area (km^2).

        The equations used in the BASINS software to estimate channel width and depth are adopted.

        W = 1.29 * A ^ 0.6
        D = 0.13 * A ^ 0.4

        where W is bankfull channel width (m), D is bankfull channel depth (m), and A is drainage
          area (km^2)

        References:
            Ames, D.P., Rafn, E.B., Kirk, R.V., Crosby, B., 2009.
              Estimation of stream channel geometry in Idaho using GIS-derived watershed
              characteristics. Environ. Model. Softw. 24, 444–448.
              https://doi.org/10.1016/j.envsoft.2008.08.008

        """
        acc_r = RasterUtilClass.read_raster(acc_file)
        xsize = acc_r.nCols
        ysize = acc_r.nRows
        dx = acc_r.dx
        cell_area = dx * dx * 0.000001  # m^2 ==> km^2

        tmp_ones = numpy.ones((ysize, xsize))
        width = tmp_ones * DEFAULT_NODATA
        depth = tmp_ones * DEFAULT_NODATA
        valid_values = numpy.where(acc_r.validZone, acc_r.data, tmp_ones)
        width = numpy.where(acc_r.validZone,
                            numpy.power((1.29 * (valid_values + 1) * cell_area), 0.6),
                            width)
        depth = numpy.where(acc_r.validZone,
                            numpy.power((0.13 * (valid_values + 1) * cell_area), 0.4),
                            depth)

        RasterUtilClass.write_gtiff_file(chwidth_file, ysize, xsize, width, acc_r.geotrans,
                                         acc_r.srs, DEFAULT_NODATA, GDT_Float32)
        RasterUtilClass.write_gtiff_file(chdepth_file, ysize, xsize, depth, acc_r.geotrans,
                                         acc_r.srs, DEFAULT_NODATA, GDT_Float32)

    @staticmethod
    def add_channel_width_depth_to_shp(reach_shp_file, stream_link_file, width_file, depth_file):
        """Calculate average channel width and depth, and add or modify the attribute table
           of reach.shp
        """
        stream_link = RasterUtilClass.read_raster(stream_link_file)
        n_rows = stream_link.nRows
        n_cols = stream_link.nCols
        nodata_value = stream_link.noDataValue
        data_stream = stream_link.data

        width = RasterUtilClass.read_raster(width_file)
        width_data = width.data
        depth = RasterUtilClass.read_raster(depth_file)
        depth_data = depth.data

        ch_width_dic = dict()
        ch_depth_dic = dict()
        ch_num_dic = dict()

        for i in range(n_rows):
            for j in range(n_cols):
                if abs(data_stream[i][j] - nodata_value) <= UTIL_ZERO:
                    continue
                tmpid = int(data_stream[i][j])
                ch_num_dic.setdefault(tmpid, 0)
                ch_width_dic.setdefault(tmpid, 0)
                ch_depth_dic.setdefault(tmpid, 0)

                ch_num_dic[tmpid] += 1
                ch_width_dic[tmpid] += width_data[i][j]
                ch_depth_dic[tmpid] += depth_data[i][j]

        for k in ch_num_dic:
            ch_width_dic[k] /= ch_num_dic[k]
            ch_depth_dic[k] /= ch_num_dic[k]

        # add channel width and depth fields to reach shp file or update values if the fields exist
        ds_reach = ogr_Open(reach_shp_file, update=True)
        layer_reach = ds_reach.GetLayer(0)
        layer_def = layer_reach.GetLayerDefn()
        i_link = layer_def.GetFieldIndex(str(ImportReaches2Mongo._LINKNO))
        i_width = layer_def.GetFieldIndex(str(ImportReaches2Mongo._WIDTH))
        i_depth = layer_def.GetFieldIndex(str(ImportReaches2Mongo._DEPTH))
        if i_width < 0:
            new_field = ogr_FieldDefn(str(ImportReaches2Mongo._WIDTH), OFTReal)
            layer_reach.CreateField(new_field)
        if i_depth < 0:
            new_field = ogr_FieldDefn(str(ImportReaches2Mongo._DEPTH), OFTReal)
            layer_reach.CreateField(new_field)

        layer_reach.ResetReading()
        ft = layer_reach.GetNextFeature()
        while ft is not None:
            tmpid = ft.GetFieldAsInteger(i_link)
            w = 5.
            d = 1.5
            if tmpid in ch_width_dic:
                w = ch_width_dic[tmpid]
            if tmpid in ch_depth_dic:
                d = ch_depth_dic[tmpid]
            ft.SetField(str(ImportReaches2Mongo._WIDTH), w)
            ft.SetField(str(ImportReaches2Mongo._DEPTH), d)
            layer_reach.SetFeature(ft)
            ft = layer_reach.GetNextFeature()

        layer_reach.SyncToDisk()
        ds_reach.Destroy()
        del ds_reach

    @staticmethod
    def parameters_extraction(cfg):
        """Main entrance for terrain related spatial parameters extraction."""
        f = cfg.logs.extract_terrain
        # To make use of old code, we have to export some rasters from MongoDB
        mask_rasterio(cfg.seims_bin,
                      [['0_MANNING', cfg.spatials.manning]],
                      mongoargs=[cfg.hostname, cfg.port, cfg.spatial_db, 'SPATIAL'],
                      maskfile=cfg.spatials.subbsn,
                      include_nodata=False, mode='MASK')

        status_output('Calculate initial channel width and added to reach.shp...', 10, f)
        TerrainUtilClass.calculate_channel_width_depth(cfg.spatials.d8acc,
                                                       cfg.spatials.chwidth,
                                                       cfg.spatials.chdepth)
        TerrainUtilClass.add_channel_width_depth_to_shp(cfg.vecs.reach,
                                                        cfg.spatials.stream_link,
                                                        cfg.spatials.chwidth,
                                                        cfg.spatials.chdepth)

        status_output('Generating depression storage capacity...', 20, f)
        TerrainUtilClass.depression_capacity(cfg.maindb, cfg.spatials.landuse,
                                             cfg.spatials.slope,
                                             cfg.spatials.soil_texture,
                                             cfg.spatials.depression,
                                             cfg.imper_perc_in_urban)

        status_output('Prepare parameters for IUH...', 30, f)
        # Note: IUH calculation and import to MongoDB are implemented in db_build_mongodb.py
        TerrainUtilClass.hydrological_radius(cfg.spatials.d8acc, cfg.spatials.radius, 'T2')
        TerrainUtilClass.flow_velocity(cfg.spatials.slope, cfg.spatials.radius,
                                       cfg.spatials.manning, cfg.spatials.velocity)
        flow_model_code = 'ArcGIS'
        TerrainUtilClass.flow_time_to_stream(cfg.spatials.stream_link,
                                             cfg.spatials.velocity,
                                             cfg.spatials.d8flow,
                                             cfg.spatials.t0_s,
                                             flow_model_code)
        TerrainUtilClass.std_of_flow_time_to_stream(cfg.spatials.stream_link,
                                                    cfg.spatials.d8flow,
                                                    cfg.spatials.slope,
                                                    cfg.spatials.radius,
                                                    cfg.spatials.velocity,
                                                    cfg.spatials.delta_s,
                                                    flow_model_code)

        status_output('Calculate latitude dependent parameters...', 40, f)
        TerrainUtilClass.calculate_latitude_dependent_parameters(cfg.spatials.cell_lat,
                                                                 cfg.spatials.dayl_min,
                                                                 cfg.spatials.dorm_hr,
                                                                 cfg.dorm_hr)

        status_output('Terrain related spatial parameters extracted done!', 100, f)


def main():
    """TEST CODE"""
    from preprocess.config import parse_ini_configuration

    seims_cfg = parse_ini_configuration()

    TerrainUtilClass.parameters_extraction(seims_cfg)


if __name__ == '__main__':
    main()
