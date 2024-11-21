/*!
 * \file SNO_SP.h
 * \brief Snow melt by snowpack daily method from SWAT.
 *
 * Changelog:
 *   - 1. 2011-05-30 - zq - Initial implementation.
 *   - 2. 2016-05-29 - lj -
 *        -# Remove m_isInitial and add initialOutputs(void)
 *        -# Add m_snowCoverMax and m_snowCover50 to adjust for areal extent of snow cover
 *        -# ReWrite the execute code according to snom.f of SWAT
 *        -# In this version, snow melt is added to net precipitation.
 *   - 3. 2022-08-22 - lj - Change float to FLTPT.
 *
 * \author Zhiqiang Yu, Liangjun Zhu
 */
#ifndef SEIMS_MODULE_SNO_SP_H
#define SEIMS_MODULE_SNO_SP_H

#include "SimulationModule.h"

/*!
 * \defgroup SNO_SP
 * \ingroup Hydrology
 * \brief Calculate snow melt by snowpack daily method from SWAT
 *
 */

/*!
 * \class SNO_SP
 * \ingroup SNO_SP
 * \brief Calculate snow melt by snowpack daily method from SWAT
 *
 */
class SNO_SP: public SimulationModule {
public:
    //! Constructor
    SNO_SP();

    //! Destructor
    ~SNO_SP();

    void SetValue(const char* key, FLTPT data) OVERRIDE;

    void Set1DData(const char* key, int n, FLTPT* data) OVERRIDE;

    bool CheckInputData() OVERRIDE;

    void InitialOutputs() OVERRIDE;

    int Execute() OVERRIDE;

    void Get1DData(const char* key, int* n, FLTPT** data) OVERRIDE;

private:
    //! Valid cells number
    int m_nCells;
    //! Mean air temperature at which snow melt will occur, sub_smtmp
    FLTPT m_t0;
    //! fraction coefficient of precipitation as snow
    FLTPT m_kblow;
    //! Snowfall temperature, i.e., precipitation as snow
    FLTPT m_snowTemp;
    //! Initial snow water equivalent
    //FLTPT m_swe0;

    /*Snow pack temperature lag factor (0-1), sub_timp in SWAT
    * 1 = no lag (snow pack temp=current day air temp)
    * as the lag factor goes to zero, the snow pack's
    * temperature will be less influenced by the current day's
    * air temperature
    */
    FLTPT m_lagSnow;
    //! Maximum melt rate for snow during year, sub_smfmx
    FLTPT m_csnow6;
    //! Minimum melt rate for snow during year, sub_smfmn
    FLTPT m_csnow12;
    //! Minimum snow water content that corresponds to 100% snow cover, mm H2O, SNOCOVMX
    FLTPT m_snowCoverMax;
    //! Fraction of SNOCOVMX that corresponds to 50% snow cover, SNO50COV
    FLTPT m_snowCover50;
    //! 1st shape parameter for snow cover equation
    FLTPT m_snowCoverCoef1;
    //! 2nd shape parameter for snow cover equation
    FLTPT m_snowCoverCoef2;

    //! average snow accumulation of the watershed
    //float m_swe;
    //! snow water equivalent of last time step
    //float m_lastSWE;

    //! Mean temperature
    FLTPT* m_meanTemp;
    //! Max temperature
    FLTPT* m_maxTemp;
    //! Net precipitation
    FLTPT* m_netPcp;

    //! snow redistribution
    FLTPT* m_snowAccum;
    //! snow sublimation, snoev in SWAT in etact.f
    FLTPT* m_SE;

    //! temperature of snow pack, snotmp in SWAT
    FLTPT* m_packT;

    /// outputs

    //! amount of water in snow melt, snomlt in SWAT
    FLTPT* m_snowMelt;
    //! snow accumulation, sno_hru in SWAT
    FLTPT* m_SA;
};
#endif /* SEIMS_MODULE_SNO_SP_H */
