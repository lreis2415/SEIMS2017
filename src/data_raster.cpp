/*!
 * \file data_raster.cpp
 * \brief Utility functions for Raster data IO in ASC, GDAL-supported, and MongoDB formats.
 *
 * \remarks
 *   - 1. Apr. 2022 - lj - Separated from clsRasterData class for widely use.
 *   - 2. Aug. 2023 - lj - Add GDAL data types added from versions 3.5 and 3.7
 *
 * \author Liangjun Zhu, zlj(at)lreis.ac.cn
 */

#include "data_raster.hpp"

namespace ccgl {
namespace data_raster {
string RasterDataTypeToString(const int type) {
    switch (type) {
        case RDT_Int8:	    return("INT8");    // 8-bit signed integer
        case RDT_UInt8:	    return("UINT8");   // 8-bit unsigned integer
        case RDT_UInt16:	return("UINT16");  // 16-bit unsigned integer
        case RDT_Int16:	    return("INT16");   // 16-bit signed integer
        case RDT_UInt32:	return("UINT32");  // 32-bit unsigned integer
        case RDT_Int32: 	return("INT32");   // 32-bit signed integer
        case RDT_UInt64:    return("UINT64");  // 64-bit unsigned integer
        case RDT_Int64:     return("INT64");   // 64-bit signed integer
        case RDT_Float: 	return("FLOAT");   // 32-bit floating point
        case RDT_Double: 	return("DOUBLE");  // 64-bit floating point
        default:	        return("Unknown"); // All others
    }
}

RasterDataType StringToRasterDataType(const string& stype) {
    if (StringMatch(stype, "UCHAR") || StringMatch(stype, "UINT8")
        || StringMatch(stype, "GDT_Byte")) { return RDT_UInt8; }
    if (StringMatch(stype, "CHAR") || StringMatch(stype, "INT8")
        || StringMatch(stype, "GDT_Int8")) { return RDT_Int8; }
    if (StringMatch(stype, "UINT16") || StringMatch(stype, "GDT_UInt16")) { return RDT_UInt16; }
    if (StringMatch(stype, "INT16") || StringMatch(stype, "GDT_Int16")) { return RDT_Int16; }
    if (StringMatch(stype, "UINT32") || StringMatch(stype, "GDT_UInt32")) { return RDT_UInt32; }
    if (StringMatch(stype, "INT32") || StringMatch(stype, "GDT_Int32")) { return RDT_Int32; }
    if (StringMatch(stype, "UINT64") || StringMatch(stype, "GDT_UInt64")) { return RDT_UInt64; }
    if (StringMatch(stype, "INT64") || StringMatch(stype, "GDT_Int64")) { return RDT_Int64; }
    if (StringMatch(stype, "FLOAT") || StringMatch(stype, "GDT_Float32")) { return RDT_Float; }
    if (StringMatch(stype, "DOUBLE") || StringMatch(stype, "GDT_Float64")) { return RDT_Double; }
    return RDT_Unknown;
}

RasterDataType TypeToRasterDataType(const std::type_info& t) {
    if (t == typeid(unsigned char) || t == typeid(vuint8_t)) { return RDT_UInt8; }
    if (t == typeid(char) || t == typeid(vint8_t)) { return RDT_Int8; }
    if (t == typeid(vuint16_t)) { return RDT_UInt16; }
    if (t == typeid(vint16_t)) { return RDT_Int16; }
    if (t == typeid(vuint32_t)) { return RDT_UInt32; }
    if (t == typeid(vint32_t)) { return RDT_Int32; }
    if (t == typeid(vuint64_t)) { return RDT_UInt64; }
    if (t == typeid(vint64_t)) { return RDT_Int64; }
    if (t == typeid(float)) { return RDT_Float; }
    if (t == typeid(double)) { return RDT_Double; }
    return RDT_Unknown;
}

double DefaultNoDataByType(const RasterDataType type) {
    switch (type) {
        case RDT_Unknown:   return NODATA_VALUE;  // Unknown type
        case RDT_UInt8:	    return UINT8_MAX;     // 8-bit unsigned integer
        case RDT_Int8:	    return INT8_MIN;      // 8-bit signed integer
        case RDT_UInt16:	return UINT16_MAX;    // 16-bit unsigned integer
        case RDT_Int16:	    return INT16_MIN;     // 16-bit signed integer
        case RDT_UInt32:	return UINT32_MAX;    // 32-bit unsigned integer
        case RDT_Int32: 	return INT32_MIN;     // 32-bit signed integer
        case RDT_UInt64:	return UINT64_MAX;    // 32-bit unsigned integer
        case RDT_Int64: 	return INT64_MIN;     // 32-bit signed integer
        case RDT_Float: 	return MISSINGFLOAT;  // 32-bit floating point
        case RDT_Double: 	return MISSINGFLOAT;  // 64-bit floating point
        default:	        return NODATA_VALUE;  // All others
    }
}

#ifdef USE_GDAL
GDALDataType CvtToGDALDataType(const RasterDataType type) {
    switch (type) {
        case RDT_Unknown:   return GDT_Unknown;  // Unknown
        case RDT_UInt8:	    return GDT_Byte;     // 8-bit unsigned integer
        case RDT_Int8:
#if GDAL_VERSION_MAJOR >= 3 && GDAL_VERSION_MINOR >= 7
            return GDT_Int8;                     // 8-bit signed integer not supported by GDAL<3.7!
#else
            return GDT_Byte;                     // 8-bit signed integer supported by GDAL>=3.7!
#endif
        case RDT_UInt16:	return GDT_UInt16;   // 16-bit unsigned integer
        case RDT_Int16:	    return GDT_Int16;    // 16-bit signed integer
        case RDT_UInt32:	return GDT_UInt32;   // 32-bit unsigned integer
        case RDT_Int32: 	return GDT_Int32;    // 32-bit signed integer
#if GDAL_VERSION_MAJOR >= 3 && GDAL_VERSION_MINOR >= 5
        case RDT_UInt64:	return GDT_UInt64;   // 64-bit unsigned integer
        case RDT_Int64: 	return GDT_Int64;    // 64-bit signed integer
#endif
        case RDT_Float: 	return GDT_Float32;  // 32-bit floating point
        case RDT_Double: 	return GDT_Float64;  // 64-bit floating point
        default:	        return GDT_Unknown;  // All others
    }
}
#endif

STRDBL_MAP InitialHeader() {
    STRDBL_MAP headinfo;
    const char* raster_headers[8] = {
        HEADER_RS_NCOLS, HEADER_RS_NROWS, HEADER_RS_XLL, HEADER_RS_YLL,
        HEADER_RS_CELLSIZE, HEADER_RS_NODATA, HEADER_RS_LAYERS, HEADER_RS_CELLSNUM
    };
    for (int i = 0; i < 8; i++) {
#ifdef HAS_VARIADIC_TEMPLATES
        headinfo.emplace(raster_headers[i], NODATA_VALUE);
#else
        headinfo.insert(make_pair(raster_headers[i], NODATA_VALUE));
#endif
    }
    return headinfo;
}

void CopyHeader(const STRDBL_MAP& refers, STRDBL_MAP& dst) {
    if (refers.empty()) { return; }
    for (auto it = refers.begin(); it != refers.end(); ++it) {
        if (dst.find(it->first) != dst.end()) {
            dst.at(it->first) = it->second;
        }
        else {
#ifdef HAS_VARIADIC_TEMPLATES
            dst.emplace(it->first, it->second);
#else
            dst.insert(make_pair(it->first, it->second));
#endif
        }
    }
}

STRING_MAP InitialStrHeader() {
    STRING_MAP options;
#ifdef HAS_VARIADIC_TEMPLATES
    options.emplace(HEADER_RS_SRS, "");
    options.emplace(HEADER_RS_DATATYPE, "");
    options.emplace(HEADER_RSOUT_DATATYPE, "");
    options.emplace(HEADER_INC_NODATA, "");
    options.emplace(HEADER_MASK_NAME, "");
#else
    options.insert(make_pair(HEADER_RS_SRS, ""));
    options.insert(make_pair(HEADER_RS_DATATYPE, ""));
    options.insert(make_pair(HEADER_RSOUT_DATATYPE, ""));
    options.insert(make_pair(HEADER_INC_NODATA, ""));
    options.insert(make_pair(HEADER_MASK_NAME, ""));
#endif
    return options;
}

void UpdateStrHeader(STRING_MAP& strheader, const string& key, const string& val) {
    if (strheader.find(key) == strheader.end()) {
#ifdef HAS_VARIADIC_TEMPLATES
        strheader.emplace(key, val);
#else
        strheader.insert(make_pair(key, val));
#endif
    }
    else { strheader.at(key) = val; }
}

RasterDataType RasterDataTypeInOptionals(const STRING_MAP& opts) {
    if (opts.find(HEADER_RSOUT_DATATYPE) != opts.end()) {
        return StringToRasterDataType(opts.at(HEADER_RSOUT_DATATYPE));
    }
    return RDT_Unknown;
}

void InitialStatsMap(STRDBL_MAP& stats, map<string, double*>& stats2d) {
    string statsnames[6] = {
        STATS_RS_VALIDNUM, STATS_RS_MIN, STATS_RS_MAX, STATS_RS_MEAN,
        STATS_RS_STD, STATS_RS_RANGE
    };
    for (int i = 0; i < 6; i++) {
#ifdef HAS_VARIADIC_TEMPLATES
        stats.emplace(statsnames[i], NODATA_VALUE);
        stats2d.emplace(statsnames[i], nullptr);
#else
        stats.insert(make_pair(statsnames[i], NODATA_VALUE));
        stats2d.insert(map<string, double*>::value_type(statsnames[i], nullptr));
#endif
    }
}

bool WriteAscHeaders(const string& filename, const STRDBL_MAP& header) {
    DeleteExistedFile(filename);
    string abs_filename = GetAbsolutePath(filename);
    std::ofstream raster_file(abs_filename.c_str(), std::ios::app | std::ios::out);
    if (!raster_file.is_open()) {
        StatusMessage("Error opening file: " + abs_filename);
        return false;
    }
    int rows = CVT_INT(header.at(HEADER_RS_NROWS));
    int cols = CVT_INT(header.at(HEADER_RS_NCOLS));
    raster_file << HEADER_RS_NCOLS << " " << cols << endl;
    raster_file << HEADER_RS_NROWS << " " << rows << endl;
    raster_file << HEADER_RS_XLL << " " << header.at(HEADER_RS_XLL) << endl;
    raster_file << HEADER_RS_YLL << " " << header.at(HEADER_RS_YLL) << endl;
    raster_file << HEADER_RS_CELLSIZE << " " << header.at(HEADER_RS_CELLSIZE) << endl;
    raster_file << HEADER_RS_NODATA << " " << setprecision(6) << header.at(HEADER_RS_NODATA) << endl;
    raster_file.close();
    return true;
}

/* Start SubsetPositions */
bool SubsetPositions::Initialization() {
    usable = true;
    n_cells = 0;
    n_lyrs = -1;
    g_srow = -1;
    g_erow = -1;
    g_scol = -1;
    g_ecol = -1;
    alloc_ = false;
    local_pos_ = nullptr;
    local_posidx_ = nullptr;
    global_ = nullptr;
    data_ = nullptr;
    data2d_ = nullptr;
    return true;
}

SubsetPositions::SubsetPositions() {
    Initialization();
}

SubsetPositions::SubsetPositions(const int srow, const int erow, const int scol, const int ecol) {
    Initialization();
    g_srow = srow;
    g_erow = erow;
    g_scol = scol;
    g_ecol = ecol;
}

SubsetPositions::SubsetPositions(SubsetPositions*& src, const bool deep_copy) {
    Initialization();
    usable = src->usable;
    n_cells = src->n_cells;
    n_lyrs = src->n_lyrs;
    g_srow = src->g_srow;
    g_erow = src->g_erow;
    g_scol = src->g_scol;
    g_ecol = src->g_ecol;
    if (deep_copy) {
        alloc_ = true;
        Initialize1DArray(n_cells, global_, src->global_);
        Initialize2DArray(n_cells, 2, local_pos_, src->local_pos_);
        Initialize1DArray(n_cells, local_posidx_, src->local_posidx_);
        if (nullptr != src->data_) {
            Initialize1DArray(n_cells, data_, src->data_);
        }
        if (nullptr != src->data2d_) {
            Initialize2DArray(n_cells, n_lyrs, data2d_, src->data2d_);
        }
    }
    else {
        alloc_ = false;
        global_ = src->global_;
        local_pos_ = src->local_pos_;
        local_posidx_ = src->local_posidx_;
        if (nullptr != src->data_) { data_ = src->data_; }
        if (nullptr != src->data2d_) { data2d_ = src->data2d_; }
    }
}

SubsetPositions::~SubsetPositions() {
    if (nullptr != local_pos_) {
        if (alloc_) { Release2DArray(local_pos_); }
        else { local_pos_ = nullptr; }
    }
    if (nullptr != local_posidx_) {
        if (alloc_) { Release1DArray(local_posidx_); }
        else { local_posidx_ = nullptr; }
    }
    if (nullptr != global_) {
        if (alloc_) { Release1DArray(global_); }
        else { global_ = nullptr; }
    }
    if (nullptr != data_) { Release1DArray(data_); }
    if (nullptr != data2d_) { Release2DArray(data2d_); }
}

#ifdef USE_MONGODB
bool SubsetPositions::ReadFromMongoDB(MongoGridFs* gfs, const string& fname, const STRING_MAP& opts) {
    double* dbdata = nullptr;
    STRDBL_MAP header_dbl = InitialHeader();
    STRING_MAP header_str = InitialStrHeader();
    if (!ReadGridFsFile(gfs, fname, dbdata, header_dbl, header_str, opts)) { return false; }
    int nrows = g_erow - g_srow + 1;
    int ncols = g_ecol - g_scol + 1;
    int nfull = nrows * ncols;
    int db_ncells = CVT_INT(header_dbl.at(HEADER_RS_CELLSNUM));
    int db_nlyrs = CVT_INT(header_dbl.at(HEADER_RS_LAYERS));
    if ((nfull != db_ncells && n_cells != db_ncells) || db_nlyrs < 0) {
        Release1DArray(dbdata);
        return false;
    }
    n_lyrs = db_nlyrs;
    if (n_lyrs == 1) {
        if (n_cells == db_ncells) {
            bool set_success = SetData(db_ncells, dbdata);
            Release1DArray(dbdata);
            return set_success;
        }
        if (nullptr == data_) {
            Initialize1DArray(n_cells, data_, NODATA_VALUE);
        }
        for (int i = 0; i < n_cells; i++) {
            //data_[i] = dbdata[local_pos_[i][0] * ncols + local_pos_[i][1]];
            data_[i] = dbdata[local_posidx_[i]];
        }
    }
    else {
        if (nullptr == data2d_) {
            Initialize2DArray(n_cells, n_lyrs, data2d_, NODATA_VALUE);
        }
        for (int i = 0; i < n_cells; i++) {
            for (int j = 0; j < n_lyrs; j++) {
                if (nfull == db_ncells) { // consider data from MongoDB is fullsize data
                    //data2d_[i][j] = dbdata[(local_pos_[i][0] * ncols + local_pos_[i][1]) * n_lyrs + j];
                    data2d_[i][j] = dbdata[local_posidx_[i] * n_lyrs + j];
                }
                else { data2d_[i][j] = dbdata[i * n_lyrs + j]; }
            }
        }
    }
    usable = true;
    Release1DArray(dbdata);
    return true;
}
#endif

void SubsetPositions::GetHeader(const double gxll, const double gyll, const int gnrows,
                                const double cellsize, const double nodata, STRDBL_MAP& subheader) {
    UpdateHeader(subheader, HEADER_RS_XLL, g_scol * cellsize + gxll);
    UpdateHeader(subheader, HEADER_RS_YLL, (gnrows - g_erow - 1) * cellsize + gyll);
    UpdateHeader(subheader, HEADER_RS_NODATA, nodata);
    UpdateHeader(subheader, HEADER_RS_NROWS, g_erow - g_srow + 1);
    UpdateHeader(subheader, HEADER_RS_NCOLS, g_ecol - g_scol + 1);
    UpdateHeader(subheader, HEADER_RS_CELLSIZE, cellsize);
    UpdateHeader(subheader, HEADER_RS_CELLSNUM, -1);
    UpdateHeader(subheader, HEADER_RS_LAYERS, -1);
}

/* End SubsetPositions */
} // namespace data_raster
} // namespace ccgl
