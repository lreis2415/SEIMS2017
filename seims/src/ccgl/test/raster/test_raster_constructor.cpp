/*!
 * \brief Test clsRasterData of blank constructor to make sure no exception thrown.
 *
 * \version 1.4
 * \authors Liangjun Zhu, zlj(at)lreis.ac.cn; crazyzlj(at)gmail.com
 * \remarks 2017-12-02 - lj - Original version.
 *          2018-05-03 - lj - Integrated into CCGL.
 *          2021-07-20 - lj - Update after changes of GetValue and GetValueByIndex.
 *          2021-11-27 - lj - Add more tests.
 *          2023-04-13 - lj - Update tests according to API changes of clsRasterData
 *
 */
#include "gtest/gtest.h"

#include "../../src/data_raster.hpp"
#include "../../src/utils_filesystem.h"
#include "../../src/utils_array.h"
#ifdef USE_MONGODB
#include "../../src/db_mongoc.h"
#endif
#include "../test_global.h"

using namespace ccgl;
using namespace ccgl::data_raster;
using namespace ccgl::utils_array;
using namespace ccgl::utils_string;
using namespace ccgl::utils_filesystem;
#ifdef USE_MONGODB
using namespace ccgl::db_mongoc;
#endif

extern GlobalEnvironment* GlobalEnv;


