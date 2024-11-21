#include "BMPArealSourceFactory.h"

#include <memory> // unique_ptr

#include "utils_string.h"
#include "utils_time.h"

#include "BMPText.h"

using namespace utils_string;
using namespace utils_time;
using namespace bmps;

BMPArealSrcFactory::BMPArealSrcFactory(int scenarioId, int bmpId, int subScenario,
                                       int bmpType, int bmpPriority, vector<string>& distribution,
                                       const string& collection, const string& location) :
    BMPFactory(scenarioId, bmpId, subScenario, bmpType, bmpPriority, distribution, collection, location),
    m_mgtFieldsRs(nullptr) {
    m_arealSrcMgtTab = m_bmpCollection;
    SplitStringForValues(location, '-', m_arealSrcIDs);
    if (m_distribution.size() == 4 && StringMatch(m_distribution[0], FLD_SCENARIO_DIST_RASTER)) {
        m_arealSrcDistName = m_distribution[1];
        m_arealSrcDistTab = m_distribution[2];
        char* end = nullptr;
        errno = 0;
        m_arealSrc = strtol(m_distribution[3].c_str(), &end, 10); // deprecated atoi
        if (errno != 0) {
            throw ModelException("BMPArealSourceFactory", "Initialization",
                                 "SrcID in the distribution field converted to integer failed!");
        }
    } else {
        throw ModelException("BMPArealSourceFactory", "Initialization",
                             "The distribution field must follow the format: "
                             "RASTER|CoreRasterName|DistrubutionTable|SrcIDs.\n");
    }
    m_loadedMgtFieldIDs = false;
}

BMPArealSrcFactory::~BMPArealSrcFactory() {
    if (!m_arealSrcLocsMap.empty()) {
        for (auto it = m_arealSrcLocsMap.begin(); it != m_arealSrcLocsMap.end(); ++it) {
            if (nullptr != it->second) {
                delete it->second;
                it->second = nullptr;
            }
            // m_arealSrcLocsMap.erase(it++);
        }
        m_arealSrcLocsMap.clear();
    }
    if (!m_arealSrcMgtMap.empty()) {
        for (auto it = m_arealSrcMgtMap.begin(); it != m_arealSrcMgtMap.end(); ++it) {
            if (nullptr != it->second) {
                delete it->second;
                it->second = nullptr;
            }
            // m_arealSrcMgtMap.erase(it++);
        }
        m_arealSrcMgtMap.clear();
    }
}

void BMPArealSrcFactory::loadBMP(MongoClient* conn, const string& bmpDBName) {
    ReadArealSourceManagements(conn, bmpDBName);
    ReadArealSourceLocations(conn, bmpDBName);
}

void BMPArealSrcFactory::ReadArealSourceManagements(MongoClient* conn, const string& bmpDBName) {
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

    std::unique_ptr<MongoCollection> collection(new MongoCollection(conn->GetCollection(bmpDBName, m_arealSrcMgtTab)));
    mongoc_cursor_t* cursor = collection->ExecuteQuery(b, opts);

    bson_iter_t iter;
    const bson_t* bsonTable;

    /// Use count to counting sequence number, in case of discontinuous or repeat of SEQUENCE in database.
    int count = 1;
    while (mongoc_cursor_next(cursor, &bsonTable)) {
        m_arealSrcMgtSeqs.emplace_back(count);
        m_arealSrcMgtMap[count] = new ArealSourceMgtParams(bsonTable, iter);
        count++;
    }
    bson_destroy(b);
    bson_destroy(opts);
    mongoc_cursor_destroy(cursor);
}

void BMPArealSrcFactory::ReadArealSourceLocations(MongoClient* conn, const string& bmpDBName) {
//    bson_t* b = bson_new();
//    bson_t* child1 = bson_new();
//    BSON_APPEND_DOCUMENT_BEGIN(b, "$query", child1);
//    BSON_APPEND_INT32(child1, BMP_ARSRC_FLD_CODE, m_arealSrc);
//    bson_append_document_end(b, child1);
//    bson_destroy(child1);

    bson_t* b = BCON_NEW(BMP_ARSRC_FLD_CODE, BCON_INT32(m_arealSrc));

    std::unique_ptr<MongoCollection> collection(new MongoCollection(conn->GetCollection(bmpDBName, m_arealSrcDistTab)));
    mongoc_cursor_t* cursor = collection->ExecuteQuery(b);

    bson_iter_t iter;
    const bson_t* bsonTable;

    while (mongoc_cursor_next(cursor, &bsonTable)) {
        ArealSourceLocations* curArSrcLoc = new ArealSourceLocations(bsonTable, iter);
        int curArSrcID = curArSrcLoc->GetArealSourceID();
        if (ValueInVector(curArSrcID, m_arealSrcIDs)) {
#ifdef HAS_VARIADIC_TEMPLATES
            m_arealSrcLocsMap.emplace(curArSrcID, curArSrcLoc);
#else
            m_arealSrcLocsMap.insert(make_pair(curArSrcID, curArSrcLoc));
#endif
        } else {
            RemoveValueInVector(curArSrcID, m_arealSrcIDs);
            delete curArSrcLoc;
        }
    }
    bson_destroy(b);
    mongoc_cursor_destroy(cursor);
}

