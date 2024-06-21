#include "BMPPlantMgtFactory.h"

#include <memory> // unique_ptr

#include "utils_string.h"

using namespace utils_string;
using namespace bmps;

BMPPlantMgtFactory::BMPPlantMgtFactory(const int scenarioId, const int bmpId, const int subScenario,
                                       const int bmpType, const int bmpPriority,
                                       vector<string>& distribution,
                                       const string& collection, const string& location) :
    BMPFactory(scenarioId, bmpId, subScenario, bmpType, bmpPriority, distribution, collection, location),
    m_mgtFieldsRs(nullptr), m_luccID(-1), m_parameters(nullptr) {
    if (m_distribution.size() >= 2 && StringMatch(m_distribution[0], FLD_SCENARIO_DIST_RASTER)) {
        m_mgtFieldsName = m_distribution[1];
    } else {
        throw ModelException("BMPPlantMgtFactory", "Initialization",
                             "The distribution field must follow the format: "
                             "RASTER|CoreRasterName.\n");
    }
    if (StringMatch(location, "ALL")) {
        m_location.clear();
    } else {
        vector<int> tmp_locations;
        SplitStringForValues(location, '-', tmp_locations);
        for (vector<int>::iterator it = tmp_locations.begin(); it != tmp_locations.end(); ++it) {
            m_location.insert(*it);
        }
    }
}

BMPPlantMgtFactory::~BMPPlantMgtFactory() {
    Release1DArray(m_parameters);
    for (auto it = m_bmpPlantOps.begin(); it != m_bmpPlantOps.end(); ++it) {
        if (nullptr != it->second) {
            delete it->second;
            it->second = nullptr;
        }
    }
    m_bmpPlantOps.clear();
}

