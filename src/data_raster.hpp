/*!
 * \file data_raster.hpp
 * \brief Define Raster class to handle raster data
 *        Part of the Common Cross-platform Geographic Library (CCGL)
 *
 * \details
 *   - 1. Using GDAL and MongoDB (currently, up to mongo-c-driver 1.16.2 was tested)
 *   - 2. Array1D and Array2D raster data are supported
 *   - 3. C++11 supported
 *   - 4. Unit Tests based Google Test.
 *
 * \remarks
 *   - 1. Dec. 2016 lj Separated from SEIMS to a common library for widely use.
 *   - 2. Mar. 2017 lj VLD check, bug fixed, function enhanced.
 *   - 3. Apr. 2017 lj Avoid try...catch block
 *   - 4. May. 2017 lj Use MongoDB wrapper
 *   - 5. Nov. 2017 lj Code review based on C++11 and use a single header file.
 *   - 6. Dec. 2017 lj Add Unittest based on Google Test.
 *   - 7. Mar. 2018 lj Make part of CCGL, and make GDAL optional. Follows Google C++ Code Style.
 *   - 8. Jun. 2018 lj Use emplace and emplace_back rather than insert and push_back whenever possible.
 *   - 9. Nov. 2018 lj Add specific field-value as options of raster data, including SRS.
 *   -10. Jul. 2021 lj No need to use pointer-to-pointer as arguments in GetValue and GetValueByIndex.
 *   -11. Apr. 2022 lj Comprehensive functional testing, bug fixing, and robustness improving.
 *                     Add subset feature to support data decomposition and combination.
 *   -12. Jul. 2023 lj Add valid position index (1D array, pos_idx_) and will remove pos_data_ in next version.
 *   -13. Aug. 2023 lj Add GDAL data types added from versions 3.5 and 3.7
 *
 * \author Liangjun Zhu, zlj(at)lreis.ac.cn
 * \version 2.8
 */
#ifndef CCGL_DATA_RASTER_H
#define CCGL_DATA_RASTER_H

// include base headers
#include <string>
#include <map>
#include <set>
#include <fstream>
#include <iomanip>
#include <typeinfo>
#include <cassert>
// include openmp if supported
#ifdef SUPPORT_OMP
#include <omp.h>
#endif /* SUPPORT_OMP */

// include GDAL, optional
#ifdef USE_GDAL
#include "gdal.h"
#include "gdal_priv.h"
#include "cpl_string.h"
#include "ogr_spatialref.h"
#endif /* USE_GDAL */

#include "basic.h"
#include "utils_filesystem.h"
#include "utils_string.h"
#include "utils_array.h"
#include "utils_math.h"
// include MongoDB, optional
#ifdef USE_MONGODB
#include "db_mongoc.h"
#endif /* USE_MONGODB */

using std::map;
using std::set;
#ifndef HAS_VARIADIC_TEMPLATES // Not support emplace(), e.g., MSVC2010
using std::make_pair;
#endif
using std::setprecision;

#ifndef CONST_CHARS
#define CONST_CHARS static const char* ///< const string
#endif

namespace ccgl {
using namespace utils_string;
using namespace utils_filesystem;
using namespace utils_array;
using namespace utils_math;
#ifdef USE_MONGODB
using namespace db_mongoc;
#endif /* USE_MONGODB */

/*!
 * \namespace ccgl::data_raster
 * \brief Raster class to handle various raster data
 */
namespace data_raster {
CONST_CHARS HEADER_RS_NODATA = "NODATA_VALUE"; /// NoData value
CONST_CHARS HEADER_RS_XLL = "XLLCENTER";  /// X coordinate value of left low center
CONST_CHARS HEADER_RS_YLL = "YLLCENTER";  /// Y coordinate value of left low center
CONST_CHARS HEADER_RS_XLLCOR = "XLLCORNER";  /// X coordinate value of left low center
CONST_CHARS HEADER_RS_YLLCOR = "YLLCORNER"; ///  Y coordinate value of left low center
CONST_CHARS HEADER_RS_NROWS = "NROWS"; /// Rows number
CONST_CHARS HEADER_RS_NCOLS = "NCOLS"; /// Column number
CONST_CHARS HEADER_RS_CELLSIZE = "CELLSIZE"; /// Cell size (length)
CONST_CHARS HEADER_RS_LAYERS = "LAYERS"; /// Layers number
CONST_CHARS HEADER_RS_CELLSNUM = "CELLSNUM"; /// Number of the first layer's valid cells
CONST_CHARS HEADER_RS_SRS = "SRS"; /// SRS
CONST_CHARS HEADER_RS_DATATYPE = "DATATYPE"; /// Data type of original raster
CONST_CHARS HEADER_RSOUT_DATATYPE = "DATATYPE_OUT"; /// Desired output data type of raster
CONST_CHARS HEADER_INC_NODATA = "INCLUDE_NODATA"; /// Include nodata ("TRUE") or not ("FALSE"), for DB only
CONST_CHARS HEADER_MASK_NAME = "MASK_NAME"; /// Mask layer's name if only store valid values
CONST_CHARS STATS_RS_VALIDNUM = "VALID_CELLNUMBER"; /// Valid cell number
CONST_CHARS STATS_RS_MEAN = "MEAN"; /// Mean value
CONST_CHARS STATS_RS_MIN = "MIN"; /// Minimum value
CONST_CHARS STATS_RS_MAX = "MAX"; /// Maximum value
CONST_CHARS STATS_RS_STD = "STD"; /// Standard derivation value
CONST_CHARS STATS_RS_RANGE = "RANGE"; /// Range value
CONST_CHARS ASCIIExtension = "asc"; /// ASCII extension
CONST_CHARS GTiffExtension = "tif"; /// GeoTIFF extension

CONST_CHARS HEADER_RS_PARAM_ABSTRACTION_TYPE = "PARAM_ABSTRACTION_TYPE"; /// spatial parameter type, physical or conceptual
CONST_CHARS PARAM_ABSTRACTION_TYPE_CONEPTUAL = "CONCEPTUAL";
CONST_CHARS PARAM_ABSTRACTION_TYPE_PHYSICAL = "PHYSICAL";


typedef std::pair<int, int> ROW_COL; /// Row and Col pair
typedef std::pair<double, double> XY_COOR; /// Coordinate pair

/*!
 * \brief Raster data types follows GDALDataType
 */
typedef enum {
    RDT_Unknown,  ///< GDT_Unknown
    RDT_UInt8,    ///< GDT_Byte
    RDT_Int8,     ///< GDT_Int8, GDAL>=3.7
    RDT_UInt16,   ///< GDT_UInt16
    RDT_Int16,    ///< GDT_Int16
    RDT_UInt32,   ///< GDT_UInt32
    RDT_Int32,    ///< GDT_Int32
    RDT_UInt64,   ///< GDT_UInt64, GDAL>=3.5
    RDT_Int64,    ///< GDT_Int64, GDAL>=3.5
    RDT_Float,    ///< GDT_Float32
    RDT_Double    ///< GDT_Float64
} RasterDataType;

/** Common functions independent to clsRasterData **/

/*!
 * \brief Convert RasterDataType to string
 */
string RasterDataTypeToString(int type);

/*!
 * \brief Convert string to RasterDataType
 */
RasterDataType StringToRasterDataType(const string& stype);

/*!
 * \brief Convert C++ data type to RasterDataType
 */
RasterDataType TypeToRasterDataType(const std::type_info& t);

/*!
 * \brief Default NoData value by data type
 */
double DefaultNoDataByType(RasterDataType type);

#ifdef USE_GDAL
GDALDataType CvtToGDALDataType(RasterDataType type);
#endif

/*!
 * \brief Initialize header information in double
 */
STRDBL_MAP InitialHeader();

/*!
 * \brief Copy header information from one to another
 * \param[in] refers Reference header
 * \param[in] dst Destination header
 */
void CopyHeader(const STRDBL_MAP& refers, STRDBL_MAP& dst);

/*!
 * \brief Update value in header information
 */
template <typename T>
void UpdateHeader(STRDBL_MAP& header, const string& key, T val) {
    if (header.find(key) == header.end()) {
#ifdef HAS_VARIADIC_TEMPLATES
        header.emplace(key, CVT_DBL(val));
#else
        header.insert(make_pair(key, CVT_DBL(val)));
#endif
    }
    else { header.at(key) = CVT_DBL(val); }
}

/*!
 * \brief Initialize header information in string
 */
STRING_MAP InitialStrHeader();

/*!
 * \brief Update header information in string
 */
void UpdateStrHeader(STRING_MAP& strheader, const string& key, const string& val);
/*!
 * \brief Get output raster data type from optional inputs
 */
RasterDataType RasterDataTypeInOptionals(const STRING_MAP& opts);

/*!
 * \brief Initialize statistics values for 1D and 2D raster data
 */
void InitialStatsMap(STRDBL_MAP& stats, map<string, double*>& stats2d);

/*!
 * \brief Read raster data from ASC file, the simply usage
 * \param[in] filename Full path of ASC raster file
 * \param[out] header Raster header information
 * \param[out] values All raster values in 1d-array including NODATA_VALUE
 * \return true if read successfully, otherwise return false.
 */
template <typename T>
bool ReadAscFile(const string& filename, STRDBL_MAP& header, T*& values) {
    StatusMessage(("Read " + filename + "...").c_str());
    vector<string> content_strs;
    if (!LoadPlainTextFile(filename, content_strs)) { return false; }
    if (content_strs.size() < 7) {
        StatusMessage("Error: ASCII raster data requires at least 7 lines!");
        return false;
    }
    double cellsize = -1.;
    double xll = MISSINGFLOAT;
    double yll = MISSINGFLOAT;
    double nodata = MISSINGFLOAT;
    bool xllcorner = false;
    bool yllcorner = false;
    int rows = -1;
    int cols = -1;
    int vidx = 0;
    for (auto iter = content_strs.begin(); iter != content_strs.end(); ++iter) {
        vector<string> tokens = SplitString(*iter);
        if (tokens.empty()) { continue; }
        bool str2value = false;
        if (StringMatch(tokens[0], HEADER_RS_NCOLS)) {
            if (tokens.size() < 2) { return false; }
            cols = CVT_INT(IsInt(tokens[1], str2value));
            if (!str2value) { return false; }
        }
        else if (StringMatch(tokens[0], HEADER_RS_NROWS)) {
            if (tokens.size() < 2) { return false; }
            rows = CVT_INT(IsInt(tokens[1], str2value));
            if (!str2value) { return false; }
        }
        else if (StringMatch(tokens[0], HEADER_RS_XLL) ||
            StringMatch(tokens[0], HEADER_RS_XLLCOR)) {
            if (tokens.size() < 2) { return false; }
            xll = IsDouble(tokens[1], str2value);
            if (StringMatch(tokens[0], HEADER_RS_XLLCOR)) { xllcorner = true; }
            if (!str2value) { return false; }
        }
        else if (StringMatch(tokens[0], HEADER_RS_YLL) ||
            StringMatch(tokens[0], HEADER_RS_YLLCOR)) {
            if (tokens.size() < 2) { return false; }
            yll = IsDouble(tokens[1], str2value);
            if (StringMatch(tokens[0], HEADER_RS_YLLCOR)) { yllcorner = true; }
            if (!str2value) { return false; }
        }
        else if (StringMatch(tokens[0], HEADER_RS_CELLSIZE)) {
            if (tokens.size() < 2) { return false; }
            cellsize = IsDouble(tokens[1], str2value);
            if (!str2value) { return false; }
        }
        else if (StringMatch(tokens[0], HEADER_RS_NODATA)) {
            if (tokens.size() < 2) { return false; }
            nodata = IsDouble(tokens[1], str2value);
            if (!str2value) { return false; }
        }
        else {
            if (rows < 0 || cols < 0) {
                StatusMessage("Warning: NCOLS and NROWS should be defined first!");
            }
            if (nullptr == values) { Initialize1DArray(rows * cols, values, nodata); }
            for (auto it_value = tokens.begin(); it_value != tokens.end(); ++it_value) {
                double tmpv = IsDouble(*it_value, str2value);
                if (!str2value) {
                    StatusMessage("Error: No value occurred in Raster data matrix!");
                    if (nullptr != values) { Release1DArray(values); }
                    return false;
                }
                if (vidx == rows * cols) {
                    StatusMessage("Error: Count of values MUST equal to rows * cols!");
                    if (nullptr != values) { Release1DArray(values); }
                    return false;
                }
                values[vidx++] = static_cast<T>(tmpv);
            }
        }
    }
    if (rows < 0 || cols < 0 || cellsize < 0 ||
        FloatEqual(xll, MISSINGFLOAT) || FloatEqual(yll, MISSINGFLOAT)) {
        StatusMessage("Error: Header information incomplete!");
        return false;
    }
    // default is center, if corner, then:
    if (xllcorner) { xll += 0.5 * cellsize; }
    if (yllcorner) { yll += 0.5 * cellsize; }
    UpdateHeader(header, HEADER_RS_NCOLS, cols);
    UpdateHeader(header, HEADER_RS_NROWS, rows);
    UpdateHeader(header, HEADER_RS_XLL, xll);
    UpdateHeader(header, HEADER_RS_YLL, yll);
    UpdateHeader(header, HEADER_RS_CELLSIZE, cellsize);
    UpdateHeader(header, HEADER_RS_NODATA, nodata);
    UpdateHeader(header, HEADER_RS_LAYERS, 1);
    UpdateHeader(header, HEADER_RS_CELLSNUM, cols * rows);

    return true;
}

/*!
 * \brief Write raster header information into a ASC file. If the file exists, delete it first.
 * \param[in] filename ASC full file path
 * \param[in] header header information
 */
bool WriteAscHeaders(const string& filename, const STRDBL_MAP& header);

/*!
 * \brief Write raster data as a single ASC file. If the file exists, delete it first.
 * \param[in] filename ASC full file path
 * \param[in] header header information
 * \param[in] values raster data array
 */
template <typename T>
bool WriteSingleAsc(const string& filename, const STRDBL_MAP& header, T* values) {
    string abs_filename = GetAbsolutePath(filename);
    string dirname = GetPathFromFullName(abs_filename);
    if (!PathExists(dirname)) { MakeDirectory(dirname); }
    if (!WriteAscHeaders(abs_filename, header)) { return false; }
    std::ofstream raster_file(abs_filename.c_str(), std::ios::app | std::ios::out);
    if (!raster_file.is_open()) {
        StatusMessage("Error opening file: " + abs_filename);
        return false;
    }
    int rows = CVT_INT(header.at(HEADER_RS_NROWS));
    int cols = CVT_INT(header.at(HEADER_RS_NCOLS));
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            raster_file << setprecision(6) << values[i * cols + j] << " ";
        }
        raster_file << endl;
    }
    raster_file << endl;
    raster_file.close();
    return true;
}

#ifdef USE_GDAL
/*!
 * \brief Read single raster file by GDAL
 * \param[in] filename Full path of raster data
 * \param[out] header Raster header information
 * \param[out] values Raster data matrix
 * \param[out] in_type Raster data type
 * \param[out] srs SRS of input raster data as string
 * \return true if read successfully, otherwise return false.
 */
template <typename T>
bool ReadRasterFileByGdal(const string& filename, STRDBL_MAP& header, T*& values,
                          RasterDataType& in_type, string& srs) {
    StatusMessage(("Read " + filename + "...").c_str());
    GDALDataset* po_dataset = static_cast<GDALDataset*>(GDALOpen(filename.c_str(),
                                                                 GA_ReadOnly));
    if (nullptr == po_dataset) {
        StatusMessage("Open file " + filename + " failed.");
        return false;
    }
    GDALRasterBand* po_band = po_dataset->GetRasterBand(1);
    int n_rows = po_band->GetYSize();
    int n_cols = po_band->GetXSize();
    int get_value_flag = false;
    double nodata = po_band->GetNoDataValue(&get_value_flag);
    int approx_minmax = false;
    double minmax[2];
    T* tmprasterdata = nullptr;
    bool read_as_signedbyte = false;
    unsigned char* uchar_data = nullptr;
    signed char* char_data = nullptr; // DO NOT use char*
    vuint16_t* uint16_data = nullptr; // 16-bit unsigned integer
    vint16_t* int16_data = nullptr;   // 16-bit signed integer
    vuint32_t* uint32_data = nullptr; // 32-bit unsigned integer
    vint32_t* int32_data = nullptr;   // 32-bit signed integer
    vuint64_t* uint64_data = nullptr; // 64-bit unsigned integer
    vint64_t* int64_data = nullptr;   // 64-bit signed integer
    float* float_data = nullptr;
    double* double_data = nullptr;
    CPLErr result;
    switch (po_band->GetRasterDataType()) {
    case GDT_Byte:
        // In GDAL, GDT_Byte is an 8-bit unsigned integer (unsigned char), ranges from 0 to 255.
        // While in ArcGIS, both 8-bit signed char (ranges from -128 to 127) and unsigned char
        //   are supported. Both types will be recognized as GDT_Byte by GDAL.
        //
        // So,
        //    1) maximum <= 127 and minimum < 0 ==> signed char
        //    2) maximum <= 127 and minimum >= 0 and no_data_value_ < 0 ==> signed char
        // Otherwise, unsigned char.
        //
        // Update (08/09/2023): GDAL>=3.7 added the support of GDT_Int8. Keep this code for compatibility!
        //
        po_band->ComputeRasterMinMax(approx_minmax, minmax);
        if ((minmax[1] <= 127 && minmax[0] < 0)
            || (minmax[1] <= 127 && minmax[0] >= 0 && (!get_value_flag || (get_value_flag && nodata < 0)))) {
            read_as_signedbyte = true;
        }
        uchar_data = static_cast<unsigned char*>(CPLMalloc(sizeof(unsigned char) * n_cols * n_rows));
        result = po_band->RasterIO(GF_Read, 0, 0, n_cols, n_rows, uchar_data,
                                   n_cols, n_rows, GDT_Byte, 0, 0);
        if (result != CE_None) {
            StatusMessage("RaterIO trouble: " + string(CPLGetLastErrorMsg()));
            GDALClose(po_dataset);
            return false;
        }
        if (read_as_signedbyte) {
            StatusMessage("Read GDT_Byte raster as signed char!");
            Initialize1DArray(n_rows * n_cols, char_data, uchar_data);
            Initialize1DArray(n_rows * n_cols, tmprasterdata, char_data);
            Release1DArray(char_data);
            in_type = RDT_Int8;
        } else {
            StatusMessage("Read GDT_Byte raster as unsigned char!");
            Initialize1DArray(n_rows * n_cols, tmprasterdata, uchar_data);
            in_type = RDT_UInt8;
        }
        CPLFree(uchar_data);
        break;
#if GDAL_VERSION_MAJOR >= 3 && GDAL_VERSION_MINOR >= 7
    case GDT_Int8:
            char_data = static_cast<signed char*>(CPLMalloc(sizeof(signed char) * n_cols * n_rows));
            result = po_band->RasterIO(GF_Read, 0, 0, n_cols, n_rows, char_data,
                                       n_cols, n_rows, GDT_Int8, 0, 0);
            if (result != CE_None) {
                StatusMessage("RaterIO trouble: " + string(CPLGetLastErrorMsg()));
                GDALClose(po_dataset);
                return false;
            }
            Initialize1DArray(n_rows * n_cols, tmprasterdata, char_data);
            CPLFree(char_data);
            in_type = RDT_Int8;
            break;
#endif
    case GDT_UInt16:
        uint16_data = static_cast<vuint16_t*>(CPLMalloc(sizeof(vuint16_t) * n_cols * n_rows));
        result = po_band->RasterIO(GF_Read, 0, 0, n_cols, n_rows, uint16_data,
                                   n_cols, n_rows, GDT_UInt16, 0, 0);
        if (result != CE_None) {
            StatusMessage("RaterIO trouble: " + string(CPLGetLastErrorMsg()));
            GDALClose(po_dataset);
            return false;
        }
        Initialize1DArray(n_rows * n_cols, tmprasterdata, uint16_data);
        CPLFree(uint16_data);
        in_type = RDT_UInt16;
        break;
    case GDT_Int16:
        int16_data = static_cast<vint16_t*>(CPLMalloc(sizeof(vint16_t) * n_cols * n_rows));
        result = po_band->RasterIO(GF_Read, 0, 0, n_cols, n_rows, int16_data,
                                   n_cols, n_rows, GDT_Int16, 0, 0);
        if (result != CE_None) {
            StatusMessage("RaterIO trouble: " + string(CPLGetLastErrorMsg()));
            GDALClose(po_dataset);
            return false;
        }
        Initialize1DArray(n_rows * n_cols, tmprasterdata, int16_data);
        CPLFree(int16_data);
        in_type = RDT_Int16;
        break;
    case GDT_UInt32:
        uint32_data = static_cast<vuint32_t*>(CPLMalloc(sizeof(vuint32_t) * n_cols * n_rows));
        result = po_band->RasterIO(GF_Read, 0, 0, n_cols, n_rows, uint32_data,
                                   n_cols, n_rows, GDT_UInt32, 0, 0);
        if (result != CE_None) {
            StatusMessage("RaterIO trouble: " + string(CPLGetLastErrorMsg()));
            GDALClose(po_dataset);
            return false;
        }
        Initialize1DArray(n_rows * n_cols, tmprasterdata, uint32_data);
        CPLFree(uint32_data);
        in_type = RDT_UInt32;
        break;
    case GDT_Int32:
        int32_data = static_cast<vint32_t*>(CPLMalloc(sizeof(vint32_t) * n_cols * n_rows));
        result = po_band->RasterIO(GF_Read, 0, 0, n_cols, n_rows, int32_data,
                                   n_cols, n_rows, GDT_Int32, 0, 0);
        if (result != CE_None) {
            StatusMessage("RaterIO trouble: " + string(CPLGetLastErrorMsg()));
            GDALClose(po_dataset);
            return false;
        }
        Initialize1DArray(n_rows * n_cols, tmprasterdata, int32_data);
        CPLFree(int32_data);
        in_type = RDT_Int32;
        break;
#if GDAL_VERSION_MAJOR >= 3 && GDAL_VERSION_MINOR >= 5
    case GDT_UInt64:
        uint64_data = static_cast<vuint64_t*>(CPLMalloc(sizeof(vuint64_t) * n_cols * n_rows));
        result = po_band->RasterIO(GF_Read, 0, 0, n_cols, n_rows, uint64_data,
                                   n_cols, n_rows, GDT_UInt64, 0, 0);
        if (result != CE_None) {
            StatusMessage("RaterIO trouble: " + string(CPLGetLastErrorMsg()));
            GDALClose(po_dataset);
            return false;
        }
        Initialize1DArray(n_rows * n_cols, tmprasterdata, uint64_data);
        CPLFree(uint64_data);
        in_type = RDT_UInt64;
        break;
    case GDT_Int64:
        int64_data = static_cast<vint64_t*>(CPLMalloc(sizeof(vint64_t) * n_cols * n_rows));
        result = po_band->RasterIO(GF_Read, 0, 0, n_cols, n_rows, int64_data,
                                   n_cols, n_rows, GDT_Int64, 0, 0);
        if (result != CE_None) {
            StatusMessage("RaterIO trouble: " + string(CPLGetLastErrorMsg()));
            GDALClose(po_dataset);
            return false;
        }
        Initialize1DArray(n_rows * n_cols, tmprasterdata, int64_data);
        CPLFree(int64_data);
        in_type = RDT_Int64;
        break;
#endif
    case GDT_Float32:
        float_data = static_cast<float*>(CPLMalloc(sizeof(float) * n_cols * n_rows));
        result = po_band->RasterIO(GF_Read, 0, 0, n_cols, n_rows, float_data,
                                   n_cols, n_rows, GDT_Float32, 0, 0);
        if (result != CE_None) {
            StatusMessage("RaterIO trouble: " + string(CPLGetLastErrorMsg()));
            GDALClose(po_dataset);
            return false;
        }
        Initialize1DArray(n_rows * n_cols, tmprasterdata, float_data);
        CPLFree(float_data);
        in_type = RDT_Float;
        break;
    case GDT_Float64:
        double_data = static_cast<double*>(CPLMalloc(sizeof(double) * n_cols * n_rows));
        result = po_band->RasterIO(GF_Read, 0, 0, n_cols, n_rows, double_data,
                                   n_cols, n_rows, GDT_Float64, 0, 0);
        if (result != CE_None) {
            StatusMessage("RaterIO trouble: " + string(CPLGetLastErrorMsg()));
            GDALClose(po_dataset);
            return false;
        }
        Initialize1DArray(n_rows * n_cols, tmprasterdata, double_data);
        CPLFree(double_data);
        in_type = RDT_Double;
        break;
    default:
        StatusMessage("Unexpected GDALDataType: " + string(RasterDataTypeToString(in_type)));
        GDALClose(po_dataset);
        return false;
    }

    if (!get_value_flag) { // NoData value is not defined!
        nodata = DefaultNoDataByType(in_type);
    }

    double geo_trans[6];
    po_dataset->GetGeoTransform(geo_trans);
    UpdateHeader(header, HEADER_RS_NCOLS, n_cols);
    UpdateHeader(header, HEADER_RS_NROWS, n_rows);
    UpdateHeader(header, HEADER_RS_NODATA, nodata);
    UpdateHeader(header, HEADER_RS_CELLSIZE, geo_trans[1]);
    UpdateHeader(header, HEADER_RS_XLL, geo_trans[0] + 0.5 * geo_trans[1]);
    UpdateHeader(header, HEADER_RS_YLL, geo_trans[3] + (n_rows - 0.5) * geo_trans[5]);
    UpdateHeader(header, HEADER_RS_LAYERS, 1.);
    UpdateHeader(header, HEADER_RS_CELLSNUM, n_cols * n_rows);
    srs = string(po_dataset->GetProjectionRef());

    GDALClose(po_dataset);

    values = tmprasterdata;
    return true;
}

