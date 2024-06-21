/*!
 * \file db_mongoc.cpp
 * \brief Implementation of utility functions of MongoDB.
 *
 * \remarks
 *   - 1. 2017-12-02 - lj - Add unittest based on gtest/gmock.
 *   - 2. 2018-05-02 - lj - Make part of CCGL.
 *   - 3. 2019-08-16 - lj - Add or move detail description in the implementation code.
 *
 * \author Liangjun Zhu, zlj(at)lreis.ac.cn
 * \version 1.2
 */
#include "db_mongoc.h"

#include <cassert>
#include <utility>
#include "basic.h"
#include "utils_string.h"
#include "utils_math.h"
#include "utils_time.h"

using std::cout;
using std::endl;

#ifdef USE_MONGODB
namespace ccgl {
using namespace utils_string;
using namespace utils_time;

namespace db_mongoc {
///////////////////////////////////////////////////
////////////////  MongoClient    //////////////////
///////////////////////////////////////////////////

/*!
 * Get a client for a MongoDB instance using IP address and port number
 * \param[in] host IP address, e.g., 127.0.0.1
 * \param[in] port Port number, the default is 27017
 *
 * \note The constructor should not be used directly, since it does
 *          not check the validation of IP address and the status of
 *          the database, and also without `mongoc_init()`.
 *          So, please use the alternative initialization usage,
 *          `MongoClient *client = MongoClient::Init(host, port)`.
 * \sa MongoClient::Init()
 */
MongoClient::MongoClient(const char* host, const vuint16_t port) : host_(host), port_(port) {
    assert(IsIpAddress(host));
    // mongoc_init();
    mongoc_uri_t* uri = mongoc_uri_new_for_host_port(host_, port_);
    conn_ = mongoc_client_new_from_uri(uri);

    mongoc_uri_destroy(uri);
}

MongoClient::MongoClient(mongoc_client_t* conn): conn_(conn) {
    // Do nothing
}

/*!
 *  1. Check IP address
 *  2. Check database status
 *
 * \param[in] host IP address, e.g., 127.0.0.1
 * \param[in] port Port number, the default is 27017
 * \return MongoClient instance
 *
 * Examples:
 * \code
 *       MongoClient *client = MongoClient::Init(host, port)
 *       if (nullptr == client) {
 *           throw exception("MongoClient initialization failed!");
 *           // or other error handling code.
 *       }
 * \endcode
 * \sa MongoClient()
 */
MongoClient* MongoClient::Init(const char* host, const vuint16_t port) {
    // mongo host not only limit to IP address
    // if (!IsIpAddress(host)) {
    //     cout << "IP address: " + string(host) + " is invalid, Please check!" << endl;
    //     return nullptr;
    // }
    mongoc_init();
    mongoc_uri_t* uri = mongoc_uri_new_for_host_port(host, port);
    mongoc_client_t* conn = mongoc_client_new_from_uri(uri);
    if (!conn) {
        mongoc_uri_destroy(uri);
        mongoc_client_destroy(conn);
        return nullptr;
    }

    mongoc_uri_destroy(uri);
    mongoc_client_destroy(conn);

    return new MongoClient(host, port);
}

MongoClient::~MongoClient() {
    // Do nothing
}

void MongoClient::Destroy() {
    // Invoke this function ONLY if you know what you are doing. -LJ.
    if (conn_) {
        mongoc_client_destroy(conn_);
    }
    // Note that mongoc_init() does not reinitialize the driver after mongoc_cleanup().
    //   So, mongoc_cleanup() can only be invoked once in an application!
    mongoc_cleanup();
}

/*!
 * \param[in,out] dbnames String vector of existing Database names
 */
void MongoClient::GetDatabaseNames(vector<string>& dbnames) {
    char** dbnames_char = NULL;
    bson_error_t err;
#if MONGOC_CHECK_VERSION(1, 9, 0)
    dbnames_char = mongoc_client_get_database_names_with_opts (conn_, NULL, &err);
#else // deprecated from 1.9.0
    dbnames_char = mongoc_client_get_database_names(conn_, &err);
#endif
    if (dbnames_char) {
        if (!dbnames.empty()) {
            dbnames.clear(); // get clean vector before push database names
        }
        for (vuint i = 0; dbnames_char[i]; i++) {
            dbnames.emplace_back(string(dbnames_char[i]));
        }
        vector<string>(dbnames).swap(dbnames);
        bson_strfreev(dbnames_char);
    } else {
        cout << "MongoClient::GetDatabaseNames failed: " << err.message << endl;
    }
}

/*!
 * \param[in] dbname Database name string
 * \param[in,out] collnames Collection names vector in the database
 */
void MongoClient::GetCollectionNames(string const& dbname, vector<string>& collnames) {
    MongoDatabase(GetDatabase(dbname)).GetCollectionNames(collnames);
}

mongoc_database_t* MongoClient::GetDatabase(string const& dbname) {
    /// Databases are automatically created on the MongoDB server
    /// upon insertion of the first document into a collection.
    /// Therefore, there is no need to create a database manually.
    return mongoc_client_get_database(conn_, dbname.c_str());
}

mongoc_collection_t* MongoClient::GetCollection(string const& dbname, string const& collectionname) {
    mongoc_database_t* db = mongoc_client_get_database(conn_, dbname.c_str());
    bson_error_t err;
    if (!mongoc_database_has_collection(db, collectionname.c_str(), &err)) {
        cout << "MongoClient::GetCollection (" << dbname << ": " << collectionname
                << ") failed! Error message: " << err.message << endl;
        return NULL;
    }
    mongoc_database_destroy(db);
    mongoc_collection_t* collection = mongoc_client_get_collection(conn_, dbname.c_str(),
                                                                   collectionname.c_str());
    return collection;
}

mongoc_gridfs_t* MongoClient::GetGridFs(string const& dbname, string const& gfsname) {
    bson_error_t err;
    mongoc_gridfs_t* gfs = mongoc_client_get_gridfs(conn_, dbname.c_str(), gfsname.c_str(), &err);
    if (NULL == gfs) {
        // The database may not exist, create it first.
        mongoc_database_t* db = mongoc_client_get_database(conn_, dbname.c_str());
        gfs = mongoc_client_get_gridfs(conn_, dbname.c_str(), gfsname.c_str(), &err);
        mongoc_database_destroy(db);
        if (NULL == gfs) {
            cout << "Failed to connect to " + gfsname + " GridFS! Error: " << err.message << endl;
            return NULL;
        }
    }
    return gfs;
}

MongoGridFs* MongoClient::GridFs(string const& dbname, string const& gfsname) {
    return new MongoGridFs(GetGridFs(dbname, gfsname));
}

/*!
 * \param[in] dbname Database name
 * \param[in] gfsname GridFS name
 * \param[in,out] gfs_exists Existing GridFS file names
 */
void MongoClient::GetGridFsFileNames(string const& dbname, string const& gfsname, vector<string>& gfs_exists) {
    mongoc_gridfs_t* gfs = GetGridFs(dbname, gfsname);
    MongoGridFs(gfs).GetFileNames(gfs_exists);
}

///////////////////////////////////////////////////
////////////////  MongoDatabase  //////////////////
///////////////////////////////////////////////////
MongoDatabase::MongoDatabase(mongoc_database_t* db) : db_(db) {
    dbname_ = string(mongoc_database_get_name(db_));
}

MongoDatabase::MongoDatabase(mongoc_client_t* conn, string& dbname) : dbname_(dbname) {
    db_ = mongoc_client_get_database(conn, dbname_.c_str());
}

MongoDatabase::~MongoDatabase() {
    mongoc_database_destroy(db_);
}

/*!
 * \param[in,out] collnames Collection names vector in current database
 */
void MongoDatabase::GetCollectionNames(vector<string>& collnames) {
    char **collnames_chars = NULL;
    bson_error_t err;
#if MONGOC_CHECK_VERSION(1, 9, 0)
    collnames_chars = mongoc_database_get_collection_names_with_opts (db_, NULL, &err);
#else // deprecated from 1.9.0
    collnames_chars = mongoc_database_get_collection_names(db_, &err);
#endif
    if (collnames_chars) {
        if (!collnames.empty()) {
            collnames.clear(); // get clean vector before push database names
        }
        for (vuint i = 0; collnames_chars[i]; i++) {
            string cname = string(collnames_chars[i]);
            auto tmp_iter = find(collnames.begin(), collnames.end(), cname);
            if (tmp_iter == collnames.end()) {
                collnames.emplace_back(cname);
            }
        }
        vector<string> realnames; // e.g., SPATIAL.files and SPATIAL.chunks has the real GridFS name: SPATIAL
        for (auto it = collnames.begin(); it != collnames.end(); ++it) {
            vector<string> splitstrs = SplitString((*it), '.');
            if (splitstrs.back() == "files" || splitstrs.back() == "chunks") {
                string realname;
                // Refers to https://stackoverflow.com/a/18703743/4837280
                std::for_each(splitstrs.begin(), splitstrs.end() - 1,
                              [&](const std::string &piece){ realname += piece; });
                string another_coll = splitstrs.back() == "files" ?
                                          realname + ".chunks" :
                                          realname + ".files";
                if (find(collnames.begin(), collnames.end(), another_coll) == collnames.end()) {
                    continue;
                }
                if (find(realnames.begin(), realnames.end(), realname) == realnames.end()) {
                    realnames.emplace_back(realname);
                }
            }
        }
        for (auto it = realnames.begin(); it != realnames.end(); ++it) {
            collnames.emplace_back((*it));
        }
        vector<string>(collnames).swap(collnames);
        bson_strfreev(collnames_chars);
    } else {
        cout << "MongoDatabase::GetCollectionNames failed: " << err.message << endl;
    }
    vector<string>(collnames).swap(collnames);
}

///////////////////////////////////////////////////
////////////////  MongoCollection  ////////////////
///////////////////////////////////////////////////
MongoCollection::MongoCollection(mongoc_collection_t* coll) : collection_(coll) {
    // Do nothing.
}

MongoCollection::~MongoCollection() {
    mongoc_collection_destroy(collection_);
}

mongoc_cursor_t* MongoCollection::ExecuteQuery(const bson_t* b, const bson_t* opts /* = nullptr */) {
    mongoc_cursor_t* cursor = nullptr;
    cursor = mongoc_collection_find_with_opts(collection_, b, opts, NULL);
    return cursor;
}

vint MongoCollection::QueryRecordsCount() {
    const bson_t* q_count = bson_new();
    bson_error_t err;
#if MONGOC_CHECK_VERSION(1, 11, 0)
    int64_t count = mongoc_collection_count_documents(collection_, q_count,
                                                      NULL, NULL, NULL, &err);
#else
    int64_t count = mongoc_collection_count(collection_, MONGOC_QUERY_NONE, q_count, 0, 0, NULL, &err);
#endif
    if (count < 0) {
        cout << "MongoCollection::QueryRecordsCount failed: " << err.message << endl;
        return -1;
    }
    return CVT_VINT(count);
}

///////////////////////////////////////////////////
////////////////  MongoGridFs  ////////////////////
///////////////////////////////////////////////////
MongoGridFs::MongoGridFs(mongoc_gridfs_t* gfs /* = NULL */) : gfs_(gfs) {
    // Do nothing.
}

MongoGridFs::~MongoGridFs() {
    if (gfs_ != NULL) { mongoc_gridfs_destroy(gfs_); }
}

mongoc_gridfs_file_t* MongoGridFs::GetFile(string const& gfilename, mongoc_gridfs_t* gfs /* = NULL */,
                                           const STRING_MAP& opts /* = STRING_MAP() */) {
    if (gfs_ != NULL) { gfs = gfs_; }
    if (NULL == gfs) {
        StatusMessage("mongoc_gridfs_t must be provided for MongoGridFs!");
        return NULL;
    }
    mongoc_gridfs_file_t* gfile = NULL;
    bson_error_t err;
    bson_t filter = BSON_INITIALIZER;
    BSON_APPEND_UTF8(&filter, "filename", gfilename.c_str());
    AppendStringOptionsToBson(&filter, opts, "metadata.");
    int count = 0;
    while (count < 5) {
        gfile = mongoc_gridfs_find_one_with_opts(gfs, &filter, NULL, &err);
        if (NULL == gfile) {
            StatusMessage(("MongoGridFs::GetFile(" + gfilename + ") failed: " + err.message).c_str());
            SleepMs(2); // Sleep for two milliseconds, in case of network blocking. By lj.
            count++;
        } else { break; }
    }
    bson_destroy(&filter);
    if (NULL == gfile) {
        StatusMessage(("The file " + gfilename + " does not exist.").c_str());
    }
    return gfile;
}

bool MongoGridFs::RemoveFile(string const& gfilename, mongoc_gridfs_t* gfs /* = NULL */,
                             STRING_MAP opts /* = STRING_MAP() */) {
    if (gfs_ != NULL) { gfs = gfs_; }
    if (NULL == gfs) {
        StatusMessage("mongoc_gridfs_t must be provided for MongoGridFs!");
        return false;
    }
    bson_error_t err;
    bson_t filter = BSON_INITIALIZER;
    BSON_APPEND_UTF8(&filter, "filename", gfilename.c_str());
    AppendStringOptionsToBson(&filter, opts, "metadata.");
    // Deprecated: this function will remove all files with the same filename
    // if (mongoc_gridfs_remove_by_filename(gfs, gfilename.c_str(), &err)) {
    //     return true;
    // }
    mongoc_gridfs_file_list_t* filelist;
    mongoc_gridfs_file_t* gfile = NULL;
    filelist = mongoc_gridfs_find_with_opts(gfs, &filter, NULL);
    while ((gfile = mongoc_gridfs_file_list_next(filelist))) {
        const bson_value_t* tmpid = mongoc_gridfs_file_get_id(gfile);
        char charid[25];
        bson_oid_to_string(&tmpid->value.v_oid, charid);
        string strid = charid;
        if (!mongoc_gridfs_file_remove(gfile, &err)) {
            StatusMessage(("MongoGridFs::RemoveFile(" + gfilename + ") failed: " + err.message).c_str());
        } else {
            StatusMessage(("Removed GridFs: " + gfilename + ", _id: " + strid).c_str());
        }
        mongoc_gridfs_file_destroy(gfile);
    }

    mongoc_gridfs_file_list_destroy(filelist);
    bson_destroy(&filter);
    return true;
}

void MongoGridFs::GetFileNames(vector<string>& files_existed, mongoc_gridfs_t* gfs /* = NULL */) {
    if (gfs_ != NULL) { gfs = gfs_; }
    if (NULL == gfs) {
        StatusMessage("mongoc_gridfs_t must be provided for MongoGridFs!");
    }
    //vector<string> filesExisted;
    bson_t* query = bson_new();
    bson_init(query);
    mongoc_gridfs_file_list_t* glist = mongoc_gridfs_find_with_opts(gfs, query, NULL);
    mongoc_gridfs_file_t* file;
    while ((file = mongoc_gridfs_file_list_next(glist))) {
        files_existed.emplace_back(string(mongoc_gridfs_file_get_filename(file)));
        mongoc_gridfs_file_destroy(file);
    }
    mongoc_gridfs_file_list_destroy(glist);
    bson_destroy(query);
    vector<string>(files_existed).swap(files_existed);
}

bson_t* MongoGridFs::GetFileMetadata(string const& gfilename,
                                     mongoc_gridfs_t* gfs /* = NULL */,
                                     STRING_MAP opts /* = STRING_MAP() */) {
    if (gfs_ != NULL) gfs = gfs_;
    if (NULL == gfs) {
        StatusMessage("mongoc_gridfs_t must be provided for MongoGridFs!");
        return NULL;
    }
    mongoc_gridfs_file_t* gfile = GetFile(gfilename, gfs, opts);
    if (NULL == gfile) {
        StatusMessage(("MongoGridFs::GetFileMetadata(" + gfilename + ") failed!").c_str());
        return NULL;
    }
    // `mongoc_gridfs_file_get_metadata` returns a bson_t that should not be modified or freed.
    const bson_t* bmata = mongoc_gridfs_file_get_metadata(gfile);
    bson_t* mata = bson_copy(bmata);
    mongoc_gridfs_file_destroy(gfile);
    return mata;
}

bool MongoGridFs::GetStreamData(string const& gfilename, char*& databuf,
                                vint& datalength, mongoc_gridfs_t* gfs /* = NULL */,
                                const STRING_MAP* opts /* = nullptr */) {
    if (gfs_ != NULL) { gfs = gfs_; }
    if (NULL == gfs) {
        StatusMessage("mongoc_gridfs_t must be provided for MongoGridFs!");
        return false;
    }
    STRING_MAP opts_temp;
    if (nullptr == opts) {
        opts = &opts_temp;
    }
    mongoc_gridfs_file_t* gfile = GetFile(gfilename, gfs, *opts);
    if (NULL == gfile) {
        databuf = NULL;
        StatusMessage(("MongoGridFs::GetStreamData(" + gfilename + ") failed!").c_str());
        return false;
    }
    datalength = mongoc_gridfs_file_get_length(gfile);
    databuf = static_cast<char *>(malloc(datalength));
    mongoc_iovec_t iov;
    iov.iov_base = databuf;
    iov.iov_len = static_cast<u_long>(datalength);
    mongoc_stream_t* stream = mongoc_stream_gridfs_new(gfile);
    // Set 10 milliseconds for timeout
    vint flag = mongoc_stream_readv(stream, &iov, 1, -1, 10);
    mongoc_stream_destroy(stream);
    mongoc_gridfs_file_destroy(gfile);
    return flag >= 0;
}

bool MongoGridFs::WriteStreamData(const string& gfilename, char*& buf,
                                  vint length, const bson_t* p,
                                  mongoc_gridfs_t* gfs /* = NULL */) {
    if (gfs_ != NULL) { gfs = gfs_; }
    if (NULL == gfs) {
        StatusMessage("mongoc_gridfs_t must be provided for MongoGridFs!");
        return false;
    }
    mongoc_gridfs_file_t* gfile = NULL;
    bson_error_t gfileerr;
    mongoc_gridfs_file_opt_t gopt = {0};
    gopt.filename = gfilename.c_str();
    gopt.content_type = "NumericStream";
    gopt.metadata = p;
    gfile = mongoc_gridfs_create_file(gfs, &gopt);
    mongoc_iovec_t ovec;
    ovec.iov_base = buf;
    ovec.iov_len = static_cast<u_long>(length);
    // Modifying GridFS files is NOT thread-safe. Only one thread or process
    //   can access a GridFS file while it is being modified!
    ssize_t writesize = mongoc_gridfs_file_writev(gfile, &ovec, 1, 0);
    bool gfilestatus = mongoc_gridfs_file_save(gfile) && // Returns true if successful
            !mongoc_gridfs_file_error(gfile, &gfileerr); // Returns false if no registered error
    if (writesize == -1 || !gfilestatus) { // Failed to save GridFS file data
        StatusMessage(("MongoGridFs::WriteStreamData(" + gfilename + ") failed!" +
                          ". ERROR: " + gfileerr.message).c_str());
    }
    mongoc_gridfs_file_destroy(gfile);
    return gfilestatus;
}

///////////////////////////////////////////////////
/////////  bson related utilities   ///////////////
///////////////////////////////////////////////////

/*!
 * \param[in,out] bson_opts Instance of `bson_t`
 * \param[in] opts STRING_MAP key-value
 */
void AppendStringOptionsToBson(bson_t* bson_opts, const STRING_MAP& opts,
                               const string& prefix /* = string() */) {
    if (opts.empty()) { return; }
    for (auto iter = opts.begin(); iter != opts.end(); ++iter) {
        string meta_field;
        if (prefix.empty()) { meta_field = iter->first; }
        else { meta_field = prefix + iter->first; }
        bool is_dbl = false;
        double dbl_value = IsDouble(iter->second, is_dbl);
        if (StringMatch("", iter->second) || !is_dbl) {
            BSON_APPEND_UTF8(bson_opts, meta_field.c_str(), iter->second.c_str());
        } else {
            double intpart; // https://stackoverflow.com/a/1521682/4837280
            if (std::modf(dbl_value, &intpart) == 0.0) {
                BSON_APPEND_INT32(bson_opts, meta_field.c_str(), CVT_INT(dbl_value));
            } else {
                BSON_APPEND_DOUBLE(bson_opts, meta_field.c_str(), dbl_value);
            }
        }
    }
}

string GetStringFromBsonIterator(bson_iter_t* iter) {
    const bson_value_t* vv = bson_iter_value(iter);
    if (vv->value_type == BSON_TYPE_UTF8) {
        return vv->value.v_utf8.str;
    }
    if (vv->value_type == BSON_TYPE_INT32) {
        return ValueToString(vv->value.v_int32);
    }
    if (vv->value_type == BSON_TYPE_INT64) {
        return ValueToString(vv->value.v_int64);
    }
    if (vv->value_type == BSON_TYPE_DOUBLE) {
        return ValueToString(vv->value.v_double);
    }
    StatusMessage("bson iterator did not contain or can not convert to string.");
    return "";
}

string GetStringFromBson(bson_t* bmeta, const char* key) {
    bson_iter_t iter;
    if (bson_iter_init(&iter, bmeta) && bson_iter_find(&iter, key)) {
        return GetStringFromBsonIterator(&iter);
    }
    StatusMessage(("Failed in get value of: " + string(key)).c_str());
    return "";
}

bool GetBoolFromBsonIterator(bson_iter_t* iter) {
    const bson_value_t* vv = bson_iter_value(iter);
    float fltvalue;
    if (vv->value_type == BSON_TYPE_INT32) {
        fltvalue = CVT_FLT(vv->value.v_int32);
    } else if (vv->value_type == BSON_TYPE_INT64) {
        fltvalue = CVT_FLT(vv->value.v_int64);
    } else if (vv->value_type == BSON_TYPE_DOUBLE) {
        fltvalue = CVT_FLT(vv->value.v_double);
    } else if (vv->value_type == BSON_TYPE_UTF8) {
        string tmp = vv->value.v_utf8.str;
        return StringMatch(tmp.c_str(), "TRUE");
    } else {
        StatusMessage("Failed in get Boolean value.");
        return false;
    }
    return utils_math::FloatEqual(fltvalue, 0.f);
}

bool GetBoolFromBson(bson_t* bmeta, const char* key) {
    bson_iter_t iter;
    if (bson_iter_init(&iter, bmeta) && bson_iter_find(&iter, key)) {
        return GetBoolFromBsonIterator(&iter);
    }
    StatusMessage(("Failed in get boolean value of: " + string(key)).c_str());
    return false;
}

time_t GetDatetimeFromBsonIterator(bson_iter_t* iter) {
    const bson_value_t* vv = bson_iter_value(iter);
    if (vv->value_type == BSON_TYPE_DATE_TIME) {
		string str_milisec = std::to_string(vv->value.v_datetime);
		string str_second = str_milisec.substr(0, str_milisec.length() - 3);
		int intStr = atoi(str_second.c_str());
        return CVT_TIMET(intStr);
    }
    if (vv->value_type == BSON_TYPE_UTF8) {
        string tmp_time_str = vv->value.v_utf8.str;
        if (tmp_time_str.size() > 12) {
            return ConvertToTime(tmp_time_str, "%4d-%2d-%2d %2d:%2d:%2d", true);
        }
        return ConvertToTime(tmp_time_str, "%4d-%2d-%2d", false);
    }
    StatusMessage("Failed in get Date Time value.");
    return -1;
}

time_t GetDatetimeFromBson(bson_t* bmeta, const char* key) {
    bson_iter_t iter;
    if (bson_iter_init(&iter, bmeta) && bson_iter_find(&iter, key)) {
        return GetDatetimeFromBsonIterator(&iter);
    }
    StatusMessage(("Failed in get Datetime value of: " + string(key)).c_str());
    return -1;
}

} /* namespace: db_mongoc */
} /* namespace: ccgl */
#endif /* USE_MONGODB */
