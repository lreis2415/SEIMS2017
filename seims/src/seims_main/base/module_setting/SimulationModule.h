/*!
 * \file SimulationModule.h
 * \brief Parent class for all modules in SEIMS
 *
 * Changelog:
 *   - 1. 2010-07-31 - jz - Initial implementation.
 *   - 2. 2016-06-14 - lj - Add SetScenario etc. functions.
 *   - 3. 2018-03-03 - lj - Add CHECK_XXX series macros for data checking.
 *   - 4. 2020-09-18 - lj - Using Easyloggingpp
 *   - 5. 2021-10-29 - ss,lj - Add InitialIntermediates to initialize intermediate params.
 *
 * \author Junzhi Liu, Liangjun Zhu
 */
#ifndef SIMULATION_MOUDULE_BASE
#define SIMULATION_MOUDULE_BASE

#include "basic.h"
#include "utils_time.h"
#include "utils_math.h"
#include "Scenario.h"
#include "clsReach.h"
#include "clsSubbasin.h"

#include <string>
#include <ctime>

using namespace ccgl;
using namespace utils_time;
using namespace bmps;

/*!
 * \enum TimeStepType
 * \ingroup module_setting
 * \brief Time step types.
 */
enum TimeStepType {
    TIMESTEP_HILLSLOPE, ///< Hillslope scale
    TIMESTEP_CHANNEL,   ///< Channel scale
    TIMESTEP_SIMULATION ///< Whole simulation scale
};
/*!
 * \ingroup module_setting
 * \class SimulationModule
 * \brief Base module for all simulation modules in SEIMS
 */
class SimulationModule: Interface {
public:
    //! Constructor
    SimulationModule();

    //! Execute the simulation. Return 0 for success.
    virtual int Execute() { return -1; }
    //! Set date time, as well as the sequence number of the entire simulation. Added by LJ for statistics convenient.
    virtual void SetDate(time_t t, int year_idx);

    //! Set thread number for OpenMP
    virtual void SetTheadNumber(const int thread_num) {
        SetOpenMPThread(thread_num);
    }

    //! Set climate data type, P, M, PET etc.
    virtual void SetClimateDataType(int data_type) {}
    
    //! Set data, DT_Single, integer
    virtual void SetValue(const char* key, int value) {
        throw ModelException("SimulationModule", "SetValue",
            "Set function of parameter " + string(key) + " is not implemented.");
    }

    //! Set data, DT_Single, float point number (float or double)
    virtual void SetValue(const char* key, FLTPT value) {
        throw ModelException("SimulationModule", "SetValue",
            "Set function of parameter " + string(key) + " is not implemented.");
    }

    //! Set single value to array1D by index, used in MPI version for passing values of subbasins, integer
    virtual void SetValueByIndex(const char* key, int index, int value) {
        throw ModelException("SimulationModule", "SetValueByIndex",
            "Set function of parameter " + string(key) + " is not implemented.");
    }

    //! Set single value to array1D by index, used in MPI version for passing values of subbasins, float
    virtual void SetValueByIndex(const char* key, int index, FLTPT value) {
        throw ModelException("SimulationModule", "SetValueByIndex",
            "Set function of parameter " + string(key) + " is not implemented.");
    }

    //! Set 1D data, by default, DT_Raster1D, integer
    void Set1DDataBase(const char* key, int n, int* data) {
        CheckInputSize(key, n);
        Set1DData(key, n, data);
    }
    virtual void Set1DData(const char* key, int n, int* data) {
        throw ModelException("SimulationModule", "Set1DData",
            "Set function of parameter " + string(key) + " is not implemented.");
    }

    //! Set 1D data, by default, DT_Raster1D, float
    void Set1DDataBase(const char* key, int n, FLTPT* data) {
        //cout << key << " Set1DData size: " << n << endl;
        CheckInputSize(key, n);
        Set1DData(key, n, data);
    }
    virtual void Set1DData(const char* key, int n, FLTPT* data) {
        throw ModelException("SimulationModule", "Set1DData",
            "Set function of parameter " + string(key) + " is not implemented.");
    }