/*!
 * \brief Write single geotiff file
 * If the file exists, delete it first.
 * \param[in] filename \a string, output ASC file path
 * \param[in] header header information
 * \param[in] opts Options, e.g., `srs` - Coordinate system string
 * \param[in] values raster data array
 */
template <typename T>
bool WriteSingleGeotiff(const string& filename, const STRDBL_MAP& header,
                        const STRING_MAP& opts, T* values) {
    int n_rows = CVT_INT(header.at(HEADER_RS_NROWS));
    int n_cols = CVT_INT(header.at(HEADER_RS_NCOLS));
    char** papsz_options = nullptr;
    void* new_values = nullptr;
    bool recreate_flag = true; // recreate data array because inconsistent of datatypes
    double old_nodata = header.at(HEADER_RS_NODATA);
    bool change_nodata = false;
    double new_nodata = old_nodata; // change nodata when convert signed datatype to unsigned
    bool convert_permit = true; // DO NOT allow negative cell values be converted to unsigned datatype
    RasterDataType tmptype = TypeToRasterDataType(typeid(T));
    RasterDataType outtype = RasterDataTypeInOptionals(opts);
    if (outtype == RDT_Unknown) {
        outtype = tmptype;
        recreate_flag = false;
    }
    else if (outtype == tmptype && outtype != RDT_Int8) { // RDT_Int8 need special handling
        recreate_flag = false;
    }
    else {
        if (outtype == RDT_UInt8) { // [0, 255] --> GDT_Byte in GDAL, use unsigned char
            new_values = static_cast<unsigned char*>(CPLMalloc(sizeof(unsigned char) * n_cols * n_rows));
            unsigned char* values_uchar = static_cast<unsigned char*>(new_values);
            if (old_nodata < 0 || old_nodata > UINT8_MAX) {
                new_nodata = UINT8_MAX;
                change_nodata = true;
            }
            int illegal_count = 0;
#pragma omp parallel for reduction(+:illegal_count)
            for (int i = 0; i < n_cols * n_rows; i++) {
                if (FloatEqual(values[i], old_nodata) && change_nodata) {
                    values_uchar[i] = UINT8_MAX;
                    continue;
                }
                if (values[i] < 0 || values[i] > UINT8_MAX) illegal_count += 1;
                values_uchar[i] = static_cast<unsigned char>(values[i]);
            }
            if (illegal_count > 0) convert_permit = false;
        }
        else if (outtype == RDT_Int8) { // [-128, 127]
            // https://gdal.org/drivers/raster/gtiff.html
#if GDAL_VERSION_MAJOR < 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR < 7)
            papsz_options = CSLSetNameValue(papsz_options, "PIXELTYPE", "SIGNEDBYTE");
#endif
            new_values = static_cast<signed char*>(CPLMalloc(sizeof(signed char) * n_cols * n_rows));
            signed char* values_char = static_cast<signed char*>(new_values);
            if (old_nodata < INT8_MIN || old_nodata > INT8_MAX) {
                new_nodata = INT8_MIN;
                change_nodata = true;
            }
            int illegal_count = 0;
#pragma omp parallel for reduction(+:illegal_count)
            for (int i = 0; i < n_cols * n_rows; i++) {
                if (FloatEqual(values[i], old_nodata) && change_nodata) {
                    values_char[i] = INT8_MIN;
                    continue;
                }
                if (values[i] < INT8_MIN || values[i] > INT8_MAX) { illegal_count += 1; }
                values_char[i] = static_cast<signed char>(values[i]);
            }
            if (illegal_count > 0) convert_permit = false;
        }
        else if (outtype == RDT_UInt16) { // [0, 65535]
            new_values = static_cast<vuint16_t*>(CPLMalloc(sizeof(vuint16_t) * n_cols * n_rows));
            vuint16_t* values_uint16 = static_cast<vuint16_t*>(new_values);
            if (old_nodata < 0 || old_nodata > UINT16_MAX) {
                new_nodata = UINT16_MAX;
                change_nodata = true;
            }
            int illegal_count = 0;
#pragma omp parallel for reduction(+:illegal_count)
            for (int i = 0; i < n_cols * n_rows; i++) {
                if (FloatEqual(values[i], old_nodata) && change_nodata) {
                    values_uint16[i] = UINT16_MAX;
                    continue;
                }
                if (values[i] < 0 || values[i] > UINT16_MAX) illegal_count += 1;
                values_uint16[i] = static_cast<vuint16_t>(values[i]);
            }
            if (illegal_count > 0) convert_permit = false;
        }
        else if (outtype == RDT_Int16) { // [-32768, 32767]
            new_values = static_cast<vint16_t*>(CPLMalloc(sizeof(vint16_t) * n_cols * n_rows));
            vint16_t* values_int16 = static_cast<vint16_t*>(new_values);
            if (old_nodata < INT16_MIN || old_nodata > INT16_MAX) {
                new_nodata = INT16_MIN;
                change_nodata = true;
            }
            int illegal_count = 0;
#pragma omp parallel for reduction(+:illegal_count)
            for (int i = 0; i < n_cols * n_rows; i++) {
                if (FloatEqual(values[i], old_nodata) && change_nodata) {
                    values_int16[i] = INT16_MIN;
                    continue;
                }
                if (values[i] < INT16_MIN || values[i] > INT16_MAX) illegal_count += 1;
                values_int16[i] = static_cast<vint16_t>(values[i]);
            }
            if (illegal_count > 0) convert_permit = false;
        }
        else if (outtype == RDT_UInt32) { // [0, 4294967295]
            new_values = static_cast<vuint32_t*>(CPLMalloc(sizeof(vuint32_t) * n_cols * n_rows));
            vuint32_t* values_uint32 = static_cast<vuint32_t*>(new_values);
            if (old_nodata < 0 || old_nodata > UINT32_MAX) {
                new_nodata = UINT32_MAX;
                change_nodata = true;
            }
            int illegal_count = 0;
#pragma omp parallel for reduction(+:illegal_count)
            for (int i = 0; i < n_cols * n_rows; i++) {
                if (FloatEqual(values[i], old_nodata) && change_nodata) {
                    values_uint32[i] = UINT32_MAX;
                    continue;
                }
                if (values[i] < 0 || values[i] > UINT32_MAX) illegal_count += 1;
                values_uint32[i] = static_cast<vuint32_t>(values[i]);
            }
            if (illegal_count > 0) convert_permit = false;
        }
        else if (outtype == RDT_Int32) { // [-2147483648, 2147483647]
            new_values = static_cast<vint32_t*>(CPLMalloc(sizeof(vint32_t) * n_cols * n_rows));
            vint32_t* values_int32 = static_cast<vint32_t*>(new_values);
            if (old_nodata < INT32_MIN || old_nodata > INT32_MAX) {
                new_nodata = INT32_MIN;
                change_nodata = true;
            }
            int illegal_count = 0;
#pragma omp parallel for reduction(+:illegal_count)
            for (int i = 0; i < n_cols * n_rows; i++) {
                if (FloatEqual(values[i], old_nodata) && change_nodata) {
                    values_int32[i] = INT32_MIN;
                    continue;
                }
                if (values[i] < INT32_MIN || values[i] > INT32_MAX) illegal_count += 1;
                values_int32[i] = static_cast<vint32_t>(values[i]);
            }
            if (illegal_count > 0) convert_permit = false;
        }
#if GDAL_VERSION_MAJOR >= 3 && GDAL_VERSION_MINOR >=5
        else if (outtype == RDT_UInt64) { // [0, 18446744073709551615]
            new_values = static_cast<vuint64_t*>(CPLMalloc(sizeof(vuint64_t) * n_cols * n_rows));
            vuint64_t* values_uint64 = static_cast<vuint64_t*>(new_values);
            if (old_nodata < 0 || old_nodata > UINT64_MAX) {
                new_nodata = UINT64_MAX;
                change_nodata = true;
            }
            int illegal_count = 0;
#pragma omp parallel for reduction(+:illegal_count)
            for (int i = 0; i < n_cols * n_rows; i++) {
                if (FloatEqual(values[i], old_nodata) && change_nodata) {
                    values_uint64[i] = UINT64_MAX;
                    continue;
                }
                if (values[i] < 0 || values[i] > UINT64_MAX) illegal_count += 1;
                values_uint64[i] = static_cast<vuint64_t>(values[i]);
            }
            if (illegal_count > 0) convert_permit = false;
        }
        else if (outtype == RDT_Int64) { // [-18446744073709551615, 18446744073709551615]
            new_values = static_cast<vint64_t*>(CPLMalloc(sizeof(vint64_t) * n_cols * n_rows));
            vint64_t* values_int64 = static_cast<vint64_t*>(new_values);
            if (old_nodata < INT64_MIN || old_nodata > INT64_MAX) {
                new_nodata = INT64_MIN;
                change_nodata = true;
            }
            int illegal_count = 0;
#pragma omp parallel for reduction(+:illegal_count)
            for (int i = 0; i < n_cols * n_rows; i++) {
                if (FloatEqual(values[i], old_nodata) && change_nodata) {
                    values_int64[i] = INT64_MIN;
                    continue;
                }
                if (values[i] < INT64_MIN || values[i] > INT64_MAX) illegal_count += 1;
                values_int64[i] = static_cast<vint64_t>(values[i]);
            }
            if (illegal_count > 0) convert_permit = false;
        }
#endif
        else if (outtype == RDT_Float) {
            new_values = static_cast<float*>(CPLMalloc(sizeof(float) * n_cols * n_rows));
            float* values_float = static_cast<float*>(new_values);
#pragma omp parallel for
            for (int i = 0; i < n_cols * n_rows; i++) values_float[i] = static_cast<float>(values[i]);
        }
        else if (outtype == RDT_Double) {
            new_values = static_cast<double*>(CPLMalloc(sizeof(double) * n_cols * n_rows));
            double* values_double = static_cast<double*>(new_values);
#pragma omp parallel for
            for (int i = 0; i < n_cols * n_rows; i++) values_double[i] = static_cast<double>(values[i]);
        }
    }
    if ((outtype == RDT_Unknown || (recreate_flag && nullptr == new_values))
        || (!convert_permit)) {
        cout << "Error: The specific raster output data type is not allowed!\n";
        if (papsz_options != nullptr) { CSLDestroy(papsz_options); }
        if (new_values != nullptr) { CPLFree(new_values); }
        return false;
    }
    GDALDriver* po_driver = GetGDALDriverManager()->GetDriverByName("GTiff");
    if (nullptr == po_driver) { return false; }
    string dirname = GetPathFromFullName(filename);
    if (!PathExists(dirname)) { MakeDirectory(dirname); }
    GDALDataset* po_dst_ds = po_driver->Create(filename.c_str(), n_cols, n_rows, 1,
                                               CvtToGDALDataType(outtype), papsz_options);
    if (nullptr == po_dst_ds) { return false; }
    GDALRasterBand* po_dst_band = po_dst_ds->GetRasterBand(1);
    CPLErr result = CE_None;
    if (!recreate_flag) {
        result = po_dst_band->RasterIO(GF_Write, 0, 0, n_cols, n_rows, values,
                                       n_cols, n_rows, CvtToGDALDataType(outtype), 0, 0);
    }
    else {
        result = po_dst_band->RasterIO(GF_Write, 0, 0, n_cols, n_rows, new_values,
                                       n_cols, n_rows, CvtToGDALDataType(outtype), 0, 0);
    }
    if (result != CE_None || nullptr == po_dst_band) {
        StatusMessage("RaterIO Error: " + string(CPLGetLastErrorMsg()));
        if (papsz_options != nullptr) { CSLDestroy(papsz_options); }
        if (new_values != nullptr) { CPLFree(new_values); }
        GDALClose(po_dst_ds);
        return false;
    }
    po_dst_band->SetNoDataValue(new_nodata);

    double geo_trans[6]; // Write header information
    geo_trans[0] = header.at(HEADER_RS_XLL) - 0.5 * header.at(HEADER_RS_CELLSIZE);
    geo_trans[1] = header.at(HEADER_RS_CELLSIZE);
    geo_trans[2] = 0.;
    geo_trans[3] = header.at(HEADER_RS_YLL) + (n_rows - 0.5) * header.at(HEADER_RS_CELLSIZE);
    geo_trans[4] = 0.;
    geo_trans[5] = -header.at(HEADER_RS_CELLSIZE);
    po_dst_ds->SetGeoTransform(geo_trans);
    if (opts.find(HEADER_RS_SRS) == opts.end()) {
        po_dst_ds->SetProjection("");
    } else {
        po_dst_ds->SetProjection(opts.at(HEADER_RS_SRS).c_str());
    }
    if (papsz_options != nullptr) { CSLDestroy(papsz_options); }
    if (new_values != nullptr) { CPLFree(new_values); }
    GDALClose(po_dst_ds);

    return true;
}
#endif /* USE_GDAL */

/*!
 * \brief Write single raster file, if the file exists, delete it first.
 * \param[in] filename output full file path
 * \param[in] header header information
 * \param[in] opts Options, e.g., srs: Coordinate system string
 * \param[in] values raster data array
 */
template <typename T>
bool WriteRasterToFile(const string& filename, const STRDBL_MAP& header,
                       const STRING_MAP& opts, T* values) {
    if (GetPathFromFullName(filename).empty()) { return false; }
    string abs_filename = GetAbsolutePath(filename);
    string filetype = GetUpper(GetSuffix(abs_filename));
    if (StringMatch(filetype, ASCIIExtension)) {
        return WriteSingleAsc(abs_filename, header, values);
    }
#ifdef USE_GDAL
    if (StringMatch(filetype, GTiffExtension)) {
        return WriteSingleGeotiff(abs_filename, header, opts, values);
    }
    return WriteSingleGeotiff(ReplaceSuffix(abs_filename, GTiffExtension),
                              header, opts, values);
#else
    StatusMessage("Warning: Without GDAL, ASC file will be exported as default!");
    return WriteSingleAsc(ReplaceSuffix(abs_filename, ASCIIExtension), header, values);
#endif /* USE_GDAL */
}

#ifdef USE_MONGODB
/*!
 * \brief Read GridFs file from MongoDB
 * \param[in] gfs MongoGridFs pointer
 * \param[in] filename GridFs filename
 * \param[out] data Data stored in GridFs file
 * \param[out] header Header information
 * \param[out] header_str Header information in strings
 * \param[in] opts Optional key-value stored in metadata, used to filter GridFs file
 */
template <typename T>
bool ReadGridFsFile(MongoGridFs* gfs, const string& filename,
                    T*& data, STRDBL_MAP& header,
                    STRING_MAP& header_str,
                    const STRING_MAP& opts /* = STRING_MAP() */) {
    // Get stream data and metadata by file name
    vint length;
    char* buf = nullptr;
    if (!gfs->GetStreamData(filename, buf, length, nullptr, &opts) ||
        nullptr == buf) {
        return false;
    }
    bson_t* bmeta = gfs->GetFileMetadata(filename, nullptr, opts);

    // Retrieve raster header values
    bson_iter_t iter; // Loop the metadata, add to `header_str` or `header`
    if (nullptr != bmeta && bson_iter_init(&iter, bmeta)) {
        while (bson_iter_next(&iter)) {
            const char* key = bson_iter_key(&iter);
            if (header.find(key) != header.end()) {
                GetNumericFromBsonIterator(&iter, header[key]);
            }
            else {
                header_str[key] = GetStringFromBsonIterator(&iter);
            }
        }
    }
    bson_destroy(bmeta); // Destroy bson of metadata immediately after use

    int n_rows = CVT_INT(header.at(HEADER_RS_NROWS));
    int n_cols = CVT_INT(header.at(HEADER_RS_NCOLS));
    int n_lyrs = CVT_INT(header.at(HEADER_RS_LAYERS));
    int n_cells = CVT_INT(header.at(HEADER_RS_CELLSNUM));
    if (n_rows < 0 || n_cols < 0 || n_lyrs < 0) { // missing essential metadata
        delete[] buf;
        return false;
    }
    int value_count = n_cells * n_lyrs;
    size_t size_dtype = length / value_count;

    RasterDataType rstype = RDT_Unknown;
    if (header_str.find(HEADER_RSOUT_DATATYPE) != header_str.end()) {
        rstype = StringToRasterDataType(header_str.at(HEADER_RSOUT_DATATYPE));
    }
    if (rstype == RDT_Unknown) {
        StatusMessage("Unknown data type in MongoDB GridFS!");
        delete[] buf;
        return false;
    }

    if (rstype == RDT_Double && size_dtype == sizeof(double)) {
        double* data_dbl = reinterpret_cast<double*>(buf);
        Initialize1DArray(value_count, data, data_dbl);
        //Release1DArray(data_dbl);
    }
    else if (rstype == RDT_Float && size_dtype == sizeof(float)) {
        float* data_flt = reinterpret_cast<float*>(buf);
        Initialize1DArray(value_count, data, data_flt);
        //Release1DArray(data_flt);
    }
    else if (rstype == RDT_Int32 && size_dtype == sizeof(vint32_t)) {
        vint32_t* data_int32 = reinterpret_cast<vint32_t*>(buf);
        Initialize1DArray(value_count, data, data_int32);
        //Release1DArray(data_int32);
    }
    else if (rstype == RDT_UInt32 && size_dtype == sizeof(vuint32_t)) {
        vuint32_t* data_uint32 = reinterpret_cast<vuint32_t*>(buf);
        Initialize1DArray(value_count, data, data_uint32);
        //Release1DArray(data_uint32);
    }
    else if (rstype == RDT_Int64 && size_dtype == sizeof(vint64_t)) {
        vint64_t* data_int64 = reinterpret_cast<vint64_t*>(buf);
        Initialize1DArray(value_count, data, data_int64);
        //Release1DArray(data_int64);
    }
    else if (rstype == RDT_UInt64 && size_dtype == sizeof(vuint64_t)) {
        vuint64_t* data_uint64 = reinterpret_cast<vuint64_t*>(buf);
        Initialize1DArray(value_count, data, data_uint64);
        //Release1DArray(data_uint64);
    }
    else if (rstype == RDT_Int16 && size_dtype == sizeof(vint16_t)) {
        vint16_t* data_int16 = reinterpret_cast<vint16_t*>(buf);
        Initialize1DArray(value_count, data, data_int16);
        //Release1DArray(data_int16);
    }
    else if (rstype == RDT_UInt16 && size_dtype == sizeof(vuint16_t)) {
        vuint16_t* data_uint16 = reinterpret_cast<vuint16_t*>(buf);
        Initialize1DArray(value_count, data, data_uint16);
        //Release1DArray(data_uint16);
    }
    else if (rstype == RDT_Int8 && size_dtype == sizeof(vint8_t)) {
        vint8_t* data_int8 = reinterpret_cast<vint8_t*>(buf);
        Initialize1DArray(value_count, data, data_int8);
        //Release1DArray(data_int8);
    }
    else if (rstype == RDT_UInt8 && size_dtype == sizeof(vuint8_t)) {
        vuint8_t* data_uint8 = reinterpret_cast<vuint8_t*>(buf);
        Initialize1DArray(value_count, data, data_uint8);
        //Release1DArray(data_uint8);
    }
    else {
        StatusMessage("Unconsistent of data type and size!");
        delete[] buf;
        return false;
    }
    delete[] buf;
    return true;
}

/*!
 * \brief Write array data (both valid and full-sized raster data) as GridFS file.
 *        If the file exists, delete it first.
 * \param[in] gfs GridFs of MongoDB
 * \param[in] filename GridFS file name
 * \param[in] header header information
 * \param[in] values float raster data array
 * \param[in] datalength Length of data
 * \param[in] opts (optional) Key-value map for user-specific metadata
 */
