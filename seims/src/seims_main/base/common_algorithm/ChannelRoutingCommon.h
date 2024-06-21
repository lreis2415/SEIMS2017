/*!
 * \file ChannelRoutingCommon.h
 * \brief Define some common used function in channel routing related modules, e.g., MUSK_CH.
 * \author Liang-Jun Zhu
 * \date 2018-8-11
 */
#ifndef SEIMS_CHANNEL_ROUTING_COMMON_H
#define SEIMS_CHANNEL_ROUTING_COMMON_H
#include <seims.h>

/*!
 * \defgroup ChannelRouting 
 * \ingroup common_algorithm
 * \brief Functions for channel routing related modules, e.g., MUSK_CH.
 */

/*!
 * \ingroup ChannelRouting
 * \brief Calculates flow rate or flow velocity using Manning's
 *        equation. If x1 is set to 1, the velocity is calculated. If x1 is set to
 *        cross-sectional area of flow, the flow rate is calculated.
 *        Refers code Qman.f in SWAT.
 * \param[in] x1 cross-sectional flow area or 1, m^2 or none.
 * \param[in] x2 hydraulic radius, m.
 * \param[in] x3 Manning's "n" value for channel.
 * \param[in] x4 average slope of channel, m/m.
 * \return flow rate or flow velocity, m^3/s or m/s.
 */
FLTPT manningQ(FLTPT x1, FLTPT x2, FLTPT x3, FLTPT x4);

/*!
 * \ingroup ChannelRouting
 * \brief Calculate channel bottom width by channel width, side slope, and depth.
 *        Refers code ttcoef.f in SWAT.
 * \param[in] ch_wth Channel upper width
 * \param[in,out] ch_sideslp The inverse of channel side slope (default is 2, slope = 0.5), which maybe updated when bottom width < 0
 * \param[in,out] ch_depth Channel depth, which maybe updated when bottom width < 0
 * \return Channel bottom width
 */
FLTPT ChannleBottomWidth(FLTPT ch_wth, FLTPT& ch_sideslp, FLTPT& ch_depth);

/*!
 * \ingroup ChannelRouting
 * \brief Channel wetting perimeter for both floodplain and not full channel
 * \param[in] ch_btmwth Channel bottom width
 * \param[in] ch_depth Channel depth
 * \param[in] wtr_depth Channel water depth
 * \param[in] ch_sideslp The inverse of channel side slope (default is 2, slope = 0.5)
 * \param[in] ch_wth Channel width at bankfull
 * \param[in] fps The inverse of floodplain side slope (default is 4, slope = 0.25)
 * \return Channel wetting perimeter
 */
FLTPT ChannelWettingPerimeter(FLTPT ch_btmwth, FLTPT ch_depth, FLTPT wtr_depth,
                              FLTPT ch_sideslp, FLTPT ch_wth, FLTPT fps = 4.);

/*!
 * \ingroup ChannelRouting
 * \brief Channel wetting perimeter for not full channel
 * \param[in] ch_btmwth Channel bottom width
 * \param[in] wtr_depth Channel water depth
 * \param[in] ch_sideslp The inverse of channel side slope (default is 2, slope = 0.5)
 * \return Channel wetting perimeter
 */
FLTPT ChannelWettingPerimeter(FLTPT ch_btmwth, FLTPT wtr_depth, FLTPT ch_sideslp);

/*!
 * \ingroup ChannelRouting
 * \brief Cross-sectional area of channel for both floodplain and not full channel
 * \param[in] ch_btmwth Channel bottom width
 * \param[in] ch_depth Channel depth
 * \param[in] wtr_depth Channel water depth
 * \param[in] ch_sideslp The inverse of channel side slope (default is 2, slope = 0.5)
 * \param[in] ch_wth Channel width at bankfull
 * \param[in] fps The inverse of floodplain side slope (default is 4, slope = 0.25)
 * \return Channel cross-sectional area
 */
FLTPT ChannelCrossSectionalArea(FLTPT ch_btmwth, FLTPT ch_depth, FLTPT wtr_depth,
                                FLTPT ch_sideslp, FLTPT ch_wth, FLTPT fps = 4.);

/*!
 * \ingroup ChannelRouting
 * \brief Cross-sectional area of channel for not full channel
 * \param[in] ch_btmwth Channel bottom width
 * \param[in] wtr_depth Channel water depth
 * \param[in] ch_sideslp The inverse of channel side slope (default is 2, slope = 0.5)
 * \return Channel cross-sectional area
 */
FLTPT ChannelCrossSectionalArea(FLTPT ch_btmwth, FLTPT wtr_depth, FLTPT ch_sideslp);

/*!
 * \ingroup ChannelRouting
 * \brief Compute storage time constant for channel (ratio of storage to discharge)
 * \param[in] ch_manning Manning's n value of channel
 * \param[in] ch_slope Channel slope
 * \param[in] ch_len Channel length, m
 * \param[in] radius Hydraulic radius, m
 * \return Storage time constant
 */
FLTPT StorageTimeConstant(FLTPT ch_manning, FLTPT ch_slope, FLTPT ch_len,
                          FLTPT radius);

#endif /* SEIMS_CHANNEL_ROUTING_COMMON_H */
