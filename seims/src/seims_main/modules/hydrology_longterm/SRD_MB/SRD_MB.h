/*!
 * \brief Snow redistribution calculated by the original WetSpa algorithm.
 * \author Chunping Ou
 * \date May 2011
 * \revised LiangJun Zhu
 * \date 2016-5-29
 */
#ifndef SEIMS_SRD_MB_H
#define SEIMS_SRD_MB_H

#include "SimulationModule.h"

// using namespace std;  // Avoid this statement! by lj.

/*!
 * \defgroup SRD_MB
 * \ingroup Hydrology
 * \brief Soil redistribution calculation
 */
/*!
 * \class SRD_MB
 * \ingroup SRD_MB
 *
 * \brief Soil redistribution calculation
 *
 */
class SRD_MB : public SimulationModule {
public:
    //! Constructor
    SRD_MB(void);

    //! Destructor
    ~SRD_MB(void);

    virtual int Execute(void);

    virtual void SetValue(const char *key, float data);

    virtual void Set1DData(const char *key, int n, float *data);

    virtual void Get1DData(const char *key, int *n, float **data);

    bool CheckInputSize(const char *key, int n);

    bool CheckInputData(void);

private:

    time_t m_Date;
    /// valid cells number
    int m_nCells;

    //single values from parameter database
    float m_ut0;
    float m_u0;
    float m_t_wind;
    float m_shc_crop;
    float m_k_slope;
    float m_k_curvature;
    float m_swe;
    float m_swe0;
    float m_lastSWE;
    float m_kblow;
    float m_t0;
    float m_tsnow;

    bool m_isInitial;

    //time series from module
    float *m_ws;    //wind speed
    int m_wsSize;    //number of wind speed sites

    //grid from parameter database
    float *m_curva_wind;
    float *m_slope_wind;
    float *m_shc;

    //grid from other modules
    float *m_tMin;
    float *m_tMax;
    float *m_tMean;
    float *m_SA;
    float *m_Pnet;

    //result
    float *m_SR;

    //temp array
    float *m_w;
    float *m_wt;

    void InitialOutputs(void);
};
#endif /* SEIMS_SRD_MB_H */
