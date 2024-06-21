#include "BMPArealStructFactory.h"

#include "utils_string.h"

#include "BMPText.h"
#include "Logging.h"

using namespace utils_string;
using namespace bmps;

BMPArealStruct::BMPArealStruct(const bson_t*& bsonTable, bson_iter_t& iter): m_id(-1), m_lastUpdateTime(-1){
    if (bson_iter_init_find(&iter, bsonTable, BMP_FLD_SUB)) {
        GetNumericFromBsonIterator(&iter, m_id);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_FLD_NAME)) {
        m_name = GetStringFromBsonIterator(&iter);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_ARSTRUCT_FLD_DESC)) {
        m_desc = GetStringFromBsonIterator(&iter);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_ARSTRUCT_FLD_REF)) {
        m_refer = GetStringFromBsonIterator(&iter);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_ARSTRUCT_FLD_LANDUSE)) {
        string landuse_str = GetStringFromBsonIterator(&iter);
        SplitStringForValues(landuse_str, '-', m_landuse);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_ARSTRUCT_FLD_PARAMS)) {
        string params_str = GetStringFromBsonIterator(&iter);
        vector<string> params_strs = SplitString(params_str, '-');
        for (auto it = params_strs.begin(); it != params_strs.end(); ++it) {
            vector<string> tmp_param_items = SplitString(*it, ':');
            assert(tmp_param_items.size() == 4);
            ParamInfo<FLTPT>* p = new ParamInfo<FLTPT>();
            p->Name = tmp_param_items[0];
            p->Description = tmp_param_items[1];
            p->Change = tmp_param_items[2]; /// can be "RC", "AC", "NC", "VC", and "".
            vector<string> impactsStrings = SplitString(tmp_param_items[3],'|');
            FLTPT lastImpact = 1.;

            // use absolute value directly
            //for (auto impactStrIt = impactsStrings.begin(); impactStrIt != impactsStrings.end(); ++impactStrIt){
            //    lastImpact = CVT_FLT(ToDouble((*impactStrIt).c_str()));
            //    p->ImpactSeries.push_back(lastImpact);
            //}

            // convert absolute impact value to relative impact value
            for (auto impactStrIt = impactsStrings.begin(); impactStrIt != impactsStrings.end(); ++impactStrIt) {
                if (impactStrIt == impactsStrings.begin()) {
                    lastImpact = ToDouble((*impactStrIt).c_str());
                    p->ImpactSeries.push_back(lastImpact);
                }
                else{
                    FLTPT temp = ToDouble((*impactStrIt).c_str());
                    p->ImpactSeries.push_back(temp/lastImpact);
                    lastImpact = temp;
                }
            }
            p->Impact = p->ImpactSeries[0];//For compatibility with previous versions
            cout << "BMPID: " << m_id << ", param_name: " << tmp_param_items[0] << ",value: " <<p->Impact<< endl;
#ifdef HAS_VARIADIC_TEMPLATES
            if (!m_parameters.emplace(GetUpper(p->Name), p).second) {
#else
            if (!m_parameters.insert(make_pair(GetUpper(p->Name), p)).second) {
#endif
                cout << "WARNING: Load parameter during constructing BMPArealStructFactory, BMPID: "
                        << m_id << ", param_name: " << tmp_param_items[0] << endl;
            }
        }
    }
}

BMPArealStruct::~BMPArealStruct() {
    CLOG(TRACE, LOG_RELEASE) << "---release map of parameters in BMPArealStruct ...";
    for (auto it = m_parameters.begin(); it != m_parameters.end(); ++it) {
        if (nullptr != it->second) {
            CLOG(TRACE, LOG_RELEASE) << "-----" << it->first + " ...";
            delete it->second;
            it->second = nullptr;
        }
    }
    m_parameters.clear();
}

