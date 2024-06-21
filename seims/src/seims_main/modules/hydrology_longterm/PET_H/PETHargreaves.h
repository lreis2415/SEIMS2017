/*!
 * \file PETHargreaves.h
 * \brief Potential evapotranspiration using Hargreaves method.
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
#ifndef SEIMS_MODULE_PET_H_H
#define SEIMS_MODULE_PET_H_H

#include "SimulationModule.h"

/** \defgroup PET_H
 * \ingroup Hydrology
 * \brief Calculate potential evapotranspiration using Hargreaves method
 */

/*!
 * \class PETHargreaves
 * \ingroup PET_H
 *
 * \brief Hargreaves method to Compute PET
 *
 */
class PETHargreaves: public SimulationModule {
public:
    PETHargreaves();

    ~PETHargreaves();

    void SetValue(const char* key, FLTPT value) OVERRIDE;

    void Set1DData(const char* key, int n, FLTPT* value) OVERRIDE;

    bool CheckInputData() OVERRIDE;

    void InitialOutputs() OVERRIDE;

    int Execute() OVERRIDE;

    void Get1DData(const char* key, int* n, FLTPT** data) OVERRIDE;

private:
    // Parameters from Database
    int m_nCells; ///< valid units number
    FLTPT m_HCoef_pet; ///< coefficient related to radiation used in Hargreaves method
    FLTPT m_petFactor; ///< Correction Factor for PET
    FLTPT* m_cellLat; ///< latitude of each valid units
    FLTPT* m_phuAnn; ///< annual PHU of each valid units

    // Inputs from the output of other modules
    FLTPT* m_meanTemp; ///< mean air temperature for a given day (deg C)
    FLTPT* m_maxTemp; ///< maximum air temperature for a given day (deg C)
    FLTPT* m_minTemp; ///< minimum air temperature for a given day (deg C)
    FLTPT* m_rhd; ///< relative humidity (%)

    // Output variables
    FLTPT* m_dayLen; ///< day length (hr)
    FLTPT* m_phuBase; ///< base zero total heat units (used when no land cover is growing)
    FLTPT* m_pet; ///< potential evapotranspiration on the day
    FLTPT* m_vpd; ///< vapor pressure deficit
};
#endif /* SEIMS_MODULE_PET_H_H */
