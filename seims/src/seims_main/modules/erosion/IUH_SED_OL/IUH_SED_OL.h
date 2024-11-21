/*!
 * \file IUH_SED_OL.h
 * \brief IUH overland method to calculate overland sediment routing
 *
 * Changelog:
 *   - 1. 2016-08-12 - jz - Initial implementation.
 *   - 2. 2018-03-22 - lj - The length of subbasin related array should equal to the count of
 *                          subbasins, for both mpi version and omp version.
 *   - 3. 2018-03-26 - lj - Solve inconsistent results when using openmp to reducing raster data according to subbasin ID.\n
 *   - 4. 2018-05-14 - lj - Code review and reformat.
 *   - 5. 2022-08-22 - lj - Change float to FLTPT.
 *
 * \author Junzhi Liu, Liangjun Zhu
 */
#ifndef SEIMS_MODULE_IUH_SED_OL_H
#define SEIMS_MODULE_IUH_SED_OL_H

#include "SimulationModule.h"

/** \defgroup IUH_SED_OL
 * \ingroup Hydrology
 * \brief IUH overland method to calculate overland flow routing
 *
 */

/*!
 * \class IUH_SED_OL
 * \ingroup IUH_SED_OL
 * \brief IUH overland method to calculate overland flow routing
 *
 */
class IUH_SED_OL: public SimulationModule {
public:
    IUH_SED_OL();

    ~IUH_SED_OL();

    int Execute() OVERRIDE;

    void SetValue(const char* key, int value) OVERRIDE;

    void Set1DData(const char* key, int n, FLTPT* data) OVERRIDE;

    void Set1DData(const char* key, int n, int* data) OVERRIDE;

    void Set2DData(const char* key, int nrows, int ncols, FLTPT** data) OVERRIDE;

    bool CheckInputData() OVERRIDE;

    void InitialOutputs() OVERRIDE;

    void Get1DData(const char* key, int* n, FLTPT** data) OVERRIDE;

private:
    /// time step (sec)
    int m_TimeStep;
    /// validate cells number
    int m_nCells;
    /// the total number of subbasins
    int m_nSubbsns;
    /// current subbasin ID, 0 for the entire watershed
    int m_inputSubbsnID;
    /// subbasin grid (subbasins ID)
    int* m_subbsnID;

    /// IUH of each grid cell (1/s)
    FLTPT** m_iuhCell;
    /// the number of columns of Ol_iuh
    int m_iuhCols;
    /// sediment yield in each cell
    FLTPT* m_sedYield;

    //temporary

    /// the maximum of second column of OL_IUH plus 1.
    int m_cellFlowCols;
    /// store the sediment of each cell in each day between min time and max time
    FLTPT** m_cellSed;

    //////////////////////////////////////////////////////////////////////
    //output
    /// sediment to streams
    FLTPT* m_sedtoCh;
    /// sediment to channel at each cell at current time step
    FLTPT* m_olWtrEroSed;
};
#endif /* SEIMS_MODULE_IUH_SED_OL_H */
