/*!
 * \brief
 * \author
 * \date
 */
#ifndef SEIMS_SSM_PE_H
#define SEIMS_SSM_PE_H

#include "SimulationModule.h"

// using namespace std;  // Avoid this statement! by lj.

/*!
 * \defgroup SSM_PE
 * \ingroup Hydrology
 * \brief Distribution of snow sublimation (water equivalent) for a user defined period
 *
 */

/*!
 * \class SSM_PE
 * \ingroup SSM_PE
 * \brief Calculate distribution of snow sublimation
 *
 */
class SSM_PE : public SimulationModule {
public:
    //! Constructor
    SSM_PE(void);

    //! Destructor
    ~SSM_PE(void);

    virtual int Execute(void);

    virtual void SetValue(const char *key, float data);

    virtual void Set1DData(const char *key, int n, float *data);

    virtual void Get1DData(const char *key, int *n, float **data);

    bool CheckInputSize(const char *key, int n);

    bool CheckInputData(void);

private:
    //! Valid cells number
    int m_nCells;

    float m_t0;
    float m_tsnow;
    float m_ksubli;
    float m_kblow;
    float m_lastSWE;

    float *m_PET;
    float *m_Pnet;
    float *m_SA;
    float m_swe;
    float m_swe0;
    float *m_SR;
    float *m_tMean;

    //result
    float *m_SE;

    bool m_isInitial;

    void InitialOutputs(void);
};
#endif /* SEIMS_SSM_PE_H */
