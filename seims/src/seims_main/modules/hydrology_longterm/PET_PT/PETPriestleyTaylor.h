/*!
 * \file PETPriestleyTaylor.h
 * \brief Potential evapotranspiration using PriestleyTaylor method
 *
 * Changelog:
 *   - 1. 2010-11-30 - jz - Initial implementation.
 *   - 2. 2016-05-30 - lj -
 *        -# Add m_tMean from database, which may be measurement value or the mean of tMax and tMin
 *        -# The PET calculate is changed from site-based to cell-based, because PET is not only dependent on Climate site data
 *        -# Add m_VPD, m_dayLen as outputs, which will be used in PG_EPIC module
 *        -# Add m_phuBase as outputs, which will be used in MGT_SWAT module
 *   - 3. 2022-08-22 - lj - Change float to FLTPT.
 *
 * \author Junzhi Liu, Liangjun Zhu
 */
#ifndef SEIMS_MODULE_PET_PT_H
#define SEIMS_MODULE_PET_PT_H

#include "SimulationModule.h"

/*!
 * \defgroup PET_PT
 * \ingroup Hydrology
 * \brief Calculate potential evapotranspiration using PriestleyTaylor method
 *
 */
/*!
 * \class PETPriestleyTaylor
 * \ingroup PET_PT
 *
 * \brief Priestley Taylor Method to Compute PET
 *
 */
class PETPriestleyTaylor: public SimulationModule {
public:
    PETPriestleyTaylor();

    ~PETPriestleyTaylor();

    void SetValue(const char* key, FLTPT value) OVERRIDE;

    void Set1DData(const char* key, int n, FLTPT* value) OVERRIDE;

    bool CheckInputData() OVERRIDE;

    void InitialOutputs() OVERRIDE;

    int Execute() OVERRIDE;

    void Get1DData(const char* key, int* n, FLTPT** data) OVERRIDE;

private:
    /// mean air temperature for a given day(degree)
    FLTPT* m_meanTemp;
    /// maximum air temperature for a given day(degree)
    FLTPT* m_maxTemp;
    /// minimum air temperature for a given day(degree)
    FLTPT* m_minTemp;
    /// solar radiation(MJ/m2/d)
    FLTPT* m_sr;
    /// relative humidity(%)
    FLTPT* m_rhd;
    /// elevation(m)
    FLTPT* m_dem;
    /// valid cells number
    int m_nCells;
    /// Correction Factor for PET
    FLTPT m_petFactor;
    ///latitude of the stations
    FLTPT* m_cellLat;
    /// annual PHU
    FLTPT* m_phuAnn;
    ///The temperature of snow melt
    FLTPT m_snowTemp;

    /// output

    /// day length (hr)
    FLTPT* m_dayLen;
    /// base zero total heat units (used when no land cover is growing)
    FLTPT* m_phuBase;
    /// pet
    FLTPT* m_pet;
    /// vapor pressure deficit
    FLTPT* m_vpd;
};
#endif /* SEIMS_MODULE_PET_PT_H */
