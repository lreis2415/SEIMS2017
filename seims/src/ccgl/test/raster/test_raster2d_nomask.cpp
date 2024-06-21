/*!
 * \brief Test description:
 *
 *        TEST CASE NAME (or TEST SUITE):
 *            clsRasterDataTest2DNoMask: Read 2D raster without a mask
 *
 *        Since we mainly support ASC and GDAL(e.g., TIFF),
 *        value-parameterized tests of Google Test will be used.
 * \version 1.3
 * \authors Liangjun Zhu, zlj(at)lreis.ac.cn; crazyzlj(at)gmail.com
 * \remarks 2017-12-02 - lj - Original version.
 *          2018-05-03 - lj - Integrated into CCGL.
 *          2021-07-20 - lj - Update after changes of GetValue and GetValueByIndex.
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
using namespace ccgl::utils_filesystem;
using namespace ccgl::utils_array;
#ifdef USE_MONGODB
using namespace ccgl::db_mongoc;
#endif

extern GlobalEnvironment* GlobalEnv;

namespace {
using ::testing::TestWithParam;
using ::testing::Values;

string Rspath = GetAppPath() + "./data/raster/";
string Dstpath = Rspath + "result/";
string Corename = "tinydemo_raster_r4c3";

string rs1_asc = Rspath + Corename + ".asc";
string rs2_asc = Rspath + Corename + "_2.asc";
string rs3_asc = Rspath + Corename + "_3.asc";

string rs1_tif = Rspath + Corename + ".tif";
string rs2_tif = Rspath + Corename + "_2.tif";
string rs3_tif = Rspath + Corename + "_3.tif";

struct InputRasterFiles {
public:
    InputRasterFiles(const string& rs1, const string& rs2,
                     const string& rs3) {
        raster_name1 = rs1.c_str();
        raster_name2 = rs2.c_str();
        raster_name3 = rs3.c_str();
    }

    ~InputRasterFiles() { ; }

    const char* raster_name1;
    const char* raster_name2;
    const char* raster_name3;
};

//Inside the test body, fixture constructor, SetUp(), and TearDown() you
//can refer to the test parameter by GetParam(). In this case, the test
//parameter is a factory function which we call in fixture's SetUp() to
//create and store an instance of clsRasterData<float>.
class clsRasterDataTest2DNoMask : public TestWithParam<InputRasterFiles*> {
public:
    clsRasterDataTest2DNoMask() : rs_(nullptr) {
    }

    virtual ~clsRasterDataTest2DNoMask() { ; }

    void SetUp() OVERRIDE {
        vector<string> filenames;
        filenames.emplace_back(GetParam()->raster_name1);
        filenames.emplace_back(GetParam()->raster_name2);
        filenames.emplace_back(GetParam()->raster_name3);

        rs_ = FltRaster::Init(filenames);
        ASSERT_NE(nullptr, rs_);
    }

    void TearDown() OVERRIDE {
        delete rs_;
    }

protected:
    FltRaster* rs_;
};

// calc_pos = False
TEST_P(clsRasterDataTest2DNoMask, RasterIO) {
    /// 1. Test members after constructing.
    EXPECT_EQ(36, rs_->GetDataLength()); // m_nCells * n_lyrs_
    EXPECT_EQ(12, rs_->GetCellNumber()); // m_nCells
    EXPECT_EQ(3, rs_->GetLayers());

    EXPECT_EQ(-9999., rs_->GetNoDataValue());  // m_noDataValue
    EXPECT_EQ(-9999., rs_->GetDefaultValue()); // m_defaultValue
    
    EXPECT_TRUE(rs_->Initialized());           // m_initialized
    EXPECT_TRUE(rs_->Is2DRaster());            // m_is2DRaster
    EXPECT_FALSE(rs_->PositionsCalculated());  // m_calcPositions
    EXPECT_FALSE(rs_->PositionsAllocated());   // m_storePositions
    EXPECT_FALSE(rs_->MaskExtented());         // m_useMaskExtent
    EXPECT_FALSE(rs_->StatisticsCalculated()); // m_statisticsCalculated

    ASSERT_TRUE(rs_->ValidateRasterData());

    EXPECT_EQ(nullptr, rs_->GetRasterDataPointer());         // m_rasterData
    EXPECT_NE(nullptr, rs_->Get2DRasterDataPointer());       // m_raster2DData
    EXPECT_EQ(nullptr, rs_->GetRasterPositionDataPointer()); // m_rasterPositionData
    EXPECT_EQ(nullptr, rs_->GetRasterPositionIndexPointer()); // m_rasterPositionIndex

    /** Get metadata, m_headers **/
    STRDBL_MAP header_info = rs_->GetRasterHeader();
    EXPECT_EQ(CVT_INT(header_info.at(HEADER_RS_LAYERS)), rs_->GetLayers());
    EXPECT_EQ(CVT_INT(header_info.at(HEADER_RS_CELLSNUM)), rs_->GetCellNumber());

    EXPECT_EQ(4, rs_->GetRows());
    EXPECT_EQ(3, rs_->GetCols());
    EXPECT_DOUBLE_EQ(1., rs_->GetXllCenter());
    EXPECT_DOUBLE_EQ(1., rs_->GetYllCenter());
    EXPECT_DOUBLE_EQ(2., rs_->GetCellWidth());
    EXPECT_EQ("", rs_->GetSrsString());

    /** Calc and get basic statistics, m_statsMap **/
    EXPECT_EQ(6, rs_->GetValidNumber()); // without calculate valid positions
    EXPECT_DOUBLE_EQ(1., rs_->GetMinimum()); // by default, layer 1
    EXPECT_DOUBLE_EQ(6., rs_->GetMaximum());
    EXPECT_DOUBLE_EQ(3.5, rs_->GetAverage());
    EXPECT_DOUBLE_EQ(1.707825127659933, rs_->GetStd());
    EXPECT_DOUBLE_EQ(5., rs_->GetRange());
    EXPECT_DOUBLE_EQ(7., rs_->GetMinimum(2)); // layer 2
    EXPECT_DOUBLE_EQ(12., rs_->GetMaximum(2));
    EXPECT_DOUBLE_EQ(9.5, rs_->GetAverage(2));
    EXPECT_DOUBLE_EQ(1.707825127659933, rs_->GetStd(2));
    EXPECT_DOUBLE_EQ(5., rs_->GetRange(2));
    EXPECT_DOUBLE_EQ(13., rs_->GetMinimum(3)); // layer 3
    EXPECT_DOUBLE_EQ(17., rs_->GetMaximum(3));
    EXPECT_DOUBLE_EQ(15., rs_->GetAverage(3));
    EXPECT_DOUBLE_EQ(1.4142135623730951, rs_->GetStd(3));
    EXPECT_DOUBLE_EQ(4., rs_->GetRange(3));
    EXPECT_TRUE(rs_->StatisticsCalculated());

    EXPECT_EQ(nullptr, rs_->GetMask()); // m_mask

    /** Test getting raster data **/
    int ncells = 0;
    float* rs_data = nullptr;
    EXPECT_FALSE(rs_->GetRasterData(&ncells, &rs_data)); // m_rasterData
    EXPECT_EQ(-1, ncells);
    EXPECT_EQ(nullptr, rs_data);

    float** rs_2ddata = nullptr;
    int nlyrs = -1;
    EXPECT_TRUE(rs_->Get2DRasterData(&ncells, &nlyrs, &rs_2ddata)); // m_raster2DData
    EXPECT_EQ(12, ncells);
    EXPECT_EQ(3, nlyrs);
    EXPECT_NE(nullptr, rs_2ddata);
    EXPECT_EQ(rs_2ddata[0][0], 1.);
    EXPECT_EQ(rs_2ddata[0][1], 7.);
    EXPECT_EQ(rs_2ddata[0][2], 13.);
    EXPECT_EQ(rs_2ddata[1][0], 2.);
    EXPECT_EQ(rs_2ddata[1][1], 8.);
    EXPECT_EQ(rs_2ddata[1][2], 14.);
    EXPECT_EQ(rs_2ddata[2][0], -9999.);
    EXPECT_EQ(rs_2ddata[2][1], -9999.);
    EXPECT_EQ(rs_2ddata[2][2], -9999.);
    EXPECT_EQ(rs_2ddata[3][0], -9999.);
    EXPECT_EQ(rs_2ddata[3][1], -9999.);
    EXPECT_EQ(rs_2ddata[3][2], -9999.);
    EXPECT_EQ(rs_2ddata[4][0], 3.);
    EXPECT_EQ(rs_2ddata[4][1], 9.);
    EXPECT_EQ(rs_2ddata[4][2], 15.);
    EXPECT_EQ(rs_2ddata[5][0], 4.);
    EXPECT_EQ(rs_2ddata[5][1], 10.);
    EXPECT_EQ(rs_2ddata[5][2], -9999.);
    EXPECT_EQ(rs_2ddata[6][0], -9999.);
    EXPECT_EQ(rs_2ddata[6][1], -9999.);
    EXPECT_EQ(rs_2ddata[6][2], -9999.);
    EXPECT_EQ(rs_2ddata[7][0], 5.);
    EXPECT_EQ(rs_2ddata[7][1], 11.);
    EXPECT_EQ(rs_2ddata[7][2], 16.);
    EXPECT_EQ(rs_2ddata[8][0], -9999.);
    EXPECT_EQ(rs_2ddata[8][1], -9999.);
    EXPECT_EQ(rs_2ddata[8][2], -9999.);
    EXPECT_EQ(rs_2ddata[9][0], -9999.);
    EXPECT_EQ(rs_2ddata[9][1], -9999.);
    EXPECT_EQ(rs_2ddata[9][2], -9999.);
    EXPECT_EQ(rs_2ddata[10][0], -9999.);
    EXPECT_EQ(rs_2ddata[10][1], -9999.);
    EXPECT_EQ(rs_2ddata[10][2], -9999.);
    EXPECT_EQ(rs_2ddata[11][0], 6.);
    EXPECT_EQ(rs_2ddata[11][1], 12.);
    EXPECT_EQ(rs_2ddata[11][2], 17.);

    // Actual data stored in memory
    EXPECT_EQ(rs_2ddata[0][0], 1.);
    EXPECT_EQ(rs_2ddata[0][1], 7.);
    EXPECT_EQ(rs_2ddata[0][2], 13.);
    EXPECT_EQ(rs_2ddata[0][3], 2.);
    EXPECT_EQ(rs_2ddata[0][4], 8.);
    EXPECT_EQ(rs_2ddata[0][5], 14.);
    EXPECT_EQ(rs_2ddata[0][6], -9999.);
    EXPECT_EQ(rs_2ddata[0][7], -9999.);
    EXPECT_EQ(rs_2ddata[0][8], -9999.);
    EXPECT_EQ(rs_2ddata[0][9], -9999.);
    EXPECT_EQ(rs_2ddata[0][10], -9999.);
    EXPECT_EQ(rs_2ddata[0][11], -9999.);
    EXPECT_EQ(rs_2ddata[0][12], 3.);
    EXPECT_EQ(rs_2ddata[0][13], 9.);
    EXPECT_EQ(rs_2ddata[0][14], 15.);
    EXPECT_EQ(rs_2ddata[0][15], 4.);
    EXPECT_EQ(rs_2ddata[0][16], 10.);
    EXPECT_EQ(rs_2ddata[0][17], -9999.);
    EXPECT_EQ(rs_2ddata[0][18], -9999.);
    EXPECT_EQ(rs_2ddata[0][19], -9999.);
    EXPECT_EQ(rs_2ddata[0][20], -9999.);
    EXPECT_EQ(rs_2ddata[0][21], 5.);
    EXPECT_EQ(rs_2ddata[0][22], 11.);
    EXPECT_EQ(rs_2ddata[0][23], 16.);
    EXPECT_EQ(rs_2ddata[0][24], -9999.);
    EXPECT_EQ(rs_2ddata[0][25], -9999.);
    EXPECT_EQ(rs_2ddata[0][26], -9999.);
    EXPECT_EQ(rs_2ddata[0][27], -9999.);
    EXPECT_EQ(rs_2ddata[0][28], -9999.);
    EXPECT_EQ(rs_2ddata[0][29], -9999.);
    EXPECT_EQ(rs_2ddata[0][30], -9999.);
    EXPECT_EQ(rs_2ddata[0][31], -9999.);
    EXPECT_EQ(rs_2ddata[0][32], -9999.);
    EXPECT_EQ(rs_2ddata[0][33], 6.);
    EXPECT_EQ(rs_2ddata[0][34], 12.);
    EXPECT_EQ(rs_2ddata[0][35], 17.);

    /** Get raster cell value by various way **/
    // Get cell value by index of raster_data_
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(-1));
    EXPECT_FLOAT_EQ(1.f, rs_->GetValueByIndex(0)); // by default, layer 1
    EXPECT_FLOAT_EQ(2.f, rs_->GetValueByIndex(1));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(3));
    EXPECT_FLOAT_EQ(3.f, rs_->GetValueByIndex(4));
    EXPECT_FLOAT_EQ(4.f, rs_->GetValueByIndex(5));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(6));
    EXPECT_FLOAT_EQ(5.f, rs_->GetValueByIndex(7));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(8));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(9));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(10));
    EXPECT_FLOAT_EQ(6.f, rs_->GetValueByIndex(11));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(12)); // out of bound
    EXPECT_FLOAT_EQ(7.f, rs_->GetValueByIndex(0, 2)); // layer 2
    EXPECT_FLOAT_EQ(8.f, rs_->GetValueByIndex(1, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(2, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(3, 2));
    EXPECT_FLOAT_EQ(9.f, rs_->GetValueByIndex(4, 2));
    EXPECT_FLOAT_EQ(10.f, rs_->GetValueByIndex(5, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(6, 2));
    EXPECT_FLOAT_EQ(11.f, rs_->GetValueByIndex(7, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(8, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(9, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(10, 2));
    EXPECT_FLOAT_EQ(12.f, rs_->GetValueByIndex(11, 2));
    EXPECT_FLOAT_EQ(13.f, rs_->GetValueByIndex(0, 3)); // layer 3
    EXPECT_FLOAT_EQ(14.f, rs_->GetValueByIndex(1, 3));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(2, 3));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(3, 3));
    EXPECT_FLOAT_EQ(15.f, rs_->GetValueByIndex(4, 3));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(5, 3));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(6, 3));
    EXPECT_FLOAT_EQ(16.f, rs_->GetValueByIndex(7, 3));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(8, 3));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(9, 3));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(10, 3));
    EXPECT_FLOAT_EQ(17.f, rs_->GetValueByIndex(11, 3));

    /// Get cell value by index of raster_data_/raster_2d_, compatible way
    int tmp_lyr = rs_->GetLayers();
    float* tmp_values = nullptr;
    Initialize1DArray(tmp_lyr, tmp_values, -9999.f);
    rs_->GetValueByIndex(-1, tmp_values); // index exceed, tmp_values will be released
    EXPECT_EQ(nullptr, tmp_values);
    rs_->GetValueByIndex(0, tmp_values); // nullptr input, tmp_values will be initialized
    EXPECT_NE(nullptr, tmp_values);
    EXPECT_FLOAT_EQ(1.f, tmp_values[0]);
    EXPECT_FLOAT_EQ(7.f, tmp_values[1]);
    EXPECT_FLOAT_EQ(13.f, tmp_values[2]);

    /// Get cell value by (row, col)
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(-1, 0));
    EXPECT_FLOAT_EQ(1.f, rs_->GetValue(0, 0)); // by default, layer 1
    EXPECT_FLOAT_EQ(2.f, rs_->GetValue(0, 1));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(0, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(1, 0));
    EXPECT_FLOAT_EQ(3.f, rs_->GetValue(1, 1));
    EXPECT_FLOAT_EQ(4.f, rs_->GetValue(1, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(2, 0));
    EXPECT_FLOAT_EQ(5.f, rs_->GetValue(2, 1));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(2, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(3, 0));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(3, 1));
    EXPECT_FLOAT_EQ(6.f, rs_->GetValue(3, 2));
    EXPECT_FLOAT_EQ(6.f, rs_->GetValue(3, 2, 1));
    EXPECT_FLOAT_EQ(7.f, rs_->GetValue(0, 0, 2)); // layer 2
    EXPECT_FLOAT_EQ(8.f, rs_->GetValue(0, 1, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(0, 2, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(1, 0, 2));
    EXPECT_FLOAT_EQ(9.f, rs_->GetValue(1, 1, 2));
    EXPECT_FLOAT_EQ(10.f, rs_->GetValue(1, 2, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(2, 0, 2));
    EXPECT_FLOAT_EQ(11.f, rs_->GetValue(2, 1, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(2, 2, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(3, 0, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(3, 1, 2));
    EXPECT_FLOAT_EQ(12.f, rs_->GetValue(3, 2, 2));
    EXPECT_FLOAT_EQ(13.f, rs_->GetValue(0, 0, 3)); // layer 3
    EXPECT_FLOAT_EQ(14.f, rs_->GetValue(0, 1, 3));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(0, 2, 3));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(1, 0, 3));
    EXPECT_FLOAT_EQ(15.f, rs_->GetValue(1, 1, 3));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(1, 2, 3));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(2, 0, 3));
    EXPECT_FLOAT_EQ(16.f, rs_->GetValue(2, 1, 3));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(2, 2, 3));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(3, 0, 3));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(3, 1, 3));
    EXPECT_FLOAT_EQ(17.f, rs_->GetValue(3, 2, 3));

    /// Get cell values by (row, col, *&array)
    rs_->GetValue(-1, 0, tmp_values);
    EXPECT_EQ(nullptr, tmp_values);
    rs_->GetValue(0, -1, tmp_values);
    EXPECT_EQ(nullptr, tmp_values);
    rs_->GetValue(0, 0, tmp_values);
    EXPECT_NE(nullptr, tmp_values);
    EXPECT_FLOAT_EQ(1.f, tmp_values[0]);
    EXPECT_FLOAT_EQ(7.f, tmp_values[1]);
    EXPECT_FLOAT_EQ(13.f, tmp_values[2]);
    rs_->GetValue(1, 2, tmp_values);
    EXPECT_NE(nullptr, tmp_values);
    EXPECT_FLOAT_EQ(4.f, tmp_values[0]);
    EXPECT_FLOAT_EQ(10.f, tmp_values[1]);
    EXPECT_FLOAT_EQ(-9999.f, tmp_values[2]);

    Release1DArray(tmp_values);
    EXPECT_EQ(nullptr, tmp_values);

    // Get position index, if cal_poc is False, no chance to return -1
    EXPECT_EQ(-2, rs_->GetPosition(-1., -2.));
    EXPECT_EQ(-2, rs_->GetPosition(-1., 9.));
    EXPECT_EQ(0, rs_->GetPosition(1., 7.));
    EXPECT_EQ(1, rs_->GetPosition(3.99, 7.05));
    EXPECT_EQ(2, rs_->GetPosition(5.01, 7.00));
    EXPECT_EQ(3, rs_->GetPosition(0.01, 5.5));
    EXPECT_EQ(4, rs_->GetPosition(2.01, 5.1));
    EXPECT_EQ(5, rs_->GetPosition(4.5, 5.5));
    EXPECT_EQ(6, rs_->GetPosition(1., 3.));
    EXPECT_EQ(7, rs_->GetPosition(3., 3.));
    EXPECT_EQ(8, rs_->GetPosition(5., 3.));
    EXPECT_EQ(9, rs_->GetPosition(1., 1.));
    EXPECT_EQ(10, rs_->GetPosition(3., 1.));
    EXPECT_EQ(11, rs_->GetPosition(5., 1.));

    /** Set value **/
    // Set core file name
    string newcorename = Corename + "_2D-nomask-nopos";
    rs_->SetCoreName(newcorename);
    EXPECT_EQ(newcorename, rs_->GetCoreName());

    // Set raster data value
    rs_->SetValue(3, 1, 6.f, 1); // change nodata, only allowed when calc_pos=false
    rs_->SetValue(3, 2, -9999.f);
    EXPECT_FLOAT_EQ(6.f, rs_->GetValue(3, 1));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(3, 2));

    // update statistics
    rs_->UpdateStatistics(); // Should be manually invoked in your project!
    EXPECT_DOUBLE_EQ(1., rs_->GetMinimum()); // layer 1
    EXPECT_DOUBLE_EQ(6., rs_->GetMaximum());
    EXPECT_DOUBLE_EQ(3.5, rs_->GetAverage());
    EXPECT_DOUBLE_EQ(1.707825127659933, rs_->GetStd());
    EXPECT_DOUBLE_EQ(5., rs_->GetRange());

    /** Output to new file **/
    string oldfullname = rs_->GetFilePath();
    string newfullname = Dstpath + newcorename + "." + GetSuffix(oldfullname);
    EXPECT_TRUE(rs_->OutputToFile(newfullname));

    // Change values back
    rs_->SetValue(3, 1, -9999.f, 1);
    rs_->SetValue(3, 2, 6.f);
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(3, 1));
    EXPECT_FLOAT_EQ(6.f, rs_->GetValue(3, 2));

