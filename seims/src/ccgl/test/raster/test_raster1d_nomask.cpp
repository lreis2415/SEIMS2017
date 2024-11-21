/*!
 * \brief Test description:
 *
 *        TEST CASE NAME (or TEST SUITE):
 *            clsRasterDataTestNoMask: Read raster without a mask
 *
 *        Since we mainly support ASC and GDAL(e.g., TIFF),
 *        value-parameterized tests of Google Test will be used.
 * \cite https://github.com/google/googletest/blob/master/googletest/samples/sample7_unittest.cc
 * \version 1.3
 * \authors Liangjun Zhu, zlj(at)lreis.ac.cn; crazyzlj(at)gmail.com
 * \remarks 2017-12-02 - lj - Original version.
 *          2018-05-03 - lj - Integrated into CCGL.
 *          2021-07-20 - lj - Update after changes of GetValue and GetValueByIndex.
 *          2021-11-18 - lj - Rewrite unittest cases, avoid redundancy. 
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

string Apppath = GetAppPath();
string Corename = "tinydemo_raster_r4c3";
string AscFile = Apppath + "./data/raster/" + Corename + ".asc";
string TifFile = Apppath + "./data/raster/" + Corename + ".tif";
const char* AscFileChars = AscFile.c_str();
const char* TifFileChars = TifFile.c_str();

//Inside the test body, fixture constructor, SetUp(), and TearDown() you
//can refer to the test parameter by GetParam().  In this case, the test
//parameter is a factory function which we call in fixture's SetUp() to
//create and store an instance of clsRasterData<float>.
class clsRasterDataTestNoMask: public TestWithParam<const char *> {
public:
    clsRasterDataTestNoMask() : rs_(nullptr) {
    }

    virtual ~clsRasterDataTestNoMask() { ; }

    void SetUp() OVERRIDE {
        rs_ = FltRaster::Init(GetParam(), false);
        ASSERT_NE(nullptr, rs_);
    }

    void TearDown() OVERRIDE {
        delete rs_;
    }

protected:
    FltRaster* rs_;
};

// calc_pos = False
TEST_P(clsRasterDataTestNoMask, RasterIO) {
    /// 1. Test members after constructing.
    EXPECT_EQ(12, rs_->GetDataLength()); // m_nCells, which will be nRows * nCols
    EXPECT_EQ(12, rs_->GetCellNumber()); // m_nCells

    EXPECT_EQ(-9999., rs_->GetNoDataValue());  // m_noDataValue
    EXPECT_EQ(-9999., rs_->GetDefaultValue()); // m_defaultValue

    // m_filePathName depends on the path of build, so no need to test.
    EXPECT_EQ(Corename, rs_->GetCoreName()); // m_coreFileName

    EXPECT_TRUE(rs_->Initialized());           // m_initialized
    EXPECT_FALSE(rs_->Is2DRaster());           // m_is2DRaster
    EXPECT_FALSE(rs_->PositionsCalculated());  // m_calcPositions
    EXPECT_FALSE(rs_->PositionsAllocated());   // m_storePositions
    EXPECT_FALSE(rs_->MaskExtented());         // m_useMaskExtent
    EXPECT_FALSE(rs_->StatisticsCalculated()); // m_statisticsCalculated

    ASSERT_TRUE(rs_->ValidateRasterData());

    EXPECT_NE(nullptr, rs_->GetRasterDataPointer());         // m_rasterData
    EXPECT_EQ(nullptr, rs_->Get2DRasterDataPointer());       // m_raster2DData
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
    EXPECT_EQ(1, rs_->GetLayers());
    EXPECT_EQ("", rs_->GetSrsString());

    /** Calc and get basic statistics, m_statsMap **/
    EXPECT_EQ(6, rs_->GetValidNumber());
    EXPECT_DOUBLE_EQ(1., rs_->GetMinimum());
    EXPECT_DOUBLE_EQ(6., rs_->GetMaximum());
    EXPECT_DOUBLE_EQ(3.5, rs_->GetAverage());
    EXPECT_DOUBLE_EQ(1.707825127659933, rs_->GetStd());
    EXPECT_DOUBLE_EQ(5., rs_->GetRange());
    EXPECT_TRUE(rs_->StatisticsCalculated());

    EXPECT_EQ(nullptr, rs_->GetMask()); // m_mask

    /** Test getting raster data **/
    int ncells = 0;
    float* rs_data = nullptr;
    EXPECT_TRUE(rs_->GetRasterData(&ncells, &rs_data)); // m_rasterData
    EXPECT_EQ(12, ncells);
    EXPECT_NE(nullptr, rs_data);
    EXPECT_FLOAT_EQ(1.f, rs_data[0]);
    EXPECT_FLOAT_EQ(2.f, rs_data[1]);
    EXPECT_FLOAT_EQ(-9999.f, rs_data[2]);
    EXPECT_FLOAT_EQ(-9999.f, rs_data[3]);
    EXPECT_FLOAT_EQ(3.f, rs_data[4]);
    EXPECT_FLOAT_EQ(4.f, rs_data[5]);
    EXPECT_FLOAT_EQ(-9999.f, rs_data[6]);
    EXPECT_FLOAT_EQ(5.f, rs_data[7]);
    EXPECT_FLOAT_EQ(-9999.f, rs_data[8]);
    EXPECT_FLOAT_EQ(-9999.f, rs_data[9]);
    EXPECT_FLOAT_EQ(-9999.f, rs_data[10]);
    EXPECT_FLOAT_EQ(6.f, rs_data[11]);

    float** rs_2ddata = nullptr;
    int nlyrs = -1;
    EXPECT_FALSE(rs_->Get2DRasterData(&ncells, &nlyrs, &rs_2ddata)); // m_raster2DData
    EXPECT_EQ(-1, ncells);
    EXPECT_EQ(-1, nlyrs);
    EXPECT_EQ(nullptr, rs_2ddata);

    /** Get raster cell value by various way **/
    /// Get cell value by index of raster_data_
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(-1));
    EXPECT_FLOAT_EQ(1.f, rs_->GetValueByIndex(0));
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
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(12));

    /// Get cell value by index of raster_data_/raster_2d_, compatible way
    int tmp_lyr = rs_->GetLayers();
    float* tmp_values = nullptr;
    Initialize1DArray(tmp_lyr, tmp_values, -9999.f);
    rs_->GetValueByIndex(-1, tmp_values); // index exceed, tmp_values will be released
    EXPECT_EQ(nullptr, tmp_values);
    rs_->GetValueByIndex(0, tmp_values); // nullptr input, tmp_values will be initialized
    EXPECT_NE(nullptr, tmp_values);
    EXPECT_FLOAT_EQ(1.f, tmp_values[0]);

    /// Get cell value by (row, col)
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(-1, 0));
    EXPECT_FLOAT_EQ(1.f, rs_->GetValue(0, 0));
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
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(3, 2, -1));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(3, 2, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(3, 3));

    /// Get cell values by (row, col, *&array)
    rs_->GetValue(-1, 0, tmp_values);
    EXPECT_EQ(nullptr, tmp_values);
    rs_->GetValue(0, -1, tmp_values);
    EXPECT_EQ(nullptr, tmp_values);
    rs_->GetValue(0, 0, tmp_values);
    EXPECT_NE(nullptr, tmp_values);
    EXPECT_FLOAT_EQ(1.f, tmp_values[0]);
    rs_->GetValue(0, 1, tmp_values);
    EXPECT_NE(nullptr, tmp_values);
    EXPECT_FLOAT_EQ(2.f, tmp_values[0]);

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
    string newcorename = Corename + "_1D-nomask-nopos";
    rs_->SetCoreName(newcorename);
    EXPECT_EQ(newcorename, rs_->GetCoreName());

    // Set raster data value
    rs_->SetValue(3, 2, 9.f);
    EXPECT_FLOAT_EQ(9.f, rs_->GetValue(3, 2));

    // update statistics
    rs_->UpdateStatistics(); // Should be manually invoked in your project!
    EXPECT_DOUBLE_EQ(1., rs_->GetMinimum());
    EXPECT_DOUBLE_EQ(9., rs_->GetMaximum());
    EXPECT_DOUBLE_EQ(4., rs_->GetAverage());
    EXPECT_DOUBLE_EQ(2.581988897471611, rs_->GetStd());
    EXPECT_DOUBLE_EQ(8., rs_->GetRange());

    rs_->SetValue(1, 2, 10); // allow to change nodata
    EXPECT_FLOAT_EQ(10.f, rs_->GetValue(1, 2));

    /** Output to new file **/
    string oldfullname = rs_->GetFilePath();
    string newfullname = GetPathFromFullName(oldfullname) + "result" + SEP +
            newcorename + "." + GetSuffix(oldfullname);
    EXPECT_TRUE(rs_->OutputToFile(newfullname));
    EXPECT_TRUE(FileExists(newfullname));

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
    EXPECT_EQ(mongors->GetLayers(), mongors_valid->GetLayers());
    EXPECT_EQ(mongors->GetRows(), mongors_valid->GetRows());
    EXPECT_EQ(mongors->GetCols(), mongors_valid->GetCols());
    for (int ir = 0; ir < mongors->GetRows(); ir++) {
        for (int ic = 0; ic < mongors->GetCols(); ic++) {
            EXPECT_FLOAT_EQ(mongors->GetValue(ir, ic), mongors_valid->GetValue(ir, ic));
        }
    }
    delete mongors;
    delete mongors_valid;
