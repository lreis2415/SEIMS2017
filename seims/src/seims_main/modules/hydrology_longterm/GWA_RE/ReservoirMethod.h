/*!
 * \file ReservoirMethod.h
 * \brief Reservoir Method to calculate groundwater balance and baseflow.
 *
 * Changelog:
 *   - 1. 2011-01-24 - wh - Initial implementation.
 *   - 2. 2011-02-18 - zq -
 *        -# Add judgment to calculation of EG (Revap). The average percolation of
 *		       one subbasin is first calculated. If the percolation is less than 0.01,
 *		       EG is set to 0 directly. (in function setInputs of class subbasin)
 *	      -# Add member variable m_isRevapChanged to class subbasin. This variable
 *		       is the flag whether the Revap is changed by current time step. This flag
 *		       can avoid repeating setting values when converting subbasin average Revap
 *		       to cell Revap.(in function Execute of class ReservoirMethod)
 *   - 3. 2011-03-14 - zq - Add codes to process the groundwater which comes from bank storage in
 *		                      channel routing module. The water volume of this part of groundwater is
 *		                      added to the groundwater storage. The input variable "T_GWNEW" is used
 *		                      for this purpose. One additional parameter is added to function setInputs
 *		                      of class subbasin.
 *		                      See equation 8 in memo "Channel water balance" for detailed reference.
 *   - 4. 2016-07-27 - lj - Move subbasin class to base/data module for sharing with other modules.
 *	 - 5. 2018-06-28 - lj - Move SetSubbasinInfos() to dataCenter class.
 *	 - 6. 2018-07-18 - sf - revap should be calculated by cell first.
 *   - 7. 2022-08-22 - lj - Change float to FLTPT.
 *
 * \author Hui Wu, Zhiqiang Yu, Liangjun Zhu, Fang Shen
 */
#ifndef SEIMS_MODULE_GWA_RE_H
#define SEIMS_MODULE_GWA_RE_H

#include "SimulationModule.h"
#include "clsSubbasin.h"

/** \defgroup GWA_RE
 * \ingroup Hydrology
 * \brief Reservoir Method to calculate groundwater balance and baseflow of longterm model
 *
 */

/*!
 * \class ReservoirMethod
 * \ingroup GWA_RE
 * \brief Reservoir Method to calculate groundwater balance and baseflow of longterm model
 *
 */
class ReservoirMethod: public SimulationModule {
public:
    ReservoirMethod();

    ~ReservoirMethod();

    void SetValue(const char* key, FLTPT value) OVERRIDE;

    void SetValue(const char* key, int value) OVERRIDE;

    void Set1DData(const char* key, int n, FLTPT* data) OVERRIDE;

    void Set1DData(const char* key, int n, int* data) OVERRIDE;

    void Set2DData(const char* key, int nrows, int ncols, FLTPT** data) OVERRIDE;

    void SetSubbasins(clsSubbasins* subbsns) OVERRIDE;

    bool CheckInputData() OVERRIDE;

    void InitialOutputs() OVERRIDE;

    int Execute() OVERRIDE;

    void Get1DData(const char* key, int* nrows, FLTPT** data) OVERRIDE;

    void Get2DData(const char* key, int* nrows, int* ncols, FLTPT*** data) OVERRIDE;

    TimeStepType GetTimeStepType() OVERRIDE{ return TIMESTEP_CHANNEL; }

private:
    //inputs

    //! time step, second
    int m_dt;
    //! Valid cells number
    int m_nCells;
    //! cell size of the grid (m)
    FLTPT m_cellWth;
    //! maximum soil layers number
    int m_maxSoilLyrs;
    //! soil layers number of each cell
    int* m_nSoilLyrs;
    //! soil thickness of each layer
    FLTPT** m_soilThk;

    //! groundwater Revap coefficient
    FLTPT m_dp_co;
    //! baseflow recession coefficient
    FLTPT m_Kg;
    //! baseflow recession exponent
    FLTPT m_Base_ex;
    //! the amount of water percolated from the soil water reservoir and input to the groundwater reservoir from the percolation module(mm)
    FLTPT** m_soilPerco;
    //! evaporation from interception storage (mm) from the interception module
    FLTPT* m_IntcpET;
    //! evaporation from the depression storage (mm) from the depression module
    FLTPT* m_deprStoET;
    //! evaporation from the soil water storage (mm) from the soil ET module
    FLTPT* m_soilET;
    //! actual amount of transpiration (mm H2O)
    FLTPT* m_actPltET;
    //! PET(mm) from the PET modules
    FLTPT* m_pet;
    //! revap needed of cell
    FLTPT* m_revap;
    //! initial ground water storage (or at time t-1)
    FLTPT m_GW0;
    //! maximum ground water storage
    FLTPT m_GWMAX;

    FLTPT* m_petSubbsn; ///< Average PET of each subbasin, mm
    FLTPT* m_gwSto;     ///<  Groundwater storage (mm) of the subbasin

    /// slope (percent, or drop/distance, or tan) of each cell
    FLTPT* m_slope;

    //! soil storage
    FLTPT** m_soilWtrSto;
    //! soil depth of each layer, the maximum soil depth is used here, i.e., m_soilDepth[i][(int)m_soilLayers[i]]
    FLTPT** m_soilDepth;
    //! ground water from bank storage, passed from channel routing module
    FLTPT* m_VgroundwaterFromBankStorage;

    //output
    //!
    FLTPT* m_T_Perco;
    //!
    FLTPT* m_T_PerDep;
    //!
    FLTPT* m_T_RG;
    //!
    FLTPT* m_T_QG;
    //!
    FLTPT* m_T_Revap;
    //! groundwater water balance statistics
    FLTPT** m_T_GWWB;

    //! subbasin number
    int m_nSubbsns;
    //! current subbasin ID, 0 for the entire watershed
    int m_inputSubbsnID;
    //! subbasin IDs
    vector<int> m_subbasinIDs;
    //! All subbasins information
    clsSubbasins* m_subbasinsInfo;

};
#endif /* SEIMS_MODULE_GWA_RE_H */