#ifdef USE_MONGODB
    /** MongoDB I/O test **/
    MongoGridFs* gfs_ = GlobalEnv->gfs_;
    EXPECT_NE(nullptr, gfs_);
    string gfsfilename = newcorename + "_" + GetSuffix(oldfullname);
    string gfsfilename_valid = newcorename + "_valid_" + GetSuffix(oldfullname);
    gfs_->RemoveFile(gfsfilename); // remove if already existed
    gfs_->RemoveFile(gfsfilename_valid);
    EXPECT_TRUE(rs_->OutputToMongoDB(gfs_, gfsfilename)); // store fullsize raster
    map<string, string> additional_header;
    additional_header[HEADER_MASK_NAME] = rs_->GetCoreName();
    EXPECT_TRUE(rs_->OutputToMongoDB(gfs_, gfsfilename_valid, additional_header, false));

    if (HasFailure()) { return; }

    // Read from MongoDB without mask layer
    FltRaster* mongors = FltRaster::Init(gfs_, gfsfilename.c_str(), false);

    // Read from MongoDB with predefined valid positions
    FltRaster* mongors_valid = new FltRaster(); // create empty raster, set and read data
    mongors_valid->SetHeader(rs_->GetRasterHeader()); // set header
    int** posdata;
    int* posidx;
    int poslen;
    rs_->GetRasterPositionData(&poslen, &posdata);
    mongors_valid->SetPositions(poslen, posdata);
    rs_->GetRasterPositionData(&poslen, &posidx);
    mongors_valid->SetPositions(poslen, posidx);
    STRING_MAP opts;
    UpdateStringMap(opts, HEADER_INC_NODATA, "FALSE");
    mongors_valid->ReadFromMongoDB(gfs_, gfsfilename_valid, false, nullptr, true, NODATA_VALUE, opts);

    // Check the consistency of mongors and mongors_valid
    EXPECT_NE(mongors->GetCellNumber(), mongors_valid->GetCellNumber());
    EXPECT_NE(mongors->GetDataLength(), mongors_valid->GetDataLength());
    EXPECT_EQ(mongors->GetValidNumber(), mongors_valid->GetValidNumber());
    EXPECT_DOUBLE_EQ(mongors->GetAverage(), mongors_valid->GetAverage());
    EXPECT_DOUBLE_EQ(mongors->GetStd(), mongors_valid->GetStd());
    EXPECT_EQ(mongors->GetLayers(), 3);
    EXPECT_EQ(mongors->GetLayers(), mongors_valid->GetLayers());
    EXPECT_EQ(mongors->GetRows(), mongors_valid->GetRows());
    EXPECT_EQ(mongors->GetCols(), mongors_valid->GetCols());
    for (int ir = 0; ir < mongors->GetRows(); ir++) {
        for (int ic = 0; ic < mongors->GetCols(); ic++) {
            for (int il = 0; il < mongors->GetLayers(); il++) {
                EXPECT_FLOAT_EQ(mongors->GetValue(ir, ic, il), mongors_valid->GetValue(ir, ic, il));
            }
        }
    }
    delete mongors;
    delete mongors_valid;
