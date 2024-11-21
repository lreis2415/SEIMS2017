/*!
 * \file db_mongoc.h
 * \brief Simple wrappers of the API of MongoDB C driver `mongo-c-driver`,
 * see <a href="http://mongoc.org/">MongoDB C Driver</a> for more information.
 *
 * \remarks
 *   - 1. 2017-12-02 - lj - Add unittest based on gtest/gmock.
 *   - 2. 2018-05-02 - lj - Make part of CCGL.
 *   - 3. 2019-08-16 - lj - Simplify brief desc. and move detail desc. to implementation.
 *
 * \note No exceptions will be thrown.
 * \author Liangjun Zhu, zlj(at)lreis.ac.cn
 * \version 1.2
 */
#ifndef CCGL_DB_MONGOC_H
#define CCGL_DB_MONGOC_H
#ifdef USE_MONGODB

#include <vector>
#include <map>
#include <iostream>

#include <mongoc.h>

#include "basic.h"

using std::string;
using std::vector;
using std::map;
using std::cout;
using std::endl;

namespace ccgl {
/*!
 * \namespace ccgl::db_mongoc
 * \brief Simple wrappers of the API of MongoDB C driver `mongo-c-driver`,
 * see <a href="http://mongoc.org/">MongoDB C Driver</a> for more information.
 */
namespace db_mongoc {
class MongoGridFs;

/*!
 * \class MongoClient
 * \brief A simple wrapper of the class of MongoDB Client `mongoc_client_t`.
 */
class MongoClient: NotCopyable {
public:
    /*! Constructor using IP address and port number */
    MongoClient(const char* host, vuint16_t port);

    /*! Constructor using mongoc_client_t* */
    MongoClient(mongoc_client_t* conn);

    /*! Initialization of MongoClient with the validation check of database */
    static MongoClient* Init(const char* host, vuint16_t port);

    /*! Destructor */
    ~MongoClient();

    /*! Destroy explicitly */
    void Destroy();

    /*! Get `mongoc_client_t` instance */
    mongoc_client_t* GetConn() { return conn_; }

    /*! Get existing or newly created `mongoc_database_t` instance */
    mongoc_database_t* GetDatabase(string const& dbname);

    /*! Get `mongoc_collection_t` instance */
    mongoc_collection_t* GetCollection(string const& dbname, string const& collectionname);

    /*! Get `mongoc_gridfs_t` instance */
    mongoc_gridfs_t* GetGridFs(string const& dbname, string const& gfsname);

    /*! Get MongoGridFs instance */
    MongoGridFs* GridFs(string const& dbname, string const& gfsname);

    /*! Get existing database names */
    void GetDatabaseNames(vector<string>& dbnames);

    /*! Get collection names in MongoDB database */
    void GetCollectionNames(string const& dbname, vector<string>& collnames);

    /*! Get GridFs file names in MongoDB database */
    void GetGridFsFileNames(string const& dbname, string const& gfsname, vector<string>& gfs_exists);

private:
    const char* host_;      ///< Host IP address of MongoDB
    vuint16_t port_;        ///< Port number
    mongoc_client_t* conn_; ///< Instance of `mongoc_client_t`
};

/*!
 * \class MongoDatabase
 * \brief A simple wrapper of the class of MongoDB database `mongoc_database_t`.
 */
class MongoDatabase: NotCopyable {
public:
    /*! Constructor by a `mongoc_database_t` pointer */
    explicit MongoDatabase(mongoc_database_t* db);

    /*! Constructor by mongodb client (`mongoc_client_t` pointer) and database name */
    MongoDatabase(mongoc_client_t* conn, string& dbname);

    /*! Destructor */
    ~MongoDatabase();

    /*! Get collection names in current database */
    void GetCollectionNames(vector<string>& collnames);

private:
    mongoc_database_t* db_; ///< Instance of `mongoc_database_t`
    string dbname_;         ///< Database name
};

/*!
* \class MongoCollection
* \brief A simple wrapper of the class of MongoDB Collection `mongoc_collection_t`.
*/
class MongoCollection {
public:
    /*! Constructor by a `mongoc_collection_t` pointer */
    explicit MongoCollection(mongoc_collection_t* coll);

    /*! Destructor */
    ~MongoCollection();

    /*! Execute query */
    mongoc_cursor_t* ExecuteQuery(const bson_t* b, const bson_t* opts = nullptr);