template <typename T>
bool WriteStreamDataAsGridfs(MongoGridFs* gfs, const string& filename,
                             STRDBL_MAP& header, T* values, const int datalength,
                             const STRING_MAP& opts = STRING_MAP()) {
    STRING_MAP curopts;
    CopyStringMap(opts, curopts);
    RasterDataType temp_type = TypeToRasterDataType(typeid(T));
    if (curopts.find(HEADER_RSOUT_DATATYPE) == curopts.end()) {
        UpdateStringMap(curopts, HEADER_RSOUT_DATATYPE, RasterDataTypeToString(temp_type));
    }
    gfs->RemoveFile(filename, nullptr, curopts);
    bson_t p = BSON_INITIALIZER;
    double intpart; // https://stackoverflow.com/a/1521682/4837280
    for (auto iter = header.begin(); iter != header.end(); ++iter) {
        if (!StringMatch(HEADER_RS_NODATA, iter->first)
            && std::modf(iter->second, &intpart) == 0.0) {
            // std::modf consider inf as an integer,
            // hence cannot handle -3.40282346639e+38 which is one of commonly used Nodata
            BSON_APPEND_INT32(&p, iter->first.c_str(), CVT_INT(iter->second));
        }
        else {
            BSON_APPEND_DOUBLE(&p, iter->first.c_str(), iter->second);
        }
    }
    // Check DATATYPE in metadata is consistent with T, transform if necessary
    char* buf = nullptr;
    vint buflength = -1;
    bool transformed = true;
    RasterDataType opt_type = StringToRasterDataType(curopts.at(HEADER_RSOUT_DATATYPE));
    if (opt_type == temp_type) {
        buf = reinterpret_cast<char*>(values);
        buflength = datalength * sizeof(T);
        transformed = false;
    } else {
        if (opt_type == RDT_UInt8) {
            vuint8_t* newdata = nullptr;
            Initialize1DArray(datalength, newdata, values);
            buf = reinterpret_cast<char*>(newdata);
            buflength = datalength * sizeof(vuint8_t);
        } else if (opt_type == RDT_Int8) {
            vint8_t* newdata = nullptr;
            Initialize1DArray(datalength, newdata, values);
            buf = reinterpret_cast<char*>(newdata);
            buflength = datalength * sizeof(vint8_t);
        } else if (opt_type == RDT_UInt16) {
            vuint16_t* newdata = nullptr;
            Initialize1DArray(datalength, newdata, values);
            buf = reinterpret_cast<char*>(newdata);
            buflength = datalength * sizeof(vuint16_t);
        } else if (opt_type == RDT_Int16) {
            vint16_t* newdata = nullptr;
            Initialize1DArray(datalength, newdata, values);
            buf = reinterpret_cast<char*>(newdata);
            buflength = datalength * sizeof(vint16_t);
        } else if (opt_type == RDT_UInt32) {
            vuint32_t* newdata = nullptr;
            Initialize1DArray(datalength, newdata, values);
            buf = reinterpret_cast<char*>(newdata);
            buflength = datalength * sizeof(vuint32_t);
        } else if (opt_type == RDT_Int32) {
            vint32_t* newdata = nullptr;
            Initialize1DArray(datalength, newdata, values);
            buf = reinterpret_cast<char*>(newdata);
            buflength = datalength * sizeof(vint32_t);
        } else if (opt_type == RDT_Float) {
            float* newdata = nullptr;
            Initialize1DArray(datalength, newdata, values);
            buf = reinterpret_cast<char*>(newdata);
            buflength = datalength * sizeof(float);
        } else if (opt_type == RDT_Double) {
            double* newdata = nullptr;
            Initialize1DArray(datalength, newdata, values);
            buf = reinterpret_cast<char*>(newdata);
            buflength = datalength * sizeof(double);
        }
    }
    // Add user-specific key-values into metadata
    AppendStringOptionsToBson(&p, curopts);

    int try_times = 0;
    bool gstatus = false;
    while (try_times <= 3) { // Try 3 times
        gstatus = gfs->WriteStreamData(filename, buf, buflength, &p);
        if (gstatus) { break; }
        SleepMs(2); // Sleep 0.002 sec and retry
        try_times++;
    }
    bson_destroy(&p);
    if (transformed) { delete[] buf; }
    return gstatus;
}

#endif /* USE_MONGODB */

/*!
 * \class SubsetPositions
 * \brief Subset positions of raster data
 */
class SubsetPositions: NotCopyable {
public:
    SubsetPositions();

    SubsetPositions(int srow, int erow, int scol, int ecol);

    explicit SubsetPositions(SubsetPositions*& src, bool deep_copy = false);

    ~SubsetPositions();

    bool Initialization();

    template <typename T>
    bool SetData(const int n, T* data) {
        if (n != n_cells) { return false; }
        if (nullptr == data) { return false; }
        if (1 != n_lyrs) { n_lyrs = 1; }
        if (nullptr != data_) {
            for (int i = 0; i < n_cells; i++) { data_[i] = CVT_DBL(data[i]); }
        } else {
            Initialize1DArray(n_cells, data_, data);
        }
        usable = true;
        return true;
    }

    template <typename T>
    bool Set2DData(const int n, const int lyr, T** data2d) {
        if (n != n_cells) { return false; }
        if (nullptr == data2d) { return false; }
        if (lyr != n_lyrs) { n_lyrs = lyr; }
        if (nullptr != data2d_) {
            for (int i = 0; i < n_cells; i++) {
                for (int j = 0; j < n_lyrs; j++) {
                    data2d_[i][j] = CVT_DBL(data2d[i][j]);
                }
            }
        } else {
            Initialize2DArray(n_cells, n_lyrs, data2d_, data2d);
        }
        usable = true;
        return true;
    }
#ifdef USE_MONGODB
    bool ReadFromMongoDB(MongoGridFs* gfs, const string& fname,
                         const STRING_MAP& opts = STRING_MAP());
#endif
    void GetHeader(double gxll, double gyll, int gnrows, double cellsize,
                   double nodata, STRDBL_MAP& subheader);
    template <typename T>
    void Output(T nodata, vector<T*>& fulldata) {
        if (nullptr == data_ && nullptr == data2d_) { return; }
        int nrows = g_erow - g_srow + 1;
        int ncols = g_ecol - g_scol + 1;
        int fullsize = nrows * ncols;
        for (int ilyr = 0; ilyr < n_lyrs; ilyr++) {
            T* tmpdata = nullptr;
            Initialize1DArray(fullsize, tmpdata, nodata);
            for (int vi = 0; vi < n_cells; vi++) {
                //int j = local_pos_[vi][0] * ncols + local_pos_[vi][1];
                int j = local_posidx_[vi];
                if (n_lyrs > 1 && nullptr != data2d_) {
                    tmpdata[j] = static_cast<T>(data2d_[vi][ilyr]);
                }
                else if (n_lyrs == 1 && nullptr != data_) {
                    tmpdata[j] = static_cast<T>(data_[vi]);
                }
            }
            fulldata.emplace_back(tmpdata);
        }
    }

    bool usable; ///< flag for usable subset data
    int n_cells; ///< valid cell count
    int n_lyrs; ///< layer count
    int g_srow; ///< start row in global data
    int g_erow; ///< end row in global data
    int g_scol; ///< start col in global data
    int g_ecol; ///< end col in global data
    bool alloc_; ///< local_pos_ and global_ are allocated?
    int** local_pos_; ///< local position data
    int* local_posidx_; ///< local position index
    int* global_; ///< global position index
    double* data_; ///< valid data array
    double** data2d_; ///< valid 2d data array
};

/*!
 * \class clsRasterData
 * \brief Raster data (1D and 2D) I/O class
 *        Support I/O among ASCII file, TIFF, and MongoDB database.
 */
template <typename T, typename MASK_T = T>
class clsRasterData {
public:
    /************* Construct functions ***************/

    /*!
     * \brief Constructor an empty clsRasterData instance for 1D or 2D raster
     */
    explicit clsRasterData(bool is_2d = false);

    /*!
     * \brief Constructor 1D raster from necessary data.
     */
    clsRasterData(T* data, int cols, int rows, T nodata, double dx, double xll,
                  double yll, const STRING_MAP& opts = STRING_MAP());

    /*!
     * \brief Constructor 1D raster from necessary data.
     * \deprecated The input parameter `srs` is highly recommended replaced by `map<string, string>`.
     */
    clsRasterData(T* data, int cols, int rows, T nodata, double dx, double xll,
                  double yll, const string& srs);

    /*!
    * \brief Constructor 2D raster from necessary data
    */
    clsRasterData(T** data2d, int cols, int rows, int nlayers, T nodata,
                  double dx, double xll, double yll, const STRING_MAP& opts = STRING_MAP());

    /*!
     * \brief Constructor 2D raster from necessary data
     * \deprecated The input parameter `srs` is highly recommended replaced by `map<string, string>`.
     */
    clsRasterData(T** data2d, int cols, int rows, int nlayers, T nodata,
                  double dx, double xll, double yll, const string& srs);

    /*!
     * \brief Constructor of clsRasterData instance from TIFF, ASCII, or other GDAL supported format,
     *        which has one data layer, referred to as Raster1D.
     *
     * \param[in] filename Full path of the raster file
     * \param[in] calc_pos Calculate positions of valid cells excluding NODATA. The default is false.
     * \param[in] mask \a clsRasterData<MASK_T> Mask layer
     * \param[in] use_mask_ext Use mask layer extent, even NoDATA exists.
     * \param[in] default_value Default value when mask data exceeds the raster extend.
     * \param[in] opts (Optional) Additional options of the raster data with the format of key-value
     */
    explicit clsRasterData(const string& filename, bool calc_pos = false,
                           clsRasterData<MASK_T>* mask = nullptr, bool use_mask_ext = true,
                           double default_value = NODATA_VALUE, const STRING_MAP& opts = STRING_MAP());

    /*!
     * \brief Validation check before the construction of clsRasterData,
     *        i.e., the raster file is existed and the data format supported.
     *        This is the recommended way to construct an instance of clsRasterData.
     *
     * \code
     *       clsRasterData<T> *rs = clsRasterData<T>::Init(filename)
     *       if (nullptr == rs) {
     *           throw exception("clsRasterData initialization failed!");
     *           // or other error handling code.
     *       }
     * \endcode
     */
    static clsRasterData<T, MASK_T>* Init(const string& filename, bool calc_pos = false,
                                          clsRasterData<MASK_T>* mask = nullptr, bool use_mask_ext = true,
                                          double default_value = NODATA_VALUE, const STRING_MAP& opts = STRING_MAP());

    /*!
     * \brief Constructor of clsRasterData instance from TIFF, ASCII, or other GDAL supported raster file,
     *        which may have one or more data layers, i.e., compatible with Raster1D and Rater2D.
     *
     * \param[in] filenames Full paths vector of the 2D raster data
     * \param[in] calc_pos Calculate positions of valid cells excluding NODATA. The default is false.
     * \param[in] mask \a clsRasterData<MASK_T> Mask layer
     * \param[in] use_mask_ext Use mask layer extent, even NoDATA exists.
     * \param[in] default_value Default value when mask data exceeds the raster extend.
     * \param[in] opts (Optional) Additional options of the raster data with the format of key-value
     */
    explicit clsRasterData(vector<string>& filenames, bool calc_pos = false,
                           clsRasterData<MASK_T>* mask = nullptr, bool use_mask_ext = true,
                           double default_value = NODATA_VALUE, const STRING_MAP& opts = STRING_MAP());

    /*!
     * \brief Validation check before the constructor of clsRasterData,
     *        i.e., the raster files are all existed and the data format supported.
     *        This is the recommended way to construct an instance of clsRasterData,
     *        compatible with Raster1D and Rater2D.
     *
     * \code
     *       vector<string> filenames = {"layer1.tif", "layer2.tif", "layer3.tif"};
     *       clsRasterData<T> *rs = clsRasterData<T>::Init(filenames)
     *       if (nullptr == rs) {
     *           throw exception("clsRasterData initialization failed!");
     *           // or other error handling code.
     *       }
     * \endcode
     */
    static clsRasterData<T, MASK_T>* Init(vector<string>& filenames, bool calc_pos = false,
                                          clsRasterData<MASK_T>* mask = nullptr, bool use_mask_ext = true,
                                          double default_value = NODATA_VALUE, const STRING_MAP& opts = STRING_MAP());

    /*!
     * \brief Construct an clsRasterData instance by 1D array data and mask
     */
    clsRasterData(clsRasterData<MASK_T>* mask, T* values, int len,
                  const STRING_MAP& opts = STRING_MAP());

    /*!
     * \brief Construct an clsRasterData instance by 2D array data and mask
     */
    clsRasterData(clsRasterData<MASK_T>* mask, T** values, int len, int lyrs,
                  const STRING_MAP& opts = STRING_MAP());

#ifdef USE_MONGODB

    /*!
     * \brief Constructor based on mongoDB
     * \sa ReadFromMongoDB()
     *
     * \param[in] gfs \a mongoc_gridfs_t
     * \param[in] remote_filename \a char*
     * \param[in] calc_pos Calculate positions of valid cells excluding NODATA. The default is false.
     * \param[in] mask \a clsRasterData<MASK_T> Mask layer
     * \param[in] use_mask_ext Use mask layer extent, even NoDATA exists.
     * \param[in] default_value Default value when mask data exceeds the raster extend.
     * \param[in] opts Optional key-value stored in metadata
     */
    clsRasterData(MongoGridFs* gfs, const char* remote_filename, bool calc_pos = false,
                  clsRasterData<MASK_T>* mask = nullptr, bool use_mask_ext = true,
                  double default_value = NODATA_VALUE, const STRING_MAP& opts = STRING_MAP());

    /*!
     * \brief Validation check before the constructor of clsRasterData.
     */
    static clsRasterData<T, MASK_T>* Init(MongoGridFs* gfs, const char* remote_filename,
                                          bool calc_pos = false, clsRasterData<MASK_T>* mask = nullptr,
                                          bool use_mask_ext = true, double default_value = NODATA_VALUE,
                                          const STRING_MAP& opts = STRING_MAP());
#endif

    /*!
     * \brief Copy constructor
     *
     * \code
     *   // Method 1:
     *   clsRasterData<T> newraster(baseraster);
     *   // Method 2:
     *   clsRasterData<T> newraster;
     *   newraster.Copy(baseraster);
     *   // Method 3:
     *   clsRasterData<T>* newraster = new clsRasterData<T>(baseraster);
     *   delete newraster;
     * \endcode
     */
    explicit clsRasterData(clsRasterData<T, MASK_T>* another);

    //! Destructor
    ~clsRasterData();

    /************* Read functions ***************/
    /*!
     * \brief Read raster data from file, mask data is optional
     * \param[in] filename \a string
     * \param[in] calc_pos Calculate positions of valid cells excluding NODATA. The default is false.
     * \param[in] mask \a clsRasterData<MASK_T>
     * \param[in] use_mask_ext Use mask layer extent, even NoDATA exists.
     * \param[in] default_value Default value when mask data exceeds the raster extend.
     * \param[in] opts Optional key-value stored in metadata
     */
    bool ReadFromFile(const string& filename, bool calc_pos = false, clsRasterData<MASK_T>* mask = nullptr,
                      bool use_mask_ext = true, double default_value = NODATA_VALUE,
                      const STRING_MAP& opts = STRING_MAP());
    /*!
     * \brief Read raster data from two or more files, mask data is optional
     * \sa ReadFromFile
     */
    bool ReadFromFiles(vector<string>& filenames, bool calc_pos = false, clsRasterData<MASK_T>* mask = nullptr,
                       bool use_mask_ext = true, double default_value = NODATA_VALUE,
                       const STRING_MAP& opts = STRING_MAP());

#ifdef USE_MONGODB
    /*!
     * \brief Read raster data from MongoDB
     * \param[in] gfs \a mongoc_gridfs_t
     * \param[in] filename \a char*, raster file name
     * \param[in] calc_pos Calculate positions of valid cells excluding NODATA. The default is false.
     * \param[in] mask \a clsRasterData<MASK_T>
     * \param[in] use_mask_ext Use mask layer extent, even NoDATA exists.
     * \param[in] default_value Default value when mask data exceeds the raster extend.
     * \param[in] opts Optional key-value stored in metadata, used to filter GridFs file
     */
    bool ReadFromMongoDB(MongoGridFs* gfs, const string& filename, bool calc_pos = false,
                         clsRasterData<MASK_T>* mask = nullptr, bool use_mask_ext = true,
                         double default_value = NODATA_VALUE,
                         const STRING_MAP& opts = STRING_MAP());

#endif /* USE_MONGODB */
    /************* Write functions ***************/

    /*!
     * \brief Write the whole raster to raster file, if 2D raster, output name will be corename_LyrNum
     * \param filename filename with prefix, e.g. ".asc" and ".tif"
     * \param out_origin (Optional) Write combination of original data or subset's data
     */
    bool OutputToFile(const string& filename, bool out_origin = true);

    /*!
     * \brief Write one or more raster's subset to files, default name format: corename_SubID_LyrNum
     * \param out_origin (Optional) Output original raster data or data_ assigned for subset
     * \param out_combined (Optional) Output combined data of subset
     * \param outname (Optional) Output filename, if not specified, use origin directory and default name
     * \param recls (Optional) Reclassification map, no need to SetData before output subset
     * \param default_value (Optional) Default value for missed type of reclassification map
     */
    bool OutputSubsetToFile(bool out_origin = false, bool out_combined = true,
                            const string& outname = string(),
                            const map<vint, vector<double> >& recls = map<vint, vector<double> >(),
                            double default_value = NODATA_VALUE);

    /*!
     * \brief Write 1D or 2D raster data into ASC file(s)
     */
    bool OutputAscFile(const string& filename);

#ifdef USE_GDAL
    /*!
     * \brief Write 1D or 2D raster data into TIFF file by GDAL
     */
    bool OutputFileByGdal(const string& filename);
#endif /* USE_GDAL */

#ifdef USE_MONGODB

    /*!
     * \brief Write the whole raster data to MongoDB, name format: corename__LyrNum
     * \param gfs \a mongoc_gridfs_t
     * \param filename (Optional) File name, default is the core file name of input
     * \param opts (Optional) Key-value map for user-specific metadata
     * \param include_nodata (Optional) Include nodata or not
     * \param out_origin (Optional) Output original raster data or subset's data
     */
    bool OutputToMongoDB(MongoGridFs* gfs, const string& filename = string(),
                         const STRING_MAP& opts = STRING_MAP(),
                         bool include_nodata = true,
                         bool out_origin = true);

    /*!
     * \brief Write one or more raster's subset to MongoDB,
     *
     *        File name format in GridFS: corename_SubID_LyrNum, if SubID < 0, use 'n' as the prefix.
     *        e.g., corename_n1_1, corename_2_1, corename_3_1
     *
     * \param gfs \a mongoc_gridfs_t
     * \param filename (Optional) Core file name
     * \param opts (Optional) Key-value map for additional user-specific metadata
     * \param include_nodata (Optional) Include nodata or not
     * \param out_origin (Optional) Output original raster data or data_ assigned for subset
     * \param out_combined (Optional) Output combined data of subset
     * \param recls (Optional) Reclassification map, no need to SetData before output subset
     * \param default_value (Optional) Default value for missed type of reclassification map
     */
    bool OutputSubsetToMongoDB(MongoGridFs* gfs, const string& filename = string(),
                               const STRING_MAP& opts = STRING_MAP(),
                               bool include_nodata = true,
                               bool out_origin = false, bool out_combined = true,
                               const map<vint, vector<double> >& recls = map<vint, vector<double> >(),
                               double default_value = NODATA_VALUE);

#endif /* USE_MONGODB */

    /************************************************************************/
    /*    Set information functions                                         */
    /************************************************************************/

    void SetHeader(const STRDBL_MAP& refers);

    //! Set new core file name
    void SetCoreName(const string& name) { core_name_ = name; }

    /*!
     * \brief Set value to the given position and layer
     */
    void SetValue(int row, int col, T value, int lyr = 1);

    /*!
     * \brief Set the flag of calc_pos_ to true and
     *        recalculate positions and stored raster data if necessary
     */
    bool SetCalcPositions();

    /*!
     * \brief Set valid positions data, without mask raster layer
     */
    bool SetPositions(int len, int** pdata);
    bool SetPositions(int len, int* pdata);

    /*!
     * \brief Set the flag of use_mask_ext_ to true and
     *        recalculate positions and stored raster data if necessary
     */
    bool SetUseMaskExt();

    /// Set raster data type
    void SetDataType(RasterDataType const type) { rs_type_ = type; }

    /// Set raster data type
    void SetDataType(const string& strtype) { rs_type_ = StringToRasterDataType(strtype); }

    /// Set output raster data type
    void SetOutDataType(RasterDataType type);

    /// Set output raster data type
    void SetOutDataType(const string& strtype);

    /// Set default value
    void SetDefaultValue(const double defaultv) { default_value_ = defaultv; }

    /*!
     * \brief Build subsets by given groups (cell value->group ID) or by discrete values (default)
     */
    bool BuildSubSet(map<int, int> groups = map<int, int>());

    /*!
     * \brief Release subsets
     */
    bool ReleaseSubset();

    /*!
     * \brief Re-build subsets by given groups (cell value->group ID) or by discrete values
     */
    bool RebuildSubSet(map<int, int> groups = map<int, int>());

    /************************************************************************/
    /*    Get information functions                                         */
    /************************************************************************/

    /*!
     * \brief Calculate basic statistics values in one time
     * Mean, Max, Min, STD, Range, etc.
     */
    void CalculateStatistics();

    /*!
     * \brief Force to update basic statistics values
     * Mean, Max, Min, STD, Range, etc.
     */
    void UpdateStatistics();

    /*!
     * \brief Release statistics map of 2D raster data
     */
    void ReleaseStatsMap2D();

    /*!
     * \brief Get basic statistics value
     * Mean, Max, Min, STD, Range, etc.
     * \param[in] sindex \a string case insensitive
     * \param[in] lyr optional for 1D and the first layer of 2D raster data.
     * \return Statistics value or NoData
     */
    double GetStatistics(string sindex, int lyr = 1);

    /*!
     * \brief Get basic statistics values for 2D raster data.
     * Mean, Max, Min, STD, Range, etc.
     * \param[in] sindex \a string case insensitive
     * \param[out] lyrnum \a int layer number
     * \param[out] values \a double* Statistics array or nullptr
     */
    void GetStatistics(string sindex, int* lyrnum, double** values);

    /*!
     * \brief Get the average of raster data
     * \param[in] lyr optional for 1D and the first layer of 2D raster data.
     */
    double GetAverage(const int lyr = 1) { return GetStatistics(STATS_RS_MEAN, lyr); }

    /*!
     * \brief Get the maximum of raster data
     * \sa GetAverage
     */
    double GetMaximum(const int lyr = 1) { return GetStatistics(STATS_RS_MAX, lyr); }

    /*!
     * \brief Get the minimum of raster data
     * \sa GetAverage
     */
    double GetMinimum(const int lyr = 1) { return GetStatistics(STATS_RS_MIN, lyr); }

    /*!
     * \brief Get the stand derivation of raster data
     * \sa GetAverage
     */
    double GetStd(const int lyr = 1) { return GetStatistics(STATS_RS_STD, lyr); }

    /*!
     * \brief Get the range of raster data
     * \sa GetMaximum
     * \sa GetMinimum
     */
    double GetRange(const int lyr = 1) { return GetStatistics(STATS_RS_RANGE, lyr); }

    /*!
     * \brief Get the average of 2D raster data
     * \param[out] lyrnum \a int layer number
     * \param[out] values \a double* Statistics array
     */
    void GetAverage(int* lyrnum, double** values) { GetStatistics(STATS_RS_MEAN, lyrnum, values); }

