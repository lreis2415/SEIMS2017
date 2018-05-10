/*!
 * \brief Nutrient removed and lost in surface runoff.
 *        Support three carbon model, static method (orgn.f), C-FARM one carbon model (orgncswat.f), and CENTURY C/N cycling model (NCsed_leach.f90) from SWAT
 *        As for phosphorus, psed.f of SWAT calculates the attached to sediment in surface runoff.
 * \author Huiran Gao, Liangjun Zhu
 * \date April 2016
 * \changelog: 2016-04-30 - hr - Initial implementation.\n
 *             2016-09-28 - lj - 1. Code revision.\n
 *                               2. Add CENTURY model of calculating organic nitrogen removed in surface runoff.\n
 *             2017-08-23 - lj -  Solve inconsistent results when using openmp to reducing raster data according to subbasin ID.\n
 *             2018-05-08 - lj - 1. Reformat, especially naming style (sync update in "text.h").\n
 *                               2. Code optimization, e.g., use multiply instead of divide.\n
 * \TODO       1. Ammonian adsorbed to soil should be considered.
 */
#ifndef SEIMS_MODULE_NUTRSED_H
#define SEIMS_MODULE_NUTRSED_H

#include "SimulationModule.h"

/** \defgroup NUTRSED
 * \ingroup Nutrient
 * \brief Nutrient removed and lost with the eroded sediment in surface runoff
 */

/*!
 * \class NutrientTransportSediment
 * \ingroup NUTRSED
 *
 * \brief Nutrient removed and lost with the eroded sediment in surface runoff
 *
 */

class NutrientTransportSediment: public SimulationModule {
public:
    NutrientTransportSediment();

    ~NutrientTransportSediment();

    void Set1DData(const char* key, int n, float* data) OVERRIDE;

    void Set2DData(const char* key, int nRows, int nCols, float** data) OVERRIDE;

    void SetValue(const char* key, float value) OVERRIDE;

    void SetSubbasins(clsSubbasins*) OVERRIDE;

    int Execute() OVERRIDE;

    void Get1DData(const char* key, int* n, float** data) OVERRIDE;

    void Get2DData(const char* key, int* nRows, int* nCols, float*** data) OVERRIDE;

private:
    /*!
    * \brief check the input data. Make sure all the input data is available.
    * \return bool The validity of the input data.
    */
    bool CheckInputData();

    /*!
    * \brief check the input data for running CENTURY model. Make sure all the inputs data is available.
    * \return bool The validity of the inputs data.
    */
    bool CheckInputDataCenturyModel();

    /*!
    * \brief check the input data for running C-FARM one carbon model. Make sure all the inputs data is available.
    * \return bool The validity of the inputs data.
    */
    bool CheckInputDataCFarmModel();

    /*!
    * \brief check the input size. Make sure all the input data have same dimension.
    *
    * \param[in] key The key of the input data
    * \param[in] n The input data dimension
    * \return bool The validity of the dimension
    */
    bool CheckInputSize(const char* key, int n);

    /*!
    * \brief calculates the amount of organic nitrogen removed in surface runoff.
    *        orgn.f of SWAT, when CSWAT = 0
    * \return void
    */
    void OrgNRemovedInRunoffStaticMethod(int i);

    /*!
    * \brief calculates the amount of organic nitrogen removed in surface runoff.
    *        orgnswat.f of SWAT, when CSWAT = 1
    * \TODO THIS IS ON TODO LIST
    * \return void
    */
    void OrgNRemovedInRunoffCFarmOneCarbonModel(int i);

    /*!
    * \brief calculates the amount of organic nitrogen removed in surface runoff.
    *        NCsed_leach.f90 of SWAT, when CSWAT = 2
    * \return void
    */
    void OrgNRemovedInRunoffCenturyModel(int i);

    /*!
    * \brief Calculates the amount of organic and mineral phosphorus attached to sediment in surface runoff.
    * psed.f of SWAT
    * \return void
    */
    void OrgPAttachedtoSed(int i);

