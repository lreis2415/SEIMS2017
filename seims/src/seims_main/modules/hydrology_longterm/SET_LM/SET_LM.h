/*!
 * \file SET_LM.h
 * \brief The method of soil actual ET linearly with actual soil moisture developed by
 *          Thornthwaite and Mather (1955), which was also adapted by WetSpa Extension.
 *
 * Changelog:
 *   - 1. 2018-06-26 - lj - Remove Wilting point since SOL_AWC is preprocessed by FC-WP.
 *   - 2. 2022-08-22 - lj - Change float to FLTPT.
 *
 * \author Chunping Ou, Liangjun Zhu
 */
#ifndef SEIMS_MODULE_SET_LM_H
#define SEIMS_MODULE_SET_LM_H

#include "SimulationModule.h"

/*!
 * \defgroup SET_LM
 * \ingroup Hydrology
 * \brief Calculate soil Temperature according to the linearly relationship with actual soil moisture
 */

/*!
 * \class SET_LM
 * \ingroup SET_LM
 * \brief Calculate soil Temperature according to the linearly relationship with actual soil moisture
 */
class SET_LM: public SimulationModule {
public:
    SET_LM();

    ~SET_LM();

    void SetValue(const char* key, FLTPT value) OVERRIDE;

    void Set1DData(const char* key, int nRows, FLTPT* data) OVERRIDE;

    void Set1DData(const char* key, int nRows, int* data) OVERRIDE;

    void Set2DData(const char* key, int nrows, int ncols, FLTPT** data) OVERRIDE;

    bool CheckInputData() OVERRIDE;

    void InitialOutputs() OVERRIDE;

    int Execute() OVERRIDE;

    void Get1DData(const char* key, int* nrows, FLTPT** data) OVERRIDE;

private:
    int m_nCells; ///< valid cells number
    int m_maxSoilLyrs; ///< maximum number of soil layers
    int* m_nSoilLyrs; ///< Soil layers number
    FLTPT** m_soilThk;  ///< Soil thickness of each layer, mm

    FLTPT** m_soilWtrSto; ///< soil moisture, mm
    FLTPT** m_soilFC;     ///< field capacity (FC-WP, same as SWAT), mm
    FLTPT* m_pet;         ///< Potential evapotranspiration
    FLTPT* m_IntcpET;     ///< Evaporation from interception
    FLTPT* m_deprStoET;   ///< Evaporation from depression storage
    FLTPT* m_maxPltET;    ///< Evaporation from plant

    FLTPT* m_soilTemp;      ///< Soil temperature
    FLTPT m_soilFrozenTemp; ///< Freezing temperature

    FLTPT* m_soilET; ///< Output, actual soil evaporation
};
#endif /* SEIMS_MODULE_SET_LM_H */
