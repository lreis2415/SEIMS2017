/*!
 * \file PER_STR.h
 * \brief Percolation calculated by storage routing method
 * reference SWAT theory manual, p151-152
 *
 * Changelog:
 *   - 1. 2011-05-30 - jz - Initial implementation.
 *   - 2. 2016-09-08 - lj - ReWrite according to percmain.f and sat_excess.f of SWAT
 *   - 3. 2022-08-22 - lj - Change float to FLTPT.
 *
 * \author Liangjun Zhu, Junzhi Liu
 */
#ifndef SEIMS_MODULE_PER_STR_H
#define SEIMS_MODULE_PER_STR_H

#include "SimulationModule.h"

/*!
 * \defgroup PER_STR
 * \ingroup Hydrology
 * \brief Percolation calculated by storage routing method.
 */

/*!
 * \class PER_STR
 * \ingroup PER_STR
 * \brief Percolation calculated by storage routing method.
 *
 */
class PER_STR: public SimulationModule {
public:
    PER_STR();

    ~PER_STR();

    void SetValue(const char* key, FLTPT value) OVERRIDE;

    void SetValue(const char* key, int value) OVERRIDE;

    void Set1DData(const char* key, int nRows, FLTPT* data) OVERRIDE;

    void Set1DData(const char* key, int nRows, int* data) OVERRIDE;

    void Set2DData(const char* key, int nrows, int ncols, FLTPT** data) OVERRIDE;

    bool CheckInputData() OVERRIDE;

    void InitialOutputs() OVERRIDE;

    int Execute() OVERRIDE;

    void Get2DData(const char* key, int* nrows, int* ncols, FLTPT*** data) OVERRIDE;

private:
    /// maximum number of soil layers
    int m_maxSoilLyrs;
    /// soil layers
    int* m_nSoilLyrs;
    /// soil thickness
    FLTPT** m_soilThk;
    /// time step
    int m_dt;
    /// valid cells number
    int m_nCells;
    /// threshold soil freezing temperature
    FLTPT m_soilFrozenTemp;
    /// saturated conductivity
    FLTPT** m_ks;
    /// amount of water held in the soil layer at saturation (sat - wp water), mm
    FLTPT** m_soilSat;
    /// amount of water held in the soil layer at field capacity (fc - wp water) mm H2O
    FLTPT** m_soilFC;
    /// soil moisture, mm H2O
    FLTPT** m_soilWtrSto;
    /// amount of water stored in soil profile on current day, sol_sw in SWAT
    FLTPT* m_soilWtrStoPrfl;
    /// soil temperature
    FLTPT* m_soilTemp;
    /// infiltration, mm
    FLTPT* m_infil;
    /// surface runoff, mm
    FLTPT* m_surfRf;
    /// pothole volume, mm
    FLTPT* m_potVol;
    /// impounding trigger
    int* m_impoundTrig;
    /// Output: percolation
    FLTPT** m_soilPerco;
};
#endif /* SEIMS_MODULE_PER_STR_H */