    /*! Query the records number */
    vint QueryRecordsCount();
private:
    mongoc_collection_t* collection_; ///< Instance of `mongoc_collection_t`
};

/*!
 * \class MongoGridFs
 * \brief A simple wrapper of the class of MongoDB database `mongoc_gridfs_t`.
 */
class MongoGridFs {
public:
    /*! Constructor by a `mongoc_gridfs_t` pointer or NULL */
    explicit MongoGridFs(mongoc_gridfs_t* gfs = NULL);

    /*! Destructor */
    ~MongoGridFs();

    /*! Get the current instance of `mongoc_gridfs_t` */
    mongoc_gridfs_t* GetGridFs() { return gfs_; }

    /*! Get GridFS file by name */
    mongoc_gridfs_file_t* GetFile(string const& gfilename, mongoc_gridfs_t* gfs = NULL,
                                  const STRING_MAP& opts = STRING_MAP());

    /*! Remove GridFS all matching files and their data chunks. */
    bool RemoveFile(string const& gfilename, mongoc_gridfs_t* gfs = NULL,
                    STRING_MAP opts = STRING_MAP());

    /*! Get GridFS file names */
    void GetFileNames(vector<string>& files_existed, mongoc_gridfs_t* gfs = NULL,
                    STRING_MAP opts = STRING_MAP());

    /*! Get metadata of a given GridFS file name, remember to destory bson_t after use */
    bson_t* GetFileMetadata(string const& gfilename, mongoc_gridfs_t* gfs = NULL,
                            STRING_MAP opts = STRING_MAP());

    /*! Get stream data of a given GridFS file name */
    bool GetStreamData(string const& gfilename, char*& databuf, vint& datalength,
                       mongoc_gridfs_t* gfs = NULL,
                       const STRING_MAP* opts = nullptr);

