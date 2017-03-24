#! /usr/bin/env python
# coding=utf-8
#
# @Author: Junzhi Liu, 2013-1-10
# @Revised: Liang-Jun Zhu, Huiran Gao, Fang Shen
# @Revised date: 2016-7-22
# @Note: 1. Names and units of soil physical parameter are referred to readsol.f, soil_par.f, and soil_phys.f in SWAT
#        2. Data validation checking is also conducted here.
#        3. Basic protocols: a. all names are Capitalized. b. output Geotiff names may be appended in text.py
# @Revised: Liang-Jun Zhu, 2017-2-19
#
import math
import types

import numpy
from gdal import GDT_Float32
from pygeoc.raster.raster import RasterUtilClass

from config import *
from utility import LoadConfiguration, status_output, ReadDataItemsFromTxt
from utility import DEFAULT_NODATA, UTIL_ZERO, MINI_SLOPE


class SoilProperty(object):
    """
    base class of Soil physical and general chemical properties
    Attributes:
        SOILLAYERS (int, None): (nly in SWAT model, the same below) number of soil layers
        SOILDEPTH (float, mm): (sol_z) depth from the surface to bottom of soil layer
        SOILTHICK (float, mm): soil thickness for calculation convenient, derived from `SOILDEPTH`
        OM (float, %): organic matter content (weight percent)
        SOL_CBN (float, %): (sol_cbn) percent organic carbon in soil layer, calculated by `OM`
        SOL_N (float, %): (sol_n) used when using CSWAT = 1, i.e, C-FARM one carbon pool model, derived from `SOL_CBN`
        CLAY (float, %): (sol_clay) percent clay content in soil material, diameter < 0.002 mm
        SILT (float, %): (sol_silt) percent silt content in soil material,diameter between 0.002 mm and 0.05 mm
        SAND (float, %): (sol_sand) percent sand content in soil material,diameter between 0.05 mm and 2 mm
        ROCK (float, %): (sol_rock) percent of rock fragments content in soil material,diameter > 2 mm
        SOIL_TEXTURE (int, None): soil texture code used in WetSpa and SWAT model based on soil texture triangle by USDA
        HYDRO_GROUP (int, None): Hydrological soil group, 1, 2, 3, and 4 to represent A, B, C, and D
        SOL_ZMX (float, mm): (sol_zmx) maximum rooting depth of soil profile
        ANION_EXCL (float, None): (anion_excl) fraction of porosity from which anions are excluded, default is 0.5
        SOL_CRK (float, None): (sol_crk) crack volume potential of soil expressed as a fraction of the total soil volume
        DENSITY (float, Mg/m3 or g/cm3): (sol_bd) bulk density of each soil layer
        SOL_AVBD (float, Mg/m3 or g/cm3): (sol_avbd) average bulk density for soil profile
        CONDUCTIVITY (float, mm/hr): (sol_k) saturated hydraulic conductivity
        SOL_HK (float, None): (sol_hk) beta coefficent to calculate hydraulic conductivity
        WILTINGPOINT (float, mm H2O / mm soil): (sol_wp) water content of soil at -1.5 MPa (wilting point)
        SOL_WPMM (float, mm H2O): (sol_wpmm) water content of soil at -1.5 MPa (wilting point)
        SOL_SUMWP (float, mm H2O): (sol_sumwp) amount of water held in the soil profile at wilting point
        FIELDCAP (float,mm H2O / mm soil): (sol_up) water content of soil at -0.033 MPa (field capacity)
        AWC (float,mm H2O / mm soil): (sol_awc) available water capacity of soil layer
        SOL_AWC  (float,mm H2O): (sol_fc) amount of water available to plants in soil layer at field capacity (fc - wp)
        SOL_SUMAWC (float,mm H2O): (sol_sumfc) amount of water held in soil profile at field capacity
        POROSITY (float,None): (sol_por) total porosity of soil layer expressed as a fraction of the total volume
        POREINDEX (float,None): pore size distribution index
        SOL_AVPOR (float,None): (sol_avpor) average porosity for entire soil profile
        SOL_UL (float,mm H2O): (sol_ul) amount of water held in the soil layer at saturation (sat - wp water)
        SOL_SUMUL (float,mm H2O): (sol_sumul) amount of water held in soil profile at saturation
        USLE_K (float,None):  USLE K factor
        SOL_ALB (float,None): albedo of top soil surface
        WFSH (float,mm): wetting front matric potential (usde in Green-Ampt method)
        ESCO (float,None): soil evaporation compensation factor
        VWT (float,None): (vwt) variable water table factor, used in percolation modules
        DET_SAND (float,None): (det_san) detached sediment size distribution, sand fraction
        DET_SILT (float,None): (det_sil) detached sediment size distribution, silt fraction
        DET_CLAY (float,None): (det_cla) detached sediment size distribution, clay fraction
        DET_SMAGG (float,None): (det_sag) detached sediment size distribution, small aggregation fraction
        DET_LGAGG (float,None): (det_lag) detached sediment size distribution, large aggregation fraction
        CRDEP (float,mm): (crdep) maximum or potential crack volume
        VOLCR (float,mm): (volcr) crack volume for soil layer, should be calculated in SEIMS, using moist_ini
        SOL_NO3 (float,kg/ha): (sol_no3) concentration of nitrate in soil layers
        SOL_NH4 (float,kg/ha): (sol_nh4) concentration of ammonium-N in soil layers
        SOL_ORGN (float,kg/ha): (sol_orgn) organic N concentration in soil layers
        SOL_ORGP (float,kg/ha): (sol_orgp) organic P concentration in soil layers
        SOL_SOLP (float,kg/ha): (sol_solp) soluble P concentration in soil layers
    """

    def __init__(self, SEQN, SNAM):
        """
        Initialize a soil property object.
        Args:
            SEQN (int): Soil sequence number, Unique identifier
            SNAME (str): The corresponding soil name
        """
        self.SEQN = SEQN
        self.SNAM = SNAM
        self.SOILLAYERS = DEFAULT_NODATA
        self.SOILDEPTH = []
        self.SOILTHICK = []
        self.OM = []
        self.SOL_CBN = []
        self.SOL_N = []
        self.CLAY = []
        self.SILT = []
        self.SAND = []
        self.ROCK = []
        self.SOIL_TEXTURE = DEFAULT_NODATA
        self.HYDRO_GROUP = DEFAULT_NODATA
        self.SOL_ZMX = DEFAULT_NODATA
        self.ANION_EXCL = DEFAULT_NODATA
        self.SOL_CRK = DEFAULT_NODATA
        self.DENSITY = []
        self.SOL_AVBD = []
        self.CONDUCTIVITY = []
        self.SOL_HK = []
        self.WILTINGPOINT = []
        self.SOL_WPMM = []
        self.SOL_SUMWP = 0.
        self.FIELDCAP = []
        self.AWC = []
        self.SOL_AWC = []
        self.SOL_SUMAWC = 0.
        self.POROSITY = []
        self.POREINDEX = []
        self.SOL_AVPOR = DEFAULT_NODATA
        self.SOL_UL = []
        self.SOL_SUMUL = 0.
        self.USLE_K = []
        self.SOL_ALB = DEFAULT_NODATA
        self.WFSH = DEFAULT_NODATA
        self.VWT = []
        self.DET_SAND = DEFAULT_NODATA
        self.DET_SILT = DEFAULT_NODATA
        self.DET_CLAY = DEFAULT_NODATA
        self.DET_SMAGG = DEFAULT_NODATA
        self.DET_LGAGG = DEFAULT_NODATA
        self.CRDEP = []
        self.ESCO = DEFAULT_NODATA
        # Here after are general soil chemical properties
        self.SOL_NO3 = []
        self.SOL_NH4 = []
        self.SOL_ORGN = []
        self.SOL_ORGP = []
        self.SOL_SOLP = []

    def SoilDict(self):
        """Convert to dict"""
        solDict = self.__dict__
        solDict.pop(SOL_NAME)
        # remove the empty element
        for ele in solDict:
            if isinstance(solDict[ele], list) and not solDict[ele]:
                del solDict[ele]
        # print solDict
        return solDict

    def CheckData(self):
        """Check the required input, and calculate all physical and general chemical properties"""
        # set a soil layer at dep_new and adjust all lower layers
        # a septic layer:0-10mm, refers to layersplit.f in SWAT
        if self.SOILLAYERS == DEFAULT_NODATA:
            raise ValueError("Soil layers number must be provided, please check the input file!")
        dep_new = 10.
        if self.SOILDEPTH[0] - dep_new >= 10.:
            self.SOILLAYERS += 1
            self.SOILDEPTH.insert(0, dep_new)
            if self.OM:
                self.OM.insert(0, self.OM[0])
            else:
                raise ValueError("Organic matter must be provided!")
            if self.CLAY:
                self.CLAY.insert(0, self.CLAY[0])
            else:
                raise ValueError("Clay content must be provided!")
            if self.SILT:
                self.SILT.insert(0, self.SILT[0])
            else:
                raise ValueError("Silt content must be provided!")
            if self.SAND:
                self.SAND.insert(0, self.SAND[0])
            else:
                raise ValueError("Sand content must be provided!")
            if self.ROCK:
                self.ROCK.insert(0, self.ROCK[0])
            else:
                raise ValueError("Rock content must be provided!")
            if self.FIELDCAP:
                self.FIELDCAP.insert(0, self.FIELDCAP[0])
            else:
                raise ValueError("Available water capacity must be provided!")
            if self.DENSITY:
                self.DENSITY.insert(0, self.DENSITY[0])
            else:
                raise ValueError("Bulk density must be provided!")
            if self.CONDUCTIVITY:
                self.CONDUCTIVITY.insert(0, self.CONDUCTIVITY[0])
            if self.WILTINGPOINT:
                self.WILTINGPOINT.insert(0, self.WILTINGPOINT[0])
            if self.AWC:
                self.AWC.insert(0, self.AWC[0])
                for i in range(self.SOILLAYERS):
                    if self.AWC[i] <= 0.:
                        self.AWC[i] = 0.005
                    elif self.AWC[i] <= 0.01:
                        self.AWC[i] = 0.01
                    elif self.AWC[i] >= 0.8:
                        self.AWC[i] = 0.8
            if self.POROSITY:
                self.POROSITY.insert(0, self.POROSITY[0])
            if self.USLE_K:
                self.USLE_K.insert(0, self.USLE_K[0])
            if self.SOL_NO3:
                self.SOL_NO3.insert(0, self.SOL_NO3[0])
            else:
                self.SOL_NO3 = list(numpy.zeros(self.SOILLAYERS))
            if self.SOL_NH4:
                self.SOL_NH4.insert(0, self.SOL_NH4[0])
            else:
                self.SOL_NH4 = list(numpy.zeros(self.SOILLAYERS))
            if self.SOL_ORGN:
                self.SOL_ORGN.insert(0, self.SOL_ORGN[0])
            else:
                self.SOL_ORGN = list(numpy.zeros(self.SOILLAYERS))
            if self.SOL_SOLP:
                self.SOL_SOLP.insert(0, self.SOL_SOLP[0])
            else:
                self.SOL_SOLP = list(numpy.zeros(self.SOILLAYERS))
            if self.SOL_ORGP:
                self.SOL_ORGP.insert(0, self.SOL_ORGP[0])
            else:
                self.SOL_ORGP = list(numpy.zeros(self.SOILLAYERS))
        if self.SOILDEPTH == [] or len(self.SOILDEPTH) != self.SOILLAYERS or DEFAULT_NODATA in self.SOILDEPTH:
            raise IndexError("Soil depth must have a size equal to NLAYERS and should not include NODATA (-9999)!")
        # Calculate soil thickness of each layer
        for l in range(self.SOILLAYERS):
            if l == 0:
                self.SOILTHICK.append(self.SOILDEPTH[l])
            else:
                self.SOILTHICK.append(self.SOILDEPTH[l] - self.SOILDEPTH[l - 1])
        if self.SOL_ZMX == DEFAULT_NODATA or self.SOL_ZMX > self.SOILDEPTH[-1]:
            self.SOL_ZMX = self.SOILDEPTH[-1]
        if self.ANION_EXCL == DEFAULT_NODATA:
            self.ANION_EXCL = 0.5
        if not self.OM or len(self.OM) != self.SOILLAYERS:
            raise IndexError("Soil organic matter must have a size equal to NLAYERS!")
        elif DEFAULT_NODATA in self.OM and self.OM.index(DEFAULT_NODATA) >= 2 and self.SOILLAYERS >= 3:
            for i in range(2, self.SOILLAYERS):
                if self.OM[i] == DEFAULT_NODATA:
                    self.OM[i] = self.OM[i - 1] * numpy.exp(-self.SOILTHICK[i])  # mm
        # Calculate sol_cbn = sol_om * 0.58
        if not self.SOL_CBN or len(self.SOL_CBN) != self.SOILLAYERS:
            self.SOL_CBN = []
            for i in range(self.SOILLAYERS):
                if self.OM[i] * 0.58 < UTIL_ZERO:
                    self.SOL_CBN.append(0.1)
                else:
                    self.SOL_CBN.append(self.OM[i] * 0.58)
        # Calculate sol_n = sol_cbn/11.
        if not self.SOL_N or len(self.SOL_N) != self.SOILLAYERS:
            self.SOL_N = []
            for i in range(self.SOILLAYERS):
                self.SOL_N.append(self.SOL_CBN[i] / 11.)
        if not self.CLAY or len(self.CLAY) != self.SOILLAYERS or DEFAULT_NODATA in self.CLAY:
            raise IndexError("Clay content must have a size equal to NLAYERS and should not include NODATA (-9999)!")
        if not self.SILT or len(self.SILT) != self.SOILLAYERS or DEFAULT_NODATA in self.SILT:
            raise IndexError("Silt content must have a size equal to NLAYERS and should not include NODATA (-9999)!")
        if not self.SAND or len(self.SAND) != self.SOILLAYERS or DEFAULT_NODATA in self.SAND:
            raise IndexError("Sand content must have a size equal to NLAYERS and should not include NODATA (-9999)!")
        if not self.ROCK or len(self.ROCK) != self.SOILLAYERS or DEFAULT_NODATA in self.ROCK:
            raise IndexError("Rock content must have a size equal to NLAYERS and should not include NODATA (-9999)!")

        # temperory variables
        tmp_fc = []
        tmp_sat = []
        tmp_wp = []
        for i in range(self.SOILLAYERS):
            s = self.SAND[i] * 0.01  # % -> decimal
            c = self.CLAY[i] * 0.01
            om = self.OM[i]
            wpt = -0.024 * s + 0.487 * c + 0.006 * om + 0.005 * s * om - 0.013 * c * om + 0.068 * s * c + 0.031
            tmp_wp.append(1.14 * wpt - 0.02)
            fct = -0.251 * s + 0.195 * c + 0.011 * om + 0.006 * s * om - 0.027 * c * om + 0.452 * s * c + 0.299
            fc = fct + 1.283 * fct * fct - 0.374 * fct - 0.015
            s33t = 0.278 * s + 0.034 * c + 0.022 * om - 0.018 * s * om - 0.027 * c * om - 0.584 * s * c + 0.078
            s33 = 1.636 * s33t - 0.107
            sat = fc + s33 - 0.097 * s + 0.043
            tmp_fc.append(fc)
            tmp_sat.append(sat)

        if self.WILTINGPOINT and len(self.WILTINGPOINT) != self.SOILLAYERS:
            raise IndexError(
                "Wilting point must have a size equal to soil layers number!")
        elif not self.WILTINGPOINT:
            self.WILTINGPOINT = tmp_wp[:]
        elif DEFAULT_NODATA in self.WILTINGPOINT:
            for i in range(self.SOILLAYERS):
                if self.WILTINGPOINT[i] == DEFAULT_NODATA:
                    self.WILTINGPOINT[i] = tmp_wp[i]
        if self.DENSITY and len(self.DENSITY) != self.SOILLAYERS:
            raise IndexError(
                "Bulk density must have a size equal to soil layers number!")
        elif not self.DENSITY or DEFAULT_NODATA in self.DENSITY:
            tmp_bd = []
            for i in range(self.SOILLAYERS):
                sat = tmp_sat[i]
                fc = tmp_fc[i]
                if self.FIELDCAP and len(self.FIELDCAP) == self.SOILLAYERS:
                    sat = sat - fc + self.FIELDCAP[i]
                tmp_bd.append(2.65 * (1.0 - sat))
            if DEFAULT_NODATA in self.DENSITY:
                for i in range(self.SOILLAYERS):
                    if self.DENSITY[i] == DEFAULT_NODATA:
                        self.DENSITY[i] = tmp_bd[i]
            elif not self.DENSITY:
                self.DENSITY = tmp_bd[:]
        if self.FIELDCAP and len(self.FIELDCAP) != self.SOILLAYERS:
            raise IndexError(
                "Field capacity must have a size equal to soil layers number!")
        elif not self.FIELDCAP or DEFAULT_NODATA in self.FIELDCAP:
            tmp_fc_bdeffect = []
            for i in range(self.SOILLAYERS):
                fc = tmp_fc[i]
                sat = tmp_sat[i]
                if self.DENSITY != [] and len(self.DENSITY) == self.SOILLAYERS:
                    p_df = self.DENSITY[i]
                else:
                    p_df = 2.65 * (1.0 - sat)
                sat_df = 1. - p_df / 2.65
                tmp_fc_bdeffect.append(fc - 0.2 * (sat - sat_df))
            if DEFAULT_NODATA in self.FIELDCAP:
                for i in range(self.SOILLAYERS):
                    if self.FIELDCAP[i] == DEFAULT_NODATA:
                        self.FIELDCAP[i] = tmp_fc_bdeffect[i]
            elif not self.FIELDCAP:
                self.FIELDCAP = tmp_fc_bdeffect[:]
        if self.AWC and len(self.AWC) != self.SOILLAYERS:
            raise IndexError("Available water capacity must have the size equal to soil layers number!")
        elif not self.AWC:
            for i in range(self.SOILLAYERS):
                self.AWC.append(self.FIELDCAP[i] - self.WILTINGPOINT[i])
        elif DEFAULT_NODATA in self.AWC:
            for i in range(self.SOILLAYERS):
                if self.AWC[i] == DEFAULT_NODATA:
                    self.AWC[i] = self.FIELDCAP[i] - self.WILTINGPOINT[i]

        if self.POREINDEX and len(self.POREINDEX) != self.SOILLAYERS:
            raise IndexError("Pore disconnectedness index must have a size equal to soil layers number!")
        elif not self.POREINDEX:
            for i in range(self.SOILLAYERS):
                # An fitted equation proposed by Cosby et al. (1984) is adopted. By LJ, 2016-9-22
                b = 0.159 * self.CLAY[i] + 2.91
                self.POREINDEX.append(b)
                # previous version, currently deprecated by LJ
                # fc = self.FIELDCAP[i]
                # wp = self.WILTINGPOINT[i]
                # b = (math.log(1500.) - math.log(33.)) / (math.log(fc) - math.log(wp))
                # self.POREINDEX.append(1.0 / b)
        if self.POROSITY and len(self.POROSITY) != self.SOILLAYERS:
            raise IndexError(
                "Soil Porosity must have a size equal to soil layers number!")
        elif not self.POROSITY:
            for i in range(self.SOILLAYERS):
                self.POROSITY.append(1. - self.DENSITY[i] / 2.65)  # from the theory of swat
        elif DEFAULT_NODATA in self.POROSITY:
            for i in range(self.SOILLAYERS):
                if self.POROSITY[i] == DEFAULT_NODATA:
                    self.POROSITY[i] = 1. - self.DENSITY[i] / 2.65
        if self.CONDUCTIVITY and len(self.CONDUCTIVITY) != self.SOILLAYERS:
            raise IndexError("Saturated conductivity must have a size equal to soil layers number!")
        elif not self.CONDUCTIVITY or DEFAULT_NODATA in self.CONDUCTIVITY:
            tmp_k = []
            for i in range(self.SOILLAYERS):
                lamda = self.POREINDEX[i]
                fc = tmp_fc[i]
                sat = tmp_sat[i]
                tmp_k.append(1930. * pow(sat - fc, 3. - lamda))
            if not self.CONDUCTIVITY:
                self.CONDUCTIVITY = tmp_k[:]
            elif DEFAULT_NODATA in self.CONDUCTIVITY:
                for i in range(self.SOILLAYERS):
                    if self.CONDUCTIVITY[i] == DEFAULT_NODATA:
                        self.CONDUCTIVITY[i] = tmp_k[i]
        # calculate water content of soil at -1.5 MPa and -0.033 MPa, refers to soil_phys.f in SWAT
        if not self.WILTINGPOINT:
            for i in range(self.SOILLAYERS):
                tmpwp = 0.4 * self.CLAY[i] * self.DENSITY[i] / 100.
                if tmpwp <= 0.:
                    tmpwp = 0.005
                self.WILTINGPOINT.append(tmpwp)
        # calculate field capcity (sol_up)
        if not self.FIELDCAP:
            for i in range(self.SOILLAYERS):
                self.FIELDCAP.append(self.WILTINGPOINT[i] + self.AWC[i])
        # calculate porosity
        if not self.POROSITY:
            for i in range(self.SOILLAYERS):
                self.POROSITY.append(1. - self.DENSITY[i] / 2.65)
        if self.SOL_CRK == DEFAULT_NODATA:
            self.SOL_CRK = numpy.mean(self.POROSITY)
        for i in range(self.SOILLAYERS):
            if self.FIELDCAP[i] >= self.POROSITY[i]:
                self.FIELDCAP[i] = self.POROSITY[i] - 0.05
                self.WILTINGPOINT[i] = self.FIELDCAP[i] - self.AWC[i]
                if self.WILTINGPOINT[i] <= 0.:
                    self.FIELDCAP[i] = self.POROSITY[i] * 0.75
                    self.WILTINGPOINT[i] = self.POROSITY[i] * 0.25
            # compute drainable porosity and variable water table factor
            drpor = self.POROSITY[i] - self.FIELDCAP[i]
            self.VWT.append((437.13 * drpor * drpor) - (95.08 * drpor) + 8.257)
        sa = self.SAND[0] / 100.
        cl = self.CLAY[0] / 100.
        si = self.SILT[0] / 100.
        # determine detached sediment size distribution typical for mid-western soils in USA (Foster et al., 1980)
        # Based on SWRRB.
        self.DET_SAND = 2.49 * sa * (1. - cl)
        self.DET_SILT = 0.13 * si
        self.DET_CLAY = 0.2 * cl
        if cl < 0.25:
            self.DET_SMAGG = 2.0 * cl
        elif cl > 0.5:
            self.DET_SMAGG = 0.57
        else:
            self.DET_SMAGG = 0.28 * (cl - 0.25) + 0.5
        self.DET_LGAGG = 1. - self.DET_SAND - self.DET_SILT - self.DET_CLAY - self.DET_SMAGG
        # Error check, may happen for soils with more sand. The fraction should not added up to 1.0
        if self.DET_LGAGG < 0.:
            self.DET_SAND /= (1. - self.DET_LGAGG)
            self.DET_SILT /= (1. - self.DET_LGAGG)
            self.DET_CLAY /= (1. - self.DET_LGAGG)
            self.DET_SMAGG /= (1. - self.DET_LGAGG)
            self.DET_LGAGG = 0.
        # initialize water/drainage coefs for each soil layer
        sumpor = 0.
        self.SOL_SUMUL = 0.
        self.SOL_SUMAWC = 0.
        self.SOL_SUMWP = 0.
        for i in range(self.SOILLAYERS):
            pormm = self.POROSITY[i] * self.SOILTHICK[i]
            sumpor += pormm
            self.SOL_UL.append((self.POROSITY[i] - self.WILTINGPOINT[i]) * self.SOILTHICK[i])
            self.SOL_SUMUL += self.SOL_UL[i]
            self.SOL_AWC.append((self.FIELDCAP[i] - self.WILTINGPOINT[i]) * self.SOILTHICK[i])
            self.SOL_SUMAWC += self.SOL_AWC[i]
            self.SOL_HK.append((self.SOL_UL[i] - self.SOL_AWC[i]) / self.CONDUCTIVITY[i])
            if self.SOL_HK[i] < 1.:
                self.SOL_HK[i] = 1.
            self.SOL_WPMM.append(self.WILTINGPOINT[i] * self.SOILTHICK[i])
            self.SOL_SUMWP += self.SOL_WPMM[i]
            self.CRDEP.append(
                self.SOL_CRK * 0.916 * math.exp(-0.0012 * self.SOILDEPTH[i]) * self.SOILTHICK[i])

        self.SOL_AVPOR = sumpor / self.SOILDEPTH[self.SOILLAYERS - 1]
        self.SOL_AVBD = 2.65 * (1. - self.SOL_AVPOR)
        # calculate infiltration parameters for sub-daily time step
        self.WFSH = 10. * math.exp(6.5309 - 7.32561 * self.POROSITY[0] + 3.809479 * math.pow(self.POROSITY[0], 2) +
                                   0.001583 * math.pow(self.CLAY[0], 2) + 0.000344 * self.SAND[0] *
                                   self.CLAY[0] - 0.049837 * self.POROSITY[0] * self.SAND[0] + 0.001608 *
                                   math.pow(self.POROSITY[0], 2) * math.pow(self.SAND[0], 2) + 0.001602 *
                                   math.pow(self.POROSITY[0], 2) * math.pow(self.CLAY[0], 2) - 0.0000136 *
                                   math.pow(self.SAND[0], 2) * self.CLAY[0] - 0.003479 * math.pow(self.CLAY[0], 2) *
                                   self.POROSITY[0] - 0.000799 * math.pow(self.SAND[0], 2) * self.POROSITY[0])
        if self.SOL_ALB == DEFAULT_NODATA:
            self.SOL_ALB = 0.2227 * math.exp(-1.8672 * self.SOL_CBN[0])
        if self.ESCO == DEFAULT_NODATA:
            self.ESCO = 0.95
        if self.USLE_K and len(self.USLE_K) != self.SOILLAYERS:
            raise IndexError("USLE K factor must have a size equal to NLAYERS!")
        elif not self.USLE_K or DEFAULT_NODATA in self.USLE_K:
            tmp_usle_k = []
            for i in range(self.SOILLAYERS):  # According to Liu Baoyuan et al., (1999)
                sand = self.SAND[i]
                silt = self.SILT[i]
                clay = self.CLAY[i]
                cbn = self.OM[i] * 0.58
                sn = 1. - sand * 0.01
                a = (0.2 + 0.3 * math.exp(-0.0256 * sand * (1. - silt * 0.01)))
                b = math.pow(silt / (clay + silt), 0.3)
                c = (1. - 0.25 * cbn / (cbn + math.exp(3.72 - 2.95 * cbn)))
                d = (1. - 0.25 * sn / (sn + math.exp(-5.51 + 22.9 * sn)))
                k = a * b * c * d
                tmp_usle_k.append(k)
            if not self.USLE_K:
                self.USLE_K = tmp_usle_k[:]
            elif DEFAULT_NODATA in self.USLE_K:
                for i in range(self.SOILLAYERS):
                    if self.USLE_K[i] == DEFAULT_NODATA:
                        self.USLE_K[i] = tmp_usle_k[i]
        if self.SOIL_TEXTURE == DEFAULT_NODATA or self.HYDRO_GROUP == DEFAULT_NODATA:
            st, hg, uslek = SoilUtilClass.getsoiltexture_usda(self.CLAY[0], self.SILT[0], self.SAND[0])
            self.SOIL_TEXTURE = st
            self.HYDRO_GROUP = hg
        # Unit conversion for general soil chemical properties
        wt1 = []
        for j in range(self.SOILLAYERS):
            # g/kg => kg/ha
            wt1.append(self.DENSITY[j] * self.SOILTHICK[j] * 10.)
        if self.SOL_NO3 and len(self.SOL_NO3) == self.SOILLAYERS:
            for j in range(self.SOILLAYERS):
                self.SOL_NO3[j] = self.SOL_NO3[j] * wt1[j]
        if self.SOL_NH4 and len(self.SOL_NH4) == self.SOILLAYERS:
            for j in range(self.SOILLAYERS):
                self.SOL_NH4[j] = self.SOL_NH4[j] * wt1[j]
        if self.SOL_ORGN and len(self.SOL_ORGN) == self.SOILLAYERS:
            for j in range(self.SOILLAYERS):
                self.SOL_ORGN[j] = self.SOL_ORGN[j] * wt1[j]
        if self.SOL_SOLP and len(self.SOL_SOLP) == self.SOILLAYERS:
            for j in range(self.SOILLAYERS):
                self.SOL_SOLP[j] = self.SOL_SOLP[j] * wt1[j]
        if self.SOL_ORGP and len(self.SOL_ORGP) == self.SOILLAYERS:
            for j in range(self.SOILLAYERS):
                self.SOL_ORGP[j] = self.SOL_ORGP[j] * wt1[j]