#endif
}

// calc_pos = True
TEST_P(clsRasterDataTest2DNoMask, RasterIOWithCalcPos) {
    EXPECT_FALSE(rs_->PositionsCalculated());
    /* Get position data, which will be calculated if not existed */
    int ncells = -1;
    int** positions = nullptr;
    rs_->GetRasterPositionData(&ncells, &positions); // m_rasterPositionData
    EXPECT_TRUE(rs_->PositionsCalculated());
    EXPECT_TRUE(rs_->PositionsAllocated());
    EXPECT_NE(nullptr, rs_->GetRasterPositionDataPointer());
    EXPECT_NE(nullptr, rs_->GetRasterPositionIndexPointer());
    EXPECT_EQ(6, ncells);
    EXPECT_NE(nullptr, positions);

    EXPECT_EQ(0, positions[0][0]); // row of the first valid cell
    EXPECT_EQ(0, positions[0][1]); // col of the first valid cell
    EXPECT_EQ(0, positions[1][0]);
    EXPECT_EQ(1, positions[1][1]);
    EXPECT_EQ(1, positions[2][0]);
    EXPECT_EQ(1, positions[2][1]);
    EXPECT_EQ(1, positions[3][0]);
    EXPECT_EQ(2, positions[3][1]);
    EXPECT_EQ(2, positions[4][0]);
    EXPECT_EQ(1, positions[4][1]);
    EXPECT_EQ(3, positions[5][0]);
    EXPECT_EQ(2, positions[5][1]);

    /// 1. Test members after constructing.
    EXPECT_EQ(18, rs_->GetDataLength()); // m_nCells * n_lyrs_
    EXPECT_EQ(6, rs_->GetCellNumber()); // m_nCells
    EXPECT_EQ(3, rs_->GetLayers());

    EXPECT_EQ(-9999., rs_->GetNoDataValue());  // m_noDataValue
    EXPECT_EQ(-9999., rs_->GetDefaultValue()); // m_defaultValue
    
    EXPECT_TRUE(rs_->Initialized());           // m_initialized
    EXPECT_TRUE(rs_->Is2DRaster());            // m_is2DRaster
    EXPECT_TRUE(rs_->PositionsCalculated());   // m_calcPositions
    EXPECT_TRUE(rs_->PositionsAllocated());    // m_storePositions
    EXPECT_FALSE(rs_->MaskExtented());         // m_useMaskExtent
    EXPECT_FALSE(rs_->StatisticsCalculated()); // m_statisticsCalculated

    ASSERT_TRUE(rs_->ValidateRasterData());

    EXPECT_EQ(nullptr, rs_->GetRasterDataPointer());         // m_rasterData
    EXPECT_NE(nullptr, rs_->Get2DRasterDataPointer());       // m_raster2DData
    EXPECT_NE(nullptr, rs_->GetRasterPositionDataPointer()); // m_rasterPositionData
    EXPECT_NE(nullptr, rs_->GetRasterPositionIndexPointer()); // m_rasterPositionIndex

    /** Get metadata, m_headers **/
    STRDBL_MAP header_info = rs_->GetRasterHeader();
    EXPECT_EQ(CVT_INT(header_info.at(HEADER_RS_LAYERS)), rs_->GetLayers());
    EXPECT_EQ(CVT_INT(header_info.at(HEADER_RS_CELLSNUM)), rs_->GetCellNumber());

    EXPECT_EQ(4, rs_->GetRows());
    EXPECT_EQ(3, rs_->GetCols());
    EXPECT_DOUBLE_EQ(1., rs_->GetXllCenter());
    EXPECT_DOUBLE_EQ(1., rs_->GetYllCenter());
    EXPECT_DOUBLE_EQ(2., rs_->GetCellWidth());
    EXPECT_EQ("", rs_->GetSrsString());

    /** Calc and get basic statistics, m_statsMap **/
    EXPECT_EQ(6, rs_->GetValidNumber());
    EXPECT_DOUBLE_EQ(1., rs_->GetMinimum()); // by default, layer 1
    EXPECT_DOUBLE_EQ(6., rs_->GetMaximum());
    EXPECT_DOUBLE_EQ(3.5, rs_->GetAverage());
    EXPECT_DOUBLE_EQ(1.707825127659933, rs_->GetStd());
    EXPECT_DOUBLE_EQ(5., rs_->GetRange());
    EXPECT_DOUBLE_EQ(7., rs_->GetMinimum(2)); // layer 2
    EXPECT_DOUBLE_EQ(12., rs_->GetMaximum(2));
    EXPECT_DOUBLE_EQ(9.5, rs_->GetAverage(2));
    EXPECT_DOUBLE_EQ(1.707825127659933, rs_->GetStd(2));
    EXPECT_DOUBLE_EQ(5., rs_->GetRange(2));
    EXPECT_DOUBLE_EQ(13., rs_->GetMinimum(3)); // layer 3
    EXPECT_DOUBLE_EQ(17., rs_->GetMaximum(3));
    EXPECT_DOUBLE_EQ(15., rs_->GetAverage(3));
    EXPECT_DOUBLE_EQ(1.4142135623730951, rs_->GetStd(3));
    EXPECT_DOUBLE_EQ(4., rs_->GetRange(3));
    EXPECT_TRUE(rs_->StatisticsCalculated());

    EXPECT_EQ(nullptr, rs_->GetMask()); // m_mask

    /** Test getting raster data **/
    float* rs_data = nullptr;
    EXPECT_FALSE(rs_->GetRasterData(&ncells, &rs_data)); // m_rasterData
    EXPECT_EQ(-1, ncells);
    EXPECT_EQ(nullptr, rs_data);

    float** rs_2ddata = nullptr;
    int nlyrs = -1;
    EXPECT_TRUE(rs_->Get2DRasterData(&ncells, &nlyrs, &rs_2ddata)); // m_raster2DData
    EXPECT_EQ(6, ncells);
    EXPECT_EQ(3, nlyrs);
    EXPECT_NE(nullptr, rs_2ddata);
    EXPECT_EQ(rs_2ddata[0][0], 1.);
    EXPECT_EQ(rs_2ddata[0][1], 7.);
    EXPECT_EQ(rs_2ddata[0][2], 13.);
    EXPECT_EQ(rs_2ddata[1][0], 2.);
    EXPECT_EQ(rs_2ddata[1][1], 8.);
    EXPECT_EQ(rs_2ddata[1][2], 14.);
    EXPECT_EQ(rs_2ddata[2][0], 3.);
    EXPECT_EQ(rs_2ddata[2][1], 9.);
    EXPECT_EQ(rs_2ddata[2][2], 15.);
    EXPECT_EQ(rs_2ddata[3][0], 4.);
    EXPECT_EQ(rs_2ddata[3][1], 10.);
    EXPECT_EQ(rs_2ddata[3][2], -9999.);
    EXPECT_EQ(rs_2ddata[4][0], 5.);
    EXPECT_EQ(rs_2ddata[4][1], 11.);
    EXPECT_EQ(rs_2ddata[4][2], 16.);
    EXPECT_EQ(rs_2ddata[5][0], 6.);
    EXPECT_EQ(rs_2ddata[5][1], 12.);
    EXPECT_EQ(rs_2ddata[5][2], 17.);

    // Actual data stored in memory
    EXPECT_EQ(rs_2ddata[0][0], 1.);
    EXPECT_EQ(rs_2ddata[0][1], 7.);
    EXPECT_EQ(rs_2ddata[0][2], 13.);
    EXPECT_EQ(rs_2ddata[0][3], 2.);
    EXPECT_EQ(rs_2ddata[0][4], 8.);
    EXPECT_EQ(rs_2ddata[0][5], 14.);
    EXPECT_EQ(rs_2ddata[0][6], 3.);
    EXPECT_EQ(rs_2ddata[0][7], 9.);
    EXPECT_EQ(rs_2ddata[0][8], 15.);
    EXPECT_EQ(rs_2ddata[0][9], 4.);
    EXPECT_EQ(rs_2ddata[0][10], 10.);
    EXPECT_EQ(rs_2ddata[0][11], -9999.);
    EXPECT_EQ(rs_2ddata[0][12], 5.);
    EXPECT_EQ(rs_2ddata[0][13], 11.);
    EXPECT_EQ(rs_2ddata[0][14], 16.);
    EXPECT_EQ(rs_2ddata[0][15], 6.);
    EXPECT_EQ(rs_2ddata[0][16], 12.);
    EXPECT_EQ(rs_2ddata[0][17], 17.);

    /** Get raster cell value by various way **/
    // Get cell value by index of raster_data_
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(-1));
    EXPECT_FLOAT_EQ(1.f, rs_->GetValueByIndex(0)); // by default, layer 1
    EXPECT_FLOAT_EQ(2.f, rs_->GetValueByIndex(1));
    EXPECT_FLOAT_EQ(3.f, rs_->GetValueByIndex(2));
    EXPECT_FLOAT_EQ(4.f, rs_->GetValueByIndex(3));
    EXPECT_FLOAT_EQ(5.f, rs_->GetValueByIndex(4));
    EXPECT_FLOAT_EQ(6.f, rs_->GetValueByIndex(5));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(6)); // out of bound
    EXPECT_FLOAT_EQ(7.f, rs_->GetValueByIndex(0, 2)); // layer 2
    EXPECT_FLOAT_EQ(8.f, rs_->GetValueByIndex(1, 2));
    EXPECT_FLOAT_EQ(9.f, rs_->GetValueByIndex(2, 2));
    EXPECT_FLOAT_EQ(10.f, rs_->GetValueByIndex(3, 2));
    EXPECT_FLOAT_EQ(11.f, rs_->GetValueByIndex(4, 2));
    EXPECT_FLOAT_EQ(12.f, rs_->GetValueByIndex(5, 2));
    EXPECT_FLOAT_EQ(13.f, rs_->GetValueByIndex(0, 3)); // layer 3
    EXPECT_FLOAT_EQ(14.f, rs_->GetValueByIndex(1, 3));
    EXPECT_FLOAT_EQ(15.f, rs_->GetValueByIndex(2, 3));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(3, 3));
    EXPECT_FLOAT_EQ(16.f, rs_->GetValueByIndex(4, 3));
    EXPECT_FLOAT_EQ(17.f, rs_->GetValueByIndex(5, 3));

    /// Get cell value by index of raster_data_/raster_2d_, compatible way
    int tmp_lyr = rs_->GetLayers();
    float* tmp_values = nullptr;
    Initialize1DArray(tmp_lyr, tmp_values, -9999.f);
    rs_->GetValueByIndex(-1, tmp_values); // index exceed, tmp_values will be released
    EXPECT_EQ(nullptr, tmp_values);
    rs_->GetValueByIndex(0, tmp_values); // nullptr input, tmp_values will be initialized
    EXPECT_NE(nullptr, tmp_values);
    EXPECT_FLOAT_EQ(1.f, tmp_values[0]);
    EXPECT_FLOAT_EQ(7.f, tmp_values[1]);
    EXPECT_FLOAT_EQ(13.f, tmp_values[2]);
    rs_->GetValueByIndex(3, tmp_values);
    EXPECT_NE(nullptr, tmp_values);
    EXPECT_FLOAT_EQ(4.f, tmp_values[0]);
    EXPECT_FLOAT_EQ(10.f, tmp_values[1]);
    EXPECT_FLOAT_EQ(-9999.f, tmp_values[2]);

    /// Get cell value by (row, col)
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(-1, 0));
    EXPECT_FLOAT_EQ(1.f, rs_->GetValue(0, 0)); // by default, layer 1
    EXPECT_FLOAT_EQ(2.f, rs_->GetValue(0, 1));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(0, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(1, 0));
    EXPECT_FLOAT_EQ(3.f, rs_->GetValue(1, 1));
    EXPECT_FLOAT_EQ(4.f, rs_->GetValue(1, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(2, 0));
    EXPECT_FLOAT_EQ(5.f, rs_->GetValue(2, 1));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(2, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(3, 0));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(3, 1));
    EXPECT_FLOAT_EQ(6.f, rs_->GetValue(3, 2));
    EXPECT_FLOAT_EQ(6.f, rs_->GetValue(3, 2, 1));
    EXPECT_FLOAT_EQ(7.f, rs_->GetValue(0, 0, 2)); // layer 2
    EXPECT_FLOAT_EQ(8.f, rs_->GetValue(0, 1, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(0, 2, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(1, 0, 2));
    EXPECT_FLOAT_EQ(9.f, rs_->GetValue(1, 1, 2));
    EXPECT_FLOAT_EQ(10.f, rs_->GetValue(1, 2, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(2, 0, 2));
    EXPECT_FLOAT_EQ(11.f, rs_->GetValue(2, 1, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(2, 2, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(3, 0, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(3, 1, 2));
    EXPECT_FLOAT_EQ(12.f, rs_->GetValue(3, 2, 2));
    EXPECT_FLOAT_EQ(13.f, rs_->GetValue(0, 0, 3)); // layer 3
    EXPECT_FLOAT_EQ(14.f, rs_->GetValue(0, 1, 3));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(0, 2, 3));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(1, 0, 3));
    EXPECT_FLOAT_EQ(15.f, rs_->GetValue(1, 1, 3));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(1, 2, 3));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(2, 0, 3));
    EXPECT_FLOAT_EQ(16.f, rs_->GetValue(2, 1, 3));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(2, 2, 3));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(3, 0, 3));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(3, 1, 3));
    EXPECT_FLOAT_EQ(17.f, rs_->GetValue(3, 2, 3));

    /// Get cell values by (row, col, *&array)
    rs_->GetValue(-1, 0, tmp_values);
    EXPECT_EQ(nullptr, tmp_values);
    rs_->GetValue(0, -1, tmp_values);
    EXPECT_EQ(nullptr, tmp_values);
    rs_->GetValue(0, 0, tmp_values);
    EXPECT_NE(nullptr, tmp_values);
    EXPECT_FLOAT_EQ(1.f, tmp_values[0]);
    EXPECT_FLOAT_EQ(7.f, tmp_values[1]);
    EXPECT_FLOAT_EQ(13.f, tmp_values[2]);
    rs_->GetValue(1, 2, tmp_values);
    EXPECT_NE(nullptr, tmp_values);
    EXPECT_FLOAT_EQ(4.f, tmp_values[0]);
    EXPECT_FLOAT_EQ(10.f, tmp_values[1]);
    EXPECT_FLOAT_EQ(-9999.f, tmp_values[2]);
    rs_->GetValue(1, 0, tmp_values);
    EXPECT_NE(nullptr, tmp_values);
    EXPECT_FLOAT_EQ(-9999.f, tmp_values[0]);
    EXPECT_FLOAT_EQ(-9999.f, tmp_values[1]);
    EXPECT_FLOAT_EQ(-9999.f, tmp_values[2]);

    Release1DArray(tmp_values);
    EXPECT_EQ(nullptr, tmp_values);

    // Get position index, if cal_poc is False, no chance to return -1
    EXPECT_EQ(-2, rs_->GetPosition(-1., -2.));
    EXPECT_EQ(-2, rs_->GetPosition(-1., 9.));
    EXPECT_EQ(0, rs_->GetPosition(1., 7.));
    EXPECT_EQ(1, rs_->GetPosition(3.99, 7.05));
    EXPECT_EQ(-1, rs_->GetPosition(5.01, 7.00));
    EXPECT_EQ(-1, rs_->GetPosition(0.01, 5.5));
    EXPECT_EQ(2, rs_->GetPosition(2.01, 5.1));
    EXPECT_EQ(3, rs_->GetPosition(4.5, 5.5));
    EXPECT_EQ(-1, rs_->GetPosition(1., 3.));
    EXPECT_EQ(4, rs_->GetPosition(3., 3.));
    EXPECT_EQ(-1, rs_->GetPosition(5., 3.));
    EXPECT_EQ(-1, rs_->GetPosition(1., 1.));
    EXPECT_EQ(-1, rs_->GetPosition(3., 1.));
    EXPECT_EQ(5, rs_->GetPosition(5., 1.));

    /** Set value **/
    // Set core file name
    string newcorename = Corename + "_2D-nomask-calcpos";
    rs_->SetCoreName(newcorename);
    EXPECT_EQ(newcorename, rs_->GetCoreName());

    // Set raster data value
    rs_->SetValue(3, 1, 6.f, 1); // change nodata failed
    rs_->SetValue(3, 2, -9999.f); // but can change valid value to nodata
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(3, 1));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(3, 2));

    // update statistics
    rs_->UpdateStatistics(); // Should be manually invoked in your project!
    EXPECT_DOUBLE_EQ(1., rs_->GetMinimum()); // layer 1
    EXPECT_DOUBLE_EQ(5., rs_->GetMaximum());
    EXPECT_DOUBLE_EQ(3., rs_->GetAverage());
    EXPECT_DOUBLE_EQ(1.4142135623730951, rs_->GetStd());
    EXPECT_DOUBLE_EQ(4., rs_->GetRange());

    /** Output to new file **/
    string oldfullname = rs_->GetFilePath();
    string newfullname = GetPathFromFullName(oldfullname) + "result" + SEP +
            newcorename + "." + GetSuffix(oldfullname);
    EXPECT_TRUE(rs_->OutputToFile(newfullname));

    // Change values back
    rs_->SetValue(3, 2, 6.f);
    EXPECT_FLOAT_EQ(6.f, rs_->GetValue(3, 2));

