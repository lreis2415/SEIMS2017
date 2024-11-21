#include "Scenario.h"
#include "Logging.h"

namespace bmps {
Scenario::Scenario(MongoClient* conn, const string& dbName, int subbsnID /* = 0 */,
                   int scenarioID /* = 0 */, time_t startTime/* = -1 */, time_t endTime/* = -1 */) :
    m_conn(conn), m_bmpDBName(dbName), m_sceneID(scenarioID), m_subbsnID(subbsnID),
    m_startTime(startTime), m_endTime(endTime){
    assert(m_sceneID >= 0);
    assert(m_subbsnID >= 0);
    loadScenario();
}

Scenario::~Scenario() {
    CLOG(TRACE, LOG_RELEASE) << "Releasing Scenario...";
    for (auto it = m_bmpFactories.begin(); it != m_bmpFactories.end(); ++it) {
        if (nullptr != it->second) {
            delete it->second;
            it->second = nullptr;
        }
    }
    m_bmpFactories.clear();
}

void Scenario::loadScenario() {
    m_conn->GetCollectionNames(m_bmpDBName, m_bmpCollections);
    loadScenarioName();
    loadBMPs();
    loadBMPDetail();
}

void Scenario::loadScenarioName() {
    auto it = find(m_bmpCollections.begin(), m_bmpCollections.end(), string(TAB_BMP_SCENARIO));
    if (it == m_bmpCollections.end()) {
        throw ModelException("BMP Scenario", "loadScenarioName", "The BMP database '" + m_bmpDBName +
                             "' does not exist or there is not a table named '" +
                             TAB_BMP_SCENARIO + "' in BMP database.");
    }
    mongoc_collection_t* sceCollection = m_conn->GetCollection(m_bmpDBName, TAB_BMP_SCENARIO);
    /// Find the unique scenario name
    bson_t *query = bson_new(), *reply = bson_new();
    query = BCON_NEW("distinct", BCON_UTF8(TAB_BMP_SCENARIO), "key", FLD_SCENARIO_NAME,
                     "query", "{", FLD_SCENARIO_ID, BCON_INT32(m_sceneID), "}");
    bson_iter_t iter, sub_iter;
    bson_error_t err;
    if (mongoc_collection_command_simple(sceCollection, query, NULL, reply, &err)) {
        //cout<<bson_as_json(reply,NULL)<<endl;
        if (bson_iter_init_find(&iter, reply, "values") &&
            (BSON_ITER_HOLDS_DOCUMENT(&iter) || BSON_ITER_HOLDS_ARRAY(&iter)) &&
            bson_iter_recurse(&iter, &sub_iter)) {
            while (bson_iter_next(&sub_iter)) {
                m_name = GetStringFromBsonIterator(&sub_iter);
                break;
            }
        } else {
            throw ModelException("BMP Scenario", "loadScenarioName",
                                 "There is not scenario existed with the ID: " +
                                 ValueToString(m_sceneID) + " in " + TAB_BMP_SCENARIO +
                                 " table in BMP database.");
        }
    }
    bson_destroy(query);
    bson_destroy(reply);
    mongoc_collection_destroy(sceCollection);
}

void Scenario::loadBMPs() {
    auto it = find(m_bmpCollections.begin(), m_bmpCollections.end(), string(TAB_BMP_INDEX));
    if (it == m_bmpCollections.end()) {
        throw ModelException("BMP Scenario", "loadScenarioName", "The BMP database '" + m_bmpDBName +
                             "' does not exist or there is not a table named '" +
                             TAB_BMP_INDEX + "' in BMP database.");
    }
    bson_t* query = BCON_NEW(FLD_SCENARIO_ID, BCON_INT32(m_sceneID));
    //cout<<bson_as_json(query, NULL)<<endl;
    std::unique_ptr<MongoCollection>
            collection(new MongoCollection(m_conn->GetCollection(m_bmpDBName, TAB_BMP_SCENARIO)));
    std::unique_ptr<MongoCollection> collbmpidx(new MongoCollection(m_conn->GetCollection(m_bmpDBName, TAB_BMP_INDEX)));
    mongoc_cursor_t* cursor = collection->ExecuteQuery(query);

    bson_error_t err;
    if (mongoc_cursor_error(cursor, &err)) {
        throw ModelException("BMP Scenario", "loadBMPs",
                             "There are no record with scenario ID: " + ValueToString(m_sceneID));
    }
    const bson_t* info;
    while (mongoc_cursor_more(cursor) && mongoc_cursor_next(cursor, &info)) {
        //cout<<bson_as_json(info,0)<<endl;
        bson_iter_t iter;
        int BMPID = -1;
        int subScenario = -1;
        string distribution;
        string collectionName;
        string location;
        bool effectivenessChangeable = false;
        int tempEffectivenessChangeable = -1;
        int changeFrequency = 0;
        int changeTimes = 0;
        if (bson_iter_init_find(&iter, info, FLD_SCENARIO_BMPID)) GetNumericFromBsonIterator(&iter, BMPID);
        if (bson_iter_init_find(&iter, info, FLD_SCENARIO_SUB)) GetNumericFromBsonIterator(&iter, subScenario);
        if (bson_iter_init_find(&iter, info, FLD_SCENARIO_DIST)) distribution = GetStringFromBsonIterator(&iter);
        if (bson_iter_init_find(&iter, info, FLD_SCENARIO_TABLE)) collectionName = GetStringFromBsonIterator(&iter);
        if (bson_iter_init_find(&iter, info, FLD_SCENARIO_LOCATION)) location = GetStringFromBsonIterator(&iter);
        if (bson_iter_init_find(&iter, info, FLD_SCENARIO_EFFECTIVENESSVARIABLE)) GetNumericFromBsonIterator(&iter, tempEffectivenessChangeable);
        effectivenessChangeable = tempEffectivenessChangeable == 1;
        if (effectivenessChangeable) {
            if (bson_iter_init_find(&iter, info, FLD_SCENARIO_CHANGEFREQUENCY)) GetNumericFromBsonIterator(&iter, changeFrequency);
            // TODO, !!! MUST MODIFY. Or read from database later!
            time_t warmUpPeriod = 31536000;// 1 year
            changeTimes = 5;//(m_endTime - m_startTime - warmUpPeriod) / changeFrequency;
        }

        /// check if raster data is need for the current BMP
        vector<string> dist = SplitString(distribution, '|');
        if (dist.size() >= 2 && StringMatch(dist[0], FLD_SCENARIO_DIST_RASTER)) {
            string gridfs_name = ValueToString(m_subbsnID) + "_" + GetUpper(dist[1]);
            if (m_sceneRsMap.find(gridfs_name) == m_sceneRsMap.end()) {
#ifdef HAS_VARIADIC_TEMPLATES
                m_sceneRsMap.emplace(gridfs_name, nullptr);
#else
                m_sceneRsMap.insert(make_pair(gridfs_name, nullptr));
#endif
            }
            dist[1] = gridfs_name;
        }

        int BMPType = -1;
        int BMPPriority = -1;
        bson_t* queryBMP = BCON_NEW(FLD_BMP_ID, BCON_INT32(BMPID));

        mongoc_cursor_t* cursor2 = collbmpidx->ExecuteQuery(queryBMP);

        if (mongoc_cursor_error(cursor2, &err)) {
            throw ModelException("BMP Scenario", "loadBMPs",
                                 "There are no record with BMP ID: " + ValueToString(BMPID));
        }
        const bson_t* info2;
        while (mongoc_cursor_more(cursor2) && mongoc_cursor_next(cursor2, &info2)) {
            bson_iter_t sub_iter;
            if (bson_iter_init_find(&sub_iter, info2, FLD_BMP_TYPE)) {
                GetNumericFromBsonIterator(&sub_iter, BMPType);
            }
            if (bson_iter_init_find(&sub_iter, info2, FLD_BMP_PRIORITY)) {
                GetNumericFromBsonIterator(&sub_iter, BMPPriority);
            }
        }
        //cout<<BMPID<<","<<BMPType<<","<<distribution<<","<<parameter<<endl;
        bson_destroy(queryBMP);
        mongoc_cursor_destroy(cursor2);
        /// Combine BMPID, and SubScenario for a unique ID to identify "different" BMP
        int uniqueBMPID = BMPID * 100000 + subScenario;
        if (m_bmpFactories.find(uniqueBMPID) == m_bmpFactories.end()) {
#ifdef HAS_VARIADIC_TEMPLATES
            if (BMPID == BMP_TYPE_POINTSOURCE) {
                m_bmpFactories.emplace(uniqueBMPID,
                                       new BMPPointSrcFactory(m_sceneID, BMPID, subScenario,
                                                              BMPType, BMPPriority, dist,
                                                              collectionName, location));
            }
            if (BMPID == BMP_TYPE_PLANT_MGT) {
                m_bmpFactories.emplace(uniqueBMPID,
                                       new BMPPlantMgtFactory(m_sceneID, BMPID, subScenario,
                                                              BMPType, BMPPriority, dist,
                                                              collectionName, location));
            }
            if (BMPID == BMP_TYPE_AREALSOURCE) {
                m_bmpFactories.emplace(uniqueBMPID,
                                       new BMPArealSrcFactory(m_sceneID, BMPID, subScenario,
                                                              BMPType, BMPPriority, dist,
                                                              collectionName, location));
            }
            if (BMPID == BMP_TYPE_AREALSTRUCT) {
                m_bmpFactories.emplace(uniqueBMPID,
                                       new BMPArealStructFactory(m_sceneID, BMPID, subScenario,
                                                                 BMPType, BMPPriority, dist,
                                                                 collectionName, location, effectivenessChangeable,
                                                                 changeFrequency, changeTimes));
            }
#else
            if (BMPID == BMP_TYPE_POINTSOURCE) {
                m_bmpFactories.insert(make_pair(uniqueBMPID,
                                                new BMPPointSrcFactory(m_sceneID, BMPID, subScenario,
                                                                       BMPType, BMPPriority, dist,
                                                                       collectionName, location)));
            }
            if (BMPID == BMP_TYPE_PLANT_MGT) {
                m_bmpFactories.insert(make_pair(uniqueBMPID,
                                                new BMPPlantMgtFactory(m_sceneID, BMPID, subScenario,
                                                                       BMPType, BMPPriority, dist,
                                                                       collectionName, location)));
            }
            if (BMPID == BMP_TYPE_AREALSOURCE) {
                m_bmpFactories.insert(make_pair(uniqueBMPID,
                                                new BMPArealSrcFactory(m_sceneID, BMPID, subScenario,
                                                                       BMPType, BMPPriority, dist,
                                                                       collectionName, location)));
            }
            if (BMPID == BMP_TYPE_AREALSTRUCT) {
                m_bmpFactories.insert(make_pair(uniqueBMPID,
                                                new BMPArealStructFactory(m_sceneID, BMPID, subScenario,
                                                                          BMPType, BMPPriority, dist,
                                                                          collectionName, location)));
            }
#endif
        }
    }
    bson_destroy(query);
    mongoc_cursor_destroy(cursor);
}

void Scenario::loadBMPDetail() {
    for (auto it = m_bmpFactories.begin(); it != m_bmpFactories.end(); ++it) {
        it->second->loadBMP(m_conn, m_bmpDBName);
    }
}

void Scenario::Dump(string& fileName) {
    std::ofstream fs;
    fs.open(fileName.c_str(), std::ios::ate);
    if (fs.is_open()) {
        Dump(&fs);
        fs.close();
    }
}

void Scenario::Dump(std::ostream* fs) {
    if (fs == nullptr) return;
    *fs << "Scenario ID:" << m_sceneID << endl;
    *fs << "Name:" << m_name << endl;
    *fs << "*** All the BMPs ***" << endl;
    for (auto it = m_bmpFactories.begin(); it != m_bmpFactories.end(); ++it) {
        if (it->second != nullptr) it->second->Dump(fs);
    }
}

void Scenario::setRasterForEachBMP() {
    if (m_sceneRsMap.empty()) return;
    for (auto it = m_bmpFactories.begin(); it != m_bmpFactories.end(); ++it) {
        it->second->setRasterData(m_sceneRsMap);
    }
}
} /* MainBMP */