void BMPArealSrcFactory::SetArealSrcLocsMap(int n, int* mgtField) {
    for (auto it = m_arealSrcLocsMap.begin(); it != m_arealSrcLocsMap.end(); ++it) {
        ArealSourceLocations* tmpArealLoc = it->second;
        if (tmpArealLoc->GetValidCells() < 0 && tmpArealLoc->GetCellsIndex().empty()) {
            tmpArealLoc->SetValidCells(n, mgtField);
        }
    }
    m_loadedMgtFieldIDs = true;
}

void BMPArealSrcFactory::Dump(std::ostream* fs) {
    if (nullptr == fs) return;
    *fs << "Point Source Management Factory: " << endl <<
            "    SubScenario ID: " << m_subScenarioId << endl;
    for (auto it = m_arealSrcMgtSeqs.begin(); it != m_arealSrcMgtSeqs.end(); ++it) {
        auto findIdx = m_arealSrcMgtMap.find(*it);
        if (findIdx != m_arealSrcMgtMap.end()) {
            m_arealSrcMgtMap[*it]->Dump(fs);
        }
    }
    for (auto it = m_arealSrcIDs.begin(); it != m_arealSrcIDs.end(); ++it) {
        auto findIdx = m_arealSrcLocsMap.find(*it);
        if (findIdx != m_arealSrcLocsMap.end()) {
            m_arealSrcLocsMap[*it]->Dump(fs);
        }
    }
}

void BMPArealSrcFactory::setRasterData(map<string, IntRaster *>& sceneRsMap) {
    if (sceneRsMap.find(m_arealSrcDistName) != sceneRsMap.end()) {
        int n;
        sceneRsMap.at(m_arealSrcDistName)->GetRasterData(&n, &m_mgtFieldsRs);
    } else {
        // raise Exception?
    }
}

/************************************************************************/
/*                  ArealSourceMgtParams                                */
/************************************************************************/

ArealSourceMgtParams::ArealSourceMgtParams(const bson_t*& bsonTable, bson_iter_t& iter)
    : m_seqence(-1), m_startDate(0), m_endDate(0), m_waterVolume(0.), m_sedimentConc(0.),
      m_TNConc(0.), m_NO3Conc(0.), m_NH4Conc(0.), m_OrgNConc(0.), m_TPConc(0.), m_SolPConc(0.),
      m_OrgPConc(0.), m_COD(0.) {
    if (bson_iter_init_find(&iter, bsonTable, BMP_FLD_NAME)) {
        m_name = GetStringFromBsonIterator(&iter);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_FLD_SEQUENCE)) {
        GetNumericFromBsonIterator(&iter, m_seqence);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_ARSRC_FLD_Q)) {
        GetNumericFromBsonIterator(&iter, m_waterVolume);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_ARSRC_FLD_SED)) {
        GetNumericFromBsonIterator(&iter, m_sedimentConc);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_ARSRC_FLD_TN)) {
        GetNumericFromBsonIterator(&iter, m_TNConc);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_ARSRC_FLD_NO3)) {
        GetNumericFromBsonIterator(&iter, m_NO3Conc);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_ARSRC_FLD_NH4)) {
        GetNumericFromBsonIterator(&iter, m_NH4Conc);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_ARSRC_FLD_ORGN)) {
        GetNumericFromBsonIterator(&iter, m_OrgNConc);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_ARSRC_FLD_TP)) {
        GetNumericFromBsonIterator(&iter, m_TPConc);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_ARSRC_FLD_SOLP)) {
        GetNumericFromBsonIterator(&iter, m_SolPConc);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_ARSRC_FLD_ORGP)) {
        GetNumericFromBsonIterator(&iter, m_OrgPConc);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_ARSRC_FLD_COD)) {
        GetNumericFromBsonIterator(&iter, m_COD);
    }
    int sYear = -1, sMonth = -1, sDay = -1, eYear = -1, eMonth = -1, eDay = -1;
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

void ArealSourceMgtParams::Dump(std::ostream* fs) {
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
/*                      ArealSourceLocations                            */
/************************************************************************/

ArealSourceLocations::ArealSourceLocations(const bson_t*& bsonTable, bson_iter_t& iter)
    : m_arealSrcID(-1), m_nCells(-1), m_size(0.) {
    if (bson_iter_init_find(&iter, bsonTable, BMP_FLD_NAME)) {
        m_name = GetStringFromBsonIterator(&iter);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_ARSRC_FLD_PTSRCID)) {
        GetNumericFromBsonIterator(&iter, m_arealSrcID);
    }
    if (bson_iter_init_find(&iter, bsonTable, BMP_ARSRC_FLD_SIZE)) {
        GetNumericFromBsonIterator(&iter, m_size);
    }
}


void ArealSourceLocations::SetValidCells(const int n, int* mgtFieldIDs) {
    if (n > 0 && nullptr != mgtFieldIDs) {
        for (int i = 0; i < n; i++) {
            if (m_arealSrcID == mgtFieldIDs[i]) {
                m_cellsIndex.emplace_back(i);
            }
        }
        vector<int>(m_cellsIndex).swap(m_cellsIndex);
        // m_cellsIndex.shrink_to_fit();
        m_nCells = CVT_INT(m_cellsIndex.size());
    } else {
        throw ModelException("ArealSourceLocations", "SetValidCells",
                             "The array size of must be greater than 0 and the array must not be NULL.");
    }
}

void ArealSourceLocations::Dump(std::ostream* fs) {
    if (nullptr == fs) return;
    *fs << "      Point Source Location: " << endl <<
            "        ARSRCID: " << m_arealSrcID << ", Valid Cells Number: " << m_nCells <<
            ", Size: " << m_size << endl;
}