    /*! Write stream data to a GridFS file */
    bool WriteStreamData(const string& gfilename, char*& buf, vint length,
                         const bson_t* p, mongoc_gridfs_t* gfs = NULL);

private:
    mongoc_gridfs_t* gfs_; ///< Instance of `mongoc_gridfs_t`
};

/*! Append options to `bson_t` */
void AppendStringOptionsToBson(bson_t* bson_opts, const STRING_MAP& opts,
                               const string& prefix = string());

/*!
 * \brief Get numeric value from the iterator (`bson_iter_t`) of `bson_t`according to a given key
 * \param[in] iter Iterator of an instance of `bson_t`
 * \param[in,out] numericvalue The extracted value which can be `int`, `float`, or `double`
 * \return true if succeed, otherwise false.
 */
template <typename T>
bool GetNumericFromBsonIterator(bson_iter_t* iter, T& numericvalue) {
    const bson_value_t* vv = bson_iter_value(iter);
    if (vv->value_type == BSON_TYPE_INT32) {
        numericvalue = static_cast<T>(vv->value.v_int32);
    } else if (vv->value_type == BSON_TYPE_INT64) {
        numericvalue = static_cast<T>(vv->value.v_int64);
    } else if (vv->value_type == BSON_TYPE_DOUBLE) {
        numericvalue = static_cast<T>(vv->value.v_double);
    } else if (vv->value_type == BSON_TYPE_UTF8) {
        string tmp = vv->value.v_utf8.str;
        if (tmp.find_first_of("0123456789") == string::npos) {
            cout << "bson iterator isn't or not contains a numeric value." << endl;
            return false;
        }
        char* end = nullptr;
        numericvalue = static_cast<T>(strtod(tmp.c_str(), &end));
    } else {
        cout << "bson iterator isn't or not contains a numeric value." << endl;
        return false;
    }
    return true;
}

/*!
parse the following nested vector from bson_t:
[1, 2, 3]
 */
template<typename T>
void GetVectorFromBsonIter(bson_iter_t* iter, vector<T>& out){
    const bson_value_t* vv = bson_iter_value(iter);
    T value;
    if (vv->value_type == BSON_TYPE_ARRAY) {
        bson_iter_t sub_iter;
        bson_iter_recurse(iter, &sub_iter);
        while (bson_iter_next(&sub_iter)) {
            const bson_value_t* sub_vv = bson_iter_value(&sub_iter);
            if (sub_vv->value_type == BSON_TYPE_INT32) {
                out.emplace_back(CVT_INT(sub_vv->value.v_int32));
            } else if (sub_vv->value_type == BSON_TYPE_INT64) {
                out.emplace_back(CVT_INT(sub_vv->value.v_int64));
            } else if (sub_vv->value_type == BSON_TYPE_DOUBLE) {
                out.emplace_back(sub_vv->value.v_double);
            } else {
                StatusMessage("Failed in get vector value in the inner loop of GetVectorFromBsonIter.");
            }
        }
    }
    else if (GetNumericFromBsonIterator(iter, value)){
        out.emplace_back(value);
    }
    else {
        StatusMessage("Failed in get vector value in the outer loop of GetVectorFromBsonIter.");
    }
}

/*!
parse the following nested vector from bson_t:
[
    [1, 2, 3],
    [4, 5, 6],
    [7, 8, 9]
]
 */
template<typename T>
void GetVectorVectorFromBsonIter(bson_iter_t* iter, vector<vector<T>>& out){
    const bson_value_t* vv = bson_iter_value(iter);
    T value;
    if (vv->value_type == BSON_TYPE_ARRAY) {
        bson_iter_t sub_iter;
        bson_iter_recurse(iter, &sub_iter);
        while (bson_iter_next(&sub_iter)) {
            vector<T> tmp;
            GetVectorFromBsonIter(&sub_iter, tmp);
            out.emplace_back(tmp);
        }
    }
    else if (GetNumericFromBsonIterator(iter, value)) {
        vector<T> tmp;
        tmp.emplace_back(value);
        out.emplace_back(tmp);
    }
    else {
        StatusMessage("Failed in get vector value.");
    }
}

/*!
 * \brief Get numeric value from `bson_t` according to a given key
 * \param[in] bmeta Instance of `bson_t`
 * \param[in] key
 * \param[in,out] numericvalue The extracted value which can be `int`, `float`, or `double`
 * \return true if succeed, otherwise false.
 * \sa GetNumericFromBsonIterator()
 */
template <typename T>
bool GetNumericFromBson(bson_t* bmeta, const char* key, T& numericvalue) {
    bson_iter_t iter;
    if (bson_iter_init(&iter, bmeta) && bson_iter_find(&iter, key)) {
        return GetNumericFromBsonIterator(&iter, numericvalue);
    }
    StatusMessage(("WARNING: GetNumericFromBson, Failed in get value of: " + string(key) + "\n").c_str());
    return false;
}

/*!
 * \brief Get String from the iterator (`bson_iter_t`) of `bson_t`
 * \param[in] iter Iterator of an instance of `bson_t`
 * \return String of value if succeed, otherwise empty string ("").
 */
string GetStringFromBsonIterator(bson_iter_t* iter);

/*!
 * \brief Get String from `bson_t`
 * \param[in] bmeta Instance of `bson_t`
 * \param[in] key
 * \return String of value if succeed, otherwise empty string ("").
 * \sa GetStringFromBsonIterator()
 */
string GetStringFromBson(bson_t* bmeta, const char* key);

/*!
 * \brief Get Bool from the iterator (`bson_iter_t`) of `bson_t`
 * \param[in] iter Iterator of an instance of `bson_t`
 * \return true if succeed, otherwise false.
 */
bool GetBoolFromBsonIterator(bson_iter_t* iter);

/*!
 * \brief Get String from `bson_t`
 * \param[in] bmeta Instance of `bson_t`
 * \param[in] key
 * \return true if succeed, otherwise false.]
 * \sa GetBoolFromBsonIterator()
 */
bool GetBoolFromBson(bson_t* bmeta, const char* key);

/*!
 * \brief Get Datetime from the iterator (`bson_iter_t`) of `bson_t`
 * \param[in] iter Iterator of an instance of `bson_t`
 * \return float value (`time_t`) if succeed, otherwise -1.
 */
time_t GetDatetimeFromBsonIterator(bson_iter_t* iter);

/*!
 * \brief Get Datetime from `bson_t`
 * \param[in] bmeta Instance of `bson_t`
 * \param[in] key
 * \return float value (`time_t`) if succeed, otherwise -1.
 * \sa GetDatetimeFromBsonIterator()
 */
time_t GetDatetimeFromBson(bson_t* bmeta, const char* key);
} /* namespace: db_mongoc */
} /* namespace: ccgl */

#endif /* USE_MONGODB */
#endif /* CCGL_DB_MONGOC_H */