void BMPPlantMgtFactory::loadBMP(MongoClient* conn, const string& bmpDBName) {
//    bson_t* b = bson_new();
//    bson_t *child1 = bson_new(), *child2 = bson_new();
//    BSON_APPEND_DOCUMENT_BEGIN(b, "$query", child1);
//    BSON_APPEND_INT32(child1, FLD_SCENARIO_SUB, m_subScenarioId);
//    bson_append_document_end(b, child1);
//    BSON_APPEND_DOCUMENT_BEGIN(b, "$orderby", child2);
//    BSON_APPEND_INT32(child2, BMP_FLD_SEQUENCE, 1);
//    bson_append_document_end(b, child2);
//    bson_destroy(child1);
//    bson_destroy(child2);

    bson_t* b = BCON_NEW(FLD_SCENARIO_SUB, BCON_INT32(m_subScenarioId));
    bson_t* opts = BCON_NEW("sort", "{", BMP_FLD_SEQUENCE, BCON_INT32(1), "}");

    std::unique_ptr<MongoCollection> collection(new MongoCollection(conn->GetCollection(bmpDBName, m_bmpCollection)));
    mongoc_cursor_t* cursor = collection->ExecuteQuery(b, opts);

    const bson_t* bsonTable;
    bson_iter_t itertor;
    int paramNum = 10;
    m_parameters = new FLTPT[paramNum];
    int count = 1; /// Use count to counting sequence number, in case of discontinuous of SEQUENCE in database.
    while (mongoc_cursor_next(cursor, &bsonTable)) {
        //int seqNo;
        int mgtCode = -1, year = -9999, month = -9999, day = -9999;
        FLTPT husc = 0.;
        bool usebaseHU = true;
        if (bson_iter_init_find(&itertor, bsonTable, BMP_FLD_NAME)) {
            m_name = GetStringFromBsonIterator(&itertor);
        }
        if (bson_iter_init_find(&itertor, bsonTable, BMP_PLTOP_FLD_LUCC)) {
            GetNumericFromBsonIterator(&itertor, m_luccID);
        }
        if (m_luccID < 0) m_luccID = 1; // AGRL	Agricultural Land-Generic
        if (bson_iter_init_find(&itertor, bsonTable, BMP_PLTOP_FLD_MGTOP)) {
            GetNumericFromBsonIterator(&itertor, mgtCode);
        }
        //if (bson_iter_init_find(&itertor,bsonTable,BMP_FLD_SEQUENCE))
        //	GetNumericFromBsonIterator(&itertor, seqNo);
        if (bson_iter_init_find(&itertor, bsonTable, BMP_PLTOP_FLD_YEAR)) {
            GetNumericFromBsonIterator(&itertor, year);
        }
        if (bson_iter_init_find(&itertor, bsonTable, BMP_PLTOP_FLD_MONTH)) {
            GetNumericFromBsonIterator(&itertor, month);
        }
        if (bson_iter_init_find(&itertor, bsonTable, BMP_PLTOP_FLD_DAY)) {
            GetNumericFromBsonIterator(&itertor, day);
        }
        if (bson_iter_init_find(&itertor, bsonTable, BMP_PLTOP_FLD_BASEHU)) {
            usebaseHU = GetBoolFromBsonIterator(&itertor);
        }
        if (bson_iter_init_find(&itertor, bsonTable, BMP_PLTOP_FLD_HUSC)) {
            GetNumericFromBsonIterator(&itertor, husc);
        }
        for (int i = 0; i < paramNum; i++) {
            std::ostringstream oss;
            oss << BMP_PLTOP_FLD_MGT_PRE << (i + 1);
            if (bson_iter_init_find(&itertor, bsonTable, oss.str().c_str())) {
                GetNumericFromBsonIterator(&itertor, m_parameters[i]);
            }
        }
        int uniqueMgtCode = count * 1000 + mgtCode;
        m_bmpSequence.emplace_back(uniqueMgtCode);
        switch (mgtCode) {
            case BMP_PLTOP_Plant:
#ifdef HAS_VARIADIC_TEMPLATES
                m_bmpPlantOps.emplace(uniqueMgtCode, new PltOp(mgtCode, usebaseHU, husc,
                                                               year, month, day, m_parameters));
#else
                m_bmpPlantOps.insert(make_pair(uniqueMgtCode,
                                               new PltOp(mgtCode, usebaseHU, husc,
                                                         year, month, day, m_parameters)));
#endif
                break;
            case BMP_PLTOP_Irrigation:
#ifdef HAS_VARIADIC_TEMPLATES
                m_bmpPlantOps.emplace(uniqueMgtCode, new IrrOp(mgtCode, usebaseHU, husc,
                                                               year, month, day, m_parameters));
#else
                m_bmpPlantOps.insert(make_pair(uniqueMgtCode,
                                               new IrrOp(mgtCode, usebaseHU, husc,
                                                         year, month, day, m_parameters)));
#endif
                break;
            case BMP_PLTOP_Fertilizer:
#ifdef HAS_VARIADIC_TEMPLATES
                m_bmpPlantOps.emplace(uniqueMgtCode, new FertOp(mgtCode, usebaseHU, husc,
                                                                year, month, day, m_parameters));
#else
                m_bmpPlantOps.insert(make_pair(uniqueMgtCode,
                                               new FertOp(mgtCode, usebaseHU, husc,
                                                          year, month, day, m_parameters)));
#endif
                break;
            case BMP_PLTOP_Pesticide:
#ifdef HAS_VARIADIC_TEMPLATES
                m_bmpPlantOps.emplace(uniqueMgtCode, new PestOp(mgtCode, usebaseHU, husc,
                                                                year, month, day, m_parameters));
#else
                m_bmpPlantOps.insert(make_pair(uniqueMgtCode,
                                               new PestOp(mgtCode, usebaseHU, husc,
                                                          year, month, day, m_parameters)));
#endif
                break;
            case BMP_PLTOP_HarvestKill:
#ifdef HAS_VARIADIC_TEMPLATES
                m_bmpPlantOps.emplace(uniqueMgtCode, new HvstKillOp(mgtCode, usebaseHU, husc,
                                                                    year, month, day, m_parameters));
#else
                m_bmpPlantOps.insert(make_pair(uniqueMgtCode,
                                               new HvstKillOp(mgtCode, usebaseHU, husc,
                                                              year, month, day, m_parameters)));
#endif
                break;
            case BMP_PLTOP_Tillage:
#ifdef HAS_VARIADIC_TEMPLATES
                m_bmpPlantOps.emplace(uniqueMgtCode, new TillOp(mgtCode, usebaseHU, husc,
                                                                year, month, day, m_parameters));
#else
                m_bmpPlantOps.insert(make_pair(uniqueMgtCode,
                                               new TillOp(mgtCode, usebaseHU, husc,
                                                          year, month, day, m_parameters)));
#endif
                break;
            case BMP_PLTOP_Harvest:
#ifdef HAS_VARIADIC_TEMPLATES
                m_bmpPlantOps.emplace(uniqueMgtCode, new HvstOnlyOp(mgtCode, usebaseHU, husc,
                                                                    year, month, day, m_parameters));
#else
                m_bmpPlantOps.insert(make_pair(uniqueMgtCode,
                                               new HvstOnlyOp(mgtCode, usebaseHU, husc,
                                                              year, month, day, m_parameters)));
#endif
                break;
            case BMP_PLTOP_Kill:
#ifdef HAS_VARIADIC_TEMPLATES
                m_bmpPlantOps.emplace(uniqueMgtCode, new KillOp(mgtCode, usebaseHU, husc,
                                                                year, month, day, m_parameters));
#else
                m_bmpPlantOps.insert(make_pair(uniqueMgtCode,
                                               new KillOp(mgtCode, usebaseHU, husc,
                                                          year, month, day, m_parameters)));
#endif
                break;
            case BMP_PLTOP_Grazing:
#ifdef HAS_VARIADIC_TEMPLATES
                m_bmpPlantOps.emplace(uniqueMgtCode, new GrazOp(mgtCode, usebaseHU, husc,
                                                                year, month, day, m_parameters));
#else
                m_bmpPlantOps.insert(make_pair(uniqueMgtCode,
                                               new GrazOp(mgtCode, usebaseHU, husc,
                                                          year, month, day, m_parameters)));
#endif
                break;
            case BMP_PLTOP_AutoIrrigation:
#ifdef HAS_VARIADIC_TEMPLATES
                m_bmpPlantOps.emplace(uniqueMgtCode, new AutoIrrOp(mgtCode, usebaseHU, husc,
                                                                   year, month, day, m_parameters));
#else
                m_bmpPlantOps.insert(make_pair(uniqueMgtCode,
                                               new AutoIrrOp(mgtCode, usebaseHU, husc,
                                                             year, month, day, m_parameters)));
#endif
                break;
            case BMP_PLTOP_AutoFertilizer:
#ifdef HAS_VARIADIC_TEMPLATES
                m_bmpPlantOps.emplace(uniqueMgtCode, new AutoFertOp(mgtCode, usebaseHU, husc,
                                                                    year, month, day, m_parameters));
#else
                m_bmpPlantOps.insert(make_pair(uniqueMgtCode,
                                               new AutoFertOp(mgtCode, usebaseHU, husc,
                                                              year, month, day, m_parameters)));
#endif
                break;
            case BMP_PLTOP_ReleaseImpound:
#ifdef HAS_VARIADIC_TEMPLATES
                m_bmpPlantOps.emplace(uniqueMgtCode, new RelImpndOp(mgtCode, usebaseHU, husc,
                                                                    year, month, day, m_parameters));
#else
                m_bmpPlantOps.insert(make_pair(uniqueMgtCode,
                                               new RelImpndOp(mgtCode, usebaseHU, husc,
                                                              year, month, day, m_parameters)));
#endif
                break;
            case BMP_PLTOP_ContinuousFertilizer:
#ifdef HAS_VARIADIC_TEMPLATES
                m_bmpPlantOps.emplace(uniqueMgtCode, new ContFertOp(mgtCode, usebaseHU, husc,
                                                                    year, month, day, m_parameters));
#else
                m_bmpPlantOps.insert(make_pair(uniqueMgtCode,
                                               new ContFertOp(mgtCode, usebaseHU, husc,
                                                              year, month, day, m_parameters)));
#endif
                break;
            case BMP_PLTOP_ContinuousPesticide:
#ifdef HAS_VARIADIC_TEMPLATES
                m_bmpPlantOps.emplace(uniqueMgtCode, new ContPestOp(mgtCode, usebaseHU, husc,
                                                                    year, month, day, m_parameters));
#else
                m_bmpPlantOps.insert(make_pair(uniqueMgtCode,
                                               new ContPestOp(mgtCode, usebaseHU, husc,
                                                              year, month, day, m_parameters)));
#endif
                break;
            case BMP_PLTOP_Burning:
#ifdef HAS_VARIADIC_TEMPLATES
                m_bmpPlantOps.emplace(uniqueMgtCode, new BurnOp(mgtCode, usebaseHU, husc,
                                                                year, month, day, m_parameters));
#else
                m_bmpPlantOps.insert(make_pair(uniqueMgtCode,
                                               new BurnOp(mgtCode, usebaseHU, husc,
                                                          year, month, day, m_parameters)));
#endif
                break;
            default: break;
        }
        count++;
    }
    bson_destroy(b);
    bson_destroy(opts);
    mongoc_cursor_destroy(cursor);
}