    //! Set 2D data, by default, DT_Raster2D, integer
    void Set2DDataBase(const char* key, int nrows, int ncols, int** data) {
        Set2DData(key, nrows, ncols, data);
    }
    virtual void Set2DData(const char* key, int nrows, int ncols, int** data) {
        throw ModelException("SimulationModule", "Set2DData",
            "Set function of parameter " + string(key) + " is not implemented.");
    }

    //! Set 2D data, by default, DT_Raster2D, float
    void Set2DDataBase(const char* key, int nrows, int ncols, FLTPT** data) {
        Set2DData(key, nrows, ncols, data);
    }
    virtual void Set2DData(const char* key, int nrows, int ncols, FLTPT** data) {
        throw ModelException("SimulationModule", "Set2DData",
            "Set function of parameter " + string(key) + " is not implemented.");
    }

    //! Get value, DT_Single, integer
    virtual void GetValue(const char* key, int* value) {
        throw ModelException("SimulationModule", "GetValue",
            "Get function of parameter " + string(key) + " is not implemented.");
    }

    //! Get value, DT_Single, float
    virtual void GetValue(const char* key, FLTPT* value) {
        throw ModelException("SimulationModule", "GetValue",
            "Get function of parameter " + string(key) + " is not implemented.");
    }

    //! Get 1D data, by default, DT_Raster1D, integer
    void Get1DDataBase(const char* key, int* n, int** data);
    virtual void Get1DData(const char* key, int* n, int** data) {
        throw ModelException("SimulationModule", "Get1DData",
            "Get function of parameter " + string(key) + " is not implemented.");
    }

    //! Get 1D data, by default, DT_Raster1D, float
    void Get1DDataBase(const char* key, int* n, FLTPT** data);
    virtual void Get1DData(const char* key, int* n, FLTPT** data) {
        throw ModelException("SimulationModule", "Get1DData",
            "Get function of parameter " + string(key) + " is not implemented.");
    }

    //! Get 2D data, by default, DT_Raster2D, integer
    void Get2DDataBase(const char* key, int* nrows, int* ncols, int*** data);
    virtual void Get2DData(const char* key, int* nrows, int* ncols, int*** data) {
        throw ModelException("SimulationModule", "Get2DData",
            "Get function of parameter " + string(key) + " is not implemented.");
    }

    //! Get 2D data, by default, DT_Raster2D, float
    void Get2DDataBase(const char* key, int* nrows, int* ncols, FLTPT*** data);
    virtual void Get2DData(const char* key, int* nrows, int* ncols, FLTPT*** data) {
        throw ModelException("SimulationModule", "Get2DData",
            "Get function of parameter " + string(key) + " is not implemented.");
    }

    //! Set pointer of Scenario class which contains all BMP information. Added by LJ, 2016-6-14
    virtual void SetScenario(Scenario* sce) {
        throw ModelException("SimulationModule", "SetScenario", "Set scenario function is not implemented.");
    }

    //! Set pointer of clsReaches class which contains all reaches information. Added by LJ, 2016-7-2
    virtual void SetReaches(clsReaches* rches) {
        throw ModelException("SimulationModule", "SetReaches", "Set reaches function is not implemented.");
    }

    //! Set pointer of clsSubbasins class which contains all subbasins information. Added by LJ, 2016-7-28
    virtual void SetSubbasins(clsSubbasins* subbsns) {
        throw ModelException("SimulationModule", "SetSubbasins", "Set subbasins function is not implemented.");
    }

    /*!
     * \brief Check the input data. Make sure all the input data is available.
     *
     *        This function is optional to be overridden.
     *
     * \return bool The validity of the input data.
     */
    virtual bool CheckInputData() { return true; }
    
    virtual bool CheckInputSize(const char* key, int nrows) {
        return true;
    }

    /*!
     * \brief Check data length of the first dimension (i.e., nRows) of the input array-based data
     *
     * \param[in] module_id Module ID used to print exception message
     * \param[in] key the key to identify the requested data
     * \param[in] nrows size of the first dimension
     * \param[out] m_nrows the expected size, if m_nrows less or equal to 0, then m_nrows = mrows
     */
    bool CheckInputSize(const char* key, int nrows, int& m_nrows);

