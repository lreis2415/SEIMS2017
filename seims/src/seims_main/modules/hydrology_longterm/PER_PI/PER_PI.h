/*!
 * \file PER_PI.h
 * \brief Percolation calculated by Darcy's law and Brooks-Corey equation.
 * \author Junzhi Liu, Liangjun Zhu
 */
#ifndef SEIMS_MODULE_PER_PI_H
#define SEIMS_MODULE_PER_PI_H

#include "SimulationModule.h"

/** \defgroup PER_PI
 * \ingroup Hydrology
 * \brief Calculate the amount of water percolated from the soil water reservoir
 *
 */

/*!
 * \class PER_PI
 * \ingroup PER_PI
 * \brief Calculate water percolated from the soil water reservoir
 *
 */
class PER_PI: public SimulationModule {
public:
    PER_PI();

    ~PER_PI();

    void SetValue(const char* key, float value) OVERRIDE;

    void Set1DData(const char* key, int nRows, float* data) OVERRIDE;

    void Set2DData(const char* key, int nrows, int ncols, float** data) OVERRIDE;

    bool CheckInputData() OVERRIDE;

    void InitialOutputs() OVERRIDE;

    int Execute() OVERRIDE;

    void Get2DData(const char* key, int* nrows, int* ncols, float*** data) OVERRIDE;

private:
    /// maximum number of soil layers
    int m_maxSoilLyrs;
    /// soil layers
    float* m_nSoilLyrs;
    ///// soil depth
    //float **m_soilDepth;
    /// soil thickness
    float** m_soilThk;
    ///// depth of the up soil layer
    //float *m_upSoilDepth;

    /// time step
    int m_dt;
    /// valid cells number
    int m_nCells;
    /// threshold soil freezing temperature
    float m_soilFrozenTemp;
    /// saturated conductivity, mm/h
    float** m_ks;
    ///// soil porosity
    //float **m_porosity;

    /// amount of water held in the soil layer at saturation (sat - wp water), mm
    float** m_soilSat;
    /// amount of water held in the soil layer at field capacity (fc - wp water) mm H2O
    float** m_soilFC;
    /// water content of soil at -1.5 MPa (wilting point) mm H2O
    float** m_soilWP;
    /// pore size distribution index
    float** m_poreIdx;
    /// amount of water stored in soil layers on current day, sol_st in SWAT
    float** m_soilWtrSto;
    /// amount of water stored in soil profile on current day, sol_sw in SWAT
    float* m_soilWtrStoPrfl;
    /// soil temperature
    float* m_soilTemp;
    /// infiltration (mm)
    float* m_infil;
    /// impound/release
    float* m_impndTrig;
    /// Output

    ///percolation (mm)
    float** m_soilPerco;
};
#endif /* SEIMS_MODULE_PER_PI_H */