class SoilUtilClass(object):
    """Soil parameters related utility functions."""

    def __init__(self):
        pass

    @staticmethod
    def getsoiltexture_usda(clay, silt, sand):
        """
            The soil texture code system is from WetSpa Extension and SWAT model which is
        based on the soil texture triangle developed by USDA.
            The unit is percentage, silt + sand + clay [+ Rock] = 100.
            The corresponding default soil parameters (e.g. Ks, porosity) are stored in
        `seims/database/SoilLookup.txt`.
        :param clay: clay content percentage
        :param silt: silt content percentage
        :param sand: sand content percentage
        :return: [Soil texture ID, Hydrological soil group, USLE K factor]
        """
        if clay >= 40 >= silt and sand <= 45:
            return [12, 1, 0.22]  # clay / nian tu
        elif clay >= 40 and silt >= 40:
            return [11, 1, 0.26]  # silt caly / fen nian tu
        elif clay >= 35 and sand >= 45:
            return [10, 1, 0.28]  # sandy clay / sha nian tu
        elif clay >= 25 and 20 <= sand <= 45:
            return [9, 2, 0.3]  # clay loam / nian rang tu
        elif clay >= 25 and sand <= 20:
            return [8, 2, 0.32]  # silt clay loam / fen zhi nian rang tu
        elif clay >= 20 and silt <= 30 and sand >= 45:
            return [7, 2, 0.2]  # sandy clay loam / sha zhi nian rang tu
        elif clay >= 10 and 30 <= silt <= 50 and sand <= 50:
            return [6, 3, 0.3]  # loam / rang tu
        elif 50 <= silt <= 80 or clay >= 15 and silt >= 80:
            return [4, 3, 0.38]  # silt loam / fen zhi rang tu
        elif silt >= 80 and clay <= 15:
            return [5, 3, 0.34]  # silt / fen tu
        elif clay <= 10 and sand <= 50 or 50 <= sand <= 80:
            return [3, 4, 0.13]  # sandy loam / sha zhi rang tu
        elif sand <= 90:
            return [2, 4, 0.04]  # loamy sand / rang zhi sha tu
        else:
            return [1, 4, 0.02]  # sand / sha tu

    @staticmethod
    def lookup_soil_parameters(dstdir, soiltype_file, soil_lookup_file):
        """Reclassify soil parameters by lookup table."""
        #  Read soil properties from txt file
        soil_lookup_data = ReadDataItemsFromTxt(soil_lookup_file)
        soil_instances = []
        soil_prop_flds = soil_lookup_data[0][:]
        for i in range(1, len(soil_lookup_data)):
            cur_soil_data_item = soil_lookup_data[i][:]
            cur_seqn = cur_soil_data_item[0]
            cur_sname = cur_soil_data_item[1]
            cur_soil_ins = SoilProperty(cur_seqn, cur_sname)
            for j in range(2, len(soil_prop_flds)):
                cur_flds = StringClass.splitstring(cur_soil_data_item[j], ',')  # Get field values
                for k in range(len(cur_flds)):
                    cur_flds[k] = float(cur_flds[k])  # Convert to float
                if StringClass.stringmatch(soil_prop_flds[j], SOL_NLYRS):
                    cur_soil_ins.SOILLAYERS = int(cur_flds[0])
                elif StringClass.stringmatch(soil_prop_flds[j], SOL_Z):
                    cur_soil_ins.SOILDEPTH = cur_flds
                elif StringClass.stringmatch(soil_prop_flds[j], SOL_OM):
                    cur_soil_ins.OM = cur_flds
                elif StringClass.stringmatch(soil_prop_flds[j], SOL_CLAY):
                    cur_soil_ins.CLAY = cur_flds
                elif StringClass.stringmatch(soil_prop_flds[j], SOL_SILT):
                    cur_soil_ins.SILT = cur_flds
                elif StringClass.stringmatch(soil_prop_flds[j], SOL_SAND):
                    cur_soil_ins.SAND = cur_flds
                elif StringClass.stringmatch(soil_prop_flds[j], SOL_ROCK):
                    cur_soil_ins.ROCK = cur_flds
                elif StringClass.stringmatch(soil_prop_flds[j], SOL_ZMX):
                    cur_soil_ins.SOL_ZMX = cur_flds[0]
                elif StringClass.stringmatch(soil_prop_flds[j], SOL_ANIONEXCL):
                    cur_soil_ins.ANION_EXCL = cur_flds[0]
                elif StringClass.stringmatch(soil_prop_flds[j], SOL_CRK):
                    cur_soil_ins.SOL_CRK = cur_flds[0]
                elif StringClass.stringmatch(soil_prop_flds[j], SOL_BD):
                    cur_soil_ins.DENSITY = cur_flds
                elif StringClass.stringmatch(soil_prop_flds[j], SOL_K):
                    cur_soil_ins.CONDUCTIVITY = cur_flds
                elif StringClass.stringmatch(soil_prop_flds[j], SOL_WP):
                    cur_soil_ins.WILTINGPOINT = cur_flds
                elif StringClass.stringmatch(soil_prop_flds[j], SOL_FC):
                    cur_soil_ins.FIELDCAP = cur_flds
                elif StringClass.stringmatch(soil_prop_flds[j], SOL_AWC):
                    cur_soil_ins.AWC = cur_flds
                elif StringClass.stringmatch(soil_prop_flds[j], SOL_POROSITY):
                    cur_soil_ins.POROSITY = cur_flds
                elif StringClass.stringmatch(soil_prop_flds[j], SOL_USLE_K):
                    cur_soil_ins.USLE_K = cur_flds
                elif StringClass.stringmatch(soil_prop_flds[j], SOL_ALB):
                    cur_soil_ins.SOL_ALB = cur_flds
                elif StringClass.stringmatch(soil_prop_flds[j], ESCO):
                    cur_soil_ins.ESCO = cur_flds[0]
                elif StringClass.stringmatch(soil_prop_flds[j], SOL_NO3):
                    cur_soil_ins.SOL_NO3 = cur_flds
                elif StringClass.stringmatch(soil_prop_flds[j], SOL_NH4):
                    cur_soil_ins.SOL_NH4 = cur_flds
                elif StringClass.stringmatch(soil_prop_flds[j], SOL_ORGN):
                    cur_soil_ins.SOL_ORGN = cur_flds
                elif StringClass.stringmatch(soil_prop_flds[j], SOL_SOLP):
                    cur_soil_ins.SOL_SOLP = cur_flds
                elif StringClass.stringmatch(soil_prop_flds[j], SOL_ORGP):
                    cur_soil_ins.SOL_ORGP = cur_flds
            cur_soil_ins.CheckData()
            soil_instances.append(cur_soil_ins)
        soil_prop_dict = {}
        for sol in soil_instances:
            cur_sol_dict = sol.SoilDict()
            for fld in cur_sol_dict:
                if fld in soil_prop_dict:
                    soil_prop_dict[fld].append(cur_sol_dict[fld])
                else:
                    soil_prop_dict[fld] = [cur_sol_dict[fld]]
        # print soilPropDict.keys()
        # print soilPropDict.values()

        replace_dicts = []
        dst_soil_tifs = []
        seqns = soil_prop_dict[SOL_SEQN]
        max_lyr_num = int(numpy.max(soil_prop_dict[SOL_NLYRS]))
        for key in soil_prop_dict:
            if key != SOL_SEQN and key != SOL_NAME:
                key_l = 1
                for key_v in soil_prop_dict[key]:
                    if type(key_v) is types.ListType:
                        if len(key_v) > key_l:
                            key_l = len(key_v)
                if key_l == 1:
                    cur_dict = {}
                    for i in range(len(seqns)):
                        cur_dict[float(seqns[i])] = soil_prop_dict[key][i]
                    replace_dicts.append(cur_dict)
                    dst_soil_tifs.append(dstdir + os.sep + key + '.tif')
                else:
                    for i in range(max_lyr_num):
                        cur_dict = {}
                        for j in range(len(seqns)):
                            if i < soil_prop_dict[SOL_NLYRS][j]:
                                cur_dict[float(seqns[j])] = soil_prop_dict[key][j][i]
                            else:
                                cur_dict[float(seqns[j])] = DEFAULT_NODATA
                        replace_dicts.append(cur_dict)
                        dst_soil_tifs.append(dstdir + os.sep + key + '_' + str(i + 1) + '.tif')
        # print replaceDicts
        # print(len(replaceDicts))
        # print dstSoilTifs
        # print(len(dstSoilTifs))

        # Generate GTIFF
        for i in range(len(dst_soil_tifs)):
            print (dst_soil_tifs[i])
            RasterUtilClass.RasterReclassify(soiltype_file, replace_dicts[i], dst_soil_tifs[i])

    @staticmethod
    def initialsoilmoisture(acc_file, slope_file, out_file):
        """Initialize soil moisture fraction of field capacity, based on TWI"""
        acc_R = RasterUtilClass.ReadRaster(acc_file)
        dataAcc = acc_R.data
        xsize = acc_R.nCols
        ysize = acc_R.nRows
        noDataValue = acc_R.noDataValue
        srs = acc_R.srs
        geotrans = acc_R.geotrans
        dx = acc_R.dx
        dataSlope = RasterUtilClass.ReadRaster(slope_file).data
        cellArea = dx * dx

        # TWI, ln(acc_file/tan(slp))
        def wiGridCal(accvalue, slpvalue):
            if abs(accvalue - noDataValue) < UTIL_ZERO:
                return DEFAULT_NODATA
            else:
                if abs(slpvalue) < MINI_SLOPE:
                    slpvalue = MINI_SLOPE
                return math.log((accvalue + 1.) * cellArea / slpvalue)

        wiGridCal_numpy = numpy.frompyfunc(wiGridCal, 2, 1)
        wiGrid = wiGridCal_numpy(dataAcc, dataSlope)
        # wiGrid_valid = numpy.where(acc_R.validZone, wiGrid, numpy.nan)
        # wiMax = numpy.nanmax(wiGrid_valid)
        # wiMin = numpy.nanmin(wiGrid_valid)
        # WARNING: numpy.nanmax and numpy.nanmin are un-stable in Linux, so
        # replaced by the for loops. By LJ
        wiMax = -numpy.inf
        wiMin = numpy.inf
        for i in range(0, ysize):
            for j in range(0, xsize):
                if wiMax < wiGrid[i][j]:
                    wiMax = wiGrid[i][j]
                if DEFAULT_NODATA != wiGrid[i][j] < wiMin:
                    wiMin = wiGrid[i][j]
        # print "TWIMax:%f, TWIMin:%f" % (wiMax, wiMin)
        soilMoisFrMin = 0.6  # minimum relative saturation
        soilMoisFrMax = 1.0

        wiUplimit = wiMax
        a = (soilMoisFrMax - soilMoisFrMin) / (wiUplimit - wiMin)
        b = soilMoisFrMin - a * wiMin

        def moistureCal(acc, wigrid):
            if abs(acc - noDataValue) < UTIL_ZERO:
                return DEFAULT_NODATA
            else:
                tmp = a * wigrid + b
                if tmp > soilMoisFrMax:
                    return soilMoisFrMax
                elif tmp < soilMoisFrMin:
                    return soilMoisFrMin
                else:
                    return tmp

        moistureCal_numpy = numpy.frompyfunc(moistureCal, 2, 1)
        moisture = moistureCal_numpy(dataAcc, wiGrid)
        RasterUtilClass.WriteGTiffFile(out_file, ysize, xsize, moisture, geotrans, srs, DEFAULT_NODATA, GDT_Float32)