    /*!
     * \brief Get the maximum of 2D raster data
     * \sa GetAverage
     */
    void GetMaximum(int* lyrnum, double** values) { GetStatistics(STATS_RS_MAX, lyrnum, values); }

    /*!
     * \brief Get the minimum of 2D raster data
     * \sa GetAverage
     */
    void GetMinimum(int* lyrnum, double** values) { GetStatistics(STATS_RS_MIN, lyrnum, values); }

    /*!
     * \brief Get the standard derivation of 2D raster data
     * \sa GetAverage
     */
    void GetStd(int* lyrnum, double** values) { GetStatistics(STATS_RS_STD, lyrnum, values); }

    /*!
     * \brief Get the range of 2D raster data
     * \sa GetAverage
     */
    void GetRange(int* lyrnum, double** values) { GetStatistics(STATS_RS_RANGE, lyrnum, values); }

    /*!
     * \brief Get the non-NoDATA number of 2D raster data
     * \sa GetAverage
     */
    void GetValidNumber(int* lyrnum, double** values) { GetStatistics(STATS_RS_VALIDNUM, lyrnum, values); }

    /*!
     * \brief Get the non-NoDATA cells number of the given raster layer data
     * \sa GetCellNumber
     * \sa GetDataLength
     */
    int GetValidNumber(const int lyr = 1) { return CVT_INT(GetStatistics(STATS_RS_VALIDNUM, lyr)); }

    int GetCellNumber() const { return n_cells_; } /// Get the first dimension size
    /// Get the actual stored length of raster data
    int GetDataLength() const { return n_cells_ < 0 || n_lyrs_ < 0 ? -1 : n_cells_ * n_lyrs_; }
    int GetCols() const { return CVT_INT(headers_.at(HEADER_RS_NCOLS)); } /// Get column number
    int GetRows() const { return CVT_INT(headers_.at(HEADER_RS_NROWS)); } /// Get row number
    double GetCellWidth() const { return headers_.at(HEADER_RS_CELLSIZE); } /// Get cell size

    //! Get X coordinate of left lower center of raster data
    double GetXllCenter() const {
        if (headers_.find(HEADER_RS_XLL) != headers_.end()) {
            return headers_.at(HEADER_RS_XLL);
        }
        if (headers_.find(HEADER_RS_XLLCOR) != headers_.end()) {
            return headers_.at(HEADER_RS_XLLCOR) + 0.5 * GetCellWidth();
        }
        return NODATA_VALUE;
    }

    //! Get Y coordinate of left lower center of raster data
    double GetYllCenter() const {
        if (headers_.find(HEADER_RS_YLL) != headers_.end()) {
            return headers_.at(HEADER_RS_YLL);
        }
        if (headers_.find(HEADER_RS_YLLCOR) != headers_.end()) {
            return headers_.at(HEADER_RS_YLLCOR) + 0.5 * GetCellWidth();
        }
        return NODATA_VALUE;
    }

    int GetLayers() const { return n_lyrs_; } /// Get layer number
    RasterDataType GetDataType() const { return rs_type_; } /// Get data type of source
    RasterDataType GetOutDataType() const { return rs_type_out_; } /// Get data type of output
    T GetNoDataValue() const { return no_data_value_; } /// Get NoDATA value
    double GetDefaultValue() const { return default_value_; } /// Get default value

    /*!
     * \brief Get position index in 1D raster data for specific row and column
     * \return -1 --- the position is nodata
     *         -2 --- the position is out of the extent, which indicates an error
     */
    int GetPosition(int row, int col);

    //! Get position index in 1D raster data for given coordinate (x,y)
    int GetPosition(float x, float y);

    //! Get position index in 1D raster data for given coordinate (x,y)
    int GetPosition(double x, double y);

    //! Get subset
    map<int, SubsetPositions*>& GetSubset() { return subset_; }

    /*! \brief Get raster data, include valid cell number and data
     * \return true if the raster data has been initialized, otherwise return false and print error info.
     */
    bool GetRasterData(int* n_cells, T** data);

    /*!
     * \brief Get 2D raster data, include valid cell number of each layer, layer number, and data
     * \return true if the 2D raster has been initialized, otherwise return false and print error info.
     */
    bool Get2DRasterData(int* n_cells, int* n_lyrs, T*** data);

    //! Get raster header information
    const STRDBL_MAP& GetRasterHeader() const { return headers_; }

    //! Get raster statistics information
    const STRDBL_MAP& GetStatistics() const { return stats_; }

    //! Get raster statistics information of 2D raster
    const map<string, double *>& GetStatistics2D() const { return stats_2d_; };

    //! Get full path name
    string GetFilePath() const { return full_path_; }

    //! Get core name
    string GetCoreName() const { return core_name_; }

    /*!
     * \brief Get position index data and the data length
     * \param[out] datalength Data length
     * \param[out] positiondata The pointer of 2D array (pointer)
     */
    void GetRasterPositionData(int* datalength, int*** positiondata);

    void GetRasterPositionData(int* datalength, int** positiondata);

    T* GetRasterDataPointer() const { return raster_; } /// Get pointer of raster 1D data
    int** GetRasterPositionDataPointer() const { return pos_data_; } /// Get pointer of position data
    int* GetRasterPositionIndexPointer() const { return pos_idx_; } /// Get pointer of position data
    T** Get2DRasterDataPointer() const { return raster_2d_; } /// Get pointer of raster 2D data
    const char* GetSrs(); /// Get the spatial reference (char*)
    string GetSrsString(); /// Get the spatial reference (string)
    string GetOption(const char* key); /// Get option by key, including the spatial reference by "SRS"
    const STRING_MAP& GetOptions() const { return options_; } /// Get options

    /*!
     * \brief Get raster data at the valid cell index
     * The default lyr is 1, which means the 1D raster data, or the first layer of 2D data.
     */
    T GetValueByIndex(int cell_index, int lyr = 1);

    /*!
     * \brief Get raster data at the valid cell index (both for 1D and 2D raster)
     * \param[in] cell_index Cell's index in the first dimension
     * \param[out] values A float array with length as n_lyrs_ which should be release in the invoke code
     */
    void GetValueByIndex(int cell_index, T*& values);

    /*!
     * \brief Get raster data via row and col
     * The default lyr is 1, which means the 1D raster data, or the first layer of 2D data.
     */
    T GetValue(int row, int col, int lyr = 1);

    /*!
     * \brief Get raster data (both for 1D and 2D raster) at the (row, col)
     * \param[in] row Row index
     * \param[in] col Col index
     * \param[out] values A float array with the length of n_lyrs_
     */
    void GetValue(int row, int col, T*& values);

    /*!
     * \brief Check if the raster data is NoData via row and col
     * The default lyr is 1, which means the 1D raster data, or the first layer of 2D data.
     */
    bool IsNoData(const int row, const int col, const int lyr = 1) {
        return FloatEqual(GetValue(row, col, lyr), no_data_value_);
    }

    bool Is2DRaster() const { return is_2draster; } /// Is 2D raster data?
    bool PositionsCalculated() const { return calc_pos_; } /// Calculate positions or not
    bool PositionsAllocated() const { return store_pos_; } /// position data is allocated or a pointer
    bool MaskExtented() const { return use_mask_ext_; } /// Use mask extent or not
    bool StatisticsCalculated() const { return stats_calculated_; } /// Basic statistics calculated?
    bool Initialized() const { return initialized_; } /// Instance of clsRasterData initialized?

    /*!
     * \brief Validate the available of raster data, both 1D and 2D data
     */
    bool ValidateRasterData() {
        if ((!is_2draster && nullptr != raster_) || // Valid 1D raster
            (is_2draster && nullptr != raster_2d_)) // Valid 2D raster
        { return true; }
        StatusMessage("Error: Please initialize the raster object first.");
        return false;
    }

    /*!
     * \brief Validate the input row and col
     */
    bool ValidateRowCol(const int row, const int col) {
        if ((row < 0 || row >= GetRows()) || (col < 0 || col >= GetCols())) {
            StatusMessage("The row must between 0 and " + ValueToString(GetRows() - 1) +
                          ", and the col must between 0 and " + ValueToString(GetCols() - 1));
            return false;
        }
        return true;
    }

    /*!
     * \brief Validate the input layer number
     */
    bool ValidateLayer(const int lyr) {
        if (lyr <= 0 || lyr > n_lyrs_) {
            StatusMessage("The layer must be 1 ");
            if (n_lyrs_ > 1) StatusMessage(" or between 1 and " +
                                           utils_string::ValueToString(n_lyrs_));
            return false;
        }
        return true;
    }

    /*!
     * \brief Validate the input index
     */
    bool ValidateIndex(const int idx) {
        if (idx < 0 || idx >= n_cells_) {
            StatusMessage("The index must between 0 and " + utils_string::ValueToString(n_cells_ - 1));
            return false;
        }
        return true;
    }

    //! Get full filename
    string GetFullFileName() const { return full_path_; }

    //! Get mask data pointer
    clsRasterData<MASK_T>* GetMask() const { return mask_; }

    /*!
     * \brief Copy clsRasterData object
     */
    void Copy(clsRasterData<T, MASK_T>* orgraster);

    /*!
     * \brief Replace NoData value by the given value
     */
    void ReplaceNoData(T replacedv);

    /*!
     * \brief classify raster
     */
    void Reclassify(map<int, T> reclass_map);

    /************* Utility functions ***************/

    /*!
     * \brief Calculate XY coordinates by given row and col number
     * \sa GetPositionByCoordinate
     * \param[in] row Row number
     * \param[in] col Col number
     * \return pair<double x, double y>
     */
    XY_COOR GetCoordinateByRowCol(int row, int col);

    /*!
     * \brief Calculate position by given coordinate
     * \sa GetCoordinateByRowCol
     * \param[in] x X coordinate
     * \param[in] y Y coordinate
     * \param[in] header Optional, header map of raster layer data, the default is m_header
     * \return pair<int row, int col>
     */
    ROW_COL GetPositionByCoordinate(double x, double y, STRDBL_MAP* header = nullptr);

private:
    /*!
     * \brief Initialize all raster related variables.
     */
    void InitializeRasterClass(bool is_2d = false);

    /*!
     * \brief Initialize read function for ASC, GDAL, and MongoDB
     */
    void InitializeReadFunction(const string& filename, bool calc_pos = false, clsRasterData<MASK_T>* mask = nullptr,
                                bool use_mask_ext = true, double default_value = NODATA_VALUE,
                                const STRING_MAP& opts = STRING_MAP());

    /*!
     * \brief Constructor of clsRasterData instance from single file of TIFF, ASCII, or other GDAL supported raster file
     * Support 1D and/or 2D raster data
     * \param[in] filename Full path of the raster file
     * \param[in] calc_pos Calculate positions of valid cells excluding NODATA. The default is false.
     * \param[in] mask Mask layer
     * \param[in] use_mask_ext Use mask layer extent, even NoDATA exists.
     * \param[in] default_value Default value
     * \param[in] opts Optional key-value stored in metadata
     * \return true if read successfully, otherwise return false.
     */
    bool ConstructFromSingleFile(const string& filename, bool calc_pos = false, clsRasterData<MASK_T>* mask = nullptr,
                                 bool use_mask_ext = true, double default_value = NODATA_VALUE,
                                 const STRING_MAP& opts = STRING_MAP());

    /*!
     * \brief Extract by mask data and calculate position index, if necessary.
     * \return integer values to represent different situations
     *         -1: Mask data is not initialized or no raster cells covered by mask
     *          0: Do nothing (mask_==nullptr && calc_pos_==false)
     *          1: Calc pos from raster data (mask_==nullptr && calc_pos_==true)
     *          2: All situations that use mask data
     */
    int MaskAndCalculateValidPosition();

    /*!
     * \brief Calculate position index from rectangle grid values, if necessary.
     * To use this function, mask should be nullptr.
     *
     */
    void CalculateValidPositionsFromGridData();

    /*!
     * \brief Prepare combination data array of subsets for output
     */
    bool PrepareCombSubsetData(T**values, int* datalen, int* datalyrs,
                               bool out_origin = false, bool include_nodata = true,
                               const map<vint, vector<double> >&recls = map<vint, vector<double> >(),
                               double default_value = NODATA_VALUE);

    /*!
     * \brief Prepare data array of subsets for output
     */
    bool PrepareSubsetData(int sub_id, SubsetPositions* sub,
                           T** values, int* datalen, int* datalyrs,
                           bool out_origin = false, bool include_nodata = true,
                           const map<vint, vector<double> >& recls = map<vint, vector<double> >(),
                           double default_value = NODATA_VALUE);

    /*!
     * \brief Output full size raster data to files
     */
    bool OutputFullsizeToFiles(T* fullsizedata, int fsize, int datalyrs,
                               const string& fullfilename, const STRDBL_MAP& header,
                               const STRING_MAP& opts);

    /*!
     * \brief Add other layer's rater data to raster_2d_
     * \param[in] row Row number be added on, e.g. 2
     * \param[in] col Column number be added on, e.g. 3
     * \param[in] cellidx Cell index in raster_2d_, e.g. 23
     * \param[in] lyr Layer number which is greater than 1, e.g. 2, 3, ..., n
     * \param[in] lyrheader Header information of current layer
     * \param[in] lyrdata Raster layer data
     */
    void AddOtherLayerRasterData(int row, int col, int cellidx, int lyr,
                                 STRDBL_MAP lyrheader, T* lyrdata);

    /*!
     * \brief If NoDataValue not equal to NODATA_VALUE, while default value do, then change default value.
     */
    void CheckDefaultValue() {
        if (FloatEqual(default_value_, NODATA_VALUE) && !FloatEqual(no_data_value_, NODATA_VALUE)) {
            default_value_ = CVT_DBL(no_data_value_);
        }
    }

    /*!
     * \brief Operator= without implementation
     */
    clsRasterData& operator=(const clsRasterData&) { }

    /*! cell number of raster data, i.e. the data length of \sa raster_data_ or \sa raster_2d_
     * 1. all grid cell number, i.e., ncols * nrows, when m_calcPositions is False
     * 2. valid cell number excluding NoDATA, when m_calcPositions is True and m_useMaskExtent is False.
     * 3. valid cell number excluding NoDATA, the same as mask's n_cells_, when mask is valid and m_useMaskExtent is True.
     * 4. valid cell number excluding NoDATA, recal
     */
    int n_cells_;
    //! Layer number of the 2D raster
    int n_lyrs_;
    //! Data type of input raster
    RasterDataType rs_type_;
    //! Data type of output raster
    RasterDataType rs_type_out_;
    //! noDataValue
    T no_data_value_;
    //! default value when mask by mask data, if not specified, it equals to no_data_value_
    double default_value_;
    //! raster full path, e.g. "C:/tmp/studyarea.tif"
    string full_path_;
    //! core raster file name, e.g. "studyarea"
    string core_name_;
    //! 1D raster data with a data length of n_cells_ which depends on situations
    T* raster_;
    //! 2D raster data, data access format: raster_2d_[cellIndex][layer], layer starts from 1
    T** raster_2d_;
    //! valid cells' position (row, col) in raster_data_ or the first layer of raster_2d_ (2D array)
    int** pos_data_;
    //! valid cells' index (row * cols + col) in raster_data_ or the first layer of raster_2d_
    int* pos_idx_;
    //! Key-value options in string format, including spatial reference
    STRING_MAP options_;
    //! Header information, using double in case of truncation of coordinate value
    STRDBL_MAP headers_;
    //! Map to store basic statistics values for 1D raster data
    STRDBL_MAP stats_;
    //! Map to store basic statistics values for 2D raster data
    map<string, double *> stats_2d_;
    //! mask clsRasterData instance
    clsRasterData<MASK_T>* mask_;
    //! Subset by user-specific groups or discrete values of the raster data
    map<int, SubsetPositions*> subset_;
    //! initial once
    bool initialized_;
    //! Flag to identify 1D or 2D raster
    bool is_2draster;
    //! calculate valid positions or not. The default is true.
    bool calc_pos_;
    //! raster position data is newly allocated array (true), or just a pointer (false)
    bool store_pos_;
    //! To be consistent with other datesets, keep the extent of Mask layer, even include NoDATA.
    bool use_mask_ext_;
    //! Statistics calculated?
    bool stats_calculated_;
};

/******** Define common used raster types **************/
#ifndef IntRaster
#define IntRaster          clsRasterData<int>
#endif
#ifndef FltRaster
#define FltRaster          clsRasterData<float>
#endif
#ifndef DblRaster
#define DblRaster          clsRasterData<double>
#endif
#ifndef IntIntRaster
#define IntIntRaster       clsRasterData<int, int>
#endif
#ifndef FltIntRaster
#define FltIntRaster       clsRasterData<float, int>
#endif
#ifndef DblIntRaster
#define DblIntRaster       clsRasterData<double, int>
#endif

/*******************************************************/
/************* Implementation Code Begin ***************/
/*******************************************************/

/************* Construct functions ***************/

template <typename T, typename MASK_T>
void clsRasterData<T, MASK_T>::InitializeRasterClass(bool is_2d /* = false */) {
    full_path_ = "";
    core_name_ = "";
    n_cells_ = -1;
    rs_type_ = RDT_Unknown;
    rs_type_out_ = RDT_Unknown;
    no_data_value_ = DefaultNoDataByType(TypeToRasterDataType(typeid(T)));
    //no_data_value_ = static_cast<T>(NODATA_VALUE); // Be careful of unsigned data type!
    default_value_ = NODATA_VALUE;
    raster_ = nullptr;
    pos_data_ = nullptr;
    pos_idx_ = nullptr;
    mask_ = nullptr;
    subset_ = map<int, SubsetPositions*>();
    n_lyrs_ = -1;
    is_2draster = is_2d;
    raster_2d_ = nullptr;
    calc_pos_ = false;
    store_pos_ = false;
    use_mask_ext_ = false;
    stats_calculated_ = false;
    headers_ = InitialHeader();
    options_ = InitialStrHeader();
    InitialStatsMap(stats_, stats_2d_);
    initialized_ = true;
}

template <typename T, typename MASK_T>
void clsRasterData<T, MASK_T>::InitializeReadFunction(const string& filename, const bool calc_pos /* = false */,
                                                      clsRasterData<MASK_T>* mask /* = nullptr */,
                                                      const bool use_mask_ext /* = true */,
                                                      double default_value /* NODATA_VALUE */,
                                                      const STRING_MAP& opts /* = STRING_MAP() */) {
    if (!initialized_) { InitializeRasterClass(); }
    full_path_ = filename;
    mask_ = mask;
    calc_pos_ = calc_pos;
    use_mask_ext_ = use_mask_ext;
    if (nullptr == mask_) { use_mask_ext_ = false; }
    default_value_ = default_value;
    rs_type_out_ = RasterDataTypeInOptionals(opts);
    core_name_ = GetCoreFileName(full_path_);
    CopyStringMap(opts, options_);
}

template <typename T, typename MASK_T>
clsRasterData<T, MASK_T>::clsRasterData(bool is_2d /* = false */) {
    InitializeRasterClass(is_2d);
}

template <typename T, typename MASK_T>
clsRasterData<T, MASK_T>::clsRasterData(T* data, int cols, int rows, T nodata,
                                        double dx, double xll, double yll,
                                        const string& srs) {
    STRING_MAP opts = InitialStrHeader();
    UpdateStrHeader(opts, HEADER_RS_SRS, srs);
    clsRasterData(data, cols, rows, nodata, dx, xll, yll, opts);
}

template <typename T, typename MASK_T>
clsRasterData<T, MASK_T>::clsRasterData(T* data, const int cols, const int rows, T nodata,
                                        const double dx, const double xll, const double yll,
                                        const STRING_MAP& opts /* = STRING_MAP() */) {
    InitializeRasterClass(false);
    rs_type_out_ = RasterDataTypeInOptionals(opts);
    raster_ = data;
    no_data_value_ = nodata;
    CopyStringMap(opts, options_);
    n_cells_ = cols * rows;
    n_lyrs_ = 1;
    UpdateHeader(headers_, HEADER_RS_NCOLS, cols);
    UpdateHeader(headers_, HEADER_RS_NROWS, rows);
    UpdateHeader(headers_, HEADER_RS_XLL, xll);
    UpdateHeader(headers_, HEADER_RS_YLL, yll);
    UpdateHeader(headers_, HEADER_RS_CELLSIZE, dx);
    UpdateHeader(headers_, HEADER_RS_NODATA, nodata);
    UpdateHeader(headers_, HEADER_RS_LAYERS, 1);
    UpdateHeader(headers_, HEADER_RS_CELLSNUM, n_cells_);
}

template <typename T, typename MASK_T>
clsRasterData<T, MASK_T>::clsRasterData(T** data2d, const int cols, const int rows, const int nlayers,
                                        T nodata, const double dx, const double xll, const double yll,
                                        const string& srs) {
    STRING_MAP opts = InitialStrHeader();
    UpdateStrHeader(opts, HEADER_RS_SRS, srs);
    clsRasterData(data2d, cols, rows, nlayers, nodata, dx, xll, yll, opts);
}

template <typename T, typename MASK_T>
clsRasterData<T, MASK_T>::clsRasterData(T** data2d, const int cols, const int rows, const int nlayers,
                                        T nodata, const double dx, const double xll, const double yll,
                                        const STRING_MAP& opts /* = STRING_MAP() */) {
    InitializeRasterClass(true);
    rs_type_out_ = RasterDataTypeInOptionals(opts);
    raster_2d_ = data2d;
    no_data_value_ = nodata;
    CopyStringMap(opts, options_);
    n_cells_ = cols * rows;
    n_lyrs_ = nlayers;
    headers_[HEADER_RS_NCOLS] = cols;
    headers_[HEADER_RS_NROWS] = rows;
    headers_[HEADER_RS_XLL] = xll;
    headers_[HEADER_RS_YLL] = yll;
    headers_[HEADER_RS_CELLSIZE] = dx;
    headers_[HEADER_RS_NODATA] = no_data_value_;
    headers_[HEADER_RS_LAYERS] = n_lyrs_;
    headers_[HEADER_RS_CELLSNUM] = n_cells_;
}

template <typename T, typename MASK_T>
clsRasterData<T, MASK_T>::clsRasterData(const string& filename, const bool calc_pos /* = false */,
                                        clsRasterData<MASK_T>* mask /* = nullptr */,
                                        const bool use_mask_ext /* = true */,
                                        double default_value /* NODATA_VALUE */,
                                        const STRING_MAP& opts /* = STRING_MAP() */) {
    InitializeRasterClass();
    rs_type_out_ = RasterDataTypeInOptionals(opts);
    ReadFromFile(filename, calc_pos, mask, use_mask_ext, default_value, opts);
}