    /// initial outputs
    void InitialOutputs();
private:
    /// subbasins number
    int m_nSubbasins;
    /// current subbasin ID, 0 for the entire watershed
    int m_subbasinID;
    /// cell width of grid map (m)
    float m_cellWidth;
    /// cell area of grid map (ha)
    float m_cellArea;
    /// number of cells
    int m_nCells;
    /// soil layers
    float* m_nSoilLyrs;
    /// maximum soil layers
    int m_maxSoilLyrs;
    /// soil rock content, %
    float** m_soilRock;
    /// sol_ul, soil saturated water amount, mm
    float** m_soilSat;
    /* carbon modeling method
     *   = 0 Static soil carbon (old mineralization routines)
     *   = 1 C-FARM one carbon pool model
     *   = 2 Century model
     */
    int m_cbnModel;
    /// enrichment ratio
    float* m_enratio;

    ///inputs

    // soil loss caused by water erosion
    float* m_sedEroded;
    // surface runoff generated
    float* m_surfRf;
    //bulk density of the soil
    float** m_soilBD;
    // thickness of soil layer
    float** m_soilThk;
    /// Soil mass (kg/ha)
    float** m_soilMass;

    /// subbasin related
    //! subbasin IDs
    vector<int> m_subbasinIDs;
    /// subbasin grid (subbasins ID)
    float* m_subbasin;
    /// subbasins information
    clsSubbasins* m_subbasinsInfo;

    ///output data
    //amount of organic nitrogen in surface runoff
    float* m_sedorgn;
    //amount of organic phosphorus in surface runoff
    float* m_sedorgp;
    //amount of active mineral phosphorus sorbed to sediment in surface runoff
    float* m_sedminpa;
    //amount of stable mineral phosphorus sorbed to sediment in surface runoff
    float* m_sedminps;

    /// output to channel

    float* m_sedorgnToCh;  // amount of organic N in surface runoff to channel, kg
    float* m_sedorgpToCh;  // amount of organic P in surface runoff to channel, kg
    float* m_sedminpaToCh; // amount of active mineral P in surface runoff to channel, kg
    float* m_sedminpsToCh; // amount of stable mineral P in surface runoff to channel, kg

    ///input & output
    //amount of nitrogen stored in the active organic (humic) nitrogen pool, kg N/ha
    float** m_soilActvOrgN;
    //amount of nitrogen stored in the fresh organic (residue) pool, kg N/ha
    float** m_soilFrshOrgN;
    //amount of nitrogen stored in the stable organic N pool, kg N/ha
    float** m_soilStabOrgN;
    //amount of phosphorus stored in the organic P pool, kg P/ha
    float** m_soilHumOrgP;
    //amount of phosphorus stored in the fresh organic (residue) pool, kg P/ha
    float** m_soilFrshOrgP;
    //amount of phosphorus in the soil layer stored in the stable mineral phosphorus pool, kg P/ha
    float** m_soilStabMinP;
    //amount of phosphorus stored in the active mineral phosphorus pool, kg P/ha
    float** m_soilActvMinP;
    /// for C-FARM one carbon model
    float** m_soilManP;
    /// for CENTURY C/Y cycling model
    /// inputs from other modules
    float** m_sol_LSN;
    float** m_sol_LMN;
    float** m_sol_HPN;
    float** m_sol_HSN;
    float** m_sol_HPC;
    float** m_sol_HSC;
    float** m_sol_LMC;
    float** m_sol_LSC;
    float** m_sol_LS;
    float** m_sol_LM;
    float** m_sol_LSL;
    float** m_sol_LSLC;
    float** m_sol_LSLNC;
    float** m_sol_BMC;
    float** m_sol_WOC;
    float** m_soilPerco;
    float** m_subSurfRf;
    /// outputs
    float** m_sol_latC;   /// lateral flow Carbon loss in each soil layer
    float** m_sol_percoC; /// percolation Carbon loss in each soil layer
    float* m_laterC;      /// lateral flow Carbon loss in soil profile
    float* m_percoC;      /// percolation Carbon loss in soil profile
    float* m_sedCLoss;    /// amount of C lost with sediment pools
};
#endif /* SEIMS_MODULE_NUTRSED_H */