def soil_parameters_extraction():
    """Soil spatial parameters extraction."""
    geodata2dbdir = WORKING_DIR + os.sep + DIR_NAME_GEODATA2DB
    status_file = WORKING_DIR + os.sep + DIR_NAME_LOG + os.sep + FN_STATUS_EXTRACTSOILPARAM
    f = open(status_file, 'w')

    # 1. Calculate soil physical and chemical parameters
    soiltype_file = geodata2dbdir + os.sep + soiltypeMFile
    status_output("Calculating initial soil physical and chemical parameters...", 30, f)
    SoilUtilClass.lookup_soil_parameters(geodata2dbdir, soiltype_file, soilSEQNText)

    # other soil related spatial parameters
    # 3. Initial soil moisture
    status_output("Calculating initial soil moisture...", 40, f)
    geodata2dbdir = WORKING_DIR + os.sep + DIR_NAME_GEODATA2DB
    acc_file = geodata2dbdir + os.sep + accM
    slope_file = geodata2dbdir + os.sep + slopeM
    out_filename = geodata2dbdir + os.sep + initSoilMoist
    SoilUtilClass.initialsoilmoisture(acc_file, slope_file, out_filename)

    status_output("Soil related spatial parameters extracted done!", 100, f)
    f.close()


if __name__ == "__main__":
    LoadConfiguration(getconfigfile())
    soil_parameters_extraction()