namespace {
string datapath = GetAppPath() + "./data/raster/";
string dstpath = datapath + "result/";

string not_existed_rs = datapath + "not_existed_rs.tif";
string not_std_asc = datapath + "tinydemo_not-std-asc_r2c2.asc";
string miss_std_asc = datapath + "tinydemo_miss-std-asc_r2c2.asc";

string rs_mask = datapath + "mask_byte_r3c2.tif";
string rs_byte = datapath + "byte_r3c3.tif";
string rs_byte_signed = datapath + "byte_signed_r3c3.tif";
string rs_byte_signed_noneg = datapath + "byte_signed_no-negative_r3c3.tif";
string rs_uint16 = datapath + "uint16_r3c3.tif";
string rs_int16 = datapath + "int16_r3c3.tif";
string rs_uint32 = datapath + "uint32_r3c3.tif";
string rs_int32 = datapath + "int32_r3c3.tif";
string rs_float = datapath + "float32_r3c3.tif";
string rs_double = datapath + "float64_r3c3.tif";

string rs_int32_nodefnodata = datapath + "int32.tif";

TEST(clsRasterDataTestBlankCtor, ValidateAccess) {
    /// 0. Create an clsRasterData instance with blank ctor
    clsRasterData<float, int>* rs = new clsRasterData<float, int>();
    /// 1. Test members after constructing.
    EXPECT_EQ(-1, rs->GetDataLength()); // m_nCells
    EXPECT_EQ(-1, rs->GetCellNumber()); // m_nCells

    float nodata_value = -1 * FLT_MAX; // the default nodata value depends on the data type T
    EXPECT_EQ(nodata_value, rs->GetNoDataValue());  // m_noDataValue
    EXPECT_EQ(-9999., rs->GetDefaultValue()); // m_defaultValue

    EXPECT_EQ("", rs->GetFilePath()); // m_filePathName
    EXPECT_EQ("", rs->GetCoreName()); // m_coreFileName

    EXPECT_TRUE(rs->Initialized());           // m_initialized
    EXPECT_FALSE(rs->Is2DRaster());           // m_is2DRaster
    EXPECT_FALSE(rs->PositionsCalculated());  // m_calcPositions
    EXPECT_FALSE(rs->PositionsAllocated());   // m_storePositions
    EXPECT_FALSE(rs->MaskExtented());         // m_useMaskExtent
    EXPECT_FALSE(rs->StatisticsCalculated()); // m_statisticsCalculated

    EXPECT_FALSE(rs->ValidateRasterData());

    EXPECT_EQ(nullptr, rs->GetRasterDataPointer());         // m_rasterData
    EXPECT_EQ(nullptr, rs->Get2DRasterDataPointer());       // m_raster2DData
    EXPECT_EQ(nullptr, rs->GetRasterPositionDataPointer()); // m_rasterPositionData
    EXPECT_EQ(nullptr, rs->GetRasterPositionIndexPointer()); // m_rasterPositionIndex

    /** Get metadata, m_headers **/
    EXPECT_EQ(-9999, rs->GetRows());
    EXPECT_EQ(-9999, rs->GetCols());
    EXPECT_DOUBLE_EQ(-9999., rs->GetXllCenter());
    EXPECT_DOUBLE_EQ(-9999., rs->GetYllCenter());
    EXPECT_DOUBLE_EQ(-9999., rs->GetCellWidth());
    EXPECT_EQ(-1, rs->GetLayers());
    EXPECT_EQ("", rs->GetSrsString());

    /** Calc and get basic statistics, m_statsMap **/
    EXPECT_EQ(-9999, rs->GetValidNumber());
    EXPECT_DOUBLE_EQ(-9999., rs->GetMinimum());
    EXPECT_DOUBLE_EQ(-9999., rs->GetMaximum());
    EXPECT_DOUBLE_EQ(-9999., rs->GetAverage());
    EXPECT_DOUBLE_EQ(-9999., rs->GetStd());
    EXPECT_DOUBLE_EQ(-9999., rs->GetRange());
    EXPECT_FALSE(rs->StatisticsCalculated());

    EXPECT_EQ(nullptr, rs->GetMask()); // m_mask

    /** Test getting position data **/
    int ncells = -1;
    int** positions = nullptr;
    rs->GetRasterPositionData(&ncells, &positions); // m_rasterPositionData
    EXPECT_EQ(-1, ncells);
    EXPECT_EQ(nullptr, positions);

    /** Test getting raster data **/
    float* rs_data = nullptr;
    EXPECT_FALSE(rs->GetRasterData(&ncells, &rs_data)); // m_rasterData
    EXPECT_EQ(-1, ncells);
    EXPECT_EQ(nullptr, rs_data);

    float** rs_2ddata = nullptr;
    int nlyrs = -1;
    EXPECT_FALSE(rs->Get2DRasterData(&ncells, &nlyrs, &rs_2ddata)); // m_raster2DData
    EXPECT_EQ(-1, ncells);
    EXPECT_EQ(-1, nlyrs);
    EXPECT_EQ(nullptr, rs_2ddata);

    /** Get raster cell value by various way **/
    EXPECT_FLOAT_EQ(nodata_value, rs->GetValueByIndex(-1));
    EXPECT_FLOAT_EQ(nodata_value, rs->GetValueByIndex(0));
    EXPECT_FLOAT_EQ(nodata_value, rs->GetValueByIndex(540, 1));
    EXPECT_FLOAT_EQ(nodata_value, rs->GetValueByIndex(541, 1));
    EXPECT_FLOAT_EQ(nodata_value, rs->GetValueByIndex(29));
    EXPECT_FLOAT_EQ(nodata_value, rs->GetValueByIndex(29, 0));
    EXPECT_FLOAT_EQ(nodata_value, rs->GetValueByIndex(-1, 2));
    EXPECT_FLOAT_EQ(nodata_value, rs->GetValueByIndex(541, 2));

    int tmp_lyr = rs->GetLayers();
    float* tmp_values = nullptr;
    Initialize1DArray(tmp_lyr, tmp_values, -9999.f);
    rs->GetValueByIndex(-1, tmp_values);
    EXPECT_EQ(nullptr, tmp_values);
    rs->GetValueByIndex(0, tmp_values);
    EXPECT_EQ(nullptr, tmp_values);

    EXPECT_FLOAT_EQ(nodata_value, rs->GetValue(-1, 0));
    EXPECT_FLOAT_EQ(nodata_value, rs->GetValue(20, 0));
    EXPECT_FLOAT_EQ(nodata_value, rs->GetValue(0, -1));
    EXPECT_FLOAT_EQ(nodata_value, rs->GetValue(0, 30));
    EXPECT_FLOAT_EQ(nodata_value, rs->GetValue(2, 4, -1));
    EXPECT_FLOAT_EQ(nodata_value, rs->GetValue(2, 4, 2));
    EXPECT_FLOAT_EQ(nodata_value, rs->GetValue(2, 4));
    EXPECT_FLOAT_EQ(nodata_value, rs->GetValue(2, 4, 1));

    rs->GetValue(-1, 0, tmp_values);
    EXPECT_EQ(nullptr, tmp_values);
    rs->GetValue(0, -1, tmp_values);
    EXPECT_EQ(nullptr, tmp_values);
    rs->GetValue(0, 0, tmp_values);
    EXPECT_EQ(nullptr, tmp_values);
    rs->GetValue(0, 1, tmp_values);
    EXPECT_EQ(nullptr, tmp_values);

    Release1DArray(tmp_values);

    // Get position
    EXPECT_EQ(-2, rs->GetPosition(4.05f, 37.95f));
    EXPECT_EQ(-2, rs->GetPosition(5.95f, 36.05f));

    /** Get subset **/
    map<int, SubsetPositions*> subset =  rs->GetSubset();
    EXPECT_TRUE(subset.empty());

    /** Set value **/

    // Set raster data value
    rs->SetValue(2, 4, 18.06f);
    EXPECT_FLOAT_EQ(nodata_value, rs->GetValue(2, 4));
    rs->SetValue(0, 0, 1.f);
    EXPECT_FLOAT_EQ(nodata_value, rs->GetValue(0, 0));

    /** Output to new file **/
    string newfullname = GetAppPath() + SEP + "no_output.tif";
    EXPECT_FALSE(rs->OutputToFile(newfullname));

    delete rs;
}

TEST(clsRasterDataASCConstructor, SupportedCases) {
    IntRaster* not_std_rs = IntRaster::Init(not_std_asc);
    EXPECT_NE(nullptr, not_std_rs);
    if (HasFailure()) { return; }
    EXPECT_EQ(4, not_std_rs->GetCellNumber());
    EXPECT_EQ(2, not_std_rs->GetValidNumber());
    delete not_std_rs;
}

TEST(clsRasterDataFailedConstructor, FailedCases) {
    FltIntRaster* noexisted_rs = FltIntRaster::Init(not_existed_rs);
    EXPECT_EQ(nullptr, noexisted_rs);

    IntRaster* miss_std_rs = IntRaster::Init(miss_std_asc);
    EXPECT_EQ(nullptr, miss_std_rs);

    vector<string> files;
    files.push_back(not_existed_rs);
    files.push_back(not_std_asc);
    noexisted_rs = FltIntRaster::Init(files);
    EXPECT_EQ(nullptr, noexisted_rs);

    delete noexisted_rs;
}

#ifdef USE_GDAL
TEST(clsRasterDataUnsignedByte, FullIO) {
    clsRasterData<vuint8_t>* mask_rs = clsRasterData<vuint8_t>::Init(rs_mask);
    EXPECT_NE(mask_rs, nullptr);
    clsRasterData<vuint8_t>* rs = clsRasterData<vuint8_t>::Init(rs_byte, false,
                                                                mask_rs, true);

    EXPECT_TRUE(rs->GetDataType() == RDT_UInt8);
    EXPECT_TRUE(rs->GetOutDataType() == RDT_UInt8);
    int ncells = -1;
    vuint8_t* data = nullptr;
    EXPECT_TRUE(rs->GetRasterData(&ncells, &data));
    EXPECT_TRUE(ncells > 0);
    EXPECT_NE(data, nullptr);
    string newcorename = GetCoreFileName(rs_byte) + "_masked";
    string rs_out1 = dstpath + newcorename + ".tif";
    EXPECT_TRUE(rs->OutputToFile(rs_out1));
    EXPECT_TRUE(FileExists(rs_out1));

#ifdef USE_MONGODB
    EXPECT_TRUE(rs->OutputToMongoDB(GlobalEnv->gfs_, newcorename, STRING_MAP(), false)); // Save valid data
    STRING_MAP opts;
    UpdateStringMap(opts, HEADER_INC_NODATA, "FALSE");
    clsRasterData<vuint8_t>* rs_mongo = clsRasterData<vuint8_t>::Init(GlobalEnv->gfs_, newcorename.c_str(),
                                                                      false, mask_rs, true,
                                                                      NODATA_VALUE, opts);
    EXPECT_NE(rs_mongo, nullptr);
    if (HasFailure()) { return; }

    for (int i = 0; i < ncells; i++) {
        EXPECT_EQ(data[i], rs_mongo->GetValueByIndex(i));
    }
    delete rs_mongo;
#endif

    delete mask_rs;
    delete rs;
}

TEST(clsRasterDatasignedByte, FullIO) {
    clsRasterData<vint8_t>* mask_rs = clsRasterData<vint8_t>::Init(rs_mask);
    clsRasterData<vint8_t>* rs = clsRasterData<vint8_t>::Init(rs_byte_signed, false,
                                                              mask_rs, true);

    EXPECT_TRUE(rs->GetDataType() == RDT_Int8);
    EXPECT_TRUE(rs->GetOutDataType() == RDT_Int8);
    int ncells = -1;
    vint8_t* data = nullptr;
    EXPECT_TRUE(rs->GetRasterData(&ncells, &data));
    EXPECT_TRUE(ncells > 0);
    EXPECT_NE(data, nullptr);
    string newcorename = GetCoreFileName(rs_byte_signed) + "_masked";
    string rs_out = dstpath + newcorename + ".tif";
    EXPECT_TRUE(rs->OutputToFile(rs_out));
    EXPECT_TRUE(FileExists(rs_out));

#ifdef USE_MONGODB
    EXPECT_TRUE(rs->OutputToMongoDB(GlobalEnv->gfs_, newcorename, STRING_MAP(), false)); // Save valid data
    STRING_MAP opts;
    UpdateStringMap(opts, HEADER_INC_NODATA, "FALSE");
    clsRasterData<vint8_t>* rs_mongo = clsRasterData<vint8_t>::Init(GlobalEnv->gfs_, newcorename.c_str(),
                                                                    false, mask_rs, true,
                                                                    NODATA_VALUE, opts);
    EXPECT_NE(rs_mongo, nullptr);
    if (HasFailure()) { return; }
    for (int i = 0; i < ncells; i++) {
        EXPECT_EQ(data[i], rs_mongo->GetValueByIndex(i));
    }
    delete rs_mongo;
#endif
    delete mask_rs;
    delete rs;
}

TEST(clsRasterDatasignedByteNoNegative, FullIO) {
    clsRasterData<vint8_t>* mask_rs = clsRasterData<vint8_t>::Init(rs_mask);
    clsRasterData<vint8_t>* rs = clsRasterData<vint8_t>::Init(rs_byte_signed_noneg, false,
                                                              mask_rs, true);
    EXPECT_TRUE(rs->GetDataType() == RDT_Int8);
    EXPECT_TRUE(rs->GetOutDataType() == RDT_Int8);
    int ncells = -1;
    vint8_t* data = nullptr;
    EXPECT_TRUE(rs->GetRasterData(&ncells, &data));
    EXPECT_EQ(ncells, 4); // mask has 4 valid cells, raster has 3. But use mask_extent, so ncells is 4
    EXPECT_NE(data, nullptr);
    string newcorename = GetCoreFileName(rs_byte_signed_noneg) + "_masked";
    string rs_out = dstpath + newcorename + ".tif";
    EXPECT_TRUE(rs->OutputToFile(rs_out));
    EXPECT_TRUE(FileExists(rs_out));

    // Change output data type to RDT_UInt8
    rs->SetOutDataType(RDT_UInt8);
    string newcorename2 = GetCoreFileName(rs_byte_signed_noneg) + "_masked_UInt8";
    string rs_out2 = dstpath + newcorename2 + ".tif";
    EXPECT_TRUE(rs->OutputToFile(rs_out2));
    EXPECT_TRUE(FileExists(rs_out2));

    // Check consistency of the two outputs: RDT_Int8 and RDT_UInt8
    IntRaster* out_rs = IntRaster::Init(rs_out);
    IntRaster* out_rs2 = IntRaster::Init(rs_out2);
    EXPECT_EQ(out_rs->GetCellNumber(), 6);
    EXPECT_EQ(out_rs->GetCellNumber(), out_rs2->GetCellNumber());
    EXPECT_NE(out_rs->GetNoDataValue(), out_rs2->GetNoDataValue());
    EXPECT_EQ(out_rs->GetValidNumber(), 3);
    EXPECT_EQ(out_rs->GetValidNumber(), out_rs2->GetValidNumber());
    out_rs->SetCalcPositions();
    out_rs2->SetCalcPositions();
    for (int i = 0; i < out_rs->GetValidNumber(); i++) {
        EXPECT_EQ(out_rs->GetValueByIndex(i), out_rs2->GetValueByIndex(i));
    }
    delete out_rs;
    delete out_rs2;

#ifdef USE_MONGODB
    EXPECT_TRUE(rs->OutputToMongoDB(GlobalEnv->gfs_, newcorename, STRING_MAP(), false)); // Save valid data
    STRING_MAP opts;
    UpdateStringMap(opts, HEADER_INC_NODATA, "FALSE");
    clsRasterData<vint8_t>* rs_mongo = clsRasterData<vint8_t>::Init(GlobalEnv->gfs_, newcorename.c_str(),
                                                                    false, mask_rs, true,
                                                                    NODATA_VALUE, opts);
    EXPECT_NE(rs_mongo, nullptr);
    if (HasFailure()) { return; }
    for (int i = 0; i < ncells; i++) {
        EXPECT_EQ(data[i], rs_mongo->GetValueByIndex(i));
    }
    delete rs_mongo;
#endif
    delete mask_rs;
    delete rs;
}

TEST(clsRasterDataUInt16, FullIO) {
    clsRasterData<uint16_t>* mask_rs = clsRasterData<uint16_t>::Init(rs_mask);
    clsRasterData<uint16_t>* rs = clsRasterData<uint16_t>::Init(rs_uint16, false,
                                                                mask_rs, true);
    EXPECT_TRUE(rs->GetDataType() == RDT_UInt16);
    EXPECT_TRUE(rs->GetOutDataType() == RDT_UInt16);
    int ncells = -1;
    uint16_t* data = nullptr;
    EXPECT_TRUE(rs->GetRasterData(&ncells, &data));
    EXPECT_TRUE(ncells > 0);
    EXPECT_NE(data, nullptr);
    string newcorename = GetCoreFileName(rs_uint16) + "_masked";
    string rs_out = dstpath + newcorename + ".tif";
    EXPECT_TRUE(rs->OutputToFile(rs_out));
    EXPECT_TRUE(FileExists(rs_out));

#ifdef USE_MONGODB
    EXPECT_TRUE(rs->OutputToMongoDB(GlobalEnv->gfs_, newcorename, STRING_MAP(), false)); // Save valid data
    STRING_MAP opts;
    UpdateStringMap(opts, HEADER_INC_NODATA, "FALSE");
    clsRasterData<uint16_t>* rs_mongo = clsRasterData<uint16_t>:: Init(GlobalEnv->gfs_, newcorename.c_str(),
                                                                       false, mask_rs, true,
                                                                       NODATA_VALUE, opts);
    EXPECT_NE(rs_mongo, nullptr);
    if (HasFailure()) { return; }
    for (int i = 0; i < ncells; i++) {
        EXPECT_EQ(data[i], rs_mongo->GetValueByIndex(i));
    }
    delete rs_mongo;
#endif
    delete mask_rs;
    delete rs;
}

TEST(clsRasterDataInt16, FullIO) {
    clsRasterData<int16_t>* mask_rs = clsRasterData<int16_t>::Init(rs_mask);
    clsRasterData<int16_t>* rs = clsRasterData<int16_t>::Init(rs_int16, false,
                                                              mask_rs, true);
    EXPECT_TRUE(rs->GetDataType() == RDT_Int16);
    EXPECT_TRUE(rs->GetOutDataType() == RDT_Int16);
    int ncells = -1;
    int16_t* data = nullptr;
    EXPECT_TRUE(rs->GetRasterData(&ncells, &data));
    EXPECT_TRUE(ncells > 0);
    EXPECT_NE(data, nullptr);
    string newcorename = GetCoreFileName(rs_int16) + "_masked";
    string rs_out = dstpath + newcorename + ".tif";
    EXPECT_TRUE(rs->OutputToFile(rs_out));
    EXPECT_TRUE(FileExists(rs_out));

#ifdef USE_MONGODB
    EXPECT_TRUE(rs->OutputToMongoDB(GlobalEnv->gfs_, newcorename, STRING_MAP(), false)); // Save valid data
    STRING_MAP opts;
    UpdateStringMap(opts, HEADER_INC_NODATA, "FALSE");
    clsRasterData<int16_t>* rs_mongo = clsRasterData<int16_t>::Init(GlobalEnv->gfs_, newcorename.c_str(),
                                                                    false, mask_rs, true,
                                                                    NODATA_VALUE, opts);
    EXPECT_NE(rs_mongo, nullptr);
    if (HasFailure()) { return; }
    for (int i = 0; i < ncells; i++) {
        EXPECT_EQ(data[i], rs_mongo->GetValueByIndex(i));
    }
    delete rs_mongo;
#endif
    delete mask_rs;
    delete rs;
}

TEST(clsRasterDataUInt32, FullIO) {
    clsRasterData<uint32_t>* mask_rs = clsRasterData<uint32_t>::Init(rs_mask);
    clsRasterData<uint32_t>* rs = clsRasterData<uint32_t>::Init(rs_uint32, false,
                                                                mask_rs, true);
    EXPECT_TRUE(rs->GetDataType() == RDT_UInt32);
    EXPECT_TRUE(rs->GetOutDataType() == RDT_UInt32);
    int ncells = -1;
    uint32_t* data = nullptr;
    EXPECT_TRUE(rs->GetRasterData(&ncells, &data));
    EXPECT_TRUE(ncells > 0);
    EXPECT_NE(data, nullptr);
    string newcorename = GetCoreFileName(rs_uint32) + "_masked";
    string rs_out = dstpath + newcorename + ".tif";
    EXPECT_TRUE(rs->OutputToFile(rs_out));
    EXPECT_TRUE(FileExists(rs_out));

#ifdef USE_MONGODB
    EXPECT_TRUE(rs->OutputToMongoDB(GlobalEnv->gfs_, newcorename, STRING_MAP(), false)); // Save valid data
    STRING_MAP opts;
    UpdateStringMap(opts, HEADER_INC_NODATA, "FALSE");
    clsRasterData<uint32_t>* rs_mongo = clsRasterData<uint32_t>::Init(GlobalEnv->gfs_, newcorename.c_str(),
                                                                      false, mask_rs, true,
                                                                      NODATA_VALUE, opts);
    EXPECT_NE(rs_mongo, nullptr);
    if (HasFailure()) { return; }
    for (int i = 0; i < ncells; i++) {
        EXPECT_EQ(data[i], rs_mongo->GetValueByIndex(i));
    }
    delete rs_mongo;
#endif
    delete mask_rs;
    delete rs;
}

TEST(clsRasterDataInt32, FullIO) {
    clsRasterData<int32_t>* mask_rs = clsRasterData<int32_t>::Init(rs_mask);
    clsRasterData<int32_t>* rs = clsRasterData<int32_t>::Init(rs_int32, false,
                                                              mask_rs, true);
    EXPECT_TRUE(rs->GetDataType() == RDT_Int32);
    EXPECT_TRUE(rs->GetOutDataType() == RDT_Int32);
    int ncells = -1;
    int32_t* data = nullptr;
    EXPECT_TRUE(rs->GetRasterData(&ncells, &data));
    EXPECT_TRUE(ncells > 0);
    EXPECT_NE(data, nullptr);
    string newcorename = GetCoreFileName(rs_int32) + "_masked";
    string rs_out = dstpath + newcorename + ".tif";
    EXPECT_TRUE(rs->OutputToFile(rs_out));
    EXPECT_TRUE(FileExists(rs_out));

#ifdef USE_MONGODB
    EXPECT_TRUE(rs->OutputToMongoDB(GlobalEnv->gfs_, newcorename, STRING_MAP(), false)); // Save valid data
    STRING_MAP opts;
    UpdateStringMap(opts, HEADER_INC_NODATA, "FALSE");
    clsRasterData<int32_t>* rs_mongo = clsRasterData<int32_t>::Init(GlobalEnv->gfs_, newcorename.c_str(),
                                                                    false, mask_rs, true,
                                                                    NODATA_VALUE, opts);
    EXPECT_NE(rs_mongo, nullptr);
    if (HasFailure()) { return; }
    for (int i = 0; i < ncells; i++) {
        EXPECT_EQ(data[i], rs_mongo->GetValueByIndex(i));
    }
    delete rs_mongo;
#endif
    delete mask_rs;
    delete rs;
}

TEST(clsRasterDataInt32, IOWithoutDefNodata) {
    clsRasterData<int32_t>* rs = clsRasterData<int32_t>::Init(rs_int32_nodefnodata);
    EXPECT_NE(nullptr, rs);
    EXPECT_EQ(INT32_MIN, rs->GetNoDataValue());
    EXPECT_TRUE(rs->GetDataType() == RDT_Int32);
    EXPECT_TRUE(rs->GetOutDataType() == RDT_Int32);

    delete rs;
}

TEST(clsRasterDataFloat, FullIO) {
    clsRasterData<float>* mask_rs = clsRasterData<float>::Init(rs_mask);
    clsRasterData<float>* rs = clsRasterData<float>::Init(rs_float, false,
                                                          mask_rs, true);
    EXPECT_TRUE(rs->GetDataType() == RDT_Float);
    EXPECT_TRUE(rs->GetOutDataType() == RDT_Float);
    int ncells = -1;
    float* data = nullptr;
    EXPECT_TRUE(rs->GetRasterData(&ncells, &data));
    EXPECT_TRUE(ncells > 0);
    EXPECT_NE(data, nullptr);
    string newcorename = GetCoreFileName(rs_float) + "_masked";
    string rs_out = dstpath + newcorename + ".tif";
    EXPECT_TRUE(rs->OutputToFile(rs_out));
    EXPECT_TRUE(FileExists(rs_out));

#ifdef USE_MONGODB
    EXPECT_TRUE(rs->OutputToMongoDB(GlobalEnv->gfs_, newcorename, STRING_MAP(), false)); // Save valid data
    STRING_MAP opts;
    UpdateStringMap(opts, HEADER_INC_NODATA, "FALSE");
    clsRasterData<float>* rs_mongo = clsRasterData<float>::Init(GlobalEnv->gfs_, newcorename.c_str(),
                                                                false, mask_rs, true,
                                                                NODATA_VALUE, opts);
    EXPECT_NE(rs_mongo, nullptr);
    if (HasFailure()) { return; }
    for (int i = 0; i < ncells; i++) {
        EXPECT_EQ(data[i], rs_mongo->GetValueByIndex(i));
    }
    delete rs_mongo;
#endif
    delete mask_rs;
    delete rs;
}

TEST(clsRasterDataDouble, FullIO) {
    clsRasterData<double>* mask_rs = clsRasterData<double>::Init(rs_mask);
    clsRasterData<double>* rs = clsRasterData<double>::Init(rs_double, false,
                                                            mask_rs, true);
    EXPECT_TRUE(rs->GetDataType() == RDT_Double);
    EXPECT_TRUE(rs->GetOutDataType() == RDT_Double);
    int ncells = -1;
    double* data = nullptr;
    EXPECT_TRUE(rs->GetRasterData(&ncells, &data));
    EXPECT_TRUE(ncells > 0);
    EXPECT_NE(data, nullptr);
    string newcorename = GetCoreFileName(rs_double) + "_masked";
    string rs_out = dstpath + newcorename + ".tif";
    EXPECT_TRUE(rs->OutputToFile(rs_out));
    EXPECT_TRUE(FileExists(rs_out));

#ifdef USE_MONGODB
    EXPECT_TRUE(rs->OutputToMongoDB(GlobalEnv->gfs_, newcorename, STRING_MAP(), false)); // Save valid data
    STRING_MAP opts;
    UpdateStringMap(opts, HEADER_INC_NODATA, "FALSE");
    clsRasterData<double>* rs_mongo = clsRasterData<double>::Init(GlobalEnv->gfs_, newcorename.c_str(),
                                                                  false, mask_rs, true,
                                                                  NODATA_VALUE, opts);
    EXPECT_NE(rs_mongo, nullptr);
    if (HasFailure()) { return; }
    for (int i = 0; i < ncells; i++) {
        EXPECT_EQ(data[i], rs_mongo->GetValueByIndex(i));
    }
    delete rs_mongo;
#endif
    delete mask_rs;
    delete rs;
}

#endif

} /* namespace */
