/**
*	@file
*	@version	1.0
*	@author    Wu Hui
*	@date	24-January-2011
*
*	@brief	IUH overland method to calculate interflow routing
*
*	Revision:	Wu Hui
*   Date:		2011-2-23
*	Description:
*	1.	Add parameter CellWidth.
*	2.	Delete parameter uhminCell and uhmaxCell because the parameter Ol_iuh
*		contains these information. The first and second column of Ol_iuh is
*		min time and max time.
*	3.	The number of subbasins (m_nsub) should get from m_subbasin rather than
*		from main program. So does variable m_nCells.
*	4.	Add varaible m_iuhCols to store the number of columns of Ol_iuh. In the
*		meantime, add one parameter nCols to function SetIUHCell.
*	5.	Add variable m_cellFlow to store the flow of each cell in each day between
*		min time and max time. Its number of columns equals to the maximum of second
*		column of Ol_iuh add 1.
*	6.  Add function initial to initialize some variables.
*	7.	Modify function Execute.
*/
#ifndef SEIMS_IUH_IF_H
#define SEIMS_IUH_IF_H

#include "SimulationModule.h"

// using namespace std;  // Avoid this statement! by lj.

/*!
 * \defgroup IUH_IF
 * \ingroup Hydrology
 * \brief IUH overland method to calculate interflow routing
 *
 */

/*!
 * \class IUH_IF
 * \ingroup IUH_IF
 * \brief IUH overland method to calculate interflow routing
 *
 */
class IUH_IF : public SimulationModule {
public:
    IUH_IF(void);

    ~IUH_IF(void);

    virtual int Execute(void);

    virtual void SetValue(const char *key, FLTPT data);

    virtual void Set1DData(const char *key, int n, FLTPT *data);

    virtual void Set2DData(const char *key, int nRows, int nCols, FLTPT **data);

    virtual void Get1DData(const char *key, int *n, FLTPT **data);

    bool CheckInputSize(const char *key, int n);

    bool CheckInputData(void);

private:

    /// time step (hr)
    int m_TimeStep;
    /// cell size of the grid (the validate cells of the whole basin)
    int m_nCells;
    /// cell area of the unit (m^2)
    FLTPT* m_cellArea;
    /// the total number of subbasins
    int m_nsub;
    /// subbasin grid ( subwatersheds ID)
    FLTPT *m_subbasin;
    /*/// start time of IUH for each grid cell
    FLTPT* m_uhminCell;
    /// end time of IUH for each grid cell
    FLTPT* m_uhmaxCell;
    /// IUH of each grid cell (1/s)*/
    FLTPT **m_iuhCell;
    /// the number of columns of Ol_iuh
    int m_iuhCols;
    /// subsurface runoff from depression module
    FLTPT *m_ssru;
    /*/// length of rainfall series
    int m_nr;*/
    /*/// end time of simulation
    time_t m_EndDate;*/

    //temparory
    FLTPT **m_cellFlow;
    int m_cellFlowCols;
    //output
    /// interflow to streams for each subbasin (m3/s)
    FLTPT *m_Q_SBIF;

    void InitialOutputs(void);
};
#endif /* SEIMS_IUH_IF_H */