template <typename T, typename MASK_T>
clsRasterData<T, MASK_T>* clsRasterData<T, MASK_T>::Init(const string& filename,
                                                         const bool calc_pos /* = false */,
                                                         clsRasterData<MASK_T>* mask /* = nullptr */,
                                                         const bool use_mask_ext /* = true */,
                                                         double default_value /* = NODATA_VALUE */,
                                                         const STRING_MAP& opts /* = STRING_MAP() */) {
    if (!FileExists(filename)) { return nullptr; }
    clsRasterData<T, MASK_T>* rs = new clsRasterData<T, MASK_T>();
    rs->SetOutDataType(RasterDataTypeInOptionals(opts));
    if (!rs->ReadFromFile(filename, calc_pos, mask, use_mask_ext, default_value, opts)) {
        delete rs;
        return nullptr;
    }
    return rs;
}

template <typename T, typename MASK_T>
clsRasterData<T, MASK_T>::clsRasterData(vector<string>& filenames,
                                        const bool calc_pos /* = false */,
                                        clsRasterData<MASK_T>* mask /* = nullptr */,
                                        const bool use_mask_ext /* = true */,
                                        double default_value /* = NODATA_VALUE */,
                                        const STRING_MAP& opts /* = STRING_MAP() */) {
    if (!FilesExist(filenames)) {
        StatusMessage("Please make sure all file path existed!");
        return;
    }
    if (filenames.size() == 1) {
        ReadFromFile(filenames[0], calc_pos, mask,
                     use_mask_ext, default_value, opts);
    }
    ReadFromFiles(filenames, calc_pos, mask,
                  use_mask_ext, default_value, opts);
}

template <typename T, typename MASK_T>
clsRasterData<T, MASK_T>* clsRasterData<T, MASK_T>::Init(vector<string>& filenames,
                                                         bool calc_pos /* = false */,
                                                         clsRasterData<MASK_T>* mask /* = nullptr */,
                                                         bool use_mask_ext /* = true */,
                                                         double default_value /* = NODATA_VALUE */,
                                                         const STRING_MAP& opts /* = STRING_MAP() */) {
    if (!FilesExist(filenames)) {
        StatusMessage("Please make sure all file path existed!");
        return nullptr;
    }
    if (filenames.size() == 1) {
        return clsRasterData<T, MASK_T>::Init(filenames[0], calc_pos, mask, use_mask_ext,
                                              default_value, opts);
    }
    clsRasterData<T, MASK_T>* rs2d = new clsRasterData<T, MASK_T>();
    rs2d->SetOutDataType(RasterDataTypeInOptionals(opts));
    if (!rs2d->ReadFromFiles(filenames, calc_pos, mask, use_mask_ext, default_value, opts)) {
        delete rs2d;
        return nullptr;
    }
    return rs2d;
}

template <typename T, typename MASK_T>
clsRasterData<T, MASK_T>::clsRasterData(clsRasterData<MASK_T>* mask, T* const values, const int len,
                                        const STRING_MAP& opts /* = STRING_MAP() */) {
    InitializeRasterClass(false);
    rs_type_out_ = RasterDataTypeInOptionals(opts);
    calc_pos_ = false;
    mask_ = mask;
    use_mask_ext_ = true;
    n_lyrs_ = 1;
    mask->GetRasterPositionData(&n_cells_, &pos_data_);
    mask->GetRasterPositionData(&n_cells_, &pos_idx_);
    if (n_cells_ != len) {
        StatusMessage("Input data length MUST EQUALS TO valid cell's number of mask!");
        initialized_ = false;
        return;
    }
    Initialize1DArray(n_cells_, raster_, values); // DO NOT ASSIGN ARRAY DIRECTLY!
    default_value_ = mask_->GetDefaultValue();
    CopyHeader(mask_->GetRasterHeader(), headers_);
    UpdateStrHeader(options_, HEADER_RS_SRS, mask_->GetSrsString());
    CopyStringMap(opts, options_);
}

template <typename T, typename MASK_T>
clsRasterData<T, MASK_T>::clsRasterData(clsRasterData<MASK_T>* mask, T** const values, const int len,
                                        const int lyrs, const STRING_MAP& opts /* = STRING_MAP() */) {
    InitializeRasterClass(true);
    calc_pos_ = false;
    mask_ = mask;
    use_mask_ext_ = true;
    n_lyrs_ = lyrs;
    mask->GetRasterPositionData(&n_cells_, &pos_data_);
    mask->GetRasterPositionData(&n_cells_, &pos_idx_);
    if (n_cells_ != len) {
        StatusMessage("Input data length MUST EQUALS TO valid cell's number of mask!");
        initialized_ = false;
        return;
    }
    Initialize2DArray(n_cells_, n_lyrs_, raster_2d_, values); // DO NOT ASSIGN ARRAY DIRECTLY!
    CopyHeader(mask_->GetRasterHeader(), headers_);
    UpdateHeader(headers_, HEADER_RS_LAYERS, n_lyrs_);
    CopyStringMap(opts, options_);
    UpdateStrHeader(options_, HEADER_RS_SRS, mask_->GetSrsString());
}

#ifdef USE_MONGODB

template <typename T, typename MASK_T>
clsRasterData<T, MASK_T>::clsRasterData(MongoGridFs* gfs, const char* remote_filename,
                                        const bool calc_pos /* = false */,
                                        clsRasterData<MASK_T>* mask /* = nullptr */,
                                        const bool use_mask_ext /* = true */,
                                        double default_value /* NODATA_VALUE */,
                                        const STRING_MAP& opts /* = STRING_MAP() */) {
    InitializeRasterClass();
    rs_type_out_ = RasterDataTypeInOptionals(opts);
    ReadFromMongoDB(gfs, remote_filename, calc_pos, mask, use_mask_ext, default_value, opts);
}

template <typename T, typename MASK_T>
clsRasterData<T, MASK_T>* clsRasterData<T, MASK_T>::Init(MongoGridFs* gfs, const char* remote_filename,
                                                         const bool calc_pos /* = false */,
                                                         clsRasterData<MASK_T>* mask /* = nullptr */,
                                                         const bool use_mask_ext /* = true */,
                                                         double default_value /* NODATA_VALUE */,
                                                         const STRING_MAP& opts /* = STRING_MAP() */) {
    clsRasterData<T, MASK_T>* rs_mongo = new clsRasterData<T, MASK_T>();
    rs_mongo->SetOutDataType(RasterDataTypeInOptionals(opts));
    if (!rs_mongo->ReadFromMongoDB(gfs, remote_filename, calc_pos, mask, use_mask_ext, default_value, opts)) {
        delete rs_mongo;
        return nullptr;
    }
    return rs_mongo;
}

#endif /* USE_MONGODB */

template <typename T, typename MASK_T>
bool clsRasterData<T, MASK_T>::ConstructFromSingleFile(const string& filename,
                                                       const bool calc_pos /* = false */,
                                                       clsRasterData<MASK_T>* mask /* = nullptr */,
                                                       const bool use_mask_ext /* = true */,
                                                       double default_value /* NODATA_VALUE */,
                                                       const STRING_MAP& opts /* = STRING_MAP() */) {
    InitializeReadFunction(filename, calc_pos, mask, use_mask_ext, default_value, opts);

    bool readflag = false;
    string srs = string();
    if (StringMatch(GetUpper(GetSuffix(filename)), ASCIIExtension)) {
        readflag = ReadAscFile(full_path_, headers_, raster_);
    } else {
#ifdef USE_GDAL
        readflag = ReadRasterFileByGdal(full_path_, headers_, raster_, rs_type_, srs);
#else
        StatusMessage("Warning: Only ASC format is supported without GDAL!");
        return false;
#endif /* USE_GDAL */
    }
    // After read raster data from ASCII file or GeoTiff.
    no_data_value_ = static_cast<T>(headers_.at(HEADER_RS_NODATA));
    UpdateStrHeader(options_, HEADER_RS_SRS, srs);
    // if not specified, set output data type the same as input
    if (rs_type_out_ == 0) {
        rs_type_out_ = rs_type_;
        UpdateStrHeader(options_, HEADER_RSOUT_DATATYPE, RasterDataTypeToString(rs_type_out_));
    }
    CheckDefaultValue();
    if (readflag) {
        if (n_lyrs_ < 0) { n_lyrs_ = 1; }
        if (is_2draster) { is_2draster = false; }
        return MaskAndCalculateValidPosition() >= 0;
    }
    return false;
}

template <typename T, typename MASK_T>
clsRasterData<T, MASK_T>::~clsRasterData() {
    if (!core_name_.empty()) { StatusMessage(("Release raster: " + core_name_).c_str()); }
    if (nullptr != raster_) { Release1DArray(raster_); }
    if (nullptr != pos_data_ && store_pos_) { Release2DArray(pos_data_); }
    if (nullptr != pos_idx_ && store_pos_) { Release1DArray(pos_idx_); }
    if (nullptr != raster_2d_ && is_2draster) { Release2DArray(raster_2d_); }
    if (is_2draster && stats_calculated_) { ReleaseStatsMap2D(); }
    ReleaseSubset();
}

template <typename T, typename MASK_T>
bool clsRasterData<T, MASK_T>::BuildSubSet(map<int, int> groups /* = map<int, int>() */) {
    if (!ValidateRasterData()) { return false; }
    if (nullptr == pos_data_ || nullptr == pos_idx_) {
        if (!SetCalcPositions()) { return false; }
    }
    if (!subset_.empty()) { return true; }

    int global_ncols = GetCols();
    map<int, vector<int> > global_idx;
    for (int vi = 0; vi < n_cells_; vi++) {
        T curv = GetValueByIndex(vi); // compatible with 2D Raster
        if (FloatEqual(curv, no_data_value_)) { continue; }
        int groupv = CVT_INT(curv); // By default, group value is original raster value
        if (!groups.empty() && groups.find(groupv) != groups.end()) {
            groupv = groups.at(groupv); // original raster value --> specified group ID
        }
        //int currow = pos_data_[vi][0];
        //int curcol = pos_data_[vi][1];
        int currow = pos_idx_[vi] / global_ncols;
        int curcol = pos_idx_[vi] % global_ncols;
        if (subset_.find(groupv) == subset_.end()) {
#ifdef HAS_VARIADIC_TEMPLATES
            subset_.emplace(groupv, new SubsetPositions(currow, currow, curcol, curcol));
#else
            subset_.insert(make_pair(groupv, new SubsetPositions(currow, currow, curcol, curcol)));
#endif
        }
        SubsetPositions*& cursubset = subset_.at(groupv);
        if (currow > cursubset->g_erow) { cursubset->g_erow = currow; }
        if (currow < cursubset->g_srow) { cursubset->g_srow = currow; }
        if (curcol > cursubset->g_ecol) { cursubset->g_ecol = curcol; }
        if (curcol < cursubset->g_scol) { cursubset->g_scol = curcol; }
        if (global_idx.find(groupv) == global_idx.end()) {
#ifdef HAS_VARIADIC_TEMPLATES
            global_idx.emplace(groupv, vector<int>());
#else
            global_idx.insert(make_pair(groupv, vector<int>()));
#endif
        }
        global_idx[groupv].emplace_back(vi);
        cursubset->n_cells += 1;
    }
    for (auto it = subset_.begin(); it != subset_.end(); ++it) {
        Initialize1DArray(it->second->n_cells, it->second->global_, 0);
        for (auto it2 = global_idx[it->first].begin(); it2 != global_idx[it->first].end(); ++it2) {
            it->second->global_[it2 - global_idx[it->first].begin()] = *it2;
        }
    }
    for (auto it = subset_.begin(); it != subset_.end(); ++it) {
        Initialize2DArray(it->second->n_cells, 2, it->second->local_pos_, -1);
        Initialize1DArray(it->second->n_cells, it->second->local_posidx_, -1);

        int nrows = it->second->g_erow - it->second->g_srow + 1;
        int local_ncols = it->second->g_ecol - it->second->g_scol + 1;
        for (int gidx = 0; gidx < it->second->n_cells; gidx++) {
            //it->second->local_pos_[gidx][0] = pos_data_[it->second->global_[gidx]][0] - it->second->g_srow;
            //it->second->local_pos_[gidx][1] = pos_data_[it->second->global_[gidx]][1] - it->second->g_scol;
            int local_row = pos_idx_[it->second->global_[gidx]] / global_ncols - it->second->g_srow;
            int local_col = pos_idx_[it->second->global_[gidx]] % global_ncols  - it->second->g_scol;
            it->second->local_pos_[gidx][0] = local_row;
            it->second->local_pos_[gidx][1] = local_col;
            it->second->local_posidx_[gidx] = local_row * local_ncols + local_col;
        }
        it->second->alloc_ = true;
    }
    return true;
}

template <typename T, typename MASK_T>
bool clsRasterData<T, MASK_T>::ReleaseSubset() {
    if (!subset_.empty()) {
        for (auto it = subset_.begin(); it != subset_.end(); ++it) {
            delete it->second;
            it->second = nullptr;
        }
        subset_.clear();
    }
    return true;
}

template <typename T, typename MASK_T>
bool clsRasterData<T, MASK_T>::RebuildSubSet(const map<int, int> groups /* = map<int, int>() */) {
    return ReleaseSubset() && BuildSubSet(groups);
}

/************* Set information functions ***************/

template <typename T, typename MASK_T>
void clsRasterData<T, MASK_T>::SetOutDataType(RasterDataType const type) {
    rs_type_out_ = type;
    UpdateStringMap(options_, HEADER_RSOUT_DATATYPE, RasterDataTypeToString(rs_type_out_));
}

template <typename T, typename MASK_T>
void clsRasterData<T, MASK_T>::SetOutDataType(const string& strtype) {
    rs_type_out_ = StringToRasterDataType(strtype);
    UpdateStringMap(options_, HEADER_RSOUT_DATATYPE, RasterDataTypeToString(rs_type_out_));
}

/************* Get information functions ***************/
template <typename T, typename MASK_T>
void clsRasterData<T, MASK_T>::CalculateStatistics() {
    if (stats_calculated_) { return; }
    if (stats_.empty() || stats_2d_.empty()) { InitialStatsMap(stats_, stats_2d_); }
    if (is_2draster && nullptr != raster_2d_) {
        double** derivedvs = nullptr;
        BasicStatistics(raster_2d_, n_cells_, n_lyrs_, &derivedvs, no_data_value_);
        stats_2d_.at(STATS_RS_VALIDNUM) = derivedvs[0];
        stats_2d_.at(STATS_RS_MEAN) = derivedvs[1];
        stats_2d_.at(STATS_RS_MAX) = derivedvs[2];
        stats_2d_.at(STATS_RS_MIN) = derivedvs[3];
        stats_2d_.at(STATS_RS_STD) = derivedvs[4];
        stats_2d_.at(STATS_RS_RANGE) = derivedvs[5];
        // 1D array elements of derivedvs will be released by the destructor: releaseStatsMap2D()
        Release1DArray(derivedvs);
    } else {
        double* derivedv = nullptr;
        BasicStatistics(raster_, n_cells_, &derivedv, no_data_value_);
        stats_.at(STATS_RS_VALIDNUM) = derivedv[0];
        stats_.at(STATS_RS_MEAN) = derivedv[1];
        stats_.at(STATS_RS_MAX) = derivedv[2];
        stats_.at(STATS_RS_MIN) = derivedv[3];
        stats_.at(STATS_RS_STD) = derivedv[4];
        stats_.at(STATS_RS_RANGE) = derivedv[5];
        Release1DArray(derivedv);
    }
    stats_calculated_ = true;
}

template <typename T, typename MASK_T>
void clsRasterData<T, MASK_T>::ReleaseStatsMap2D() {
    for (auto it = stats_2d_.begin(); it != stats_2d_.end(); ++it) {
        if (nullptr != it->second) {
            Release1DArray(it->second);
        }
    }
    stats_2d_.clear();
}

template <typename T, typename MASK_T>
void clsRasterData<T, MASK_T>::UpdateStatistics() {
    if (is_2draster && stats_calculated_) ReleaseStatsMap2D();
    stats_calculated_ = false;
    CalculateStatistics();
}

template <typename T, typename MASK_T>
double clsRasterData<T, MASK_T>::GetStatistics(string sindex, const int lyr /* = 1 */) {
    sindex = GetUpper(sindex);
    if (!ValidateRasterData() || !ValidateLayer(lyr)) {
        StatusMessage("No available raster statistics!");
        return CVT_DBL(default_value_);
    }
    if (is_2draster && nullptr != raster_2d_) {
        // for 2D raster data
        auto it = stats_2d_.find(sindex);
        if (it != stats_2d_.end()) {
            if (nullptr == it->second) {
                stats_calculated_ = false;
                CalculateStatistics();
            }
            return stats_2d_.at(sindex)[lyr - 1];
        }
        StatusMessage("WARNING: " + ValueToString(sindex) + " is not supported currently.");
        return CVT_DBL(default_value_);
    }
    // Else, for 1D raster data
    auto it = stats_.find(sindex);
    if (it != stats_.end()) {
        if (FloatEqual(it->second, CVT_DBL(NODATA_VALUE)) || !stats_calculated_) {
            CalculateStatistics();
        }
        return stats_.at(sindex);
    }
    StatusMessage("WARNING: " + ValueToString(sindex) + " is not supported currently.");
    return CVT_DBL(default_value_);
}

template <typename T, typename MASK_T>
void clsRasterData<T, MASK_T>::GetStatistics(string sindex, int* lyrnum, double** values) {
    if (!is_2draster || nullptr != raster_2d_) {
        StatusMessage("Please initialize the raster object first.");
        *values = nullptr;
        return;
    }
    sindex = GetUpper(sindex);
    *lyrnum = n_lyrs_;
    auto it = stats_2d_.find(sindex);
    if (!Is2DRaster()) {
        *values = nullptr;
        return;
    }
    if (it == stats_2d_.end()) {
        *values = nullptr;
        StatusMessage("WARNING: " + ValueToString(sindex) + " is not supported currently.");
        return;
    }
    if (nullptr == it->second || !stats_calculated_) {
        CalculateStatistics();
    }
    *values = it->second;
}

template <typename T, typename MASK_T>
int clsRasterData<T, MASK_T>::GetPosition(const int row, const int col) {
    if (!ValidateRasterData() || !ValidateRowCol(row, col)) {
        return -2; // means error occurred!
    }
    int pos_idx = GetCols() * row + col;
    if (!calc_pos_ || nullptr == pos_data_ || nullptr == pos_idx_) {
        return pos_idx;
    }
// previous low-efficiency code
//    for (int i = 0; i < n_cells_; i++) {
//        if (row == pos_data_[i][0] && col == pos_data_[i][1]) {
//            return i;
//        }
//    }
    // Use binary search method, refers to https://leetcode.cn/problems/binary-search
    //int search(vector<int>& nums, int target) {
    int left = 0;
    int right = n_cells_ - 1; // assumes pos_idx belongs to pos_idx_[left, right]
    while (left <= right) { // when left==right，[left, right] still works，so use <=
        int middle = left + ((right - left) / 2); // equals to (left + right)/2
        if (pos_idx_[middle] > pos_idx) {
            right = middle - 1; // pos_idx belongs to [left, middle - 1]
        } else if (pos_idx_[middle] < pos_idx) {
            left = middle + 1; // pos_idx belongs to [middle + 1, right]
        } else { // pos_idx_[middle] == pos_idx
            return middle;
        }
    }
    return -1; // means the location of the raster data or mask data is NODATA
}

template <typename T, typename MASK_T>
int clsRasterData<T, MASK_T>::GetPosition(const float x, const float y) {
    return GetPosition(CVT_DBL(x), CVT_DBL(y));
}

template <typename T, typename MASK_T>
int clsRasterData<T, MASK_T>::GetPosition(const double x, const double y) {
    if (!initialized_) return -2;
    double xll_center = GetXllCenter();
    double yll_center = GetYllCenter();
    double dx = GetCellWidth();
    double dy = GetCellWidth();
    int n_rows = GetRows();
    int n_cols = GetCols();

    if (FloatEqual(xll_center, CVT_DBL(NODATA_VALUE))
        || FloatEqual(yll_center, CVT_DBL(NODATA_VALUE))
        || FloatEqual(dx, CVT_DBL(NODATA_VALUE))
        || n_rows < 0 || n_cols < 0) {
        StatusMessage("No available header information!");
        return -2;
    }

    double x_min = xll_center - dx / 2.;
    double x_max = x_min + dx * n_cols;
    if (x > x_max || x < x_min) {
        return -2;
    }

    double y_min = yll_center - dy / 2.;
    double y_max = y_min + dy * n_rows;
    if (y > y_max || y < y_min) {
        return -2;
    }

    int n_row = CVT_INT((y_max - y) / dy); //calculate from ymax
    int n_col = CVT_INT((x - x_min) / dx); //calculate from xmin

    return GetPosition(n_row, n_col);
}

template <typename T, typename MASK_T>
bool clsRasterData<T, MASK_T>::GetRasterData(int* n_cells, T** data) {
    if (ValidateRasterData() && !is_2draster) {
        *n_cells = n_cells_;
        *data = raster_;
        return true;
    }
    *n_cells = -1;
    *data = nullptr;
    return false;
}

template <typename T, typename MASK_T>
bool clsRasterData<T, MASK_T>::Get2DRasterData(int* n_cells, int* n_lyrs, T*** data) {
    if (ValidateRasterData() && is_2draster) {
        *n_cells = n_cells_;
        *n_lyrs = n_lyrs_;
        *data = raster_2d_;
        return true;
    }
    *n_cells = -1;
    *n_lyrs = -1;
    *data = nullptr;
    return false;
}

template <typename T, typename MASK_T>
void clsRasterData<T, MASK_T>::GetRasterPositionData(int* datalength, int*** positiondata) {
    if (nullptr != pos_data_) {
        *datalength = n_cells_;
        *positiondata = pos_data_;
    } else {
        // reCalculate position data
        if (!ValidateRasterData()) {
            *datalength = -1;
            *positiondata = nullptr;
            return;
        }
        CalculateValidPositionsFromGridData();
        *datalength = n_cells_;
        *positiondata = pos_data_;
    }
}

template <typename T, typename MASK_T>
void clsRasterData<T, MASK_T>::GetRasterPositionData(int* datalength, int** positiondata) {
    if (nullptr != pos_idx_) {
        *datalength = n_cells_;
        *positiondata = pos_idx_;
    } else {
        // reCalculate position data
        if (!ValidateRasterData()) {
            *datalength = -1;
            *positiondata = nullptr;
            return;
        }
        CalculateValidPositionsFromGridData();
        *datalength = n_cells_;
        *positiondata = pos_idx_;
    }
}

template <typename T, typename MASK_T>
const char* clsRasterData<T, MASK_T>::GetSrs() {
    return GetSrsString().c_str();
}

template <typename T, typename MASK_T>
string clsRasterData<T, MASK_T>::GetSrsString() {
    return GetOption(HEADER_RS_SRS);
}

template <typename T, typename MASK_T>
string clsRasterData<T, MASK_T>::GetOption(const char* key) {
    if (options_.find(key) == options_.end()) {
        StatusMessage((string(key) + " is not existed in the options of the raster data!").c_str());
        return "";
    }
    return options_.at(key);
}

template <typename T, typename MASK_T>
T clsRasterData<T, MASK_T>::GetValueByIndex(const int cell_index, const int lyr /* = 1 */) {
    if (!ValidateRasterData() || !ValidateIndex(cell_index) || !ValidateLayer(lyr)) {
        return no_data_value_;
    }
    if (is_2draster) {
        return raster_2d_[cell_index][lyr - 1];
    }
    return raster_[cell_index];
}

