#include "InputStation.h"

#include <sstream>
#include <memory>

#include "utils_string.h"
#include "RegularMeasurement.h"
#include "NotRegularMeasurement.h"
#include "text.h"
#include "Logging.h"

using namespace utils_string;

InputStation::InputStation(MongoClient* conn, const time_t dtHillslope, const time_t dtChannel) :
    m_conn(conn), m_dtCh(dtChannel), m_dtHs(dtHillslope) {
}

InputStation::~InputStation() {
    CLOG(TRACE, LOG_RELEASE) << "Start to release InputStation data ...";
    for (auto it = m_measurement.begin(); it != m_measurement.end(); ++it) {
        if (it->second != nullptr) {
            delete it->second;
            it->second = nullptr;
        }
    }
    m_measurement.clear();

    for (auto it = m_latitude.begin(); it != m_latitude.end(); ++it) {
        if (it->second != nullptr) {
            delete[] it->second;
            it->second = nullptr;
        }
    }
    m_latitude.clear();
    for (auto it = m_elevation.begin(); it != m_elevation.end(); ++it) {
        if (it->second != nullptr) {
            delete[] it->second;
            it->second = nullptr;
        }
    }
    m_elevation.clear();
}

void InputStation::build_query_bson(const int nSites, const vector<int>& siteIDList,
                                    const string& siteType, bson_t* query) {
    //query = bson_new();  query has been initialized before
    bson_t* child2 = bson_new();
    bson_t* child3 = bson_new();
    BSON_APPEND_DOCUMENT_BEGIN(query, MONG_HYDRO_DATA_SITEID, child2);
    BSON_APPEND_ARRAY_BEGIN(child2, "$in", child3);
    std::ostringstream ossIndex;
    for (int iSite = 0; iSite < nSites; iSite++) {
        ossIndex.str("");
        ossIndex << iSite;
        BSON_APPEND_INT32(child3, ossIndex.str().c_str(), siteIDList[iSite]);
    }
    bson_append_array_end(child2, child3);
    bson_append_document_end(query, child2);
    bson_destroy(child2);
    bson_destroy(child3);

    if (siteType == DataType_Precipitation) {
        BSON_APPEND_UTF8(query, MONG_HYDRO_SITE_TYPE, siteType.c_str()); //"Type"
        child2 = bson_new();
        child3 = bson_new();
        BSON_APPEND_DOCUMENT_BEGIN(query, MONG_HYDRO_SITE_TYPE, child2); //"Type"
        BSON_APPEND_ARRAY_BEGIN(child2, "$in", child3);
        BSON_APPEND_UTF8(child3, "0", DataType_Precipitation); //"P"
        BSON_APPEND_UTF8(child3, "1", DataType_Meteorology);   //"M"
        bson_append_array_end(child2, child3);
        bson_append_document_end(query, child2);
        bson_destroy(child2);
        bson_destroy(child3);
    } else {
        BSON_APPEND_UTF8(query, MONG_HYDRO_SITE_TYPE, siteType.c_str());
    } //"Type"
}

