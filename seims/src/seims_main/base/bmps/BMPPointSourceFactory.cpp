#include "BMPPointSourceFactory.h"

#include <memory> // unique_ptr

#include "utils_string.h"
#include "utils_time.h"
#include "BMPText.h"

using namespace utils_string;
using namespace utils_time;
using namespace bmps;

BMPPointSrcFactory::BMPPointSrcFactory(const int scenarioId, const int bmpId, const int subScenario,
                                       const int bmpType, const int bmpPriority,
                                       vector<string>& distribution,
                                       const string& collection, const string& location) :
    BMPFactory(scenarioId, bmpId, subScenario, bmpType, bmpPriority, distribution, collection, location) {
    m_pointSrcMgtTab = m_bmpCollection;
    SplitStringForValues(m_location, '-', m_pointSrcIDs);
    if (m_distribution.size() == 3 && StringMatch(m_distribution[0], FLD_SCENARIO_DIST_ARRAY)) {
        m_pointSrcDistTab = m_distribution[1];
        char* end = nullptr;
        errno = 0;
        m_pointSrc = strtol(m_distribution[2].c_str(), &end, 10); // deprecated atoi
        if (errno != 0) {
            throw ModelException("BMPPointSourceFactory", "Initialization",
                                 "PTSRC in the distribution field converted to integer failed!");
        }
    } else {
        throw ModelException("BMPPointSourceFactory", "Initialization",
                             "The distribution field must follow the format: "
                             "ARRAY|CollectionName|PTSRC.\n");
    }
}

BMPPointSrcFactory::~BMPPointSrcFactory() {
    if (!m_pointSrcLocsMap.empty()) {
        for (auto it = m_pointSrcLocsMap.begin(); it != m_pointSrcLocsMap.end(); ++it) {
            if (nullptr != it->second) {
                delete it->second;
                it->second = nullptr;
            }
        }
        m_pointSrcLocsMap.clear();
    }
    if (!m_pointSrcMgtMap.empty()) {
        for (auto it = m_pointSrcMgtMap.begin(); it != m_pointSrcMgtMap.end(); ++it) {
            if (nullptr != it->second) {
                delete it->second;
                it->second = nullptr;
            }
        }
        m_pointSrcMgtMap.clear();
    }
}

void BMPPointSrcFactory::loadBMP(MongoClient* conn, const string& bmpDBName) {
    ReadPointSourceManagements(conn, bmpDBName);
    ReadPointSourceLocations(conn, bmpDBName);
}

void BMPPointSrcFactory::ReadPointSourceManagements(MongoClient* conn, const string& bmpDBName) {
//    bson_t* b = bson_new();
//    bson_t *child1 = bson_new(), *child2 = bson_new();
//    BSON_APPEND_DOCUMENT_BEGIN(b, "$query", child1);
//    BSON_APPEND_INT32(child1, BMP_FLD_SUB, m_subScenarioId);
//    bson_append_document_end(b, child1);
//    BSON_APPEND_DOCUMENT_BEGIN(b, "$orderby", child2);
//    BSON_APPEND_INT32(child2, BMP_FLD_SEQUENCE, 1);
//    bson_append_document_end(b, child2);
//    bson_destroy(child1);
//    bson_destroy(child2);

    bson_t* b = BCON_NEW(BMP_FLD_SUB, BCON_INT32(m_subScenarioId));
    bson_t* opts = BCON_NEW("sort", "{", BMP_FLD_SEQUENCE, BCON_INT32(1), "}");

    std::unique_ptr<MongoCollection> collection(new MongoCollection(conn->GetCollection(bmpDBName, m_pointSrcMgtTab)));
    mongoc_cursor_t* cursor = collection->ExecuteQuery(b, opts);

    bson_iter_t iter;
    const bson_t* bsonTable;

    /// Use count to counting sequence number, in case of discontinuous or repeat of SEQUENCE in database.
    int count = 1;
    while (mongoc_cursor_next(cursor, &bsonTable)) {
        m_pointSrcMgtSeqs.emplace_back(count);
#ifdef HAS_VARIADIC_TEMPLATES
        m_pointSrcMgtMap.emplace(count, new PointSourceMgtParams(bsonTable, iter));
#else
        m_pointSrcMgtMap.insert(make_pair(count, new PointSourceMgtParams(bsonTable, iter)));
#endif
        count++;
    }
    bson_destroy(b);
    bson_destroy(opts);
    mongoc_cursor_destroy(cursor);
}