template <typename T, typename MASK_T>
void clsRasterData<T, MASK_T>::GetValueByIndex(const int cell_index, T*& values) {
    if (!ValidateRasterData() || !ValidateIndex(cell_index)) {
        if (nullptr != values) { Release1DArray(values); }
        values = nullptr;
        return;
    }
    if (nullptr == values) {
        Initialize1DArray(n_lyrs_, values, no_data_value_);
    }
    if (is_2draster) {
        for (int i = 0; i < n_lyrs_; i++) {
            values[i] = raster_2d_[cell_index][i];
        }
    } else {
        values[0] = raster_[cell_index];
    }
}

template <typename T, typename MASK_T>
T clsRasterData<T, MASK_T>::GetValue(const int row, const int col, const int lyr /* = 1 */) {
    if (!ValidateRasterData() || !ValidateRowCol(row, col) || !ValidateLayer(lyr)) {
        return no_data_value_;
    }
    // get index according to position data if possible
    if (calc_pos_ && (nullptr != pos_data_ || nullptr != pos_idx_)) {
        int valid_cell_index = GetPosition(row, col);
        if (valid_cell_index < 0) { return no_data_value_; }// error or NODATA
        return GetValueByIndex(valid_cell_index, lyr);
    }
    // get data directly from row and col
    if (is_2draster) { return raster_2d_[row * GetCols() + col][lyr - 1]; }
    return raster_[row * GetCols() + col];
}

template <typename T, typename MASK_T>
void clsRasterData<T, MASK_T>::GetValue(const int row, const int col, T*& values) {
    if (!ValidateRasterData() || !ValidateRowCol(row, col)) {
        if (nullptr != values) { Release1DArray(values); }
        values = nullptr;
        return;
    }
    if (nullptr == values) {
        Initialize1DArray(n_lyrs_, values, no_data_value_);
    }
    if (calc_pos_ && (nullptr != pos_data_ || nullptr != pos_idx_)) {
        int valid_cell_index = GetPosition(row, col);
        if (valid_cell_index == -1) {
            for (int i = 0; i < n_lyrs_; i++) {
                values[i] = no_data_value_; // NODATA
            }
        } else {
            GetValueByIndex(valid_cell_index, values);
        }
    } else {
        // get data directly from row and col
        if (is_2draster) {
            for (int i = 0; i < n_lyrs_; i++) {
                values[i] = raster_2d_[row * GetCols() + col][i];
            }
        } else {
            values[0] = raster_[row * GetCols() + col];
        }
    }
}

template <typename T, typename MASK_T>
void clsRasterData<T, MASK_T>::SetHeader(const STRDBL_MAP& refers) {
    CopyHeader(refers, headers_);
    // Update header related variables
    auto it = headers_.find(HEADER_RS_CELLSNUM);
    if (it != headers_.end()) { n_cells_ = CVT_INT(it->second); }
    it = headers_.find(HEADER_RS_LAYERS);
    if (it != headers_.end()) { n_lyrs_ = CVT_INT(it->second); }
    it = headers_.find(HEADER_RS_NODATA);
    if (it != headers_.end()) { no_data_value_ = static_cast<T>(it->second); }
}


template <typename T, typename MASK_T>
void clsRasterData<T, MASK_T>::SetValue(const int row, const int col, T value, const int lyr /* = 1 */) {
    if (!ValidateRasterData() || !ValidateRowCol(row, col) || !ValidateLayer(lyr)) {
        StatusMessage("Set value failed!");
        return;
    }
    int idx = GetPosition(row, col);
    if (idx == -1) {
        // the origin value is NODATA, and positions of valid values are calculated
        StatusMessage("Current version do not support to setting value to NoDATA location!");
    } else {
        if (is_2draster) {
            raster_2d_[idx][lyr - 1] = value;
        } else {
            raster_[idx] = value;
        }
    }
}

template <typename T, typename MASK_T>
bool clsRasterData<T, MASK_T>::SetCalcPositions() {
    if (!ValidateRasterData()) { return false; }
    if (calc_pos_ && (nullptr != pos_data_ || nullptr != pos_idx_)) {
        // already set as True, no need to recalculate.
        return false;
    }
    calc_pos_ = true;
    return MaskAndCalculateValidPosition() >= 0;
}

template <typename T, typename MASK_T>
bool clsRasterData<T, MASK_T>::SetPositions(int len, int** pdata) {
    if (nullptr != pos_data_) {
        if (len != n_cells_) { return false; } // cannot change origin n_cells_
        Release2DArray(pos_data_);
    }
    pos_data_ = pdata;
    calc_pos_ = true;
    store_pos_ = false;
    return true;
}

template <typename T, typename MASK_T>
bool clsRasterData<T, MASK_T>::SetPositions(int len, int* pdata) {
    if (nullptr != pos_idx_) {
        if (len != n_cells_) { return false; } // cannot change origin n_cells_
        Release1DArray(pos_idx_);
    }
    pos_idx_ = pdata;
    calc_pos_ = true;
    store_pos_ = false;
    return true;
}

template <typename T, typename MASK_T>
bool clsRasterData<T, MASK_T>::SetUseMaskExt() {
    if (!ValidateRasterData()) return false;
    if (nullptr == mask_) return false;
    if (use_mask_ext_) return false; // already set as True, no need to recalculate.
    use_mask_ext_ = true;
    return MaskAndCalculateValidPosition() >= 0;
}

/************* Output to file functions ***************/

template <typename T, typename MASK_T>
bool clsRasterData<T, MASK_T>::OutputToFile(const string& filename, const bool out_origin /* = true */) {
    if (GetPathFromFullName(filename).empty()) { return false; }
    string abs_filename = GetAbsolutePath(filename);
    if (!ValidateRasterData()) { return false; }
    if (!out_origin) { return OutputSubsetToFile(false, true, filename); }
    string filetype = GetUpper(GetSuffix(abs_filename));
    if (StringMatch(filetype, ASCIIExtension)) {
        return OutputAscFile(abs_filename);
    }
#ifdef USE_GDAL
    if (StringMatch(filetype, GTiffExtension)) {
        return OutputFileByGdal(abs_filename);
    }
    return OutputFileByGdal(ReplaceSuffix(abs_filename, GTiffExtension));
#else
    StatusMessage("Warning: Without GDAL, ASC file will be exported as default!");
    return OutputAscFile(ReplaceSuffix(abs_filename, ASCIIExtension));
#endif /* USE_GDAL */
}

template <typename T, typename MASK_T>
bool clsRasterData<T, MASK_T>::OutputSubsetToFile(const bool out_origin /* = false */,
                                                  const bool out_combined /* = true */,
                                                  const string& outname /* = string() */,
                                                  const map<vint, vector<double> >& recls /* map() */,
                                                  const double default_value /*  = NODATA_VALUE */) {
    if (!ValidateRasterData()) { return false; }
    if (subset_.empty()) { return false; }
    string outpathact = outname.empty() ? full_path_ : outname;
    // output combined subsets
    bool out_comb = out_combined;
    if (out_comb) {
        T* data1d = nullptr;
        int sublyrs;
        int sublen;
        if (!PrepareCombSubsetData(&data1d, &sublen, &sublyrs,
                                   out_origin, true, recls, default_value)) {
            return false;
        }
        STRDBL_MAP tmpheader;
        CopyHeader(headers_, tmpheader);
        UpdateHeader(tmpheader, HEADER_RS_LAYERS, sublyrs);
        UpdateHeader(tmpheader, HEADER_RS_CELLSNUM, sublen / sublyrs);
        bool combflag = OutputFullsizeToFiles(data1d, sublen / sublyrs, sublyrs,
                                              PrefixCoreFileName(outpathact, 0),
                                              tmpheader, options_);
        Release1DArray(data1d);
        return combflag;
    }
    STRDBL_MAP subheader;
    bool flag = true;
    for (auto it = subset_.begin(); it != subset_.end(); ++it) {
        if (!it->second->usable) { continue; }
        it->second->GetHeader(GetXllCenter(), GetYllCenter(), GetRows(),
                              GetCellWidth(), CVT_DBL(no_data_value_), subheader);
        T* tmpdata1d = nullptr;
        int tmpdatalen;
        int tmplyrs;
        if (!PrepareSubsetData(it->first, it->second, &tmpdata1d, &tmpdatalen, &tmplyrs,
                               out_origin, true, recls, default_value)) {
            continue;
        }
        UpdateHeader(subheader, HEADER_RS_LAYERS, tmplyrs);
        UpdateHeader(subheader, HEADER_RS_CELLSNUM, tmpdatalen / tmplyrs);
        flag = flag && OutputFullsizeToFiles(tmpdata1d, tmpdatalen / tmplyrs, tmplyrs,
                                             PrefixCoreFileName(outpathact, it->first),
                                             subheader, options_);
        Release1DArray(tmpdata1d);
    }
    return flag;
}

template <typename T, typename MASK_T>
bool clsRasterData<T, MASK_T>::PrepareCombSubsetData(T** values, int* datalen, int* datalyrs,
                                                     bool out_origin /* false */, bool include_nodata /* true */,
                                                     const map<vint, vector<double> >& recls /* map() */,
                                                     double default_value /* NODATA_VALUE*/) {
    if (subset_.empty()) { return false; }
    T* data1d = nullptr; // Both raster 1D and 2D data can be combined as 1D array
    int lyr_recls = -1;
    if (!recls.empty()) {
        for (auto it = recls.begin(); it != recls.end(); ++it) {
            if (lyr_recls < 0) { lyr_recls = CVT_INT(it->second.size()); }
            else if (lyr_recls != it->second.size()) {
                StatusMessage("Error: Reclassification layer count MUST be consistent!");
                return false;
            }
        }
    }
    int lyrs_subset = -1;
    for (auto it = subset_.begin(); it != subset_.end(); ++it) {
        if (!it->second->usable) { continue; }
        if (!(nullptr != it->second->data_    // Only if all subset have data_
            || nullptr != it->second->data2d_ // or data2d_,
            || !recls.empty())) {             // or reclassification map specified
            return false;
        }
        if (lyrs_subset < 0) { lyrs_subset = it->second->n_lyrs; }
        else if (it->second->n_lyrs != lyrs_subset) {
            StatusMessage("Error: Subset's layer count MUST be consistent!");
            return false;
        }
    }
    int lyrs = -1;
    if (out_origin) {
        lyrs = n_lyrs_;
        if (lyr_recls > 0) { lyrs = lyr_recls; }
    } else {
        lyrs = lyrs_subset;
        if (lyrs < 0 && lyr_recls > 0) {
            lyrs = lyr_recls;
        }
    }
    if (lyrs < 0) {
        StatusMessage("Error: Cannot determine valid layer count!");
        return false;
    }

    int gnrows = GetRows();
    int gncols = GetCols();
    if (FloatEqual(default_value, NODATA_VALUE)) {
        default_value = default_value_;
    }
    int gncells = include_nodata ? gnrows * gncols : n_cells_;
    int data_length = gncells * lyrs;
    Initialize1DArray(data_length, data1d, no_data_value_);
    for (auto it = subset_.begin(); it != subset_.end(); ++it) {
        bool use_defaultv_directly = false;
        if (!it->second->usable) {
            if (!FloatEqual(no_data_value_, default_value)) {
                use_defaultv_directly = true;
            } else {
                continue;
            }
        }
        for (int vi = 0; vi < it->second->n_cells; vi++) {
            for (int ilyr = 0; ilyr < lyrs; ilyr++) {
                int gidx = it->second->global_[vi];
                //int tmpr = pos_data_[gidx][0];
                //int tmpc = pos_data_[gidx][1];
                //int tmprc = tmpr * gncols + tmpc;
                int tmprc = pos_idx_[gidx];
                if (!include_nodata) { tmprc = gidx; }
                if (!recls.empty()) { // first priority
                    double uniqe_value = default_value;
                    int recls_key = it->first; // default reclassification key is subset's ID
                    if (out_origin) {
                        if (nullptr != raster_) { recls_key = CVT_INT(raster_[gidx]); }
                        else if (nullptr != raster_2d_) {
                            recls_key = CVT_INT(raster_2d_[gidx][ilyr]);
                        }
                    }
                    if (recls.count(recls_key) > 0 && recls.at(recls_key).size() > ilyr) {
                        uniqe_value = recls.at(recls_key).at(ilyr);
                    }
                    data1d[tmprc * lyrs + ilyr] = static_cast<T>(uniqe_value);
                }
                else if (use_defaultv_directly) {
                    data1d[tmprc * lyrs + ilyr] = static_cast<T>(default_value);
                }
                else if (!out_origin && lyrs > 1 && nullptr != it->second->data2d_) { // raster 2D
                    data1d[tmprc * lyrs + ilyr] = static_cast<T>(it->second->data2d_[vi][ilyr]);
                }
                else if (!out_origin && lyrs == 1 && nullptr != it->second->data_) { // raster 1D
                    data1d[tmprc * lyrs + ilyr] = static_cast<T>(it->second->data_[vi]);
                }
                else { // Will not happen
                    StatusMessage("Error: No subset or reclassification map can be output!");
                    if (nullptr != data1d) { Release1DArray(data1d); }
                    return false;
                }
            }
        }
    }
    *values = data1d;
    *datalen = data_length;
    *datalyrs = lyrs;
    return true;
}

template <typename T, typename MASK_T>
bool clsRasterData<T, MASK_T>::PrepareSubsetData(const int sub_id, SubsetPositions* sub,
                                                 T** values, int* datalen, int* datalyrs,
                                                 bool out_origin /* false */, bool include_nodata /* true */,
                                                 const map<vint, vector<double> >& recls /* map() */,
                                                 double default_value /* NODATA_VALUE*/) {
    if (nullptr == sub) { return false; }
    T* data1d = nullptr; // Both raster 1D and 2D data can be combined as 1D array
    int lyr_recls = -1;
    if (!recls.empty()) {
        for (auto it = recls.begin(); it != recls.end(); ++it) {
            if (lyr_recls < 0) { lyr_recls = CVT_INT(it->second.size()); }
            else if (lyr_recls != it->second.size()) {
                StatusMessage("Error: Reclassification layer count MUST be consistent!");
                return false;
            }
        }
    }
    int lyrs = - 1;
    if (out_origin) {
        lyrs = n_lyrs_;
        if (lyr_recls > 0) { lyrs = lyr_recls; }
    } else {
        lyrs = sub->n_lyrs;
    }
    if (lyrs < 0) {
        StatusMessage("Error: Cannot determine valid layer count!");
        return false;
    }
    int nrows = sub->g_erow - sub->g_srow + 1;
    int ncols = sub->g_ecol - sub->g_scol + 1;
    if (FloatEqual(default_value, NODATA_VALUE)) {
        default_value = default_value_;
    }
    int ncells = include_nodata ? nrows * ncols : sub->n_cells;
    int data_length = ncells * lyrs;
    Initialize1DArray(data_length, data1d, no_data_value_);
    for (int vi = 0; vi < sub->n_cells; vi++) {
        for (int ilyr = 0; ilyr < lyrs; ilyr++) {
            //int j = sub->local_pos_[vi][0] * ncols + sub->local_pos_[vi][1];
            int j = sub->local_posidx_[vi];
            int gidx = sub->global_[vi];
            if (!include_nodata) { j = vi; }
            if (!recls.empty()) { // first priority
                double uniqe_value = default_value;
                int recls_key = sub_id;
                if (out_origin) {
                    if (nullptr != raster_) { recls_key = CVT_INT(raster_[gidx]); }
                    else if (nullptr != raster_2d_) {
                        recls_key = CVT_INT(raster_2d_[gidx][ilyr]);
                    }
                }
                if (recls.count(recls_key) > 0 && recls.at(recls_key).size() > ilyr) {
                    uniqe_value = recls.at(recls_key).at(ilyr);
                }
                data1d[j * lyrs + ilyr] = static_cast<T>(uniqe_value);
            }
            else if (out_origin && lyrs > 1 && nullptr != raster_2d_) {
                data1d[j * lyrs + ilyr] = raster_2d_[gidx][ilyr];
            }
            else if (out_origin && lyrs == 1 && nullptr != raster_) {
                data1d[j * lyrs + ilyr] = raster_[gidx];
            }
            else if (!out_origin && lyrs > 1 && nullptr != sub->data2d_) { // raster 2D
                data1d[j * lyrs + ilyr] = static_cast<T>(sub->data2d_[vi][ilyr]);
            }
            else if (!out_origin && lyrs == 1 && nullptr != sub->data_) { // raster 1D
                data1d[j * lyrs + ilyr] = static_cast<T>(sub->data_[vi]);
            }
            else { // Will not happen
                StatusMessage("Error: No subset or reclassification map can be output!");
                if (nullptr != data1d) { Release1DArray(data1d); }
                return false;
            }
        }
    }
    *values = data1d;
    *datalen = data_length;
    *datalyrs = lyrs;
    return true;
}

template <typename T, typename MASK_T>
bool clsRasterData<T, MASK_T>::OutputFullsizeToFiles(T* fullsizedata, const int fsize, const int datalyrs,
                                                     const string& fullfilename,
                                                     const STRDBL_MAP& header, const STRING_MAP& opts) {
    if (nullptr == fullsizedata) { return false; }
    if (datalyrs < 1) { return false; }
    if (datalyrs == 1) {
        return WriteRasterToFile(fullfilename, header, opts, fullsizedata);
    }
    T* tmpdata1d = nullptr;
    Initialize1DArray(fsize, tmpdata1d, NODATA_VALUE);
    bool flag = true;
    for (int ilyr = 0; ilyr < datalyrs; ilyr++) {
        for (int gi = 0; gi < fsize; gi++) {
            tmpdata1d[gi] = fullsizedata[gi * datalyrs + ilyr];
        }
        flag = flag && WriteRasterToFile(AppendCoreFileName(fullfilename, ilyr + 1),
                                         header, opts, tmpdata1d);
    }
    if (nullptr != tmpdata1d) { Release1DArray(tmpdata1d); }
    return flag;
}

template <typename T, typename MASK_T>
bool clsRasterData<T, MASK_T>::OutputAscFile(const string& filename) {
    string abs_filename = GetAbsolutePath(filename);
    // Is there need to calculate valid position index?
    int count;
    int** position = nullptr;
    int* position_idx = nullptr;
    bool outputdirectly = true;
    if ((nullptr != pos_data_ || nullptr != pos_idx_)) {
        GetRasterPositionData(&count, &position);
        GetRasterPositionData(&count, &position_idx);
        outputdirectly = false;
        assert(nullptr != position);
        assert(nullptr != position_idx);
    }
    // Begin to write raster data
    int rows = CVT_INT(headers_.at(HEADER_RS_NROWS));
    int cols = CVT_INT(headers_.at(HEADER_RS_NCOLS));
    if (is_2draster) { // 3.1 2D raster data
        string pre_path = GetPathFromFullName(abs_filename);
        if (StringMatch(pre_path, "")) { return false; }
        for (int lyr = 0; lyr < n_lyrs_; lyr++) {
            string tmpfilename = AppendCoreFileName(abs_filename, itoa(CVT_VINT(lyr)));
            if (!WriteAscHeaders(tmpfilename, headers_)) { return false; }
            std::ofstream raster_file(tmpfilename.c_str(), std::ios::app | std::ios::out);
            if (!raster_file.is_open()) {
                StatusMessage("Error opening file: " + tmpfilename);
                return false;
            }
            int index = 0;
            for (int i = 0; i < rows; ++i) {
                for (int j = 0; j < cols; ++j) {
                    if (outputdirectly) {
                        index = i * cols + j;
                        raster_file << setprecision(6) << raster_2d_[index][lyr] << " ";
                        continue;
                    }
                    if (index < n_cells_ && (position[index][0] == i && position[index][1] == j)) {
                        raster_file << setprecision(6) << raster_2d_[index][lyr] << " ";
                        index++;
                    } else { raster_file << setprecision(6) << NODATA_VALUE << " "; }
                }
                raster_file << endl;
            }
            raster_file.close();
        }
    } else { // 1D raster data
        if (!WriteAscHeaders(abs_filename, headers_)) { return false; }
        std::ofstream raster_file(filename.c_str(), std::ios::app | std::ios::out);
        if (!raster_file.is_open()) {
            StatusMessage("Error opening file: " + abs_filename);
            return false;
        }
        int index = 0;
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                if (outputdirectly) {
                    index = i * cols + j;
                    raster_file << setprecision(6) << raster_[index] << " ";
                    continue;
                }
                if (index < n_cells_) {
                    if (position[index][0] == i && position[index][1] == j) {
                        raster_file << setprecision(6) << raster_[index] << " ";
                        index++;
                    } else { raster_file << setprecision(6) << no_data_value_ << " "; }
                } else { raster_file << setprecision(6) << no_data_value_ << " "; }
            }
            raster_file << endl;
        }
        raster_file.close();
    }
    position = nullptr;
    return true;
}

#ifdef USE_GDAL
template <typename T, typename MASK_T>
bool clsRasterData<T, MASK_T>::OutputFileByGdal(const string& filename) {
    string abs_filename = GetAbsolutePath(filename);
    bool outputdirectly = (nullptr == pos_data_ || nullptr == pos_idx_);
    int n_rows = CVT_INT(headers_.at(HEADER_RS_NROWS));
    int n_cols = CVT_INT(headers_.at(HEADER_RS_NCOLS));
    bool outflag = false;
    T* data_1d = nullptr;
    if (is_2draster) {
        string pre_path = GetPathFromFullName(abs_filename);
        if (StringMatch(pre_path, "")) { return false; }
        for (int lyr = 0; lyr < n_lyrs_; lyr++) {
            string tmpfilename = AppendCoreFileName(abs_filename, lyr + 1);
            if (outputdirectly) {
                if (nullptr == data_1d) {
                    Initialize1DArray(n_rows * n_cols, data_1d, no_data_value_);
                }
                for (int gi = 0; gi < n_rows * n_cols; gi++) {
                    data_1d[gi] = raster_2d_[gi][lyr];
                }
            } else {
                if (nullptr == data_1d) {
                    Initialize1DArray(n_rows * n_cols, data_1d, no_data_value_);
                }
                for (int vi = 0; vi < n_cells_; vi++) {
                    //data_1d[pos_data_[vi][0] * n_cols + pos_data_[vi][1]] = raster_2d_[vi][lyr];
                    data_1d[pos_idx_[vi]] = raster_2d_[vi][lyr];
                }
            }
            outflag = WriteSingleGeotiff(tmpfilename, headers_, options_, data_1d);
            if (!outflag) {
                Release1DArray(data_1d);;
                return false;
            }
        }
        Release1DArray(data_1d);
    } else {
        if (outputdirectly) {
            outflag = WriteSingleGeotiff(abs_filename, headers_, options_, raster_);
        } else {
            Initialize1DArray(n_rows * n_cols, data_1d, no_data_value_);
            for (int vi = 0; vi < n_cells_; vi++) {
                //data_1d[pos_data_[vi][0] * n_cols + pos_data_[vi][1]] = raster_[vi];
                data_1d[pos_idx_[vi]] = raster_[vi];
            }
            outflag = WriteSingleGeotiff(abs_filename, headers_, options_, data_1d);
            Release1DArray(data_1d);
        }
        if (!outflag) { return false; }
    }
    return true;
}
#endif /* USE_GDAL */