BMPArealStructFactory::BMPArealStructFactory(const int scenarioId, const int bmpId, const int subScenario,
                                             const int bmpType, const int bmpPriority, vector<string>& distribution,
                                             const string& collection, const string& location, bool effectivenessChangeable,
                                             time_t changeFrequency, int variableTimes) :
    BMPFactory(scenarioId, bmpId, subScenario, bmpType, bmpPriority, distribution, collection, location,
               effectivenessChangeable, changeFrequency, variableTimes),
    m_mgtFieldsRs(nullptr),m_unitIDsSeries(m_changeTimes),m_unitUpdateTimes(m_changeTimes),m_seriesIndex(0) {
    if (m_distribution.size() >= 2 && StringMatch(m_distribution[0], FLD_SCENARIO_DIST_RASTER)) {
        m_mgtFieldsName = m_distribution[1];
    } else {
        throw ModelException("BMPArealStructFactory", "Initialization",
                             "The distribution field must follow the format: "
                             "RASTER|CoreRasterName.\n");
    }
    if (m_effectivenessChangeable) {
        vector<string> tempLocations = SplitString(location, '-');
        for (vector<string>::iterator it = tempLocations.begin();it!=tempLocations.end();it++)
        {
            vector<int> temp;
            SplitStringForValues(*it, '|', temp);
            int loc = temp[0];
            int time = temp[1]-1; // year index start from 0
            for (int t = time; t < m_changeTimes; t++)
            {
                m_unitIDsSeries[t].push_back(loc);
                m_unitUpdateTimes[t].insert(std::make_pair(loc,t-time));
            }
        }
    }
    else{
        SplitStringForValues(location, '-', m_unitIDs);
    }
}

BMPArealStructFactory::~BMPArealStructFactory() {
    // m_mgtFieldsRs will be released in DataCenter. No need to be released here.
    for (auto it = m_bmpStructMap.begin(); it != m_bmpStructMap.end(); ++it) {
        if (nullptr != it->second) {
            delete it->second;
            it->second = nullptr;
        }
    }
    m_bmpStructMap.clear();
}

void BMPArealStructFactory::loadBMP(MongoClient* conn, const string& bmpDBName) {
//    bson_t* b = bson_new();
//    bson_t* child1 = bson_new();
//    BSON_APPEND_DOCUMENT_BEGIN(b, "$query", child1);
//    BSON_APPEND_INT32(child1, BMP_FLD_SUB, m_subScenarioId);
//    bson_append_document_end(b, child1);
//    bson_destroy(child1);

    bson_t* b = BCON_NEW(BMP_FLD_SUB, BCON_INT32(m_subScenarioId));

    std::unique_ptr<MongoCollection> collection(new MongoCollection(conn->GetCollection(bmpDBName, m_bmpCollection)));
    mongoc_cursor_t* cursor = collection->ExecuteQuery(b);

    bson_iter_t iter;
    const bson_t* bsonTable;

    /// Use count to counting sequence number, in case of discontinuous or repeat of SEQUENCE in database.
    while (mongoc_cursor_next(cursor, &bsonTable)) {
#ifdef HAS_VARIADIC_TEMPLATES
        if (!m_bmpStructMap.emplace(m_subScenarioId, new BMPArealStruct(bsonTable, iter)).second) {
#else
        if (!m_bmpStructMap.insert(make_pair(m_subScenarioId, new BMPArealStruct(bsonTable, iter))).second) {
#endif
            cout << "WARNING: Read Areal Structural BMP failed: subScenarioID: " << m_subScenarioId << endl;
        }
    }
    bson_destroy(b);
    mongoc_cursor_destroy(cursor);
}

void BMPArealStructFactory::setRasterData(map<string, IntRaster*>& sceneRsMap) {
    if (sceneRsMap.find(m_mgtFieldsName) != sceneRsMap.end()) {
        int n;
        sceneRsMap.at(m_mgtFieldsName)->GetRasterData(&n, &m_mgtFieldsRs);
    } else {
        // raise Exception?
    }
}

void BMPArealStructFactory::Dump(std::ostream* fs) {
    if (nullptr == fs) return;
    *fs << "Areal Structural BMP Management Factory: " << endl <<
            "    SubScenario ID: " << m_subScenarioId << endl;
}