    /*!
     * \brief Check data length of the two dimensions (i.e., nRows and nCols) of the input array-based data
     *
     * \param[in] module_id Module ID used to print exception message
     * \param[in] key the key to identify the requested data
     * \param[in] nrows size of the first dimension
     * \param[in] ncols size of the second dimension
     * \param[out] m_nrows the expected rows size, if m_nrows less or equal to 0, then m_nrows = mrows
     * \param[out] m_ncols the expected cols size, if m_ncols less or equal to 0, then m_ncols = ncols
     */
    virtual bool CheckInputSize2D(const char* key, int nrows, int ncols, int& m_nrows, int& m_ncols);

    /*!
     * \brief Initialize output variables.
     *
     *        This function is optional to be overridden.
     *        Only allocate memory address and initialize outputs.
     */
    virtual void InitialOutputs() {}

    /*!
     * \brief Initialize intermediate parameters for reducing computing amount.
     *
     *        This function is optional to be overridden.
     *        Intermediate parameters only need to be calculated once
     *          and will not change during simulation.
     *        This function must be separated with InitialOutputs().
     *
     *        For example, K*P*LS*11.8*exp(ROCK) in the equation of MUSLE can be
     *          considered as an intermediate parameter.
     *
     */
    virtual void InitialIntermediates() {}

    /*!
     * \brief Get time step type, default is hillslope process.
     *
     *        Remember to OVERRIDE this function to return other time step type for
     *        routing modules and others if necessary.
     */
    virtual TimeStepType GetTimeStepType() {
        return TIMESTEP_HILLSLOPE;
    }
    void SetTimeStep(int dt) {
        m_dt = dt;
        m_dt_day = static_cast<FLTPT>(dt) / 60.0 / 60.0 / 24.0;
    }
    //! Reset subtime step
    virtual void ResetSubTimeStep() {
        m_tsCounter = 1;
    }

    //! Whether the inputs (i.e., inputs derived from other modules) have been set.
    bool IsInputsSetDone() { return m_inputsSetDone; }

    //! Change the status of setting inputs parameters
    void SetInputsDone(const bool set_done) { m_inputsSetDone = set_done; }

    //! set whether intermediate parameters need to recalculated
    void SetReCalIntermediates(const bool recal) { m_reCalIntermediates = recal; }

	// set 1D Array which contains position data of raster
	virtual void SetRasterPositionDataPointer(const char* key, int** positions) {
		throw ModelException("SimulationModule", "SetRasterPositionDataPointer",
			"Set function of parameter " + string(key) + " is not implemented.");
	}

	//virtual void SetRasterRows( int rows) {
	//	throw ModelException("SimulationModule", "SetRasterRows",
	//		"Set function of parameter rows is not implemented.");
	//}

	//virtual void SetRasterCols( int cols) {
	//	throw ModelException("SimulationModule", "SetRasterCols",
	//		"Set function of parameter cols is not implemented.");
	//}

	// set 1D Array which stores reach depth data in each cell of raster
	//virtual void SetReachDepthData(FloatRaster* ch_depth) {
	//	throw ModelException("SimulationModule", "SetReachDepthData",
	//		"Set SetReachDepthData function is not implemented.");
	//}

    FLTPT getDayAngle() {
        FLTPT leap_year = 0.0;
        if (IsLeapYear(m_year)) {
            leap_year = 1.0;
        }
        FLTPT day_angle = 2.0 * M_PI * (m_dayOfYear - 1) / 365.0 + leap_year;
        return day_angle;
    }

