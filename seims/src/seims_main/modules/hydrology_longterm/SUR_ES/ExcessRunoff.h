/**
*	@version	1.0
*	@author    Junzhi Liu
*	@date	31-Octobor-2011
*
*	@brief	Calculate infiltration and excess precipitation
*
*/
#ifndef SEIMS_SUR_ES_H
#define SEIMS_SUR_ES_H

#include "SimulationModule.h"

// using namespace std;  // Avoid this statement! by lj.

/*!
 * \defgroup SUR_ES
 * \ingroup Hydrology
 * \brief Calculate infiltration and excess precipitation
 *
 */

/*!
 * \class ExcessRunoff
 * \ingroup SUR_ES
 * \brief Calculate infiltration and excess precipitation
 *
 */
class ExcessRunoff : public SimulationModule {
public:
    ExcessRunoff(void);

    ~ExcessRunoff(void);

    virtual void Set1DData(const char *key, int n, float *data);

    virtual void Get1DData(const char *key, int *n, float **data);

    virtual void SetValue(const char *key, float value);

    virtual int Execute(void);

private:
    /**
    *	@brief check the input data. Make sure all the input data is available.
    *
    *	@return bool The validity of the input data.
    */
    bool CheckInputData(void);

    /**
    *	@brief check the input size. Make sure all the input data have same dimension.
    *
    *	@param key: The key of the input data
    *	@param n: The input data dimension
    *	@return bool The validity of the dimension
    */
    bool CheckInputSize(const char *, int n);

    ///**
    //*	@brief check the output data. Make sure all the output data is available.
    //*
    //*	@param output1: the output variable PE
    //*	@param output2: the next output variable infiltration
    //*	@return bool The validity of the output data.
    //*/
    //bool CheckOutputData(float* output1, float* output2);

    void clearInputs(void);

//    string getDate(time_t *date);

private:

    /// time step(seconds)
    float m_dt;

    /// maximum temperature
    float *m_tMax;
    /// minimum temperature
    float *m_tMin;
    /// snow fall temperature
    float m_tSnow;
    /// snow melt threshold temperature
    float m_t0;
    /// snow melt from the snow melt module  (mm)
    float *m_snowMelt;
    /// snow accumulation from the snow balance module (mm) at t+1 timestep
    float *m_snowAccu;
    /// threshold soil freezing temperature (deg C)
    float m_tFrozen;
    /// frozen soil moisture relative to saturation above which no infiltration occur (m3/m3)
    float m_sFrozen;
    /// soil temperature obtained from the soil temperature module (deg C)
    float *m_soilTemp;

    /// count of valid cells
    int m_nCells;
    /// precipitation of each cell
    float *m_pNet;

    /// soil porosity
    float *m_porosity;
    /// field capacity
    float *m_fieldCap;
    /// soil moisture
    float *m_soilMoisture;
    /// root depth
    float *m_rootDepth;

    /// depression storage -- SD(t-1) from the depression storage module
    float *m_sd;

    /// parameters used in GA method
    /// saturated hydraulic conductivity from parameter database (m/s)
    float *m_ks;
    /// initial soil moisture
    float *m_initSoilMoisture;

    // output
    /// the excess precipitation (mm) of the total nCells
    float *m_pe;
    /// infiltration map of watershed (mm) of the total nCells
    float *m_infil;

};
#endif /* SEIMS_SUR_ES_H */
