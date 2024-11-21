/*!
 * \brief Test description:
 *        
 *        TEST CASE NAME (or TEST SUITE):
 *            clsRasterDataTestMask2D: Read 2D raster with a mask that within raster's extent.
 *
 *            Once a raster used as a mask, its position data must be existed or recalculated.
 *
 *        P.S.1. Copy constructor is also tested here.
 *        P.S.2. MongoDB I/O is also tested if mongo-c-driver configured.
 *
 *        Since we mainly support ASC and GDAL(e.g., TIFF),
 *        value-parameterized tests of Google Test will be used.
 * \cite https://github.com/google/googletest/blob/master/googletest/samples/sample7_unittest.cc
 * \version 1.3
 * \authors Liangjun Zhu, zlj(at)lreis.ac.cn; crazyzlj(at)gmail.com
 * \remarks 2017-12-02 - lj - Original version.
 *          2018-05-03 - lj - Integrated into CCGL.
 *          2019-11-06 - lj - Allow user specified MongoDB host and port.
 *
 */
#include "gtest/gtest.h"
#include <vector>

#include "../../src/data_raster.hpp"
#include "../../src/utils_time.h"
#include "../../src/utils_string.h"
#include "../../src/utils_filesystem.h"
#ifdef USE_MONGODB
#include "../../src/db_mongoc.h"
#endif
#include "../test_global.h"

using namespace ccgl;
using namespace ccgl::data_raster;
using namespace ccgl::utils_filesystem;
using namespace ccgl::utils_time;
using namespace ccgl::utils_string;
using namespace ccgl::utils_array;
using std::vector;
#ifdef USE_MONGODB
using namespace ccgl::db_mongoc;
#endif

extern GlobalEnvironment* GlobalEnv;

namespace {
using ::testing::TestWithParam;
using ::testing::Values;

string Rspath = GetAppPath()+ "./data/raster/";
string Dstpath = Rspath + "result/";

string maskname = "tinydemo_mask_r2c2";
string corename = "tinydemo_raster_r4c3";

string rs1_asc = Rspath + corename + ".asc";
string rs2_asc = Rspath + corename + "_2.asc";
string rs3_asc = Rspath + corename + "_3.asc";

string rs1_tif = Rspath + corename + ".tif";
string rs2_tif = Rspath + corename + "_2.tif";
string rs3_tif = Rspath + corename + "_3.tif";

string mask_asc_file = Rspath + maskname + ".asc";
string mask_tif_file = Rspath + maskname + ".tif";

struct InputRasterFiles {
public:
    InputRasterFiles(const string& rs1, const string& rs2,
                     const string& rs3, const string& maskf) {
        raster_name1 = rs1.c_str();
        raster_name2 = rs2.c_str();
        raster_name3 = rs3.c_str();
        mask_name = maskf.c_str();
    }

    ~InputRasterFiles() { ; }

    const char* raster_name1;
    const char* raster_name2;
    const char* raster_name3;
    const char* mask_name;
};

//Inside the test body, fixture constructor, SetUp(), and TearDown() you
//can refer to the test parameter by GetParam().  In this case, the test
//parameter is a factory function which we call in fixture's SetUp() to
//create and store an instance of clsRasterData<float>.
class clsRasterDataTestMask2D: public TestWithParam<InputRasterFiles *> {
public:
    clsRasterDataTestMask2D() : rs_(nullptr), maskrs_(nullptr) {
    }

    virtual ~clsRasterDataTestMask2D() { ; }

    void SetUp() OVERRIDE {
        // Read mask data with default parameters, i.e., calculate valid positions.
        maskrs_ = IntRaster::Init(GetParam()->mask_name);
        ASSERT_NE(nullptr, maskrs_);
        // Read raster data with the masked data
        vector<string> filenames;
        filenames.emplace_back(GetParam()->raster_name1);
        filenames.emplace_back(GetParam()->raster_name2);
        filenames.emplace_back(GetParam()->raster_name3);

        rs_ = FltIntRaster::Init(filenames, false, maskrs_);
        ASSERT_NE(nullptr, rs_);
    }

