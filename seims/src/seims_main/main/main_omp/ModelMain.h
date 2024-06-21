/*!
 * \file ModelMain.h
 * \brief Control the simulation of SEIMS
 *
 * Changelog:
 *   - 1. 2017-05-20 - lj - Refactoring. The ModelMain class mainly focuses on the entire workflow.
 *
 * \author Junzhi Liu, LiangJun Zhu
 * \version 2.0
 */
#ifndef SEIMS_MODEL_MAIN_H
#define SEIMS_MODEL_MAIN_H

// include build-in libs
#include <string>
#include <ctime>
#include <memory>

#include "basic.h"
#include "db_mongoc.h"
#include "data_raster.hpp"

// include utility classes and const definition of SEIMS
#include "seims.h"
// include data related
#ifdef USE_MONGODB
#include "DataCenterMongoDB.h"
#endif /* USE_MONGODB */
#include "SettingsInput.h"
#include "SettingsOutput.h"
// include module_setting related
#include "ModuleFactory.h"

/*!
 * \class ModelMain
 * \ingroup seims_omp
 * \brief SEIMS OpenMP version, Class to control the whole model
 */
class ModelMain: Interface {
public:
    /*!
     * \brief Constructor independent to any database IO, instead of the DataCenter object
     * \param[in] data_center DataCenter, DataCenterMongoDB, or others in future
     * \param[in] factory ModuleFactory, assemble the module workspace
     */
    ModelMain(DataCenterMongoDB* data_center, ModuleFactory* factory);

    //! Execute all the modules, aggregate output data, and write the total time-consuming, etc.
    void Execute();

    //! Write output files, e.g., Q.txt, return time-consuming (s).
    double Output();

    /*!
    * \brief Check whether the validation of outputs
    * 1. The output id should be valid for modules in config files;
    * 2. The date range should be in the data range of file.in;
    */
    void CheckAvailableOutput();
    /*!
     * \brief Append output data to Output Item by the corresponding aggregation type
     * \param[in] time Current simulation time
     */
    void AppendOutputData(time_t time);
    /*!
     * \brief Print execution time on the screen
     */
    void OutputExecuteTime();
    /*!
     * \brief Execute hillslope modules in current time
     * \param[in] t Current time
     * \param[in] year_idx Year index of the entire simulation period
     * \param[in] sub_index Time step index of the entire simulation period
     */
    void StepHillSlope(time_t t, int year_idx, int sub_index);
    /*!
     * \brief Execute channel modules in current time
     * \param[in] t Current time
     * \param[in] year_idx Year index of the entire simulation period
     */
    void StepChannel(time_t t, int year_idx);
    /*!
     * \brief Execute overall modules in the entire simulation period, e.g., COST module.
     * \param[in] start_t Start time period
     * \param[in] end_t End time period
     */
    void StepOverall(time_t start_t, time_t end_t);

    void GetTransferredValue(FLTPT* tfvalues);

    void SetTransferredValue(int index, const FLTPT* tfvalues);

public:
    /************************************************************************/
    /*            Get functions for MPI version                             */
    /************************************************************************/

    //! Get module counts of current SEIMS
    int GetModuleCount() const { return CVT_INT(m_simulationModules.size()); }
    //! Get module ID by index in ModuleFactory
    string GetModuleID(const int i) const { return m_factory->GetModuleID(i); }
    //! Get module execute time by index in ModuleFactory
    double GetModuleExecuteTime(const int i) const { return m_executeTime[i]; }
    //! Get time consuming of read data
    double GetReadDataTime() const { return m_readFileTime; }
    //! Include channel processes or not?
    bool IncludeChannelProcesses() { return !m_channelModules.empty(); }

private:
    /************************************************************************/
    /*             Input parameters                                         */
    /************************************************************************/

    DataCenterMongoDB* m_dataCenter; ///< inherited DataCenter
    ModuleFactory* m_factory;        ///< Modules factory
private:
    /************************************************************************/
    /*   Pointer or reference of object and data derived from input params  */
    /************************************************************************/

    SettingsInput* m_input;              ///< The basic input settings
    SettingsOutput* m_output;            ///< The user-defined outputs, Q, SED, etc
    IntRaster* m_maskRaster;             ///< Mask raster data
    string m_outputPath;                 ///< Path of output scenario
    time_t m_dtDaily;                    ///< Daily time interval, seconds
    time_t m_dtHs;                       ///< Hillslope time interval, seconds
    time_t m_dtCh;                       ///< Channel time interval, seconds
    vector<string> m_moduleIDs;          ///< Module unique IDs, the same sequences with #m_simulationModules
    vector<ParamInfo<FLTPT>*> m_tfValueInputs; ///< transferred single value across subbasins
private:
    /************************************************************************/
    /*   Variables newly allocated in this class                            */
    /************************************************************************/

    double m_readFileTime;                          ///< Time consuming for read data
    vector<SimulationModule *> m_simulationModules; ///< Modules list in the model run
    vector<int> m_hillslopeModules;                 ///< Hillslope modules index list
    vector<int> m_channelModules;                   ///< Channel modules index list
    vector<int> m_overallModules;                   ///< Whole simulation scale modules index list
    vector<double> m_executeTime;                   ///< Execute time list of each module

    int m_nTFValues;                     ///< transferred value inputs cout
    vector<int> m_tfValueFromModuleIdxs; ///< from module index corresponding to each transferred value inputs
    vector<int> m_tfValueToModuleIdxs;   ///< to module index corresponding to each transferred value inputs
    vector<string> m_tfValueNames;       ///< parameter name corresponding to each transferred value inputs

    bool m_firstRunOverland; ///< Is the first run of overland
    bool m_firstRunChannel;  ///< Is the first run of channel
};
#endif /* SEIMS_MODEL_MAIN_H */