#ifdef USE_MONGODB
    /** MongoDB I/O test **/
    MongoGridFs* gfs_ = GlobalEnv->gfs_;
    EXPECT_NE(nullptr, gfs_);
    string gfsfilename = newcorename + "_" + GetSuffix(oldfullname);
    string gfsfilename_valid = newcorename + "_valid_" + GetSuffix(oldfullname);
    gfs_->RemoveFile(gfsfilename); // remove if already existed
    gfs_->RemoveFile(gfsfilename_valid);
    EXPECT_TRUE(rs_->OutputToMongoDB(gfs_, gfsfilename)); // store fullsize raster
    map<string, string> additional_header;
    additional_header[HEADER_MASK_NAME] = rs_->GetCoreName();
    EXPECT_TRUE(rs_->OutputToMongoDB(gfs_, gfsfilename_valid, additional_header, false));

    if (HasFailure()) { return; }

    // Read from MongoDB without mask layer
    FltRaster* mongors = FltRaster::Init(gfs_, gfsfilename.c_str(), false);

    // Read from MongoDB with predefined valid positions
    FltRaster* mongors_valid = new FltRaster(); // create empty raster, set and read data
    mongors_valid->SetHeader(rs_->GetRasterHeader()); // set header
    int** posdata;
    int* posidx;
    int poslen;
    rs_->GetRasterPositionData(&poslen, &posdata);
    mongors_valid->SetPositions(poslen, posdata);
    rs_->GetRasterPositionData(&poslen, &posidx);
    mongors_valid->SetPositions(poslen, posidx);
    STRING_MAP opts;
    UpdateStringMap(opts, HEADER_INC_NODATA, "FALSE");
    mongors_valid->ReadFromMongoDB(gfs_, gfsfilename_valid, false, nullptr, true, NODATA_VALUE, opts);

    // Check the consistency of mongors and mongors_valid
    EXPECT_NE(mongors->GetCellNumber(), mongors_valid->GetCellNumber());
    EXPECT_NE(mongors->GetDataLength(), mongors_valid->GetDataLength());
    EXPECT_EQ(mongors->GetValidNumber(), mongors_valid->GetValidNumber());
    EXPECT_DOUBLE_EQ(mongors->GetAverage(), mongors_valid->GetAverage());
    EXPECT_DOUBLE_EQ(mongors->GetStd(), mongors_valid->GetStd());
    EXPECT_EQ(mongors->GetLayers(), 3);
    EXPECT_EQ(mongors->GetLayers(), mongors_valid->GetLayers());
    EXPECT_EQ(mongors->GetRows(), mongors_valid->GetRows());
    EXPECT_EQ(mongors->GetCols(), mongors_valid->GetCols());
    for (int ir = 0; ir < mongors->GetRows(); ir++) {
        for (int ic = 0; ic < mongors->GetCols(); ic++) {
            for (int il = 0; il < mongors->GetLayers(); il++) {
                EXPECT_FLOAT_EQ(mongors->GetValue(ir, ic, il), mongors_valid->GetValue(ir, ic, il));
            }
        }
    }
    delete mongors;
    delete mongors_valid;
#endif
}
// In order to run value-parameterized tests, you need to instantiate them,
// or bind them to a list of values which will be used as test parameters.
// You can instantiate them in a different translation module, or even
// instantiate them several times.
#ifdef USE_GDAL
INSTANTIATE_TEST_CASE_P(MultipleLayers, clsRasterDataTest2DNoMask,
                        Values(new InputRasterFiles(rs1_asc, rs2_asc, rs3_asc),
                            new InputRasterFiles(rs1_tif, rs2_tif, rs3_tif)),);
#else
INSTANTIATE_TEST_CASE_P(MultipleLayers, clsRasterDataTest2DNoMask,
                        Values(new InputRasterFiles(rs1_asc, rs2_asc, rs3_asc)),);
#endif /* USE_GDAL */

} /* namespace */