void BMPPlantMgtFactory::Dump(std::ostream* fs) {
    if (nullptr == fs) return;
    *fs << "Plant Management Factory: " << endl <<
            "    SubScenario ID: " << m_subScenarioId << " Name = " << m_name << endl;
    for (auto it = m_bmpSequence.begin(); it != m_bmpSequence.end(); ++it) {
        auto findIdx = m_bmpPlantOps.find(*it);
        if (findIdx != m_bmpPlantOps.end()) {
            m_bmpPlantOps[*it]->dump(fs);
        }
    }
}

void BMPPlantMgtFactory::setRasterData(map<string, IntRaster*>& sceneRsMap) {
    if (sceneRsMap.find(m_mgtFieldsName) != sceneRsMap.end()) {
        int n;
        sceneRsMap.at(m_mgtFieldsName)->GetRasterData(&n, &m_mgtFieldsRs);
    } else {
        // raise Exception?
    }
}

int* BMPPlantMgtFactory::GetRasterData() {
    return m_mgtFieldsRs;
}

int BMPPlantMgtFactory::GetLUCCID() {
    return m_luccID;
}

set<int>& BMPPlantMgtFactory::GetLocations() {
    return m_location;
}

vector<int>& BMPPlantMgtFactory::GetOperationSequence() {
    return m_bmpSequence;
}

map<int, PltMgtOp *>& BMPPlantMgtFactory::GetOperations() {
    return m_bmpPlantOps;
}

PltMgtOp* BMPPlantMgtFactory::GetOperation(const int ID) {
    return m_bmpPlantOps.at(ID);
}
