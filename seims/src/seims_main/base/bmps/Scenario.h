/*!
 * \file Scenario.h
 * \brief Scenario class in BMP database
 *
 * Changelog:
 *   - 1. 2016-06-16 - lj - Replaced SQLite by MongoDB to manager BMP scenario data.
 *
 * \author Liang-Jun Zhu
 */
#ifndef SEIMS_SCENARIO_H
#define SEIMS_SCENARIO_H

#include "basic.h"
#include "db_mongoc.h"
#include "BMPText.h"
#include "BMPFactory.h"
#include "BMPPlantMgtFactory.h"
#include "BMPPointSourceFactory.h"
#include "BMPArealSourceFactory.h"
#include "BMPArealStructFactory.h"

using namespace ccgl;

/*!
 * \namespace bmps
 * \ingroup bmps
 * \brief Namespace for all BMP related
 */
namespace bmps {
/*!
 * \class Scenario
 * \brief Main class of scenario in BMP database
 *
 * Scenario contains a collection of BMPFactory.
 * Each BMPFactory is corresponding to one type of BMP.
 *
 * Usage:
 *   - 1. Instantiate the class.
 *   - 2. Invoke DataCenter::SetRasterForScenario() function to set raster data if needed.
 *   - 3. Invoke setRasterForEachBMP().
 *   - 4. Set as an input parameter for module use.
 *
 * Revised:
 *   - 1. Replaced SQLite by MongoDB, 2016-6-16.
 *   - 2. Add setRasterForEachBMP() function, 2017-7-12.
 */
class Scenario: Interface {
public:
    //! Constructor according to BMP database name and scenario ID
    Scenario(MongoClient* conn, const string& dbName, int subbsnID = 0, int scenarioID = 0,
        time_t startTime = -1, time_t endTime = -1);

    //! Destructor
    ~Scenario();

    //! Get scenario ID, base scenario iD is 0
    int ID() const { return m_sceneID; }

    //! If this is base scenario
    bool IsBaseScenario() { return m_sceneID == 0; }

    //! Get scenario name
    string Name() { return m_name; };

    //! Get BMPs Factories
    map<int, BMPFactory *>& GetBMPFactories() { return m_bmpFactories; }

    //! Write all BMPs information of this scenario to a text file
    void Dump(string& fileName);

    //! Output all BMPs information of this scenario to ostream
    void Dump(std::ostream* fs);

    //! get scenario required raster map. DO NOT DEFINE AS CONST FUNCTION, SINCE m_sceneRsMap WILL BE CHANGED ELSEWHERE!
    map<string, IntRaster *>& getSceneRasterDataMap() { return m_sceneRsMap; }

    //! set raster data for BMPs
    void setRasterForEachBMP();


private:
    /*!
     * \brief Map of BMPs Factory
     *        the Key is unique BMP ID, and the value is BMPFactory
     */
    map<int, BMPFactory *> m_bmpFactories;
    /*!
     * \brief Map of spatial data of scenario data, both 1D and 2D
     */
    map<string, IntRaster *> m_sceneRsMap;

    /// Load scenario information
    void loadScenario();

    /// Get scenario name
    void loadScenarioName();

    /// Load each BMP in current scenario
    void loadBMPs();

    /// Load a single BMP information via BMPFactory
    void loadBMPDetail();

private:
    //! MongoDB client object, added by Liangjun
    MongoClient* m_conn;
    //! MongoDB name of BMP
    string m_bmpDBName;
    //! Collections in BMP database used for data checking
    vector<string> m_bmpCollections;
    //! Scenario ID, e.g., 0
    int m_sceneID;
    //! Scenario Name, e.g., base scenario
    string m_name;
    //! Subbasin ID, 0 for the entire basin
    int m_subbsnID;
    //! the start time of scenario simulation
    time_t m_startTime;
    //! the start time of scenario simulation
    time_t m_endTime;
};

} /* MainBMP */
#endif /* SEIMS_SCENARIO_H */