void InputStation::ReadSitesInfo(const string& siteType, const string& hydroDBName, const string& sitesList) {
    vector<string> vecSites = SplitString(sitesList, ',');
    int nSites = CVT_INT(vecSites.size());
    //convert from string to int, the IDList is in order in MongoDB
    vector<int> siteIDList;
    char* end = nullptr;
    errno = 0;
    siteIDList.reserve(nSites);
    for (int iSite = 0; iSite < nSites; iSite++) {
        siteIDList.emplace_back(strtol(vecSites[iSite].c_str(), &end, 10)); // deprecated atoi);
    }
    //sort(siteIDList.begin(), siteIDList.end());

    bson_t* query = bson_new();
    build_query_bson(nSites, siteIDList, siteType, query);
    bson_t* opts = BCON_NEW("sort", "{", MONG_HYDRO_DATA_SITEID, BCON_INT32(1), "}");
    CLOG(TRACE, LOG_INIT) << "ReadSitesInfo: " << bson_as_json(query,NULL);

    std::unique_ptr<MongoCollection> collection(new MongoCollection(m_conn->GetCollection(hydroDBName,
                                                                        DB_TAB_SITES)));
    mongoc_cursor_t* cursor = collection->ExecuteQuery(query, opts);

    const bson_t* record = NULL;
    FLTPT* pEle = new FLTPT[nSites];
    FLTPT* pLat = new FLTPT[nSites];
    FLTPT ele, lat;
    bool hasData = false;
    while (mongoc_cursor_more(cursor) && mongoc_cursor_next(cursor, &record)) {
        hasData = true;
        bson_iter_t iter;
        long long int siteIndex = -1;
        int curSiteID = -1;
        if (bson_iter_init(&iter, record) && bson_iter_find(&iter, MONG_HYDRO_DATA_SITEID)) {
            GetNumericFromBsonIterator(&iter, curSiteID);
        } else {
            throw ModelException("Measurement", "Measurement",
                                 "The Site ID field does not exist in Sites table, Please check and retry.");
        }
        auto siteIDIter = find(siteIDList.begin(), siteIDList.end(), curSiteID);
        siteIndex = distance(siteIDList.begin(), siteIDIter);
        if (bson_iter_init(&iter, record) && bson_iter_find(&iter, MONG_HYDRO_SITE_LAT)) {
            GetNumericFromBsonIterator(&iter, lat);
        } else {
            throw ModelException("InputStation", "ReadSitesInfo",
                                 "The Lat field does not exist in Sites table.");
        }

        if (bson_iter_init(&iter, record) && bson_iter_find(&iter, MONG_HYDRO_SITE_ELEV)) {
            GetNumericFromBsonIterator(&iter, ele);
        } else {
            throw ModelException("InputStation", "ReadSitesInfo",
                                 "The Lat field does not exist in Sites table.");
        }
        pLat[siteIndex] = lat;
        pEle[siteIndex] = ele;
    }
    if (!hasData) {
        throw ModelException("InputStation", "ReadSitesInfo", "Query failed.");
    }

    mongoc_cursor_destroy(cursor);
    bson_destroy(query);
    m_elevation[siteType] = pEle;
    m_latitude[siteType] = pLat;
    m_numSites[siteType] = nSites;
}

void InputStation::ReadSitesData(const string& hydroDBName, const string& sitesList, const string& siteType,
                                 const time_t startDate, const time_t endDate, const bool stormMode /* = false */) {
    string siteTypeU = GetUpper(siteType);
    if (stormMode) {
        m_measurement[siteType] = new NotRegularMeasurement(m_conn, hydroDBName, sitesList, siteTypeU,
                                                            startDate, endDate);
    } else {
        m_measurement[siteType] = new RegularMeasurement(m_conn, hydroDBName, sitesList, siteTypeU,
                                                         startDate, endDate, m_dtHs);
    }
    if (StringMatch(siteType, DataType_Precipitation)) {
        ReadSitesInfo(DataType_Precipitation, hydroDBName, sitesList);
    } else if (m_elevation.find(DataType_Meteorology) == m_elevation.end()) {
        ReadSitesInfo(DataType_Meteorology, hydroDBName, sitesList);
    }
}

bool InputStation::NumberOfSites(const char* site_type, int& site_count) {
    if (m_numSites.find(site_type) != m_numSites.end()) {
        site_count = m_numSites.at(site_type);
        return true;
    }
    site_count = -1;
    return false;
}

bool InputStation::GetElevation(const char* site_type, FLTPT*& site_elevs) {
    if (m_numSites.find(site_type) != m_numSites.end()) {
        site_elevs = m_elevation.at(site_type);
        return true;
    }
    site_elevs = nullptr;
    return false;
}

bool InputStation::GetLatitude(const char* site_type, FLTPT*& site_lats) {
    if (m_numSites.find(site_type) != m_numSites.end()) {
        site_lats = m_latitude.at(site_type);
        return true;
    }
    site_lats = nullptr;
    return false;
}



void InputStation::GetTimeSeriesData(const time_t time, const string& type, int* nRow, FLTPT** data) {
    Measurement* m = m_measurement[type];
    *nRow = m->NumberOfSites();
    //cout << type << "\t" << *nRow << endl;
    *data = m->GetSiteDataByTime(time);
}
