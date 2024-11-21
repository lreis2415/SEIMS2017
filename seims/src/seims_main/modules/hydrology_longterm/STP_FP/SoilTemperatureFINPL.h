/*!
 * \file SoilTemperatureFINPL.h
 * \brief Finn Plauborg Method to Compute Soil Temperature
 *
 * Changelog:
 *   - 1. 2011-01-05 - jz - Initial implementation.
 *   - 2. 2016-05-27 - lj - Code review and reformat.
 *   - 3. 2022-08-22 - lj - Change float to FLTPT.
 *
 * \author Junzhi Liu, Liangjun Zhu
 */
#ifndef SEIMS_MODULE_STP_FP_H
#define SEIMS_MODULE_STP_FP_H

#include "SimulationModule.h"

/*!
 * \defgroup STP_FP
 * \ingroup Hydrology
 * \brief Finn Plauborg Method to Compute Soil Temperature
 *
 */

/*!
 * \class SoilTemperatureFINPL
 * \ingroup STP_FP
 * \brief Soil temperature
 *
 */
class SoilTemperatureFINPL: public SimulationModule {
public:
    SoilTemperatureFINPL();

    ~SoilTemperatureFINPL();

    void SetValue(const char* key, FLTPT value) OVERRIDE;

    void Set1DData(const char* key, int n, int* data) OVERRIDE;

    void Set1DData(const char* key, int n, FLTPT* data) OVERRIDE;

    bool CheckInputData() OVERRIDE;

    void InitialOutputs() OVERRIDE;

    int Execute() OVERRIDE;

    void Get1DData(const char* key, int* n, FLTPT** data) OVERRIDE;

private:
    FLTPT m_a0;
    FLTPT m_a1;
    FLTPT m_a2;
    FLTPT m_a3;
    FLTPT m_b1;
    FLTPT m_b2;
    FLTPT m_d1;
    FLTPT m_d2;
    /// ratio between soil temperature at 10 cm and the mean
    FLTPT m_kSoil10;

    /// count of cells
    int m_nCells;
    /// factor of soil temperature relative to short grass (degree)
    FLTPT* m_soilTempRelFactor10;
    /// landuse type, for distinguish calculation, such as water body.
    int* m_landUse;
    /// from interpolation module
    /// mean air temperature of the current day
    FLTPT* m_meanTemp;
    ///// mean air temperature of the day(d-1)
    FLTPT* m_meanTempPre1;
    ///// mean air temperature of the day(d-2)
    FLTPT* m_meanTempPre2;

    /// output soil temperature
    FLTPT* m_soilTemp;
};
#endif /* SEIMS_MODULE_STP_FP_H */
