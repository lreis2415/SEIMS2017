/*!
 * \file SSR_DA.h
 * \brief Subsurface runoff using Darcy's law and the kinematic approximation
 *        Water is routed cell-to-cell according to D8 flow direction
 *
 * Changelog:
 *   - 1. 2016-7-24 - lj - Add support of multi soil layers of each cells.
 *   - 2. 2017-8-23 - lj - Solve inconsistent results when using openmp to reducing
 *                           raster data according to subbasin ID.
 *   - 3. 2021-04-07 - lj - Support different flow direction algorithms
 *   - 4. 2022-08-22 - lj - Change float to FLTPT.
 *
 * \author Zhiqiang Yu, Junzhi Liu, Liangjun Zhu
 */
#ifndef SEIMS_MODULE_SSR_DA_H
#define SEIMS_MODULE_SSR_DA_H

#include "SimulationModule.h"

/*!
 * \defgroup SSR_DA
 * \ingroup Hydrology
 * \brief Subsurface runoff using Darcy's law and the kinematic approximation
 *
 */

/*!
 * \class SSR_DA
 * \ingroup SSR_DA
 *
 */
class SSR_DA : public SimulationModule {
public:
    SSR_DA();

    ~SSR_DA();

    void SetValue(const char *key, FLTPT value) OVERRIDE;

    void SetValue(const char* key, int value) OVERRIDE;

    void Set1DData(const char *key, int nrows, FLTPT *data) OVERRIDE;

    void Set1DData(const char* key, int nrows, int* data) OVERRIDE;

    void Set2DData(const char *key, int nrows, int ncols, FLTPT **data) OVERRIDE;

    void Set2DData(const char* key, int nrows, int ncols, int** data) OVERRIDE;

    bool CheckInputData() OVERRIDE;

    void InitialOutputs() OVERRIDE;

    int Execute() OVERRIDE;

    void Get1DData(const char *key, int *n, FLTPT **data) OVERRIDE;

    void Get2DData(const char *key, int *nrows, int *ncols, FLTPT ***data) OVERRIDE;

private:
    FLTPT GetFlowInFraction(int id, int up_idx);

    bool FlowInSoil(int id);

    /// current subbasin ID, 0 for the entire watershed
    int m_inputSubbsnID;
    /// valid cell numbers
    int m_nCells;
    /// width of cell (m)
    FLTPT m_CellWth;
    /// max number of soil layers
    int m_maxSoilLyrs;
    /// number of soil layers of each cell
    int *m_nSoilLyrs;
    /// soil thickness
    FLTPT **m_soilThk;

    ///// depth of the up soil layer
    //float m_upSoilDepth;
    ///
    //float *m_rootDepth;

    /// timestep
    int m_dt;
    /// Interflow scale factor
    FLTPT m_ki;
    /// soil freezing temperature threshold, deg C
    FLTPT m_soilFrozenTemp;
    /// slope (tan)
    FLTPT *m_slope;
    /// conductivity
    FLTPT **m_ks;
    ///// porosity (mm/mm)
    //float **m_porosity;

    /// amount of water held in the soil layer at saturation (sat - wp water), mm
    FLTPT **m_soilSat;
    /// pore size distribution index
    FLTPT **m_poreIdx;

    /// amount of water available to plants in soil layer at field capacity (AWC=FC-WP), mm
    FLTPT **m_soilFC;
    /// water content of soil at -1.5 MPa (wilting point) mm H2O
    FLTPT **m_soilWP;
    /// soil water storage (mm)
    FLTPT **m_soilWtrSto;
    /// soil water storage in soil profile (mm)
    FLTPT *m_soilWtrStoPrfl;
    /// soil temperature, deg C
    FLTPT *m_soilTemp;

    /// channel width, m
    FLTPT *m_chWidth;
    /// stream link
    int *m_rchID;

    /*!
     * \brief 2d array recording indexes of flow in cells
     *
     *	The first element in each sub-array is the number of flow in cells in this sub-array
     */
    int **m_flowInIdx;

    /*!
     * \brief Flow fractions of flow in cells to the current cell
     *
     * It has the same data structure as m_flowInIdx.
     */
    FLTPT **m_flowInFrac;

    /*!
     * \brief Routing layers according to the flow direction
     *
     *  There are not flow relationships within each layer.
     *  The first element in each layer is the number of cells in the layer
     */
    int **m_rteLyrs;
    /// number of routing layers
    int m_nRteLyrs;
    /// number of subbasin
    int m_nSubbsns;
    /// subbasin grid (ID of subbasin)
    int *m_subbsnID;

    // outputs

    /// subsurface runoff (mm), VAR_SSRU
    FLTPT **m_subSurfRf;
    /// subsurface runoff volume (m^3), VAR_SSRUVOL
    FLTPT **m_subSurfRfVol;
    /// subsurface to streams from each subbasin, the first element is the whole watershed, m^3/s, VAR_SBIF
    FLTPT *m_ifluQ2Rch;
};

#endif /* SEIMS_MODULE_SSR_DA_H */
