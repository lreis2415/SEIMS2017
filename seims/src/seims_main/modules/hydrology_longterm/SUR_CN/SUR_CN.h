/**
*	@file
*	@version	1.0
*	@author    Wu Hui
*	@date	29-December-2010
*
*	@brief	SCS Curve Number Method to calculate infiltration and excess precipitation
*
*	Revision: Zhiqiang Yu
*   Date:     2011-2-14
*   1. Because w1 and w2 in equation 2:1.1.6 is fixed, add two variable to store these
*      two coefficients to avoid repeating calculation.
*   2. When w1 and w2 is calculated using equation 2:1.1.7 and 2:1.1.8, FC and SAT should
*	   be FC*rootdepth and SAT*rootdepth rather than (FC-WP)*rootdepth and (SAT-WP)*rootdepth.
*	   And the unit conversion of rootdepth would be from m to mm.
*   3. Overload the function Calculate_CN to calculate the CN of one cell.
*   4. Delete some parameters and input variables. They are Depre_in,Moist_in and Depression.
*      D_SOMO from soil water balance module and D_DPST from depression module is enough. The
*      initialization of soil moisture and depression storage is the task of soil water balance
*	   module and depression module.
*	5. Modify the name of input and output variables to make it consistent with other modules.
*
* Revised LiangJun Zhu
*	1. Add the support of dynamic multi-layers soil, rather than the fixed 2 layers in previous version.
*   2.The unit of soil depth and root depth (from crop.dat) are both mm.
*/
#ifndef SEIMS_SUR_CN_H
#define SEIMS_SUR_CN_H

#include "SimulationModule.h"

// using namespace std;  // Avoid this statement! by lj.

/*!
 * \defgroup SUR_CN
 * \ingroup Hydrology
 * \brief SCS Curve Number Method to calculate infiltration and excess precipitation
 *
 */

/*!
 * \class SUR_CN
 * \ingroup SUR_CN
 * \brief SCS Curve Number Method to calculate infiltration and excess precipitation
 *
 */
class SUR_CN : public SimulationModule {
public:
    //! Constructor
    SUR_CN(void);

    //! Destructor
    ~SUR_CN(void);

    virtual int Execute(void) OVERRIDE;

    virtual void SetValue(const char *key, FLTPT data) OVERRIDE;

    virtual void Set1DData(const char *key, int n, FLTPT *data) OVERRIDE;

    virtual void Set2DData(const char *key, int nrows, int ncols, FLTPT **data) OVERRIDE;

    virtual void Get1DData(const char *key, int *n, FLTPT **data) OVERRIDE;

    virtual void Get2DData(const char *key, int *nRows, int *nCols, FLTPT ***data) OVERRIDE;

    bool CheckInputSize(const char *key, int n) OVERRIDE;

    bool CheckInputData(void) OVERRIDE;

    void InitialOutputs(void) OVERRIDE;

private:
    /// number of soil layers, i.e., the maximum soil layers number of all soil types
    int m_nSoilLayers;
    /// soil depth
    FLTPT **m_soilDepth;
    ///// depth of the up two layers(The depth are 10mm and 100 mm, respectively).
    //FLTPT m_depth[2];
    /// soil depth of the current layer, replace FLTPT m_depth[2] in previous version.
    FLTPT *m_upSoilDepth;
    /// valid cells number
    int m_nCells;
    /// soil porosity
    FLTPT **m_porosity;
    /// water content of soil at field capacity
    FLTPT **m_fieldCap;
    /// plant wilting point moisture
    FLTPT **m_wiltingPoint;

    /// root depth of plants (mm)
    FLTPT *m_rootDepth;
    /// CN under moisture condition II
    FLTPT *m_cn2;
    /// Net precipitation calculated in the interception module (mm)
    FLTPT *m_P_NET;
    /// Initial soil moisture
    FLTPT *m_initSoilMoisture;

    /// depression storage
    FLTPT *m_SD;    // SD(t-1) from the depression storage module
    /// from interpolation module
    /// mean air temperature of the current day
    FLTPT *m_tMean;
    /// snowfall temperature from the parameter database (deg C)
    FLTPT m_Tsnow;
    /// threshold soil freezing temperature (deg C)
    FLTPT m_Tsoil;
    /// frozen soil moisture relative to saturation above which no infiltration occur (m3/m3)
    FLTPT m_Sfrozen;
    /// snowmelt threshold temperature from the parameter database (deg C)
    FLTPT m_T0;
    /// snowmelt from the snowmelt module  (mm)
    FLTPT *m_SM;
    /// snow accumulation from the snow balance module (mm) at t+1 timestep
    FLTPT *m_SA;
    /// soil temperature obtained from the soil temperature module (deg C)
    FLTPT *m_TS;

    /// Julian day, not used? by LJ
    //int m_julianDay;

    // output
    /// the excess precipitation (mm) of the total nCells
    FLTPT *m_PE;
    /// soil moisture of each soil layer in current time step
    FLTPT **m_soilMoisture;
    /// infiltration map of watershed (mm) of the total nCells
    FLTPT *m_INFIL;

    /// soil water storage in soil profile (mm)
    FLTPT* m_soilWtrStoPrfl;
    //add by Zhiqiang
    /// the first shape coefficient in eq. 2:1.1.7 and 2:1.1.8 in SWAT theory 2009, p104
    FLTPT *m_w1;
    /// the second shape coefficient in eq. 2:1.1.7 and 2:1.1.8 in SWAT theory 2009, p104
    FLTPT *m_w2;
    /// the retention parameter for the moisture condition I curve number
    FLTPT *m_sMax;

    /// initialize m_w1 and m_w2
    void initalW1W2(void);

    /// Calculation SCS-CN number
    FLTPT Calculate_CN(FLTPT sm, int cell);
    
};
#endif /* SEIMS_SUR_CN_H */