#ifdef USE_MONGODB
template <typename T, typename MASK_T>
bool clsRasterData<T, MASK_T>::OutputToMongoDB(MongoGridFs* gfs, const string& filename /* = string() */,
                                               const STRING_MAP& opts /* = STRING_MAP() */,
                                               bool include_nodata /* = true */,
                                               bool out_origin /* true */) {
    if (nullptr == gfs) { return false; }
    if (!out_origin) { // Output subset's data
        return OutputSubsetToMongoDB(gfs, filename, opts, include_nodata, false, true);
    }
    CopyStringMap(opts, options_); // Update metadata
    // Added by ljzhu, for compatible with yjwang's code. But, can this key-value be passed by the opts argument?
    UpdateStringMapIfNotExist(options_, HEADER_RS_PARAM_ABSTRACTION_TYPE, PARAM_ABSTRACTION_TYPE_PHYSICAL);
    if (options_.find(HEADER_RSOUT_DATATYPE) == options_.end()
        || StringMatch("Unknown", options_.at(HEADER_RSOUT_DATATYPE))) {
        UpdateStrHeader(options_, HEADER_RSOUT_DATATYPE,
                        RasterDataTypeToString(TypeToRasterDataType(typeid(T))));
    }
    // Check if we can output directly: 1) pos_data_ is not NULL and include_nodata is false;
    //                                  2) pos_data_ is NULL and include_nodata is true.
    bool outputdirectly = true; // output directly or create new full size array
    int cnt;
    int** pos = nullptr;
    if ((nullptr != pos_data_ || nullptr != pos_idx_) && include_nodata) {
        outputdirectly = false;
        GetRasterPositionData(&cnt, &pos);
    }
    if ((nullptr == pos_data_ || nullptr == pos_idx_) && !include_nodata) {
        SetCalcPositions();
        GetRasterPositionData(&cnt, &pos);
    }
    int n_rows = GetRows();
    int n_cols = GetCols();
    int n_fullsize = n_rows * n_cols;
    if (include_nodata) {
        UpdateStringMap(options_, HEADER_INC_NODATA, "TRUE");
    } else {
        UpdateStringMap(options_, HEADER_INC_NODATA, "FALSE");
    }
    // 2. Get raster data
    T* data_1d = nullptr;
    T no_data_value = GetNoDataValue();
    int datalength;
    string core_name = filename.empty() ? core_name_ : filename;
    if (is_2draster) { // 2.1 2D raster data
        if (outputdirectly) {
            data_1d = raster_2d_[0]; // refers to Initialize2DArray() for why we can do this assignment
            datalength = n_lyrs_ * n_cells_; // can be n_lyrs_ * (n_rows*n_cols or valid cells' number)
        } else {
            datalength = n_lyrs_ * n_fullsize;
            Initialize1DArray(datalength, data_1d, no_data_value);
            for (int idx = 0; idx < n_cells_; idx++) {
                int rowcol_index = pos[idx][0] * n_cols + pos[idx][1];
                for (int k = 0; k < n_lyrs_; k++) {
                    data_1d[n_lyrs_ * rowcol_index + k] = raster_2d_[idx][k];
                }
            }
        }
    } else { // 3.2 1D raster data
        if (outputdirectly) {
            data_1d = raster_;
            datalength = n_cells_; // can be n_rows*n_cols or valid cells' number
        } else {
            datalength = n_fullsize;
            Initialize1DArray(datalength, data_1d, no_data_value);
            for (int idx = 0; idx < n_cells_; idx++) {
                data_1d[pos[idx][0] * n_cols + pos[idx][1]] = raster_[idx];
            }
        }
    }
    STRDBL_MAP tmpheader;
    CopyHeader(headers_, tmpheader);
    if (include_nodata) { UpdateHeader(tmpheader, HEADER_RS_CELLSNUM, n_fullsize); }
    bool saved = WriteStreamDataAsGridfs(gfs, core_name, tmpheader,
                                         data_1d, datalength, options_);
    if (outputdirectly) {
        data_1d = nullptr;
    } else {
        Release1DArray(data_1d);
    }
    return saved;
}

template <typename T, typename MASK_T>
bool clsRasterData<T, MASK_T>::OutputSubsetToMongoDB(MongoGridFs* gfs,
                                                     const string& filename /* string() */,
                                                     const STRING_MAP& opts /* STRING_MAP() */,
                                                     bool include_nodata /* true */,
                                                     bool out_origin /* false */,
                                                     bool out_combined /* true */,
                                                     const map<vint, vector<double> >& recls /* map()*/,
                                                     double default_value /* = NODATA_VALUE */) {
    if (!ValidateRasterData()) { return false; }
    if (nullptr == gfs) { return false; }
    if (subset_.empty()) { return false; }
    CopyStringMap(opts, options_); // Update metadata
    // Added by ljzhu, for compatible with yjwang's code. But, can this key-value be passed by the opts argument?
    UpdateStringMapIfNotExist(options_, HEADER_RS_PARAM_ABSTRACTION_TYPE, PARAM_ABSTRACTION_TYPE_PHYSICAL);
    if (include_nodata) {
        UpdateStringMap(options_, HEADER_INC_NODATA, "TRUE");
    }
    else {
        UpdateStringMap(options_, HEADER_INC_NODATA, "FALSE");
    }
    if (options_.find(HEADER_RSOUT_DATATYPE) == options_.end()
        || StringMatch("Unknown", options_.at(HEADER_RSOUT_DATATYPE))) {
        UpdateStrHeader(options_, HEADER_RSOUT_DATATYPE,
                        RasterDataTypeToString(TypeToRasterDataType(typeid(T))));
    }


    int grows = GetRows();
    string outnameact = filename.empty() ? core_name_ : filename;
    bool out_comb = out_combined;
    if (out_comb) {
        T* data1d = nullptr;
        int sublyrs;
        int sublen;
        bool flag = PrepareCombSubsetData(&data1d, &sublen, &sublyrs,
                                          out_origin, include_nodata, recls, default_value);
        STRDBL_MAP tmpheader;
        CopyHeader(headers_, tmpheader);
        UpdateHeader(tmpheader, HEADER_RS_LAYERS, sublyrs);
        UpdateHeader(tmpheader, HEADER_RS_CELLSNUM, sublen / sublyrs);
        flag = flag && WriteStreamDataAsGridfs(gfs, "0_" + outnameact,
                                               tmpheader, data1d, sublen, options_);
        Release1DArray(data1d);
        return flag;
    }
    // output each subset
    STRDBL_MAP subheader;
    bool flag = true;
    for (auto it = subset_.begin(); it != subset_.end(); ++it) {
        if (!it->second->usable) { continue; }
        it->second->GetHeader(GetXllCenter(), GetYllCenter(), grows,
                              GetCellWidth(), CVT_DBL(no_data_value_), subheader);
        T* tmpdata1d = nullptr;
        int tmpdatalen;
        int tmplyrs;
        if (!PrepareSubsetData(it->first, it->second, &tmpdata1d, &tmpdatalen, &tmplyrs,
                               out_origin, include_nodata, recls, default_value)) {
            continue;
        }
        UpdateHeader(subheader, HEADER_RS_LAYERS, tmplyrs);
        UpdateHeader(subheader, HEADER_RS_CELLSNUM, tmpdatalen / tmplyrs);
        string tmpfname = itoa(CVT_VINT(it->first)) + "_" + outnameact;
        flag = flag && WriteStreamDataAsGridfs(gfs, tmpfname, subheader, tmpdata1d, tmpdatalen, options_);
        if (nullptr != tmpdata1d) { Release1DArray(tmpdata1d); }
    }
    return true;
}

#endif /* USE_MONGODB */

/************* Read functions ***************/
template <typename T, typename MASK_T>
bool clsRasterData<T, MASK_T>::ReadFromFile(const string& filename, const bool calc_pos /* = false */,
                                            clsRasterData<MASK_T>* mask /* = nullptr */,
                                            const bool use_mask_ext /* = true */,
                                            double default_value /* NODATA_VALUE */,
                                            const STRING_MAP& opts /* = STRING_MAP() */) {
    if (!FileExists(filename)) { return false; }
    if (!initialized_) { InitializeRasterClass(); }
    rs_type_out_ = RasterDataTypeInOptionals(opts);
    return ConstructFromSingleFile(filename, calc_pos, mask, use_mask_ext, default_value, opts);
}

template <typename T, typename MASK_T>
bool clsRasterData<T, MASK_T>::ReadFromFiles(vector<string>& filenames, const bool calc_pos /* = false */,
                                             clsRasterData<MASK_T>* mask /* = nullptr */,
                                             const bool use_mask_ext /* = true */,
                                             double default_value /* NODATA_VALUE */,
                                             const STRING_MAP& opts /* = STRING_MAP() */) {
    if (!FilesExist(filenames)) { return false; }
    n_lyrs_ = CVT_INT(filenames.size());
    if (n_lyrs_ == 1) { return ReadFromFile(filenames[0], calc_pos, mask, use_mask_ext, default_value, opts); }
    if (!initialized_) { InitializeRasterClass(true); }
    rs_type_out_ = RasterDataTypeInOptionals(opts);
    // 1. firstly, take the first layer as the main input, to calculate position index or extract by mask.
    if (!ConstructFromSingleFile(filenames[0], calc_pos, mask, use_mask_ext, default_value, opts)) {
        return false;
    }
    // 2. change corename and filepath template which format is: `<file dir>/CoreName_%d.<suffix>`
    //    support "corename.tif or corename_1.tif", "corename_2.tif", and "corename_3.tif", etc.
    string::size_type last_underline = core_name_.find_last_of('_');
    if (last_underline == string::npos) {
        last_underline = core_name_.length();
    }
    core_name_ = core_name_.substr(0, last_underline);
    full_path_ = GetPathFromFullName(filenames[0]) + core_name_ + "_%d." + GetSuffix(filenames[0]);
    // 3. initialize raster_2d_ and read the other layers according to position data if stated,
    //     or just read by row and col
    Initialize2DArray(n_cells_, n_lyrs_, raster_2d_, no_data_value_);
#pragma omp parallel for
    for (int i = 0; i < n_cells_; i++) {
        raster_2d_[i][0] = raster_[i];
    }
    Release1DArray(raster_);
    int rows = GetRows();
    int cols = GetCols();
    // take the first layer as mask, and use_mask_ext is true, and no need to calculate position data
    for (int fileidx = 1; fileidx < CVT_INT(filenames.size()); fileidx++) {
        STRDBL_MAP tmpheader;
        T* tmplyrdata = nullptr;
        string curfilename = filenames[fileidx];
        if (StringMatch(GetUpper(GetSuffix(curfilename)), string(ASCIIExtension))) {
            ReadAscFile(curfilename, tmpheader, tmplyrdata);
        }
        else {
#ifdef USE_GDAL
            RasterDataType tmpintype;
            string tmpsrs;
            ReadRasterFileByGdal(curfilename, tmpheader, tmplyrdata, tmpintype, tmpsrs);
#else
            StatusMessage("Warning: Only ASC format is supported without GDAL!");
            return false;
#endif
        }
        if (nullptr != pos_data_ || nullptr != pos_idx_) {
#pragma omp parallel for
            for (int i = 0; i < n_cells_; ++i) {
                //int tmp_row = pos_data_[i][0]; // row * ncols + col,
                //int tmp_col = pos_data_[i][1];
                int tmp_row = pos_idx_[i] / cols;
                int tmp_col = pos_idx_[i] % cols;
                AddOtherLayerRasterData(tmp_row, tmp_col, i, fileidx, tmpheader, tmplyrdata);
            }
        }
        else {
#pragma omp parallel for
            for (int i = 0; i < rows; i++) {
                for (int j = 0; j < cols; j++) {
                    int cellidx = i * cols + j;
                    if (FloatEqual(no_data_value_, raster_2d_[cellidx][0])) {
                        raster_2d_[cellidx][fileidx] = no_data_value_;
                        continue;
                    }
                    AddOtherLayerRasterData(i, j, cellidx, fileidx, tmpheader, tmplyrdata);
                }
            }
        }
        Release1DArray(tmplyrdata);
    }
    if(!is_2draster) is_2draster = true;
    UpdateHeader(headers_, HEADER_RS_LAYERS, n_lyrs_); // repair layers count in headers
    return true;
}

#ifdef USE_MONGODB

template <typename T, typename MASK_T>
bool clsRasterData<T, MASK_T>::ReadFromMongoDB(MongoGridFs* gfs,
                                               const string& filename,
                                               const bool calc_pos /* = false */,
                                               clsRasterData<MASK_T>* mask /* = nullptr */,
                                               const bool use_mask_ext /* = true */,
                                               double default_value /* NODATA_VALUE */,
                                               const STRING_MAP& opts /* = STRING_MAP() */) {
    InitializeReadFunction(filename, calc_pos, mask, use_mask_ext, default_value, opts);
    T* dbdata = nullptr;
    STRDBL_MAP header_dbl = InitialHeader();
    STRING_MAP header_str = InitialStrHeader();
    STRING_MAP opts_upd;
    CopyStringMap(opts, opts_upd);
    UpdateStringMapIfNotExist(opts_upd, HEADER_RS_PARAM_ABSTRACTION_TYPE, PARAM_ABSTRACTION_TYPE_PHYSICAL);
    bool is_conceptual = StringMatch(opts_upd.at(HEADER_RS_PARAM_ABSTRACTION_TYPE), PARAM_ABSTRACTION_TYPE_CONEPTUAL);
    if (opts.empty() || opts.count(HEADER_INC_NODATA) < 1) { // GFS file include nodata by default
        UpdateStringMap(opts_upd, HEADER_INC_NODATA, "TRUE");
    }
    //if(is_conceptual) {
    //    UpdateStringMap(opts_upd, HEADER_INC_NODATA, "FALSE");
    //}
    if (!ReadGridFsFile(gfs, filename, dbdata, header_dbl, header_str, opts_upd)) { return false; }

    if (headers_.at(HEADER_RS_NROWS) > 0 && headers_.at(HEADER_RS_NCOLS) > 0
        && headers_.at(HEADER_RS_LAYERS) > 0 && headers_.at(HEADER_RS_CELLSNUM) > 0) {
        // means the raster is preassigned header information, and this function is used for read data only
        if (!FloatEqual(headers_.at(HEADER_RS_NROWS), header_dbl.at(HEADER_RS_NROWS))
            || !FloatEqual(headers_.at(HEADER_RS_NCOLS), header_dbl.at(HEADER_RS_NCOLS))
            || !FloatEqual(headers_.at(HEADER_RS_LAYERS), header_dbl.at(HEADER_RS_LAYERS))
            || !FloatEqual(headers_.at(HEADER_RS_CELLSNUM), header_dbl.at(HEADER_RS_CELLSNUM))) {
            Release1DArray(dbdata);
            return false;
        }
    } else {
        CopyHeader(header_dbl, headers_);
    }
    CopyStringMap(header_str, options_);

    bool include_nodata = !StringMatch(options_.at(HEADER_INC_NODATA), "FALSE");

    int n_rows = CVT_INT(headers_.at(HEADER_RS_NROWS));
    int n_cols = CVT_INT(headers_.at(HEADER_RS_NCOLS));
    n_lyrs_ = CVT_INT(headers_.at(HEADER_RS_LAYERS));
    // if (n_rows < 0 || n_cols < 0 || n_lyrs_ < 0) { return false; } // Needless
    int fullsize = n_rows * n_cols;
    no_data_value_ = static_cast<T>(headers_.at(HEADER_RS_NODATA));
    n_cells_ = CVT_INT(headers_.at(HEADER_RS_CELLSNUM));

    if (include_nodata && n_cells_ != fullsize) { return false; }
    if (n_cells_ != fullsize) { calc_pos_ = true; }

    // check the valid values count and determine whether we can read directly.
    bool mask_pos_subset = true;
    if (nullptr != mask_ && calc_pos_ && use_mask_ext_ && n_cells_ == mask_->GetValidNumber()) {
        store_pos_ = false;
        mask_->GetRasterPositionData(&n_cells_, &pos_data_);
        mask_->GetRasterPositionData(&n_cells_, &pos_idx_);
        if (!mask->GetSubset().empty()) {
            map<int, SubsetPositions*>& mask_subset = mask_->GetSubset();
            for (auto it = mask_subset.begin(); it != mask_subset.end(); ++it) {
                SubsetPositions* tmp = new SubsetPositions(it->second, true);
                tmp->n_lyrs = n_lyrs_;
#ifdef HAS_VARIADIC_TEMPLATES
                subset_.emplace(it->first, tmp);
#else
                subset_.insert(make_pair(it->first, tmp));
#endif
            }
        }
        mask_pos_subset = false;
    }

    if (!include_nodata && (nullptr == pos_data_ || nullptr == pos_idx_)) { return false; }

    if (n_lyrs_ == 1) {
        is_2draster = false;
        if (include_nodata && !mask_pos_subset) {
            Initialize1DArray(n_cells_, raster_, no_data_value_);
#pragma omp parallel for
            for (int i = 0; i < n_cells_; i++) {
                //int tmpidx = pos_data_[i][0] * n_cols + pos_data_[i][1];
                int tmpidx = pos_idx_[i];
                raster_[i] = static_cast<T>(dbdata[tmpidx]);
            }
            Release1DArray(dbdata);
        } else {
            raster_ = dbdata;
        }
    } else {
        Initialize2DArray(n_cells_, n_lyrs_, raster_2d_, no_data_value_);
#pragma omp parallel for
        for (int i = 0; i < n_cells_; i++) {
            int tmpidx = i;
            if (include_nodata && !mask_pos_subset) {
                //tmpidx = pos_data_[i][0] * n_cols + pos_data_[i][1];
                tmpidx = pos_idx_[i];
            }
            for (int j = 0; j < n_lyrs_; j++) {
                int idx = tmpidx * n_lyrs_ + j;
                raster_2d_[i][j] = dbdata[idx];
            }
        }
        is_2draster = true;
        Release1DArray(dbdata);
    }
    CheckDefaultValue();
    if (include_nodata && mask_pos_subset) {
        return MaskAndCalculateValidPosition() >= 0;
    }
    return true;
}

#endif /* USE_MONGODB */

template <typename T, typename MASK_T>
void clsRasterData<T, MASK_T>::AddOtherLayerRasterData(const int row, const int col,
                                                       const int cellidx, const int lyr,
                                                       STRDBL_MAP lyrheader, T* lyrdata) {
    int tmpcols = CVT_INT(lyrheader.at(HEADER_RS_NCOLS));
    double tmpnodata = lyrheader.at(HEADER_RS_NODATA);
    XY_COOR tmp_xy = GetCoordinateByRowCol(row, col);
    // get current raster layer's value by XY
    ROW_COL tmp_pos = GetPositionByCoordinate(tmp_xy.first, tmp_xy.second, &lyrheader);
    if (tmp_pos.first == -1 || tmp_pos.second == -1) {
        raster_2d_[cellidx][lyr] = no_data_value_;
    } else {
        T tmpvalue = lyrdata[tmp_pos.first * tmpcols + tmp_pos.second];
        raster_2d_[cellidx][lyr] = FloatEqual(tmpvalue, tmpnodata) ? no_data_value_ : tmpvalue;
    }
}

template <typename T, typename MASK_T>
clsRasterData<T, MASK_T>::clsRasterData(clsRasterData<T, MASK_T>* another) {
    InitializeRasterClass();
    Copy(another);
}

template <typename T, typename MASK_T>
void clsRasterData<T, MASK_T>::Copy(clsRasterData<T, MASK_T>* orgraster) {
    // Release current data
    if (is_2draster && nullptr != raster_2d_ && n_cells_ > 0) {
        Release2DArray(raster_2d_);
    }
    if (!is_2draster && nullptr != raster_) {
        Release1DArray(raster_);
    }
    if (nullptr != pos_data_) {
        Release2DArray(pos_data_);
    }
    if (nullptr != pos_idx_) {
        Release1DArray(pos_idx_);
    }
    if (stats_calculated_) {
        ReleaseStatsMap2D();
        stats_calculated_ = false;
    }
    if (!subset_.empty()) {
        ReleaseSubset();
    }
    // Initialize now Raster and copy data
    InitializeReadFunction(orgraster->GetFilePath(), orgraster->PositionsCalculated(),
                           orgraster->GetMask(), orgraster->MaskExtented(),
                           orgraster->GetDefaultValue(), orgraster->GetOptions());
    n_cells_ = orgraster->GetCellNumber();
    n_lyrs_ = orgraster->GetLayers();
    rs_type_ = orgraster->GetDataType();
    rs_type_out_ = orgraster->GetOutDataType();
    no_data_value_ = orgraster->GetNoDataValue();
    if (orgraster->Is2DRaster()) {
        is_2draster = true;
        Initialize2DArray(n_cells_, n_lyrs_, raster_2d_,
                          orgraster->Get2DRasterDataPointer());
    } else {
        Initialize1DArray(n_cells_, raster_, orgraster->GetRasterDataPointer());
    }
    if (calc_pos_) {
        store_pos_ = true;
        Initialize2DArray(n_cells_, 2, pos_data_, orgraster->GetRasterPositionDataPointer());
        Initialize1DArray(n_cells_, pos_idx_, orgraster->GetRasterPositionIndexPointer());
    }
    stats_calculated_ = orgraster->StatisticsCalculated();
    if (stats_calculated_) {
        if (is_2draster) {
            map<string, double *> stats2D = orgraster->GetStatistics2D();
            for (auto iter = stats2D.begin(); iter != stats2D.end(); ++iter) {
                double* tmpstatvalues = nullptr;
                Initialize1DArray(n_lyrs_, tmpstatvalues, iter->second);
                stats_2d_.at(iter->first) = tmpstatvalues;
            }
        } else {
            STRDBL_MAP stats = orgraster->GetStatistics();
            for (auto iter = stats.begin(); iter != stats.end(); ++iter) {
                stats_[iter->first] = iter->second;
            }
        }
    }
    CopyHeader(orgraster->GetRasterHeader(), headers_);
    // deep copy subset
    if (!orgraster->GetSubset().empty()) {
        for (auto it = orgraster->GetSubset().begin(); it != orgraster->GetSubset().end(); ++it) {
            SubsetPositions* tmp = new SubsetPositions(it->second, true);
#ifdef HAS_VARIADIC_TEMPLATES
            subset_.emplace(it->first, tmp);
#else
            subset_.insert(make_pair(it->first, tmp));
#endif
        }
    }
}

