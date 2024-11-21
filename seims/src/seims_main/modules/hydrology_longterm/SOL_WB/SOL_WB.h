/*!
 * \file SOL_WB.h
 * \brief Check soil water balance.
 *
 * Changelog:
 *   - 1. 2016-07-28 - lj - Move subbasin class to base/data/clsSubbasin, to keep consistent with other modules.
 *   - 2. 2022-08-22 - lj - Change float to FLTPT.
 *
 * \author Chunping Ou, Liangjun Zhu
 */
#ifndef SEIMS_MODULE_SOL_WB_H
#define SEIMS_MODULE_SOL_WB_H

#include "SimulationModule.h"
#include "clsSubbasin.h"

/*!
 * \defgroup SOL_WB
 * \ingroup Hydrology
 * \brief Soil water balance calculation
 *
 */

/*!
 * \class SOL_WB
 * \ingroup SOL_WB
 * \brief Soil water balance calculation
 *
 */

class SOL_WB: public SimulationModule {
public:
    SOL_WB();

    ~SOL_WB();

    void SetValue(const char* key, int value) OVERRIDE;

    void Set1DData(const char* key, int nrows, FLTPT* data) OVERRIDE;

    void Set1DData(const char* key, int nrows, int* data) OVERRIDE;

    void Set2DData(const char* key, int nrows, int ncols, FLTPT** data) OVERRIDE;

    void SetSubbasins(clsSubbasins* subbasins) OVERRIDE;

    bool CheckInputData() OVERRIDE;

    void InitialOutputs() OVERRIDE;

    int Execute() OVERRIDE;

    void Get2DData(const char* key, int* nrows, int* ncols, FLTPT*** data) OVERRIDE;

private:
    /*!
     * \brief Set parameter values to subbasins
     */
    void SetValueToSubbasins();
private:
    //! valid cells number
    int m_nCells;
    //! maximum soil layers number
    int m_maxSoilLyrs;
    //! soil layers number of each cell
    int* m_nSoilLyrs;
    //! soil thickness of each layer
    FLTPT** m_soilThk;
    //! the maximum soil depth
    FLTPT* m_soilMaxRootD;

    //! Net precipitation (include snow melt if stated) (mm)
    FLTPT* m_netPcp;
    //! infiltration water (mm)
    FLTPT* m_infil;
    //! evaporation from the soil water storage, es_day in SWAT (mm)
    FLTPT* m_soilET;
    //! revaporization from groundwater to the last soil layer (mm)
    FLTPT* m_Revap;
    //! subsurface runoff
    FLTPT** m_subSurfRf;
    //! percolation (mm)
    FLTPT** m_soilPerco;
    //! soil storage (mm)
    FLTPT** m_soilWtrSto;
    // Outputs
    // used to output time series result for soil water balance

    //! precipitation on the current day (mm)
    FLTPT* m_PCP;
    //! interception loss (mm)
    FLTPT* m_intcpLoss;
    //! evaporation from the interception storage (mm)
    FLTPT* m_IntcpET;
    //! depression (mm)
    FLTPT* m_deprSto;
    //! evaporation from depression storage (mm)
    FLTPT* m_deprStoET;
    //! surface runoff generated (mm)
    FLTPT* m_surfRf;
    //! groundwater runoff
    FLTPT* m_RG;
    //! snow sublimation
    FLTPT* m_snowSublim;
    //! mean temperature
    FLTPT* m_meanTemp;
    //! soil temperature
    FLTPT* m_soilTemp;
    //! subbasins number
    int m_nSubbsns;
    //! subbasin IDs
    vector<int> m_subbasinIDs;
    //! All subbasins information
    clsSubbasins* m_subbasinsInfo;
    /* soil water balance, time series result
    * the row index is subbasinID
    */
    FLTPT** m_soilWtrBal;
};
#endif /* SEIMS_MODULE_SOL_WB_H */