    void TearDown() OVERRIDE {
        delete rs_;
        delete maskrs_;
    }

protected:
    FltIntRaster* rs_;
    IntRaster* maskrs_;
};

// calc_pos = False, use_mask_ext can be true or false
TEST_P(clsRasterDataTestMask2D, NoPos) {
    /// Mask data
    EXPECT_TRUE(maskrs_->PositionsCalculated());
    EXPECT_EQ(2, maskrs_->GetValidNumber());
    EXPECT_EQ(2, maskrs_->GetCellNumber());
    EXPECT_EQ(2, maskrs_->GetDataLength());

    EXPECT_EQ(-9999., maskrs_->GetValueByIndex(-1));
    EXPECT_EQ(1, maskrs_->GetValueByIndex(0));
    EXPECT_EQ(1, maskrs_->GetValueByIndex(1));
    EXPECT_EQ(-9999, maskrs_->GetValueByIndex(2));
    EXPECT_EQ(-9999, maskrs_->GetValueByIndex(3));

    /// Test members after constructing.
    EXPECT_EQ(6, rs_->GetDataLength()); // m_nCells * n_lyrs_
    EXPECT_EQ(2, rs_->GetCellNumber());  // m_nCells
    
    EXPECT_EQ(-9999., rs_->GetNoDataValue());  // m_noDataValue
    EXPECT_EQ(-9999., rs_->GetDefaultValue()); // m_defaultValue
    
    EXPECT_TRUE(rs_->Initialized());           // m_initialized
    EXPECT_TRUE(rs_->Is2DRaster());            // m_is2DRaster
    EXPECT_TRUE(rs_->PositionsCalculated());   // m_calcPositions, force set calc_pos=true
    EXPECT_FALSE(rs_->PositionsAllocated());   // m_storePositions
    EXPECT_TRUE(rs_->MaskExtented());          // m_useMaskExtent
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

    EXPECT_EQ(2, rs_->GetRows());
    EXPECT_EQ(2, rs_->GetCols());
    EXPECT_DOUBLE_EQ(3., rs_->GetXllCenter());
    EXPECT_DOUBLE_EQ(3., rs_->GetYllCenter());
    EXPECT_DOUBLE_EQ(2., rs_->GetCellWidth());
    EXPECT_EQ(3, rs_->GetLayers());
    EXPECT_EQ("", rs_->GetSrsString());

    /** Calc and get basic statistics, m_statsMap **/
    EXPECT_EQ(2, rs_->GetValidNumber());
    EXPECT_DOUBLE_EQ(4., rs_->GetMinimum()); // by default, layer 1
    EXPECT_DOUBLE_EQ(5., rs_->GetMaximum());
    EXPECT_DOUBLE_EQ(4.5, rs_->GetAverage());
    EXPECT_DOUBLE_EQ(0.5, rs_->GetStd());
    EXPECT_DOUBLE_EQ(1., rs_->GetRange());
    EXPECT_TRUE(rs_->StatisticsCalculated());
    EXPECT_DOUBLE_EQ(10., rs_->GetMinimum(2)); // layer 2
    EXPECT_DOUBLE_EQ(11., rs_->GetMaximum(2));
    EXPECT_DOUBLE_EQ(10.5, rs_->GetAverage(2));
    EXPECT_DOUBLE_EQ(0.5, rs_->GetStd(2));
    EXPECT_DOUBLE_EQ(1., rs_->GetRange(2));
    EXPECT_DOUBLE_EQ(16., rs_->GetMinimum(3)); // layer 3
    EXPECT_DOUBLE_EQ(16., rs_->GetMaximum(3));
    EXPECT_DOUBLE_EQ(16., rs_->GetAverage(3));
    EXPECT_DOUBLE_EQ(0., rs_->GetStd(3));
    EXPECT_DOUBLE_EQ(0., rs_->GetRange(3));

    EXPECT_NE(nullptr, rs_->GetMask()); // m_mask

    /** Test getting raster data **/
    int ncells = 0;
    float* rs_data = nullptr;
    EXPECT_FALSE(rs_->GetRasterData(&ncells, &rs_data));
    EXPECT_EQ(-1, ncells);
    EXPECT_EQ(nullptr, rs_data);

    float** rs_2ddata = nullptr;
    int nlyrs = -1;
    EXPECT_TRUE(rs_->Get2DRasterData(&ncells, &nlyrs, &rs_2ddata)); // m_raster2DData
    EXPECT_EQ(2, ncells);
    EXPECT_EQ(3, nlyrs);
    EXPECT_NE(nullptr, rs_2ddata);
    EXPECT_FLOAT_EQ(4.f, rs_2ddata[0][0]);
    EXPECT_FLOAT_EQ(10.f, rs_2ddata[0][1]);
    EXPECT_FLOAT_EQ(-9999.f, rs_2ddata[0][2]);
    EXPECT_FLOAT_EQ(5.f, rs_2ddata[1][0]);
    EXPECT_FLOAT_EQ(11.f, rs_2ddata[1][1]);
    EXPECT_FLOAT_EQ(16.f, rs_2ddata[1][2]);

    /** Get raster cell value by various way **/
    // Get cell value by index of raster_data_
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(-1));
    EXPECT_FLOAT_EQ(4.f, rs_->GetValueByIndex(0)); // by default, layer 1
    EXPECT_FLOAT_EQ(5.f, rs_->GetValueByIndex(1));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(3));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(4));
    EXPECT_FLOAT_EQ(10.f, rs_->GetValueByIndex(0, 2)); // layer 2
    EXPECT_FLOAT_EQ(11.f, rs_->GetValueByIndex(1, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(2, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(3, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(4, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(0, 3)); // layer 3
    EXPECT_FLOAT_EQ(16.f, rs_->GetValueByIndex(1, 3));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(2, 3));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(3, 3));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(4, 3));

    // Get cell value by index of raster_data_/raster_2d_, compatible way
    int tmp_lyr = rs_->GetLayers();
    float* tmp_values = nullptr;
    Initialize1DArray(tmp_lyr, tmp_values, -9999.f);
    rs_->GetValueByIndex(-1, tmp_values); // index exceed, tmp_values will be released
    EXPECT_EQ(nullptr, tmp_values);
    rs_->GetValueByIndex(0, tmp_values); // nullptr input, tmp_values will be initialized
    EXPECT_NE(nullptr, tmp_values);
    EXPECT_FLOAT_EQ(4.f, tmp_values[0]);
    EXPECT_FLOAT_EQ(10.f, tmp_values[1]);
    EXPECT_FLOAT_EQ(-9999.f, tmp_values[2]);
    rs_->GetValueByIndex(1, tmp_values);
    EXPECT_FLOAT_EQ(5.f, tmp_values[0]);
    EXPECT_FLOAT_EQ(11.f, tmp_values[1]);
    EXPECT_FLOAT_EQ(16.f, tmp_values[2]);
    rs_->GetValueByIndex(2, tmp_values);
    EXPECT_EQ(nullptr, tmp_values);

    // Get cell value by (row, col)
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(-1, 0));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(0, 0)); // by default, layer 1
    EXPECT_FLOAT_EQ(4.f, rs_->GetValue(0, 1));
    EXPECT_FLOAT_EQ(5.f, rs_->GetValue(1, 0));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(1, 1));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(0, 0, 2)); // layer 2
    EXPECT_FLOAT_EQ(10.f, rs_->GetValue(0, 1, 2));
    EXPECT_FLOAT_EQ(11.f, rs_->GetValue(1, 0, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(1, 1, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(0, 0, 3)); // layer 3
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(0, 1, 3));
    EXPECT_FLOAT_EQ(16.f, rs_->GetValue(1, 0, 3));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(1, 1, 3));

    // Get cell values by (row, col, *&array)
    rs_->GetValue(-1, 0, tmp_values);
    EXPECT_EQ(nullptr, tmp_values);
    rs_->GetValue(0, -1, tmp_values);
    EXPECT_EQ(nullptr, tmp_values);
    rs_->GetValue(0, 0, tmp_values);
    EXPECT_NE(nullptr, tmp_values);
    EXPECT_FLOAT_EQ(-9999.f, tmp_values[0]);
    EXPECT_FLOAT_EQ(-9999.f, tmp_values[1]);
    EXPECT_FLOAT_EQ(-9999.f, tmp_values[2]);
    rs_->GetValue(0, 1, tmp_values);
    EXPECT_NE(nullptr, tmp_values);
    EXPECT_FLOAT_EQ(4.f, tmp_values[0]);
    EXPECT_FLOAT_EQ(10.f, tmp_values[1]);
    EXPECT_FLOAT_EQ(-9999.f, tmp_values[2]);
    rs_->GetValue(1, 0, tmp_values);
    EXPECT_NE(nullptr, tmp_values);
    EXPECT_FLOAT_EQ(5.f, tmp_values[0]);
    EXPECT_FLOAT_EQ(11.f, tmp_values[1]);
    EXPECT_FLOAT_EQ(16.f, tmp_values[2]);
    rs_->GetValue(1, 1, tmp_values);
    EXPECT_FLOAT_EQ(-9999.f, tmp_values[0]);
    EXPECT_FLOAT_EQ(-9999.f, tmp_values[1]);
    EXPECT_FLOAT_EQ(-9999.f, tmp_values[2]);

    Release1DArray(tmp_values);
    EXPECT_EQ(nullptr, tmp_values);

    // Get position index, if cal_poc is False, no chance to return -1
    EXPECT_EQ(-2, rs_->GetPosition(-1., -2.));
    EXPECT_EQ(-2, rs_->GetPosition(-1., 9.));
    EXPECT_EQ(-2, rs_->GetPosition(1., 7.));
    EXPECT_EQ(-2, rs_->GetPosition(3.99, 7.05));
    EXPECT_EQ(-2, rs_->GetPosition(5.01, 7.00));
    EXPECT_EQ(-2, rs_->GetPosition(0.01, 5.5));
    EXPECT_EQ(-1, rs_->GetPosition(2.01, 5.1));
    EXPECT_EQ(0, rs_->GetPosition(4.5, 5.5));
    EXPECT_EQ(-2, rs_->GetPosition(1., 3.));
    EXPECT_EQ(1, rs_->GetPosition(3., 3.));
    EXPECT_EQ(-1, rs_->GetPosition(5., 3.));
    EXPECT_EQ(-2, rs_->GetPosition(1., 1.));
    EXPECT_EQ(-2, rs_->GetPosition(3., 1.));
    EXPECT_EQ(-2, rs_->GetPosition(5., 1.));

    /** Set value **/
    // Set core file name
    string newcorename = rs_->GetCoreName() + "_2D-mask-within-nopos";
    rs_->SetCoreName(newcorename);

    /** Output to new file **/
    string oldfullname = rs_->GetFilePath();
    string newfullname = Dstpath + newcorename + "." + GetSuffix(oldfullname);
    EXPECT_TRUE(rs_->OutputToFile(newfullname));

    /** Copy constructor **/
    FltIntRaster* copyrs = new FltIntRaster(rs_);

    FltIntRaster copyrs2(rs_);

    FltIntRaster copyrs3;
    copyrs3.Copy(rs_);

    // Selected tests
    EXPECT_EQ(2, copyrs->GetCellNumber()); // m_nCells
    EXPECT_EQ(3, copyrs->GetLayers());
    EXPECT_EQ(2, copyrs->GetValidNumber());

    EXPECT_EQ(copyrs->GetMask(), copyrs2.GetMask());
    EXPECT_EQ(copyrs->GetMask(), copyrs3.GetMask());

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
    additional_header[HEADER_MASK_NAME] = maskrs_->GetCoreName();
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
    maskrs_->GetRasterPositionData(&poslen, &posdata);
    maskrs_->GetRasterPositionData(&poslen, &posidx);
    mongors_valid->SetPositions(poslen, posdata);
    mongors_valid->SetPositions(poslen, posidx);
    STRING_MAP opts;
    UpdateStringMap(opts, HEADER_INC_NODATA, "FALSE");
    mongors_valid->ReadFromMongoDB(gfs_, gfsfilename_valid, false,
                                   nullptr, true, NODATA_VALUE, opts);

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
    delete copyrs;
    // No need to release copyrs2 and copyrs3 by developers.
}