template <typename T, typename MASK_T>
void clsRasterData<T, MASK_T>::ReplaceNoData(T replacedv) {
#pragma omp parallel for
    for (int i = 0; i < n_cells_; i++) {
        for (int lyr = 0; lyr < n_lyrs_; lyr++) {
            bool flag = is_2draster && nullptr != raster_2d_
                            ? FloatEqual(raster_2d_[i][lyr], no_data_value_)
                            : FloatEqual(raster_[i], no_data_value_);
            if (!flag) { continue; }
            if (is_2draster && nullptr != raster_2d_) {
                raster_2d_[i][lyr] = replacedv;
            } else if (nullptr != raster_) {
                raster_[i] = replacedv;
            }
        }
    }
    no_data_value_ = replacedv;
    default_value_ = CVT_DBL(replacedv);
    UpdateHeader(headers_, HEADER_RS_NODATA, replacedv);
}

template <typename T, typename MASK_T>
void clsRasterData<T, MASK_T>::Reclassify(const map<int, T> reclass_map) {
    map<int, T> recls;
    for(auto it = reclass_map.begin(); it != reclass_map.end(); ++it) {
#ifdef HAS_VARIADIC_TEMPLATES
        recls.emplace(it->first, it->second);
#else
        recls.insert(make_pair(it->first, it->second));
#endif
    }
    for (int i = 0; i < n_cells_; i++) {
        for (int lyr = 0; lyr < n_lyrs_; lyr++) {
            T curv = is_2draster && nullptr != raster_2d_
                         ? CVT_INT(raster_2d_[i][lyr])
                         : CVT_INT(raster_[i]);
            if (recls.count(curv) < 0) {
#ifdef HAS_VARIADIC_TEMPLATES
                recls.emplace(curv, no_data_value_);
#else
                recls.insert(make_pair(curv, no_data_value_));
#endif
            }
            if (is_2draster && nullptr != raster_2d_) {
                raster_2d_[i][lyr] = recls.at(curv);
            } else if (nullptr != raster_) {
                raster_[i] = recls.at(curv);
            }
        }
    }
}

/************* Utility functions ***************/

template <typename T, typename MASK_T>
XY_COOR clsRasterData<T, MASK_T>::GetCoordinateByRowCol(const int row, const int col) {
    return XY_COOR(GetXllCenter() + col * GetCellWidth(),
                   GetYllCenter() + (GetRows() - row - 1) * GetCellWidth());
}

template <typename T, typename MASK_T>
ROW_COL clsRasterData<T, MASK_T>::GetPositionByCoordinate(const double x, const double y,
                                                          STRDBL_MAP* header /* = nullptr */) {
    if (nullptr == header) { header = &headers_; }
    double xll_center = (*header).at(HEADER_RS_XLL);
    double yll_center = (*header).at(HEADER_RS_YLL);
    double dx = (*header).at(HEADER_RS_CELLSIZE);
    double dy = dx;
    int n_rows = CVT_INT((*header).at(HEADER_RS_NROWS));
    int n_cols = CVT_INT((*header).at(HEADER_RS_NCOLS));

    double x_min = xll_center - dx / 2.;
    double x_max = x_min + dx * n_cols;

    double y_min = yll_center - dy / 2.;
    double y_max = y_min + dy * n_rows;
    if ((x > x_max || x < xll_center) || (y > y_max || y < yll_center)) {
        return ROW_COL(-1, -1);
    }
    return ROW_COL(CVT_INT((y_max - y) / dy), CVT_INT((x - x_min) / dx));
}

template <typename T, typename MASK_T>
void clsRasterData<T, MASK_T>::CalculateValidPositionsFromGridData() {
    vector<T> values; // store 1st layer for both Rater1D and Raster2D
    vector<vector<T> > values_2d; // store layer 2~n
    vector<int> pos_rows;
    vector<int> pos_cols;
    int nrows = CVT_INT(headers_.at(HEADER_RS_NROWS));
    int ncols = CVT_INT(headers_.at(HEADER_RS_NCOLS));
    // get all valid values (i.e., exclude NODATA_VALUE)
    for (int i = 0; i < nrows; ++i) {
        for (int j = 0; j < ncols; ++j) {
            int idx = i * ncols + j;
            T tmp_value;
            if (is_2draster) {
                tmp_value = raster_2d_[idx][0];
            } else {
                tmp_value = raster_[idx];
            }
            if (FloatEqual(tmp_value, static_cast<T>(no_data_value_))) continue;
            values.emplace_back(tmp_value);
            if (is_2draster && n_lyrs_ > 1) {
                vector<T> tmpv(n_lyrs_ - 1);
                for (int lyr = 1; lyr < n_lyrs_; lyr++) {
                    tmpv[lyr - 1] = raster_2d_[idx][lyr];
                }
                values_2d.emplace_back(tmpv);
            }
            pos_rows.emplace_back(i);
            pos_cols.emplace_back(j);
        }
    }
    vector<T>(values).swap(values);
    if (is_2draster && n_lyrs_ > 1) {
        vector<vector<T> >(values_2d).swap(values_2d);
    }
    vector<int>(pos_rows).swap(pos_rows);
    vector<int>(pos_cols).swap(pos_cols);
    // reCreate raster data array
    n_cells_ = CVT_INT(values.size());
    UpdateHeader(headers_, HEADER_RS_CELLSNUM, n_cells_);
    if (is_2draster) {
        Release2DArray(raster_2d_);
        Initialize2DArray(n_cells_, n_lyrs_, raster_2d_, no_data_value_);
    } else {
        Release1DArray(raster_);
        Initialize1DArray(n_cells_, raster_, no_data_value_);
    }
    // pos_data_ is nullptr till now.
    Initialize2DArray(n_cells_, 2, pos_data_, 0);
    Initialize1DArray(n_cells_, pos_idx_, 0);
    store_pos_ = true;
#pragma omp parallel for
    for (int i = 0; i < n_cells_; ++i) {
        if (is_2draster) {
            raster_2d_[i][0] = values.at(i);
            if (n_lyrs_ > 1) {
                for (int lyr = 1; lyr < n_lyrs_; lyr++) {
                    raster_2d_[i][lyr] = values_2d[i][lyr - 1];
                }
            }
        } else {
            raster_[i] = values.at(i);
        }
        pos_data_[i][0] = pos_rows.at(i);
        pos_data_[i][1] = pos_cols.at(i);
        pos_idx_[i] = pos_rows.at(i) * ncols + pos_cols.at(i);
    }
    calc_pos_ = true;
}

template <typename T, typename MASK_T>
int clsRasterData<T, MASK_T>::MaskAndCalculateValidPosition() {
    int old_fullsize = GetRows() * GetCols();
    if (nullptr == mask_) {
        if (calc_pos_) {
            if (nullptr == pos_data_ || nullptr == pos_idx_) {
                CalculateValidPositionsFromGridData();
                return 1;
            }
            return 0;
        }
        n_cells_ = old_fullsize;
        UpdateHeader(headers_, HEADER_RS_CELLSNUM, n_cells_);
        // do nothing
        return 0;
    }
    // Use mask data
    // 1. Get new values, positions, and subsets (if exist) according to Mask's position data
    int mask_ncells;
    int** valid_pos = nullptr;
    int mask_rows = mask_->GetRows();
    int mask_cols = mask_->GetCols();
    // Get the position data from mask
    if (!mask_->PositionsCalculated()) { mask_->SetCalcPositions(); }
    mask_->GetRasterPositionData(&mask_ncells, &valid_pos);
    // Masked raster data have the same size with mask's valid positions
    vector<T> values(mask_ncells);             // store layer 1 data
    vector<vector<T> > values_2d(mask_ncells); // store layer 2~n data (excluding the first layer)
    vector<int> pos_rows(mask_ncells); // position rows in mask
    vector<int> pos_cols(mask_ncells); // position cols in mask
    // calculate the intersection extent between mask and the raster data
    int max_row = -1;
    int min_row = mask_rows;
    int max_col = -1;
    int min_col = mask_cols;
    int masked_count = 0; // position matched count
    int matched_count = 0; // valid value matched count
    // Get the valid data according to coordinate
    for (int i = 0; i < mask_ncells; i++) {
        int tmp_row = valid_pos[i][0];
        int tmp_col = valid_pos[i][1];
        XY_COOR tmp_xy = mask_->GetCoordinateByRowCol(tmp_row, tmp_col);
        ROW_COL tmp_pos = GetPositionByCoordinate(tmp_xy.first, tmp_xy.second);
        T tmp_value;
        if (tmp_pos.first == -1 || tmp_pos.second == -1) {
            tmp_value = no_data_value_; // location exceeds the extent of raster data
            if (is_2draster && n_lyrs_ > 1) {
                vector<T> tmp_values(n_lyrs_ - 1);
                for (int lyr = 1; lyr < n_lyrs_; lyr++) { tmp_values[lyr - 1] = no_data_value_; }
                values_2d[i] = tmp_values;
            }
            values[i] = tmp_value;
            pos_rows[i] = tmp_row;
            pos_cols[i] = tmp_col;
            continue;
        }
        tmp_value = GetValue(tmp_pos.first, tmp_pos.second, 1);
        if (FloatEqual(tmp_value, no_data_value_)) {
            if (!FloatEqual(default_value_, no_data_value_)) {
                tmp_value = static_cast<T>(default_value_);
                SetValue(tmp_pos.first, tmp_pos.second, tmp_value);
            }
        } else { // the intersect extents dependent on the valid raster values
            matched_count++;
            if (max_row < tmp_row) max_row = tmp_row;
            if (min_row > tmp_row) min_row = tmp_row;
            if (max_col < tmp_col) max_col = tmp_col;
            if (min_col > tmp_col) min_col = tmp_col;
        }
        if (is_2draster && n_lyrs_ > 1) {
            vector<T> tmp_values(n_lyrs_ - 1);
            for (int lyr = 1; lyr < n_lyrs_; lyr++) {
                tmp_values[lyr - 1] = GetValue(tmp_pos.first, tmp_pos.second, lyr + 1);
                if (FloatEqual(tmp_values[lyr - 1], no_data_value_)
                    && !FloatEqual(default_value_, no_data_value_)) {
                    tmp_values[lyr - 1] = static_cast<T>(default_value_);
                    SetValue(tmp_pos.first, tmp_pos.second, tmp_values[lyr - 1], lyr - 1);
                }
            }
            values_2d[i] = tmp_values;
        }
        values[i] = tmp_value;
        pos_rows[i] = tmp_row;
        pos_cols[i] = tmp_col;
        masked_count++;
    }
    if (masked_count == 0) { return -1; }
    n_cells_ = masked_count;

    // Priority Copy header of mask data, and update NoData, SRS, and Layers' count
    CopyHeader(mask_->GetRasterHeader(), headers_);
    UpdateHeader(headers_, HEADER_RS_NODATA, no_data_value_);
    UpdateStrHeader(options_, HEADER_RS_SRS, mask_->GetSrsString());
    UpdateHeader(headers_, HEADER_RS_LAYERS, n_lyrs_);

    // Priority DEEP Copy subset of mask data
    map<int, SubsetPositions*>& mask_subset = mask_->GetSubset();
    bool mask_has_subset = !mask_subset.empty(); // if mask data has subsets
    if (mask_has_subset) {
        ReleaseSubset();
        for (auto it = mask_subset.begin(); it != mask_subset.end(); ++it) {
            SubsetPositions* tmp = new SubsetPositions(it->second, true);
            tmp->n_lyrs = n_lyrs_;
#ifdef HAS_VARIADIC_TEMPLATES
            subset_.emplace(it->first, tmp);
#else
            subset_.insert(make_pair(it->first, tmp));
#endif
        }
    }

    // Set several flags
    // Flag 1: If masked cells' count equals valid positions' count of the mask layer
    bool match_exactly = matched_count == mask_ncells;
    // Flag2 : Is the valid grid extent same as the mask data, which means the mask
    //         is within the extent of the raster data
    bool within_ext = true;
    int new_rows = max_row - min_row + 1;
    int new_cols = max_col - min_col + 1;
    if (new_rows != mask_rows || new_cols != mask_cols) {
        within_ext = false;
    } else { // within_ext is true means
        // masked cells' count equals valid positions' count of the mask layer
        assert(masked_count == mask_ncells);
    }
    bool upd_header_rowcol = false; // need to update row and col counts in header info?
    bool recalc_pos = false; // need to recalculate valid positions? If true, set mask to nullptr!
    bool upd_header_valid_num = false; // need to update valid cell count in header info?
    bool store_fullsize = false; // need to store full size raster data after masked?
    bool recalc_subset = false; // need to recalculate subset? TODO, need more unittests

    // Although this if-then may redundancy, I think this still necessary for clearly thinking.
    if (within_ext) {
        upd_header_rowcol = false;
        if (use_mask_ext_) { // use_mask_ext_ has the highest priority
            if (calc_pos_) {
                recalc_pos = !match_exactly;
                upd_header_valid_num = recalc_pos;
                store_fullsize = false;
                //recalc_subset = mask_has_subset && !match_exactly;
            } else { // calc_pos_ = false
                recalc_pos = false;
                upd_header_valid_num = false;
                store_fullsize = false;
                calc_pos_ = true; // use_mask_ext_ is priority
                //recalc_subset = mask_has_subset && !match_exactly;
            }
        } else { // use_mask_ext_ is false
            if (calc_pos_) {
                recalc_pos = !match_exactly;
                upd_header_valid_num = recalc_pos;
                store_fullsize = false;
                //recalc_subset = mask_has_subset && !match_exactly;
            } else { // calc_pos_ = false
                recalc_pos = false;
                upd_header_valid_num = true;
                store_fullsize = false;
                calc_pos_ = true; // use_mask_ext_ is priority
                //recalc_subset = mask_has_subset && !match_exactly;
            }
        }
    } else { // within_ext is false
        if (use_mask_ext_) { // use_mask_ext_ has the highest priority
            upd_header_rowcol = false;
            if (calc_pos_) {
                recalc_pos = !match_exactly;
                upd_header_valid_num = recalc_pos;
                store_fullsize = false;
                //recalc_subset = mask_has_subset && !match_exactly;
            } else { // calc_pos_ = false
                recalc_pos = false;
                upd_header_valid_num = false;
                store_fullsize = false;
                calc_pos_ = true; // use_mask_ext_ is priority
                //recalc_subset = mask_has_subset && !match_exactly;
            }
        } else { // use_mask_ext_ is false
            upd_header_rowcol = true;
            upd_header_valid_num = true;
            if (calc_pos_) {
                recalc_pos = true;
                store_fullsize = false;
                //recalc_subset = mask_has_subset && !match_exactly;
            } else { // calc_pos_ = false
                recalc_pos = false;
                store_fullsize = true;
                //recalc_subset = mask_has_subset && !match_exactly;
            }
        }
    }
    if (mask_has_subset && !match_exactly && upd_header_rowcol) {
        recalc_subset = true;
    }
    // Update row and col counts in header information firstly.
    if (upd_header_rowcol) {
        UpdateHeader(headers_, HEADER_RS_NROWS, new_rows);
        UpdateHeader(headers_, HEADER_RS_NCOLS, new_cols);
        headers_.at(HEADER_RS_XLL) += min_col * mask_->GetCellWidth();
        headers_.at(HEADER_RS_YLL) += (mask_rows - max_row - 1) * mask_->GetCellWidth();
        headers_.at(HEADER_RS_CELLSIZE) = mask_->GetCellWidth();
    }

    // ReCalculate valid position
    if (recalc_pos) {
        // clean redundant values (i.e., NODATA)
        auto rit = pos_rows.begin();
        auto cit = pos_cols.begin();
        auto vit = values.begin();
        auto data2dit = values_2d.begin();
        for (auto it = values.begin(); it != values.end();) {
            size_t idx = distance(vit, it);
            int tmpr = pos_rows.at(idx);
            int tmpc = pos_cols.at(idx);
            if (tmpr > max_row || tmpr < min_row || tmpc > max_col || tmpc < min_col
                || FloatEqual(static_cast<T>(*it), no_data_value_)) {
                it = values.erase(it);
                if (is_2draster && n_lyrs_ > 1) {
                    values_2d.erase(data2dit + idx);
                    data2dit = values_2d.begin();
                }
                pos_cols.erase(cit + idx);
                pos_rows.erase(rit + idx);
                rit = pos_rows.begin(); // reset the iterators
                cit = pos_cols.begin();
                vit = values.begin();
            } else {
                pos_rows[idx] -= min_row; // get new column and row number
                pos_cols[idx] -= min_col;
                ++it;
            }
        }
        vector<T>(values).swap(values);
        if (is_2draster && n_lyrs_ > 1) { vector<vector<T> >(values_2d).swap(values_2d); }
        vector<int>(pos_rows).swap(pos_rows);
        vector<int>(pos_cols).swap(pos_cols);

        store_pos_ = true;
        n_cells_ = CVT_INT(values.size());
        if (nullptr != pos_data_) { Release2DArray(pos_data_); }
        if (nullptr != pos_idx_) { Release1DArray(pos_idx_); }
        Initialize2DArray(n_cells_, 2, pos_data_, 0);
        Initialize1DArray(n_cells_, pos_idx_, 0);
        for (size_t k = 0; k < pos_rows.size(); ++k) {
            pos_data_[k][0] = pos_rows.at(k);
            pos_data_[k][1] = pos_cols.at(k);
            if (upd_header_rowcol) {
                pos_idx_[k] = pos_rows.at(k) * new_cols + pos_cols.at(k);
            } else {
                pos_idx_[k] = pos_rows.at(k) * mask_cols + pos_cols.at(k);
            }
        }
    } else {
        if (nullptr != pos_data_) { Release2DArray(pos_data_); }
        if (nullptr != pos_idx_) { Release1DArray(pos_idx_); }
        if (calc_pos_) {
            mask_->GetRasterPositionData(&n_cells_, &pos_data_);
            mask_->GetRasterPositionData(&n_cells_, &pos_idx_);
        }
        store_pos_ = false;
    }

    // Release the original raster values, and create new
    //     raster array and positions data array (if necessary)
    assert(ValidateRasterData());
    int ncols = CVT_INT(headers_.at(HEADER_RS_NCOLS));
    int nrows = CVT_INT(headers_.at(HEADER_RS_NROWS));
    if (store_fullsize) {
        n_cells_ = ncols * nrows;
    }
    bool release_origin = true;
    if (store_fullsize && old_fullsize == n_cells_) {
        release_origin = false;
    }
    if (release_origin) {
        if (is_2draster && nullptr != raster_2d_) { // multiple layers
            Release2DArray(raster_2d_);
            Initialize2DArray(n_cells_, n_lyrs_, raster_2d_, no_data_value_);
        } else { // single layer
            Release1DArray(raster_);
            Initialize1DArray(n_cells_, raster_, no_data_value_);
        }
        // Loop the masked raster values
        size_t synthesis_idx = 0;
        for (size_t k = 0; k < pos_rows.size(); ++k) {
            synthesis_idx = k;
            int tmpr = pos_rows.at(k);
            int tmpc = pos_cols.at(k);
            if (store_fullsize) {
                if (tmpr > max_row || tmpr < min_row || tmpc > max_col || tmpc < min_col) {
                    continue;
                }
                synthesis_idx = (tmpr - min_row) * ncols + tmpc - min_col;
                if (synthesis_idx > n_cells_ - 1) {
                    continue; // error may occurred!
                }
            }
            if (is_2draster) { // multiple layers
                raster_2d_[synthesis_idx][0] = values.at(k);
                if (n_lyrs_ > 1) {
                    for (int lyr = 1; lyr < n_lyrs_; lyr++) {
                        raster_2d_[synthesis_idx][lyr] = values_2d[k][lyr - 1];
                    }
                }
            } else { // single layer
                raster_[synthesis_idx] = values.at(k);
            }
        }
    }

    if (upd_header_valid_num) {
        UpdateHeader(headers_, HEADER_RS_CELLSNUM, n_cells_);
    }

    if (mask_has_subset) { // check former assigned mask's subset
        for (auto it = subset_.begin(); it != subset_.end();) {
            int count = 0;
            int srow = nrows;
            int erow = -1;
            int scol = ncols;
            int ecol = -1;
            vector<int> globalpos;
            for (int i = 0; i < it->second->n_cells; i++) {
                int gi = it->second->global_[i];
                int tmprow = valid_pos[gi][0];
                int tmpcol = valid_pos[gi][1];
                XY_COOR tmpxy = mask_->GetCoordinateByRowCol(tmprow, tmpcol);
                if (GetPosition(tmpxy.first, tmpxy.second) < 0) { continue; }
                ROW_COL tmppos = GetPositionByCoordinate(tmpxy.first, tmpxy.second);
                if (IsNoData(tmppos.first, tmppos.second)) { continue; }
                if (tmppos.first < srow) { srow = tmppos.first; }
                if (tmppos.first > erow) { erow = tmppos.first; }
                if (tmppos.second < scol) { scol = tmppos.second; }
                if (tmppos.second > ecol) { ecol = tmppos.second; }
                globalpos.emplace_back(GetPosition(tmppos.first, tmppos.second));
                count++;
            }
            if (count == 0) {
                delete it->second;
                subset_.erase(it++);
                continue;
            }
            if (recalc_subset) {
                it->second->g_srow = srow;
                it->second->g_erow = erow;
                it->second->g_scol = scol;
                it->second->g_ecol = ecol;
                it->second->n_cells = count;
                it->second->n_lyrs = n_lyrs_;
                int local_ncols = ecol - scol + 1;
                if (it->second->alloc_) {
                    Release1DArray(it->second->global_);
                    Release2DArray(it->second->local_pos_);
                    Release1DArray(it->second->local_posidx_);
                }
                it->second->global_ = nullptr; // not affect mask's subset
                it->second->local_pos_ = nullptr;
                it->second->local_posidx_ = nullptr;
                it->second->alloc_ = true;
                Initialize1DArray(count, it->second->global_, -1);
                Initialize2DArray(count, 2, it->second->local_pos_, -1);
                Initialize1DArray(count, it->second->local_posidx_, -1);
                for (int ii = 0; ii < count; ii++) {
                    it->second->global_[ii] = globalpos[ii];
                    int local_row = -1;
                    int local_col = -1;
                    if (nullptr == pos_data_ || nullptr == pos_idx_) {
                        local_row = globalpos[ii] / ncols - it->second->g_srow;
                        local_col = globalpos[ii] % ncols - it->second->g_scol;
                    } else {
                        //local_row = pos_data_[globalpos[ii]][0] - it->second->g_srow;
                        //local_col = pos_data_[globalpos[ii]][1] - it->second->g_scol;
                        local_row = pos_idx_[globalpos[ii]] / ncols - it->second->g_srow;
                        local_col = pos_idx_[globalpos[ii]] % ncols - it->second->g_scol;
                    }
                    it->second->local_pos_[ii][0] = local_row;
                    it->second->local_pos_[ii][1] = local_col;
                    it->second->local_posidx_[ii] = local_row * local_ncols + local_col;
                }
                it->second->data_ = nullptr;
                it->second->data2d_ = nullptr;
                it->second->alloc_ = true;
            }
            ++it;
        }
    }
    if (store_fullsize || recalc_pos) {
        mask_ = nullptr;
    }
    return 2; // all situations that use mask data
}

} // namespace data_raster
} // namespace ccgl
#endif /* CCGL_DATA_RASTER_H */
