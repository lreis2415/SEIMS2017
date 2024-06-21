/*!
 * \file IUH_OL.h
 * \brief IUH overland method to calculate overland flow routing
 *
 * Changelog:
 *   - 1. 2011-01-24 - wh - Initial implementation.
 *   - 2. 2011-02-22 - zq -
 *        -# Add parameter CellWidth.
 *        -# Delete parameter uhminCell and uhmaxCell because the parameter Ol_iuh
 *               contains these information. The first and second column of Ol_iuh is
 *               min time and max time.
 *          -# The number of subbasins (m_nsub) should get from m_subbasin rather than
 *               from main program. So does variable m_nCells.
 *          -# Add variable m_iuhCols to store the number of columns of Ol_iuh. In the
 *               meantime, add one parameter nCols to function SetIUHCell.
 *          -# Add variable m_cellFlow to store the flow of each cell in each day between
 *               min time and max time. Its number of columns equals to the maximum of second
 *               column of Ol_iuh add 1.
 *          -# Add function initial to initialize some variables.
 *          -# Modify function Execute.
 *   - 3. 2016-07-29 - lj -
 *        -# Unify the code style.
 *        -# Replace VAR_SUBBASIN with VAR_SUBBASIN_PARAM, which is common used by several modules.
 *   - 4. 2018-03-20 - lj - The length of subbasin related array should equal to
 *                            the count of subbasins, for both mpi version and omp version.
 *   - 5. 2022-08-22 - lj - Change float to FLTPT.
 *
 * \author Wu hui, Zhiqiang Yu, Liangjun Zhu
 */
#ifndef SEIMS_MODULE_IUH_OL_H
#define SEIMS_MODULE_IUH_OL_H

#include "SimulationModule.h"

/** \defgroup IUH_OL
 * \ingroup Hydrology
 * \brief IUH overland method to calculate overland flow routing
 *
 */

/*!
 * \class IUH_OL
 * \ingroup IUH_OL
 * \brief IUH overland method to calculate overland flow routing
 *
 */
class IUH_OL: public SimulationModule {
public:
    IUH_OL();

    ~IUH_OL();

    void SetValue(const char* key, FLTPT value) OVERRIDE;

    void SetValue(const char* key, int value) OVERRIDE;

    void Set1DData(const char* key, int n, FLTPT* data) OVERRIDE;

    void Set1DData(const char* key, int n, int* data) OVERRIDE;

    void Set2DData(const char* key, int nrows, int ncols, FLTPT** data) OVERRIDE;

    bool CheckInputData() OVERRIDE;

    void InitialOutputs() OVERRIDE;

    void GetValue(const char* key, FLTPT* value) OVERRIDE;

    void Get1DData(const char* key, int* n, FLTPT** data) OVERRIDE;

    int Execute() OVERRIDE;

private:
    /// time step (sec)
    int m_TimeStep;
    /// validate cells number
    int m_nCells;
    /// cell width of the grid (m)
    FLTPT m_CellWth;
    /// cell area, BE CAUTION, the unit is m^2, NOT ha!!!
    FLTPT m_cellArea;
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
    /// surface runoff from depression module
    FLTPT* m_surfRf;

    //temporary

    /// store the flow of each cell in each day between min time and max time
    FLTPT** m_cellFlow;
    /// the maximum of second column of OL_IUH plus 1.
    int m_cellFlowCols;

    //output

    /// overland flow to streams for each subbasin (m3/s)
    FLTPT* m_Q_SBOF;
    // overland flow in each cell (mm) //added by Gao, as intermediate variable, 29 Jul 2016
    FLTPT* m_OL_Flow;
};
#endif /* SEIMS_MODULE_IUH_OL_H */