// calc_pos = True, use_mask_ext can be true or false
TEST_P(clsRasterDataTestMask2D, CalcPos) {
    ASSERT_TRUE(rs_->ValidateRasterData());
    EXPECT_TRUE(rs_->Initialized());           // m_initialized
    EXPECT_TRUE(rs_->Is2DRaster());            // m_is2DRaster
    EXPECT_TRUE(rs_->MaskExtented());          // m_useMaskExtent
    EXPECT_FALSE(rs_->StatisticsCalculated()); // m_statisticsCalculated
    ASSERT_TRUE(rs_->PositionsCalculated());
    EXPECT_FALSE(rs_->PositionsAllocated());
    /// Calculate position data
    int valid_count;
    int** valid_positions = nullptr;
    rs_->GetRasterPositionData(&valid_count, &valid_positions);
    EXPECT_TRUE(rs_->PositionsCalculated());
    EXPECT_FALSE(rs_->PositionsAllocated());
    EXPECT_EQ(nullptr, rs_->GetRasterDataPointer());         // m_rasterData
    EXPECT_NE(nullptr, rs_->Get2DRasterDataPointer());       // m_raster2DData
    EXPECT_NE(nullptr, rs_->GetRasterPositionDataPointer()); // m_rasterPositionData
    EXPECT_NE(nullptr, rs_->GetRasterPositionIndexPointer()); // m_rasterPositionIndex

    /// Test members after constructing.
    EXPECT_EQ(6, rs_->GetDataLength()); // m_nCells * n_lyrs_
    EXPECT_EQ(2, rs_->GetCellNumber()); // m_nCells

    EXPECT_EQ(-9999., rs_->GetNoDataValue());  // m_noDataValue
    EXPECT_EQ(-9999., rs_->GetDefaultValue()); // m_defaultValue

    /** Get metadata, m_headers **/
    STRDBL_MAP header_info = rs_->GetRasterHeader();
    EXPECT_EQ(CVT_INT(header_info.at(HEADER_RS_LAYERS)), rs_->GetLayers());
    EXPECT_EQ(CVT_INT(header_info.at(HEADER_RS_CELLSNUM)), rs_->GetCellNumber());

    EXPECT_EQ(2, rs_->GetRows());
    EXPECT_EQ(2, rs_->GetCols());
    EXPECT_DOUBLE_EQ(3., rs_->GetXllCenter());
    EXPECT_DOUBLE_EQ(3., rs_->GetYllCenter());
    EXPECT_DOUBLE_EQ(2., rs_->GetCellWidth());
    EXPECT_EQ(3, rs_->GetLayers());
    EXPECT_EQ("", rs_->GetSrsString());

    /** Calc and get basic statistics, m_statsMap **/
    EXPECT_EQ(2, rs_->GetValidNumber());
    EXPECT_DOUBLE_EQ(4., rs_->GetMinimum()); // by default, layer 1
    EXPECT_DOUBLE_EQ(5., rs_->GetMaximum());
    EXPECT_DOUBLE_EQ(4.5, rs_->GetAverage());
    EXPECT_DOUBLE_EQ(0.5, rs_->GetStd());
    EXPECT_DOUBLE_EQ(1., rs_->GetRange());
    EXPECT_TRUE(rs_->StatisticsCalculated());
    EXPECT_DOUBLE_EQ(10., rs_->GetMinimum(2)); // layer 2
    EXPECT_DOUBLE_EQ(11., rs_->GetMaximum(2));
    EXPECT_DOUBLE_EQ(10.5, rs_->GetAverage(2));
    EXPECT_DOUBLE_EQ(0.5, rs_->GetStd(2));
    EXPECT_DOUBLE_EQ(1., rs_->GetRange(2));
    EXPECT_DOUBLE_EQ(16., rs_->GetMinimum(3)); // layer 3
    EXPECT_DOUBLE_EQ(16., rs_->GetMaximum(3));
    EXPECT_DOUBLE_EQ(16., rs_->GetAverage(3));
    EXPECT_DOUBLE_EQ(0., rs_->GetStd(3));
    EXPECT_DOUBLE_EQ(0., rs_->GetRange(3));

    EXPECT_NE(nullptr, rs_->GetMask()); // m_mask

    /** Test getting raster data **/
    int ncells = 0;
    float* rs_data = nullptr;
    EXPECT_FALSE(rs_->GetRasterData(&ncells, &rs_data));
    EXPECT_EQ(-1, ncells);
    EXPECT_EQ(nullptr, rs_data);

    float** rs_2ddata = nullptr;
    int nlyrs = -1;
    EXPECT_TRUE(rs_->Get2DRasterData(&ncells, &nlyrs, &rs_2ddata)); // m_raster2DData
    EXPECT_EQ(2, ncells);
    EXPECT_EQ(3, nlyrs);
    EXPECT_NE(nullptr, rs_2ddata);
    EXPECT_FLOAT_EQ(4.f, rs_2ddata[0][0]);
    EXPECT_FLOAT_EQ(10.f, rs_2ddata[0][1]);
    EXPECT_FLOAT_EQ(-9999.f, rs_2ddata[0][2]);
    EXPECT_FLOAT_EQ(5.f, rs_2ddata[1][0]);
    EXPECT_FLOAT_EQ(11.f, rs_2ddata[1][1]);
    EXPECT_FLOAT_EQ(16.f, rs_2ddata[1][2]);

    // actual data stored in memory
    EXPECT_FLOAT_EQ(4.f, rs_2ddata[0][0]);
    EXPECT_FLOAT_EQ(10.f, rs_2ddata[0][1]);
    EXPECT_FLOAT_EQ(-9999.f, rs_2ddata[0][2]);
    EXPECT_FLOAT_EQ(5.f, rs_2ddata[0][3]);
    EXPECT_FLOAT_EQ(11.f, rs_2ddata[0][4]);
    EXPECT_FLOAT_EQ(16.f, rs_2ddata[0][5]);

    /** Get raster cell value by various way **/
    // Get cell value by index of raster_data_
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(-1));
    EXPECT_FLOAT_EQ(4.f, rs_->GetValueByIndex(0)); // by default, layer 1
    EXPECT_FLOAT_EQ(5.f, rs_->GetValueByIndex(1));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(2)); // out of bound
    EXPECT_FLOAT_EQ(10.f, rs_->GetValueByIndex(0, 2)); // layer 2
    EXPECT_FLOAT_EQ(11.f, rs_->GetValueByIndex(1, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValueByIndex(0, 3)); // layer 3
    EXPECT_FLOAT_EQ(16.f, rs_->GetValueByIndex(1, 3));

    // Get cell value by index of raster_data_/raster_2d_, compatible way
    int tmp_lyr = rs_->GetLayers();
    float* tmp_values = nullptr;
    Initialize1DArray(tmp_lyr, tmp_values, -9999.f);
    rs_->GetValueByIndex(-1, tmp_values); // index exceed, tmp_values will be released
    EXPECT_EQ(nullptr, tmp_values);
    rs_->GetValueByIndex(0, tmp_values); // nullptr input, tmp_values will be initialized
    EXPECT_NE(nullptr, tmp_values);
    EXPECT_FLOAT_EQ(4.f, tmp_values[0]);
    EXPECT_FLOAT_EQ(10.f, tmp_values[1]);
    EXPECT_FLOAT_EQ(-9999.f, tmp_values[2]);
    rs_->GetValueByIndex(1, tmp_values);
    EXPECT_FLOAT_EQ(5.f, tmp_values[0]);
    EXPECT_FLOAT_EQ(11.f, tmp_values[1]);
    EXPECT_FLOAT_EQ(16.f, tmp_values[2]);
    // Get cell value by (row, col)
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(-1, 0));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(0, 0)); // by default, layer 1
    EXPECT_FLOAT_EQ(4.f, rs_->GetValue(0, 1));
    EXPECT_FLOAT_EQ(5.f, rs_->GetValue(1, 0));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(1, 1));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(0, 0, 2)); // layer 2
    EXPECT_FLOAT_EQ(10.f, rs_->GetValue(0, 1, 2));
    EXPECT_FLOAT_EQ(11.f, rs_->GetValue(1, 0, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(1, 1, 2));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(0, 0, 3)); // layer 3
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(0, 1, 3));
    EXPECT_FLOAT_EQ(16.f, rs_->GetValue(1, 0, 3));
    EXPECT_FLOAT_EQ(-9999.f, rs_->GetValue(1, 1, 3));

    // Get cell values by (row, col, *&array)
    rs_->GetValue(-1, 0, tmp_values);
    EXPECT_EQ(nullptr, tmp_values);
    rs_->GetValue(0, -1, tmp_values);
    EXPECT_EQ(nullptr, tmp_values);
    rs_->GetValue(0, 0, tmp_values);
    EXPECT_NE(nullptr, tmp_values);
    EXPECT_FLOAT_EQ(-9999.f, tmp_values[0]);
    EXPECT_FLOAT_EQ(-9999.f, tmp_values[1]);
    EXPECT_FLOAT_EQ(-9999.f, tmp_values[2]);
    rs_->GetValue(0, 1, tmp_values);
    EXPECT_NE(nullptr, tmp_values);
    EXPECT_FLOAT_EQ(4.f, tmp_values[0]);
    EXPECT_FLOAT_EQ(10.f, tmp_values[1]);
    EXPECT_FLOAT_EQ(-9999.f, tmp_values[2]);
    rs_->GetValue(1, 0, tmp_values);
    EXPECT_NE(nullptr, tmp_values);
    EXPECT_FLOAT_EQ(5.f, tmp_values[0]);
    EXPECT_FLOAT_EQ(11.f, tmp_values[1]);
    EXPECT_FLOAT_EQ(16.f, tmp_values[2]);
    rs_->GetValue(1, 1, tmp_values);
    EXPECT_FLOAT_EQ(-9999.f, tmp_values[0]);
    EXPECT_FLOAT_EQ(-9999.f, tmp_values[1]);
    EXPECT_FLOAT_EQ(-9999.f, tmp_values[2]);

    Release1DArray(tmp_values);
    EXPECT_EQ(nullptr, tmp_values);

    // Get position index
    EXPECT_EQ(-2, rs_->GetPosition(-1., -2.));
    EXPECT_EQ(-2, rs_->GetPosition(-1., 9.));
    EXPECT_EQ(-2, rs_->GetPosition(1., 7.));
    EXPECT_EQ(-2, rs_->GetPosition(3.99, 7.05));
    EXPECT_EQ(-2, rs_->GetPosition(5.01, 7.00));
    EXPECT_EQ(-2, rs_->GetPosition(0.01, 5.5));
    EXPECT_EQ(-1, rs_->GetPosition(2.01, 5.1));
    EXPECT_EQ(0, rs_->GetPosition(4.5, 5.5));
    EXPECT_EQ(-2, rs_->GetPosition(1., 3.));
    EXPECT_EQ(1, rs_->GetPosition(3., 3.));
    EXPECT_EQ(-1, rs_->GetPosition(5., 3.));
    EXPECT_EQ(-2, rs_->GetPosition(1., 1.));
    EXPECT_EQ(-2, rs_->GetPosition(3., 1.));
    EXPECT_EQ(-2, rs_->GetPosition(5., 1.));

    /** Set value **/
    // Set core file name
    string newcorename = rs_->GetCoreName() + "_2D-mask-within-calcpos";
    rs_->SetCoreName(newcorename);

    /** Output to new file **/
    string oldfullname = rs_->GetFilePath();
    string newfullname = Dstpath + newcorename + "." + GetSuffix(oldfullname);
    EXPECT_TRUE(rs_->OutputToFile(newfullname));

    /** Copy constructor **/
    FltIntRaster* copyrs = new FltIntRaster(rs_);

    FltIntRaster copyrs2(rs_);

    FltIntRaster copyrs3;
    copyrs3.Copy(rs_);

    // Selected tests
    EXPECT_EQ(2, copyrs->GetCellNumber()); // m_nCells
    EXPECT_EQ(3, copyrs->GetLayers());
    EXPECT_EQ(2, copyrs->GetValidNumber());

    EXPECT_EQ(copyrs->GetMask(), copyrs2.GetMask());
    EXPECT_EQ(copyrs->GetMask(), copyrs3.GetMask());

#ifdef USE_MONGODB
    /** MongoDB I/O test **/

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
    additional_header[HEADER_MASK_NAME] = maskrs_->GetCoreName();
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
    maskrs_->GetRasterPositionData(&poslen, &posdata);
    maskrs_->GetRasterPositionData(&poslen, &posidx);
    mongors_valid->SetPositions(poslen, posdata);
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
#endif
    delete copyrs;
    // No need to release copyrs2 and copyrs3 by developers.
}

#ifdef USE_GDAL
INSTANTIATE_TEST_CASE_P(MultipleLayers, clsRasterDataTestMask2D,
                        Values(new InputRasterFiles(rs1_asc, rs2_asc, rs3_asc, mask_asc_file),
                            new InputRasterFiles(rs1_tif, rs2_tif, rs3_tif, mask_tif_file)),);
#else
INSTANTIATE_TEST_CASE_P(MultipleLayers, clsRasterDataTestMask2D,
                        Values(new InputRasterFiles(rs1_asc, rs2_asc, rs3_asc, mask_asc_file)),);
#endif /* USE_GDAL */

} /* namespace */
