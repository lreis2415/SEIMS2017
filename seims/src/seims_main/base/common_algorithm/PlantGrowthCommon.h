/*!
 * \file PlantGrowthCommon.h
 * \brief Define some common used function in Plant growth module, e.g., PG_EPIC
 * \author Zhiqiang Yu, Liang-Jun Zhu
 * \date June, 2016
 */
#ifndef SEIMS_PLANTGROWTH_COMMON_H
#define SEIMS_PLANTGROWTH_COMMON_H

#include "basic.h"
#include "BMPText.h"
#include "seims.h"

//using namespace ccgl;

/*!
 * \defgroup PlantGrowthCommon
 * \ingroup common_algorithm
 * \brief Functions for Plant growth related modules, e.g., PG_EPIC.
 */

#define MAX_CROP_LAND_USE_ID   97
#define BARE_SOIL_LAND_USE     98
#define WATER_LAND_USE_ID      18

/*!
 * \ingroup PlantGrowthCommon
 * \brief Get N/P shape parameter, See readplant.f of SWAT
 */
void GetNPShapeParameter(FLTPT fr1, FLTPT fr2, FLTPT fr3, FLTPT* shape1, FLTPT* shape2);

/*!
 * \ingroup PlantGrowthCommon
 * \brief Computes shape parameters \a shape1 and \a shape2 for the S curve. See ascrv.f of SWAT
 *
 *  The equation \f$x=\frac{y}{y+exp(S_1+S_2^y)}\f$ give two \f$(x,y)\f$ points along the curve.
 */
void GetScurveShapeParameter(FLTPT x_mid, FLTPT x_end, FLTPT y_mid, FLTPT y_end,
                             FLTPT* shape1, FLTPT* shape2);

/*!
 * \ingroup PlantGrowthCommon
 * \brief Biomass fraction
 */
FLTPT NPBiomassFraction(FLTPT x1, FLTPT x2, FLTPT x3, FLTPT fr_phu);

/*!
 * \ingroup PlantGrowthCommon
 * \brief calculate the fraction of nitrogen/phosphorus in the plant biomass
 *
 * Plant nitrogen/phosphorus equation, p300 5:2.3.1/p305 5:2.3.19
 */
FLTPT GetNPFraction(FLTPT fr1, FLTPT fr3, FLTPT shape1, FLTPT shape2, FLTPT fr_phu);

/*!
 * \ingroup PlantGrowthCommon
 * \brief Heat unit accumulation
 */
FLTPT DoHeatUnitAccumulation(FLTPT potential_heat_unit, FLTPT t_min, FLTPT t_max, FLTPT t_base);

/*!
 * \ingroup PlantGrowthCommon
 * \brief the adjusted radiation-use efficiency by vapor pressure deficit
 */
FLTPT RadiationUseEfficiencyAdjustByVPD(FLTPT vpd, FLTPT rad_use_eff_dec_rate_with_vpd);

/*!
 * \ingroup PlantGrowthCommon
 * \brief Normalization
 */
FLTPT GetNormalization(FLTPT distribution);

/*!
 * \ingroup PlantGrowthCommon
 * \brief Is tree or not
 */
inline bool IsTree(const int classification) { return classification == 7; }

/*!
 * \ingroup PlantGrowthCommon
 * \brief Is annual plant or not
 */
inline bool IsAnnual(const int classification) {
    return classification == CROP_IDC_WARM_SEASON_ANNUAL_LEGUME ||
            classification == CROP_IDC_COLD_SEASON_ANNUAL_LEGUME ||
            classification == CROP_IDC_WARM_SEASON_ANNUAL ||
            classification == CROP_IDC_COLD_SEASON_ANNUAL;
}
/*!
 * \ingroup PlantGrowthCommon
 * \brief Is legume or not
 */
inline bool IsLegume(const int classification) {
    return classification <= CROP_IDC_PERENNIAL_LEGUME;
}

/*!
 * \ingroup PlantGrowthCommon
 * \brief Is perennial plant or not
 */
inline bool IsPerennial(const int classification) {
    return classification == CROP_IDC_PERENNIAL_LEGUME ||
            classification == CROP_IDC_PERENNIAL;
}
/*!
 * \ingroup PlantGrowthCommon
 * \brief Is cool seanon annual plant or not
 */
inline bool IsCoolSeasonAnnual(const int classification) {
    return classification == CROP_IDC_COLD_SEASON_ANNUAL_LEGUME ||
            classification == CROP_IDC_COLD_SEASON_ANNUAL;
}
/*!
 * \ingroup PlantGrowthCommon
 * \brief Is grain or not
 */
inline bool IsGrain(const int classification) {
    return classification == CROP_IDC_WARM_SEASON_ANNUAL;
}
/*!
 * \ingroup PlantGrowthCommon
 * \brief Is plant or not
 */
inline bool IsPlant(const int lu_id) {
    return lu_id <= MAX_CROP_LAND_USE_ID && lu_id != WATER_LAND_USE_ID;
}

/*!
 * \ingroup PlantGrowthCommon
 * \brief Calculates the plant stress factor caused by limited supply of nitrogen or phosphorus.
 *        From ntus.f of SWAT, rev 637
 */
void CalPlantStressByLimitedNP(FLTPT u1, FLTPT u2, FLTPT* uu);

#endif /* SEIMS_PLANTGROWTH_COMMON_H */