    /**
     * \brief Convey the value (e.g. water) from one variable to another.
     * \param from 
     * \param to 
     * \param rate The rate of conveyance, to be multiplied by timestep. Because the unit of rate is by day, like mm/day.
     * \param timestep 0 means use the module's default timestep, otherwise use the given timestep. 1 means 1 day, also used as one-off conveyance.
     * \param realConvey If true, the value will be subtracted from `from`, otherwise not.
     */
    inline FLTPT Convey(FLTPT& from, FLTPT& to, FLTPT rate, FLTPT timestep=0 ,bool realConvey=true) {
        if (timestep <= 0) {
            timestep = m_dt_day;
        }
        FLTPT convey = 0;
        if (rate > 0) {
            convey = Min(rate * timestep, from);
            if (realConvey) { from -= convey; }
            to += convey;
        }
        else {
            convey = Min(-rate * timestep, to);
            if (realConvey) { from += convey; }
            to -= convey;
        }
        return convey;
    }
    inline void Convey(FLTPT* from, FLTPT* to, int m_nCells, FLTPT rate) {
#pragma omp parallel for
        for (int i = 0; i < m_nCells; i++) {
            Convey(from[i], to[i], rate);
        }
    }
    inline void Flush(FLTPT& from, FLTPT& to) {
        to += NonNeg(from);
        from = 0;
    }
    inline void Supply(FLTPT& to, FLTPT rate) {
        to += rate * m_dt_day;
    }

    void SetModuleName(const char* name) {
        m_moduleName = string(name);
    }
    string GetModuleName() {
        return m_moduleName;
    }
protected:
    /// date time
    time_t m_date;
    /// index of current year of simulation, e.g., the simulation period from 2010 to 2015,  m_yearIdx is 2 when simulate 2012.
    int m_yearIdx;
    /// year
    int m_year;
    /// month since January - [1,12]
    int m_month;
    /// day of the month - [1,31]
    int m_day;
    /// day of year - [1, 366]
    int m_dayOfYear;
    /// sub-timestep counter
    int m_tsCounter;
    int m_dt;
    FLTPT m_dt_day;
    /// Whether the inputs parameters (i.e., parameters derived from other modules) have been set.
    bool m_inputsSetDone;
    bool m_outputsInitialized;
    /// need to recalculate intermediate parameters?
    bool m_reCalIntermediates;
private:
    string m_moduleName;

};

/*!
 * Macros for CheckInputData function
 * BE REMEMBER OF SEMICOLON!
 */
//! CHECK_DATA is used for the unforeseen situation
#define CHECK_DATA(moduleID, expression, desc) if ((expression)) \
                   throw ModelException(moduleID, "CheckInputData", string(desc))
//! CHECK_POINTER is used for 1D or 2D raster and other pointer of data
#define CHECK_POINTER(moduleID, param) if (nullptr == (param)) \
                   throw ModelException(moduleID, "CheckInputData", string(#param) + string(" MUST NOT be NULL!"))
//! CHECK_POSITIVE is used for single value that must be positive
#define CHECK_POSITIVE(moduleID, param) if ((param) <= 0) \
                   throw ModelException(moduleID, "CheckInputData", string(#param) + string(" MUST be positive!"))
//! CHECK_NONNEGATIVE is used for single value that must be greater or equal than zero
#define CHECK_NONNEGATIVE(moduleID, param) if ((param) < 0) \
                   throw ModelException(moduleID, "CheckInputData", string(#param) + string(" MUST be greater or equal than zero!"))
//! CHECK_NEGATIVE is used for single value that must be negative
#define CHECK_NEGATIVE(moduleID, param) if ((param) >= 0) \
                   throw ModelException(moduleID, "CheckInputData", string(#param) + string(" MUST be negative!"))
//! CHECK_ZERO is used for single value that must not be ZERO
#define CHECK_ZERO(moduleID, param) if (FloatEqual(CVT_DBL(param), 0.)) \
                   throw ModelException(moduleID, "CheckInputData", string(#param) + string(" MUST NOT be zero!"))
//! CHECK_NODATA is used for single value that must not be NODATA_VALUE
#define CHECK_NODATA(moduleID, param) if (FloatEqual(CVT_DBL(param), NODATA_VALUE)) \
                     throw ModelException(moduleID, "CheckInputData", string(#param) + string(" MUST NOT be NODATA_VALUE!"))

#endif /* SIMULATION_MOUDULE_BASE */