#endif
}

// calc_pos = True
TEST_P(clsRasterDataTestNoMask, RasterIOWithCalcPos) {
     /* Get position data, which will be calculated if not existed */
    EXPECT_FALSE(rs_->PositionsCalculated());
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
    EXPECT_EQ(6, rs_->GetDataLength()); // m_nCells, which will be nRows * nCols
    EXPECT_EQ(6, rs_->GetCellNumber()); // m_nCells

    EXPECT_EQ(-9999., rs_->GetNoDataValue());  // m_noDataValue
    EXPECT_EQ(-9999., rs_->GetDefaultValue()); // m_defaultValue

    // m_filePathName depends on the path of build, so no need to test.
    EXPECT_EQ(Corename, rs_->GetCoreName()); // m_coreFileName

    EXPECT_TRUE(rs_->Initialized());           // m_initialized
    EXPECT_FALSE(rs_->Is2DRaster());           // m_is2DRaster
    EXPECT_TRUE(rs_->PositionsCalculated());   // m_calcPositions
    EXPECT_TRUE(rs_->PositionsAllocated());    // m_storePositions
    EXPECT_FALSE(rs_->MaskExtented());         // m_useMaskExtent
    EXPECT_FALSE(rs_->StatisticsCalculated()); // m_statisticsCalculated

    ASSERT_TRUE(rs_->ValidateRasterData());

    EXPECT_NE(nullptr, rs_->GetRasterDataPointer());         // m_rasterData
    EXPECT_EQ(nullptr, rs_->Get2DRasterDataPointer());       // m_raster2DData
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
    EXPECT_EQ(1, rs_->GetLayers());
    EXPECT_EQ("", rs_->GetSrsString());

    /** Calc and get basic statistics, m_statsMap **/
    EXPECT_EQ(6, rs_->GetValidNumber());
    EXPECT_DOUBLE_EQ(1., rs_->GetMinimum());
    EXPECT_DOUBLE_EQ(6., rs_->GetMaximum());
    EXPECT_DOUBLE_EQ(3.5, rs_->GetAverage());
    EXPECT_DOUBLE_EQ(1.707825127659933, rs_->GetStd());
    EXPECT_DOUBLE_EQ(5., rs_->GetRange());
    EXPECT_TRUE(rs_->StatisticsCalculated());

    EXPECT_EQ(nullptr, rs_->GetMask()); // m_mask

    /** Test getting raster data **/
    ncells = 0;
    float* rs_data = nullptr;
    EXPECT_TRUE(rs_->GetRasterData(&ncells, &rs_data)); // m_rasterData
    EXPECT_EQ(6, ncells);
    EXPECT_NE(nullptr, rs_data);
    EXPECT_FLOAT_EQ(1.f, rs_data[0]);
    EXPECT_FLOAT_EQ(2.f, rs_data[1]);
    EXPECT_FLOAT_EQ(3.f, rs_data[2]);
    EXPECT_FLOAT_EQ(4.f, rs_data[3]);
    EXPECT_FLOAT_EQ(5.f, rs_data[4]);
    EXPECT_FLOAT_EQ(6.f, rs_data[5]);

    float** rs_2ddata = nullptr;
    int nlyrs = -1;
    EXPECT_FALSE(rs_->Get2DRasterData(&ncells, &nlyrs, &rs_2ddata)); // m_raster2DData
    EXPECT_EQ(-1, ncells);
    EXPECT_EQ(-1, nlyrs);
    EXPECT_EQ(nullptr, rs_2ddata);

    /** Get raster cell value by various way **/
    /// Get cell value by index of raster_data_
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(-1));
    EXPECT_FLOAT_EQ(1.f, rs_->GetValueByIndex(0));
    EXPECT_FLOAT_EQ(2.f, rs_->GetValueByIndex(1));
    EXPECT_FLOAT_EQ(3.f, rs_->GetValueByIndex(2));
    EXPECT_FLOAT_EQ(4.f, rs_->GetValueByIndex(3));
    EXPECT_FLOAT_EQ(5.f, rs_->GetValueByIndex(4));
    EXPECT_FLOAT_EQ(6.f, rs_->GetValueByIndex(5));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(6));

    /// Get cell value by index of raster_data_/raster_2d_, compatible way
    int tmp_lyr = rs_->GetLayers();
    float* tmp_values = nullptr;
    Initialize1DArray(tmp_lyr, tmp_values, -9999.f);
    rs_->GetValueByIndex(-1, tmp_values); // index exceed, tmp_values will be released
    EXPECT_EQ(nullptr, tmp_values);
    rs_->GetValueByIndex(0, tmp_values); // nullptr input, tmp_values will be initialized
    EXPECT_NE(nullptr, tmp_values);
    EXPECT_FLOAT_EQ(1.f, tmp_values[0]);

    /// Get cell value by (row, col)
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(-1, 0));
    EXPECT_FLOAT_EQ(1.f, rs_->GetValue(0, 0));
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
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(3, 2, -1));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(3, 2, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(3, 3));

    /// Get cell values by (row, col, *&array)
    rs_->GetValue(-1, 0, tmp_values);
    EXPECT_EQ(nullptr, tmp_values);
    rs_->GetValue(0, -1, tmp_values);
    EXPECT_EQ(nullptr, tmp_values);
    rs_->GetValue(0, 0, tmp_values);
    EXPECT_NE(nullptr, tmp_values);
    EXPECT_FLOAT_EQ(1.f, tmp_values[0]);
    rs_->GetValue(0, 1, tmp_values);
    EXPECT_NE(nullptr, tmp_values);
    EXPECT_FLOAT_EQ(2.f, tmp_values[0]);

    Release1DArray(tmp_values);
    EXPECT_EQ(nullptr, tmp_values);

    // Get position index
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
    string newcorename = Corename + "_1D-nomask-calcpos";
    rs_->SetCoreName(newcorename);
    EXPECT_EQ(newcorename, rs_->GetCoreName());

    // Set raster data value
    rs_->SetValue(3, 2, 9.f);
    EXPECT_FLOAT_EQ(9.f, rs_->GetValue(3, 2));

    // update statistics
    rs_->UpdateStatistics(); // Should be manually invoked in your project!
    EXPECT_DOUBLE_EQ(1., rs_->GetMinimum());
    EXPECT_DOUBLE_EQ(9., rs_->GetMaximum());
    EXPECT_DOUBLE_EQ(4., rs_->GetAverage());
    EXPECT_DOUBLE_EQ(2.581988897471611, rs_->GetStd());
    EXPECT_DOUBLE_EQ(8., rs_->GetRange());

    rs_->SetValue(0, 2, 10); // NO NOT allow to change nodata
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(0, 2));

    /** Output to new file **/
    string oldfullname = rs_->GetFilePath();
    string newfullname = GetPathFromFullName(oldfullname) + "result" + SEP +
            newcorename + "." + GetSuffix(oldfullname);
    EXPECT_TRUE(rs_->OutputToFile(newfullname));
    EXPECT_TRUE(FileExists(newfullname));


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
    EXPECT_EQ(mongors->GetLayers(), mongors_valid->GetLayers());
    EXPECT_EQ(mongors->GetRows(), mongors_valid->GetRows());
    EXPECT_EQ(mongors->GetCols(), mongors_valid->GetCols());
    for (int ir = 0; ir < mongors->GetRows(); ir++) {
        for (int ic = 0; ic < mongors->GetCols(); ic++) {
            EXPECT_FLOAT_EQ(mongors->GetValue(ir, ic), mongors_valid->GetValue(ir, ic));
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
INSTANTIATE_TEST_CASE_P(SingleLayer, clsRasterDataTestNoMask,
                        Values(AscFileChars, TifFileChars),);
#else
INSTANTIATE_TEST_CASE_P(SingleLayer, clsRasterDataTestNoMask,
                        Values(AscFileChars),);
#endif /* USE_GDAL */

} /* namespace */
