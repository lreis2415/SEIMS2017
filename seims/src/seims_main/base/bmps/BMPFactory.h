/*!
 * \file BMPFactory.h
 * \brief Base namespace for implementation of BMP configuration
 *
 * Changelog:
 *   - 1. 2018-04-12 - lj - Code reformat.
 *
 * \author Liangjun Zhu
 */
#ifndef SEIMS_BMP_FACTORY_H
#define SEIMS_BMP_FACTORY_H

#include "db_mongoc.h"
#include "data_raster.hpp"

#include "seims.h"

using namespace ccgl;
using namespace db_mongoc;
using namespace data_raster;

/*!
 * \namespace bmps
 * \brief All BMPs scenario related data, classes, and functions.
 * \ingroup bmps
 */
namespace bmps {
/*!
 * \class bmps::BMPFactory
 * \brief Base class of all kind of BMPs Factory.
 *        Read from BMP_SCENARIOS collection of MongoDB
 */
class BMPFactory: Interface {
public:
    /// Constructor
    BMPFactory(int scenario_id, int bmp_id, int sub_scenario, int bmp_type,
               int bmp_priority, vector<string>& distribution, const string& collection,
               const string& location, bool effectivenessChangeable = false,
               time_t changeFrequency = -1, int variableTimes = -1);
    
    /// Destructor
    ~BMPFactory();

    /// Load BMP parameters from MongoDB
    virtual void loadBMP(MongoClient* conn, const string& bmpDBName) = 0;

    /*!
     * \brief Set raster data if needed
     * This function is not required for each BMP, so DO NOT define as pure virtual function.
     * i.e., DO NOT CHANGE THE DEFINITION!!!
     */
    virtual void setRasterData(map<string, IntRaster *>& sceneRsMap);

    /*!
    * \brief Get raster data if needed
    * This function is not required for each BMP, so DO NOT define as pure virtual function.
    */
    virtual int* GetRasterData();

    /*!  Get BMP type
       1 - reach BMPs which are attached to specific reaches and will change the character of the reach.
       2 - areal structural BMPs which are corresponding to a specific structure in the watershed and will change the character of subbasins/cells.
       3 - areal non-structure BMPs which are NOT corresponding to a specific structure in the watershed and will change the character of subbasins/cells.
       4 - point structural BMPs
     */
    int bmpType();

    /// Get BMP priority
    int bmpPriority();

    /// Get subScenario ID
    int GetSubScenarioId();

    /// Output
    virtual void Dump(std::ostream* fs) = 0;

    bool IsEffectivenessChangeable();
    
    time_t GetChangeFrequency();
    
    int GetChangeTimes();

protected:
    const int m_scenarioId; ///< Scenario ID
    const int m_bmpId; ///< BMP ID
    const int m_subScenarioId; ///< SubScenario ID within one BMP iD
    const int m_bmpType; ///< BMP Type
    const int m_bmpPriority; ///< BMP Priority
    /*! Distribution vector of BMP
     *  Origin format is [distribution data type]|[distribution parameter name]|Collection name|...
     */
    vector<string> m_distribution;
    const string m_bmpCollection; ///< Collection name
    const string m_location; ///< Define where the BMP will be applied
    const bool m_effectivenessChangeable;
    const time_t m_changeFrequency;
    const int m_changeTimes;
};
}
#endif /* SEIMS_BMP_FACTORY_H */
