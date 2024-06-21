#include "NotRegularMeasurement.h"

#include <sstream>

#include "utils_time.h"
#include "text.h"
#include "Logging.h"

using namespace utils_time;

NotRegularMeasurement::NotRegularMeasurement(MongoClient* conn, const string& hydroDBName,
                                             const string& sitesList, const string& siteType,
                                             const time_t startTime, const time_t endTime)
    : Measurement(conn, hydroDBName, sitesList, siteType, startTime, endTime) {
    size_t nSites = m_siteIDList.size();
    m_valueList.reserve(nSites);
    m_timeList.reserve(nSites);
    m_curIndexList.resize(nSites, 0);
    for (size_t iSite = 0; iSite < nSites; iSite++) {
        /// build query statement
//        bson_t* query = bson_new();
//        bson_t* child = bson_new();
//        bson_t* child2 = bson_new();
//        bson_t* child3 = bson_new();
//        BSON_APPEND_DOCUMENT_BEGIN(query, "$query", child);
//        BSON_APPEND_DOCUMENT_BEGIN(child, MONG_HYDRO_DATA_SITEID, child2);
//        BSON_APPEND_ARRAY_BEGIN(child2, "$in", child3);
//        std::ostringstream ossIndex;
//        ossIndex.str("");
//        ossIndex << iSite;
//        BSON_APPEND_INT32(child3, ossIndex.str().c_str(), m_siteIDList[iSite]);
//        bson_append_array_end(child2, child3);
//        bson_append_document_end(child, child2);
//        bson_destroy(child2);
//        bson_destroy(child3);
//        BSON_APPEND_UTF8(child, MONG_HYDRO_SITE_TYPE, siteType.c_str());
//        child2 = bson_new();
//        BSON_APPEND_DOCUMENT_BEGIN(child, MONG_HYDRO_DATA_UTC, child2);
//        /// startTime <= t <= endTime,
//        BSON_APPEND_TIME_T(child2, "$gte", startTime);
//        BSON_APPEND_TIME_T(child2, "$lte", endTime);
//        bson_append_document_end(child, child2);
//        bson_append_document_end(query, child);
//        bson_destroy(child2);
//        bson_destroy(child);
//        child = bson_new();
//        BSON_APPEND_DOCUMENT_BEGIN(query, "$orderby", child);
//        /// sort by stationID and time
//        BSON_APPEND_INT32(child, MONG_HYDRO_DATA_SITEID, 1);
//        BSON_APPEND_INT32(child, MONG_HYDRO_DATA_UTC, 1);
//        bson_append_document_end(query, child);
//        bson_destroy(child);
        bson_t* site_array = bson_new();
        BSON_APPEND_INT32(site_array, ValueToString(iSite).c_str(), m_siteIDList[iSite]);

        bson_t* query = BCON_NEW(MONG_HYDRO_DATA_SITEID, "{", "$in", BCON_ARRAY(site_array), "}",
                                 MONG_HYDRO_SITE_TYPE, siteType.c_str(),
                                 MONG_HYDRO_DATA_UTC, "{", "$gte", BCON_DATE_TIME(CVT_VINT(startTime) * 1000),
                                                           "$lte", BCON_DATE_TIME(CVT_VINT(endTime) * 1000),
                                                      "}");
        bson_t* opts = BCON_NEW("sort", "{", MONG_HYDRO_DATA_SITEID, BCON_INT32(1),
                                             MONG_HYDRO_DATA_UTC, BCON_INT32(1),
                                        "}");

        CLOG(TRACE, LOG_INIT) << "NotRegularMeasurement: " << bson_as_json(query,NULL);

        // perform query and read measurement data
        std::unique_ptr<MongoCollection>
                collection(new MongoCollection(m_conn->GetCollection(hydroDBName, DB_TAB_DATAVALUES)));
        mongoc_cursor_t* cursor = collection->ExecuteQuery(query, opts);

        FLTPT value;
        time_t dt;
        bool hasData = false;
        const bson_t* doc;
        vector<time_t> m_times;
        vector<float> m_values;
        while (mongoc_cursor_more(cursor) && mongoc_cursor_next(cursor, &doc)) {
            hasData = true;
            bson_iter_t iter;
            if (bson_iter_init(&iter, doc) && bson_iter_find(&iter, MONG_HYDRO_DATA_VALUE)) {
                GetNumericFromBsonIterator(&iter, value);
            } else {
                throw ModelException("NotRegularMeasurement", "NotRegularMeasurement",
                                     "The Value field: " + string(MONG_HYDRO_DATA_VALUE) +
                                     " does not exist in DataValues table.");
            }
            
            if (bson_iter_init(&iter, doc) && bson_iter_find(&iter, MONG_HYDRO_DATA_UTC)) {
                // xdw modify, *0.001f will causes accuracy issues, so use string clip to convert millisecond to second
                // dt = static_cast<time_t>(GetDatetimeFromBsonIterator(&iter) * 0.001);
                dt = static_cast<time_t>(GetDatetimeFromBsonIterator(&iter));
            } else {
                throw ModelException("NotRegularMeasurement", "NotRegularMeasurement",
                                     "The Value field: " + string(MONG_HYDRO_DATA_UTC) +
                                     " does not exist in DataValues table.");
            }
            m_times.emplace_back(dt);
            m_values.emplace_back(value);
        }
        m_timeList.emplace_back(m_times);
        m_valueList.emplace_back(m_values);
        bson_destroy(query);
        mongoc_cursor_destroy(cursor);

        if (!hasData) {
            std::ostringstream oss;
            oss << "There are no " << siteType << " data available for sites:[" << m_siteIDList[iSite] <<
                    "] in database:" << hydroDBName
                    << " during " << ConvertToString2(m_startTime) << " to " << ConvertToString2(m_endTime);
            throw ModelException("NotRegularMeasurement", "Constructor", oss.str());
        }
    }
}
string mutiple(string num1, int num2) {
    string res;
    int c = 0;
    for (int i = num1.size() - 1; i >= 0; i--) {
        int tmp = (num1[i] - '0') * num2 + c;
        c = tmp / 10;
        tmp = tmp % 10;
        res.insert(res.begin(), tmp + '0');
    }
    while (c) {
        int tmp = c % 10;
        c = c / 10;
        res.insert(res.begin(), tmp + '0');
    }
    return res;
}

FLTPT* NotRegularMeasurement::GetSiteDataByTime(const time_t t) {
    for (int iSite = 0; iSite < CVT_INT(m_siteIDList.size()); iSite++) {
        vector<time_t>& tlist = m_timeList[iSite];
        vector<FLTPT>& vlist = m_valueList[iSite];
        size_t curIndex = size_t(m_curIndexList[iSite]);
        // find the index for current time
        // the nearest record before t
        while (curIndex < tlist.size() && tlist[curIndex] <= t) {
            curIndex++;
        }
        curIndex--;
        pData[iSite] = vlist[curIndex];
        m_curIndexList[iSite] = CVT_INT(curIndex);
    }
    return pData;
}