void BMPPointSrcFactory::ReadPointSourceLocations(MongoClient* conn, const string& bmpDBName) {
//    bson_t* b = bson_new();
//    bson_t* child1 = bson_new();
//    BSON_APPEND_DOCUMENT_BEGIN(b, "$query", child1);
//    BSON_APPEND_INT32(child1, BMP_PTSRC_FLD_CODE, m_pointSrc);
//    bson_append_document_end(b, child1);
//    bson_destroy(child1);

    bson_t* b = BCON_NEW(BMP_PTSRC_FLD_CODE, BCON_INT32(m_pointSrc));

    std::unique_ptr<MongoCollection> collection(new MongoCollection(conn->GetCollection(bmpDBName, m_pointSrcDistTab)));
    mongoc_cursor_t* cursor = collection->ExecuteQuery(b);

    bson_iter_t iter;
    const bson_t* bsonTable;

    while (mongoc_cursor_next(cursor, &bsonTable)) {
        PointSourceLocations* curPtSrcLoc = new PointSourceLocations(bsonTable, iter);
        int curPtSrcID = curPtSrcLoc->GetPointSourceID();
        if (ValueInVector(curPtSrcID, m_pointSrcIDs)) {
#ifdef HAS_VARIADIC_TEMPLATES
            m_pointSrcLocsMap.emplace(curPtSrcID, curPtSrcLoc);
#else
            m_pointSrcLocsMap.insert(make_pair(curPtSrcID, curPtSrcLoc));
#endif
        } else {
            RemoveValueInVector(curPtSrcID, m_pointSrcIDs);
            delete curPtSrcLoc;
        }
    }
    bson_destroy(b);
    mongoc_cursor_destroy(cursor);
}

vector<int>& BMPPointSrcFactory::GetPointSrcMgtSeqs() {
    return m_pointSrcMgtSeqs;
}

map<int, PointSourceMgtParams *>& BMPPointSrcFactory::GetPointSrcMgtMap() {
    return m_pointSrcMgtMap;
}

vector<int>& BMPPointSrcFactory::GetPointSrcIDs() {
    return m_pointSrcIDs;
}

map<int, PointSourceLocations *>& BMPPointSrcFactory::GetPointSrcLocsMap() {
    return m_pointSrcLocsMap;
}

void BMPPointSrcFactory::Dump(std::ostream* fs) {
    if (nullptr == fs) return;
    *fs << "Point Source Management Factory: " << endl <<
            "    SubScenario ID: " << m_subScenarioId << " PTSRC: " << m_pointSrc << endl;
    for (auto it = m_pointSrcMgtSeqs.begin(); it != m_pointSrcMgtSeqs.end(); ++it) {
        auto findIdx = m_pointSrcMgtMap.find(*it);
        if (findIdx != m_pointSrcMgtMap.end()) {
            m_pointSrcMgtMap[*it]->Dump(fs);
        }
    }
    for (auto it = m_pointSrcIDs.begin(); it != m_pointSrcIDs.end(); ++it) {
        auto findIdx = m_pointSrcLocsMap.find(*it);
        if (findIdx != m_pointSrcLocsMap.end()) {
            m_pointSrcLocsMap[*it]->Dump(fs);
        }
    }
}


/************************************************************************/
/*                  PointSourceMgtParams                                */
/************************************************************************/

