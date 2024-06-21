/*!
 * \brief Snow melt water balance
 * \author Chunping Ou
 * \date May 2011
 * \revised LiangJun Zhu
 * \date 2016-5-29
 *  1. Remove m_isInitial and add initialOutputs(void)
 *  2. Wind speed is DT_Raster1D
 */
#ifndef SEIMS_SNO_WB_H
#define SEIMS_SNO_WB_H

#include "SimulationModule.h"

// using namespace std;  // Avoid this statement! by lj.

/*!
 * \defgroup SNO_WB
 * \ingroup Hydrology
 * \brief Calculate snow water balance
 *
 */

/*!
 * \class SNO_WB
 * \ingroup SNO_WB
 * \brief Calculate snow water balance
 *
 */
class SNO_WB : public SimulationModule {
public:
    //! Constructor
    SNO_WB(void);

    //! Destructor
    ~SNO_WB(void);

    virtual int Execute(void);

    virtual void SetValue(const char *key, float data);

    virtual void GetValue(const char *key, float *value);

    virtual void Set1DData(const char *key, int n, float *data);

    virtual void Get1DData(const char *key, int *n, float **data);
    //virtual void Get2DData(const char* key, int* nRows, int* nCols, float*** data);

    bool CheckInputSize(const char *key, int n);

    bool CheckInputData(void);

private:
    //! Valid cells number
    int m_nCells;
    //time_t m_Date; // there is no need to define m_Date again. By LJ.

    float m_t0;
    float m_tsnow;
    float m_kblow;
    float m_swe0;

    float *m_SA;  //snow accumulation
    //float** m_snowWaterBalance; //the output average values of selected subbasin
    float *m_Pnet;
    //net precipitation
    float *m_SR; //snow redistribution
    float *m_SE; //snow sublimation
    float *m_SM; //snow melt

    float m_SWE; //total average SA of the watershed

    //following four variable don't participate calculation, just for output
    float *m_P;    //precipitation
    float *m_tMax;
    float *m_tMean;

    float *m_WindSpeed;
    //int m_nCells;

    //float* m_subbasin;	//subbasin grid
    //int m_subbasinSelectedCount;
    //float* m_subbasinSelected;	//subbasin selected to output
    //map<int,subbasin*>* m_subbasinList;
    /// removed by LJ
    ///bool m_isInitial;

    void InitialOutputs(void);

    //void setValueToSubbasin(void);

    //void getSubbasinList(int cellCount, float* subbasinGrid, int subbasinSelectedCount, float* subbasinSelected);
};
#endif /* SEIMS_SNO_WB_H */
