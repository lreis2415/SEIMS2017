/*!
 * \brief channel flow routing using ikw method
 * \author Junzhi Liu, Liangjun Zhu
 * \date May 2017
 * \revised LJ - Replace Tag_RchParam by VAR_REACH_PARAM[0]
 *               Algorithm review and code clean
 */
#ifndef SEIMS_MODULE_IKW_REACH_H
#define SEIMS_MODULE_IKW_REACH_H

#include "SimulationModule.h"

// using namespace std;  // Avoid this statement! by lj.

/*!
 * \defgroup IKW_REACH
 * \ingroup Hydrology
 * \brief channel flow routing using Muskingum method
 *
 */

/*!
 * \class IKW_REACH
 * \ingroup IKW_REACH
 * \brief Overland routing using 4-point implicit finite difference method
 *
 */
struct MuskWeights {
    FLTPT c1;
    FLTPT c2;
    FLTPT c3;
    FLTPT c4;
    FLTPT dt;
    int n;  // number of division of the origin time step
};

class IKW_REACH : public SimulationModule {
public:
    IKW_REACH();

    ~IKW_REACH();

    int Execute();

    void SetValue(const char *key, FLTPT data);
    
    void SetValue(const char *key, const int value);

    // void GetValue(const char *key, FLTPT *value);

    void Set1DData(const char *key, int n, FLTPT *data);

    void Get1DData(const char *key, int *n, FLTPT **data);

//    void Set2DData(const char *key, int nrows, int ncols, FLTPT **data);

    void Get2DData(const char *key, int *nRows, int *nCols, FLTPT ***data);

    void SetReaches(clsReaches *reaches);

    bool CheckInputSize(const char *key, int n);

    // bool CheckInputSizeChannel(const char *key, int n);

    bool CheckInputData();

    void  InitialOutputs();

    TimeStepType GetTimeStepType() {
        return TIMESTEP_CHANNEL;
    };

private:
    /// time step (hr)
    int m_dt;
    /// reach number (= subbasin number)
    int m_nreach;
    /// layering method, 0 means UP_DOWN, 1 means DOWN_UP
    //LayeringMethod m_layeringMethod;
    ///// diversion loss (Vdiv) of the river reach .. m_Vid[id], id is the reach id
    //FLTPT *m_Vdiv;
    ///// The point source discharge .. m_Vpoint[id], id is the reach id
    //FLTPT *m_Vpoint;

    /// hydraulic conductivity of the channel bed (mm/h)
    FLTPT *m_Kchb;
    /// hydraulic conductivity of the channel bank (mm/h)
    FLTPT *m_Kbank;
    /// reach evaporation adjustment factor;
    FLTPT m_Epch;
    /// initial bank storage per meter of reach length (m3/m)
    FLTPT m_Bnk0;
    /// initial channel storage per meter of reach length (m3/m)
    FLTPT m_Chs0;
    /// the initial volume of transmission loss to the deep aquifer over the time interval (m3/s)
    FLTPT m_Vseep0;   //added
    /// bank flow recession constant
    FLTPT m_aBank;
    /// bank storage loss coefficient
    FLTPT m_bBank;
    /// subbasin grid
    FLTPT *m_subbasin;
    /// the subbasin area (m2)  //add to the reach parameters file
    FLTPT *m_area;

    /// Average PET for each subbasin
    FLTPT *m_petCh;

    /// overland flow to streams from each subbasin (m3/s)
    FLTPT *m_qsSub;
    /// interflow to streams from each subbasin (m3/s)
    FLTPT *m_qiSub;
    /// groundwater flow out of the subbasin (m3/s)
    FLTPT *m_qgSub;
    ///  Groundwater storage (mm) of the subbasin
    FLTPT *m_gwStorage;

    /// channel outflow
    FLTPT *m_qsCh;
    FLTPT *m_qiCh;
    FLTPT *m_qgCh;

    FLTPT *m_chWidth;
    FLTPT *m_chDepth;
    FLTPT *m_chLen;
    FLTPT *m_chVel;
    FLTPT *m_chManning;
    FLTPT *m_chSlope;

    FLTPT *m_bankStorage;

    FLTPT m_deepGroudwater;

    /// seepage to deep aquifer
    FLTPT *m_seepage;

    /// downstream id (The value is 0 if there if no downstream reach)
    FLTPT *m_reachDownStream;
    /// Index of upstream Ids (The value is -1 if there if no upstream reach)
    vector<vector<int> > m_reachUpStream;

    // for muskingum
    FLTPT m_x;
    FLTPT m_co1;

    FLTPT m_qUpReach;

    //temporary at routing time
    /// reach storage (m3) at time t
    FLTPT *m_chStorage;

    /// reach outflow (m3/s) at time t
    FLTPT *m_qOut;
    /// flowin discharge at the last time step
    FLTPT *m_qIn;
    /// channel water depth m
    FLTPT *m_chWTdepth;

    map<int, vector<int> > m_reachLayers;


    void ChannelFlow(int i);

    FLTPT GetNewQ(FLTPT qIn, FLTPT qLast, FLTPT surplus, FLTPT alpha, FLTPT dt, FLTPT dx);
};

#endif /* SEIMS_MODULE_IKW_REACH_H */