PointSourceMgtParams::PointSourceMgtParams(const bson_t*& bsonTable, bson_iter_t& iter)
    : m_seqence(-1), m_startDate(0), m_endDate(0), m_waterVolume(0.f), m_sedimentConc(0.f),
      m_TNConc(0.f), m_NO3Conc(0.f), m_NH4Conc(0.f), m_OrgNConc(0.f), m_TPConc(0.f), m_SolPConc(0.f),
      m_OrgPConc(0.f), m_COD(0.f) {
    if (bson_iter_init_find(&iter, bsonTable, BMP_FLD_NAME)) {
        m_name = GetStringFromBsonIterator(&iter);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_FLD_SEQUENCE)) {
        GetNumericFromBsonIterator(&iter, m_seqence);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_PTSRC_FLD_Q)) {
        GetNumericFromBsonIterator(&iter, m_waterVolume);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_PTSRC_FLD_SED)) {
        GetNumericFromBsonIterator(&iter, m_sedimentConc);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_PTSRC_FLD_TN)) {
        GetNumericFromBsonIterator(&iter, m_TNConc);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_PTSRC_FLD_NO3)) {
        GetNumericFromBsonIterator(&iter, m_NO3Conc);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_PTSRC_FLD_NH4)) {
        GetNumericFromBsonIterator(&iter, m_NH4Conc);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_PTSRC_FLD_ORGN)) {
        GetNumericFromBsonIterator(&iter, m_OrgNConc);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_PTSRC_FLD_TP)) {
        GetNumericFromBsonIterator(&iter, m_TPConc);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_PTSRC_FLD_SOLP)) {
        GetNumericFromBsonIterator(&iter, m_SolPConc);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_PTSRC_FLD_ORGP)) {
        GetNumericFromBsonIterator(&iter, m_OrgPConc);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_PTSRC_FLD_COD)) {
        GetNumericFromBsonIterator(&iter, m_COD);
    }
    int sYear = -1;
    int sMonth = -1;
    int sDay = -1;
    int eYear = -1;
    int eMonth = -1;
    int eDay = -1;
    if (bson_iter_init_find(&iter, bsonTable, BMP_FLD_SYEAR)) {
        GetNumericFromBsonIterator(&iter, sYear);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_FLD_SMONTH)) {
        GetNumericFromBsonIterator(&iter, sMonth);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_FLD_SDAY)) {
        GetNumericFromBsonIterator(&iter, sDay);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_FLD_EYEAR)) {
        GetNumericFromBsonIterator(&iter, eYear);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_FLD_EMONTH)) {
        GetNumericFromBsonIterator(&iter, eMonth);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_FLD_EDAY)) {
        GetNumericFromBsonIterator(&iter, eDay);
    }
    if (sYear > 0 && sMonth > 0 && sDay > 0) {
        m_startDate = ConvertYMDToTime(sYear, sMonth, sDay);
    }
    if (eYear > 0 && eMonth > 0 && eDay > 0) {
        m_endDate = ConvertYMDToTime(eYear, eMonth, eDay);
    }
}

void PointSourceMgtParams::Dump(std::ostream* fs) {
    if (nullptr == fs) return;
    *fs << "    Point Source Managements: " << endl;
    if (m_startDate != 0) {
        *fs << "      Start Date: " << ConvertToString(m_startDate) << endl;
    }
    if (m_endDate != 0) {
        *fs << "      End Date: " << ConvertToString(m_endDate) << endl;
    }
    *fs << "      WaterVolume: " << m_waterVolume << ", Sediment: " << m_sedimentConc <<
            ", TN: " << m_TNConc << ", NO3: " << m_NO3Conc <<
            ", NH4: " << m_NH4Conc << ", OrgN: " << m_OrgNConc <<
            ", TP: " << m_TPConc << ", MinP: " << m_SolPConc <<
            ", OrgP: " << m_OrgPConc << endl;
}


/************************************************************************/
/*                      PointSourceLocations                            */
/************************************************************************/

PointSourceLocations::PointSourceLocations(const bson_t*& bsonTable, bson_iter_t& iter)
    : m_size(0.f), m_distDown(0.f) {
    if (bson_iter_init_find(&iter, bsonTable, BMP_FLD_NAME)) {
        m_name = GetStringFromBsonIterator(&iter);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_PTSRC_FLD_PTSRCID)) {
        GetNumericFromBsonIterator(&iter, m_pointSrcID);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_PTSRC_FLD_LAT)) {
        GetNumericFromBsonIterator(&iter, m_lat);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_PTSRC_FLD_LON)) {
        GetNumericFromBsonIterator(&iter, m_lon);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_PTSRC_FLD_LOCALX)) {
        GetNumericFromBsonIterator(&iter, m_localX);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_PTSRC_FLD_LOCALY)) {
        GetNumericFromBsonIterator(&iter, m_localY);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_PTSRC_FLD_SUBBSN)) {
        GetNumericFromBsonIterator(&iter, m_subbasinID);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_PTSRC_FLD_SIZE)) {
        GetNumericFromBsonIterator(&iter, m_size);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_PTSRC_FLD_DISTDOWN)) {
        GetNumericFromBsonIterator(&iter, m_distDown);
    }
}

void PointSourceLocations::Dump(std::ostream* fs) {
    if (nullptr == fs) return;
    *fs << "      Point Source Location: " << endl <<
            "        PTSRCID: " << m_pointSrcID << ", SubBasinID: " << m_subbasinID <<
            ", Lon: " << m_lon << ", Lat: " << m_lat <<
            ", LocalX: " << m_localX << ", LocalY: " << m_localY <<
            ", Size: " << m_size << ", DistanceDown: " << m_distDown << endl;
}
