/*!
 * \brief Test description
 *
 *        TEST CASE NAME (or TEST SUITE):
 *            clsRasterData2DSplit: Split 2D raster with mask by the subset feature of clsRasterData.
 *
 *        Since we mainly support ASC and GDAL(e.g., TIFF),
 *        value-parameterized tests of Google Test will be used.
 * \cite https://github.com/google/googletest/blob/master/googletest/samples/sample7_unittest.cc
 * \version 1.2
 * \authors Liangjun Zhu, zlj(at)lreis.ac.cn; crazyzlj(at)gmail.com
 * \remarks 2021-12-12 - lj - Original version.
 *          2022-04-02 - lj - Add MongoDB supports.
 *          2023-04-14 - lj - Update tests according to API changes of clsRasterData
 *
 */
#include "gtest/gtest.h"
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
using testing::TestWithParam;
using testing::Values;

string Rspath = GetAppPath() + "./data/raster/";
string Dstpath = Rspath + "result/";

string maskname = "tinydemo_raster_r4c7";
string corename = "tinydemo_raster_r5c8";

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
class clsRasterData2DSplitMerge : public TestWithParam<InputRasterFiles*> {
public:
    clsRasterData2DSplitMerge() : maskrs_(nullptr), maskrsflt_(nullptr) {
    }

    virtual ~clsRasterData2DSplitMerge() { ; }

    void SetUp() OVERRIDE {
        maskrs_ = IntRaster::Init(GetParam()->mask_name);
        ASSERT_NE(nullptr, maskrs_);

        maskrsflt_ = FltRaster::Init(GetParam()->mask_name);
        ASSERT_NE(nullptr, maskrsflt_);

        EXPECT_TRUE(MakeDirectory(Dstpath));
    }

    void TearDown() OVERRIDE {
        delete maskrs_;
        delete maskrsflt_;
    }

protected:
    IntRaster* maskrs_;
    FltRaster* maskrsflt_;
};

TEST_P(clsRasterData2DSplitMerge, MaskLyrIO) {
    EXPECT_FALSE(maskrs_->PositionsCalculated());
    EXPECT_TRUE(maskrs_->BuildSubSet());
    EXPECT_TRUE(maskrs_->PositionsCalculated());
    map<int, SubsetPositions*>& subset = maskrs_->GetSubset();
    int n_subset = CVT_INT(subset.size());
    EXPECT_EQ(n_subset, 4);
    SubsetPositions* sub1 = subset.at(1);
    EXPECT_EQ(sub1->g_srow, 0);
    EXPECT_EQ(sub1->g_erow, 3);
    EXPECT_EQ(sub1->g_scol, 0);
    EXPECT_EQ(sub1->g_ecol, 2);
    EXPECT_EQ(sub1->n_cells, 7);
    EXPECT_EQ(sub1->global_[0], 0);
    EXPECT_EQ(sub1->global_[1], 2);
    EXPECT_EQ(sub1->global_[2], 3);
    EXPECT_EQ(sub1->global_[3], 7);
    EXPECT_EQ(sub1->global_[4], 8);
    EXPECT_EQ(sub1->global_[5], 9);
    EXPECT_EQ(sub1->global_[6], 14);
    SubsetPositions* sub2 = subset.at(2);
    EXPECT_EQ(sub2->g_srow, 1);
    EXPECT_EQ(sub2->g_erow, 3);
    EXPECT_EQ(sub2->g_scol, 2);
    EXPECT_EQ(sub2->g_ecol, 3);
    EXPECT_EQ(sub2->n_cells, 4);
    EXPECT_EQ(sub2->global_[0], 4);
    EXPECT_EQ(sub2->global_[1], 5);
    EXPECT_EQ(sub2->global_[2], 10);
    EXPECT_EQ(sub2->global_[3], 15);
    SubsetPositions* sub3 = subset.at(3);
    EXPECT_EQ(sub3->g_srow, 0);
    EXPECT_EQ(sub3->g_erow, 3);
    EXPECT_EQ(sub3->g_scol, 3);
    EXPECT_EQ(sub3->g_ecol, 6);
    EXPECT_EQ(sub3->n_cells, 6);
    EXPECT_EQ(sub3->global_[0], 1);
    EXPECT_EQ(sub3->global_[1], 6);
    EXPECT_EQ(sub3->global_[2], 11);
    EXPECT_EQ(sub3->global_[3], 16);
    EXPECT_EQ(sub3->global_[4], 17);
    EXPECT_EQ(sub3->global_[5], 18);
    SubsetPositions* sub4 = subset.at(4);
    EXPECT_EQ(sub4->g_srow, 2);
    EXPECT_EQ(sub4->g_erow, 3);
    EXPECT_EQ(sub4->g_scol, 5);
    EXPECT_EQ(sub4->g_ecol, 6);
    EXPECT_EQ(sub4->n_cells, 3);
    EXPECT_EQ(sub4->global_[0], 12);
    EXPECT_EQ(sub4->global_[1], 13);
    EXPECT_EQ(sub4->global_[2], 19);

    map<int, int> new_group;
    new_group[1] = 1;
    new_group[2] = 2;
    new_group[3] = 3;
    new_group[4] = 1;
    EXPECT_TRUE(maskrs_->RebuildSubSet(new_group));
    map<int, SubsetPositions*>& newsubset = maskrs_->GetSubset();
    n_subset = CVT_INT(newsubset.size());
    EXPECT_EQ(n_subset, 3);
    SubsetPositions* newsub1 = newsubset.at(1);
    EXPECT_EQ(newsub1->g_srow, 0);
    EXPECT_EQ(newsub1->g_erow, 3);
    EXPECT_EQ(newsub1->g_scol, 0);
    EXPECT_EQ(newsub1->g_ecol, 6);
    EXPECT_EQ(newsub1->n_cells, 10);
    EXPECT_EQ(newsub1->global_[0], 0);
    EXPECT_EQ(newsub1->global_[1], 2);
    EXPECT_EQ(newsub1->global_[2], 3);
    EXPECT_EQ(newsub1->global_[3], 7);
    EXPECT_EQ(newsub1->global_[4], 8);
    EXPECT_EQ(newsub1->global_[5], 9);
    EXPECT_EQ(newsub1->global_[6], 12);
    EXPECT_EQ(newsub1->global_[7], 13);
    EXPECT_EQ(newsub1->global_[8], 14);
    EXPECT_EQ(newsub1->global_[9], 19);
    SubsetPositions* newsub2 = newsubset.at(2);
    EXPECT_EQ(newsub2->g_srow, 1);
    EXPECT_EQ(newsub2->g_erow, 3);
    EXPECT_EQ(newsub2->g_scol, 2);
    EXPECT_EQ(newsub2->g_ecol, 3);
    EXPECT_EQ(newsub2->n_cells, 4);
    EXPECT_EQ(newsub2->global_[0], 4);
    EXPECT_EQ(newsub2->global_[1], 5);
    EXPECT_EQ(newsub2->global_[2], 10);
    EXPECT_EQ(newsub2->global_[3], 15);
    SubsetPositions* newsub3 = newsubset.at(3);
    EXPECT_EQ(newsub3->g_srow, 0);
    EXPECT_EQ(newsub3->g_erow, 3);
    EXPECT_EQ(newsub3->g_scol, 3);
    EXPECT_EQ(newsub3->g_ecol, 6);
    EXPECT_EQ(newsub3->n_cells, 6);
    EXPECT_EQ(newsub3->global_[0], 1);
    EXPECT_EQ(newsub3->global_[1], 6);
    EXPECT_EQ(newsub3->global_[2], 11);
    EXPECT_EQ(newsub3->global_[3], 16);
    EXPECT_EQ(newsub3->global_[4], 17);
    EXPECT_EQ(newsub3->global_[5], 18);

    /** Set new data of subset and output to new files **/
    map<int, float**> newdata = map<int, float**>();
    int newlyrs = 2;
    float** data1 = nullptr;
    Initialize2DArray(newsub1->n_cells, newlyrs, data1, 1.f);
    data1[0][0] = 2008.f;
    data1[0][1] = 4016.f;
    data1[1][0] = 11.f;
    data1[1][1] = 22.f;
    data1[2][0] = 9.f;
    data1[2][1] = 18.f;
    data1[7][0] = 2017.f;
    data1[7][1] = 4034.f;
    data1[8][0] = 5.f;
    data1[8][1] = 10.f;
    data1[9][0] = 1.f;
    data1[9][1] = 2.f;
    EXPECT_FALSE(newsub1->Set2DData(newsub1->n_cells + 1, newlyrs, data1));
    EXPECT_TRUE(newsub1->Set2DData(newsub1->n_cells, newlyrs, data1));
    newdata[1] = data1;

    float** data2 = nullptr;
    Initialize2DArray(newsub2->n_cells, newlyrs, data2, 2.f);
    data2[0][0] = 2017.f;
    data2[0][1] = 4034.f;
    data2[1][0] = 1.f;
    data2[1][1] = 2.f;
    data2[2][0] = 7.f;
    data2[2][1] = 14.f;
    EXPECT_TRUE(newsub2->Set2DData(newsub2->n_cells, newlyrs, data2));
    newdata[2] = data2;

    float** data3 = nullptr;
    Initialize2DArray(newsub3->n_cells, newlyrs, data3, 3.f);
    data3[0][0] = 2019.f;
    data3[0][1] = 4038.f;
    data3[1][0] = 2.f;
    data3[1][1] = 4.f;
    data3[2][0] = 18.f;
    data3[2][1] = 36.f;
    EXPECT_TRUE(newsub3->Set2DData(newsub3->n_cells, newlyrs, data3));
    newdata[3] = data3;

    float** datafull = nullptr;
    Initialize2DArray(maskrs_->GetValidNumber(), 2, datafull, -9999);
    datafull[0][0] = 2008.f;
    datafull[0][1] = 4016.f;
    datafull[1][0] = 2019.f;
    datafull[1][1] = 4038.f;
    datafull[2][0] = 11.f;
    datafull[2][1] = 22.f;
    datafull[3][0] = 9.f;
    datafull[3][1] = 18.f;
    datafull[4][0] = 2017.f;
    datafull[4][1] = 4034.f;
    datafull[5][0] = 1.f;
    datafull[5][1] = 2.f;
    datafull[6][0] = 2.f;
    datafull[6][1] = 4.f;
    datafull[7][0] = 1.f;
    datafull[7][1] = 1.f;
    datafull[8][0] = 1.f;
    datafull[8][1] = 1.f;
    datafull[9][0] = 1.f;
    datafull[9][1] = 1.f;
    datafull[10][0] = 7.f;
    datafull[10][1] = 14.f;
    datafull[11][0] = 18.f;
    datafull[11][1] = 36.f;
    datafull[12][0] = 1.f;
    datafull[12][1] = 1.f;
    datafull[13][0] = 2017.f;
    datafull[13][1] = 4034.f;
    datafull[14][0] = 5.f;
    datafull[14][1] = 10.f;
    datafull[15][0] = 2.f;
    datafull[15][1] = 2.f;
    datafull[16][0] = 3.f;
    datafull[16][1] = 3.f;
    datafull[17][0] = 3.f;
    datafull[17][1] = 3.f;
    datafull[18][0] = 3.f;
    datafull[18][1] = 3.f;
    datafull[19][0] = 1.f;
    datafull[19][1] = 2.f;

    /** Output subset to new files **/
    string outfile = Dstpath + maskrs_->GetCoreName() + "." + GetSuffix(GetParam()->mask_name);
    EXPECT_TRUE(maskrs_->OutputSubsetToFile(false, false, outfile));
    map<int, SubsetPositions*> subsets = maskrs_->GetSubset();
    for (auto it = subsets.begin(); it != subsets.end(); ++it) {
        vector<string> outfiles(newlyrs);
        for (int ilyr = 0; ilyr < newlyrs; ilyr++) {
            string outfilesubtmp = PrefixCoreFileName(outfile, it->first);
            outfiles[ilyr] = AppendCoreFileName(outfilesubtmp, ilyr + 1);
        }
        EXPECT_TRUE(FilesExist(outfiles));
        float** tmp = newdata.at(it->first);
        FltRaster* tmp_rs = FltRaster::Init(outfiles, true);
        EXPECT_FALSE(nullptr == tmp_rs);
        EXPECT_TRUE(tmp_rs->PositionsCalculated());
        int len;
        int lyr;
        float** validdata = nullptr;
        tmp_rs->Get2DRasterData(&len, &lyr, &validdata);
        EXPECT_EQ(lyr, newlyrs);
        EXPECT_EQ(len, it->second->n_cells);
        for (int ki = 0; ki < len; ki++) {
            for (int kj = 0; kj < newlyrs; kj++) {
                EXPECT_FLOAT_EQ(validdata[ki][kj], tmp[ki][kj]);
            }
        }
        delete tmp_rs;
    }

    /** Output subset data to new single file **/
    EXPECT_TRUE(maskrs_->OutputSubsetToFile(false, true, outfile));
    string outfile_full = PrefixCoreFileName(outfile, 0);
    /* Same as:
    EXPECT_TRUE(maskrs_->OutputToFile(outfile_full, false));
    */
    vector<string> outfilesfull(newlyrs);
    for (int ilyr = 0; ilyr < newlyrs; ilyr++) {
        outfilesfull[ilyr] = AppendCoreFileName(outfile_full, ilyr + 1);
    }
    EXPECT_TRUE(FilesExist(outfilesfull));
    FltRaster* newrs = FltRaster::Init(outfilesfull, true);
    EXPECT_TRUE(newrs->PositionsCalculated());
    int len;
    int lyr;
    float** validdata = nullptr;
    newrs->Get2DRasterData(&len, &lyr, &validdata);
    EXPECT_EQ(len, maskrs_->GetCellNumber());
    EXPECT_EQ(lyr, newlyrs);
    EXPECT_NE(nullptr, validdata);
    for (int k = 0; k < len; k++) {
        for (int l = 0; l < newlyrs; l++) {
            EXPECT_FLOAT_EQ(validdata[k][l], datafull[k][l]);
        }
    }
    delete newrs;

#ifdef USE_MONGODB
    /** Output subset data to MongoDB **/
    string mask_subset_name = maskrs_->GetCoreName() + "_2d";
    EXPECT_TRUE(maskrs_->OutputSubsetToMongoDB(GlobalEnv->gfs_, mask_subset_name, ccgl::STRING_MAP(),
                    true, false, false)); // store fullsize data

    EXPECT_TRUE(maskrs_->OutputSubsetToMongoDB(GlobalEnv->gfs_, mask_subset_name, ccgl::STRING_MAP(),
                    false, false, false)); // store valid data only

    IntRaster* maskrs4mongodata = new IntRaster(maskrs_);
    IntRaster* maskrs4mongodata2 = new IntRaster(maskrs_);
    map<int, SubsetPositions*>& subsetsfull = maskrs4mongodata->GetSubset();
    map<int, SubsetPositions*>& subsetsvalid = maskrs4mongodata2->GetSubset();

    STRING_MAP opts_full;
    STRING_MAP opts_valid;
    UpdateStrHeader(opts_full, HEADER_INC_NODATA, "TRUE");
    UpdateStrHeader(opts_valid, HEADER_INC_NODATA, "FALSE");

    for (auto it = subsetsfull.begin(); it != subsetsfull.end(); ++it) {
        string gfsfull = itoa(it->first) + "_" + mask_subset_name;
        EXPECT_TRUE(it->second->ReadFromMongoDB(GlobalEnv->gfs_, gfsfull, opts_full));
    }
    for (auto it = subsetsvalid.begin(); it != subsetsvalid.end(); ++it) {
        string gfsvalid = itoa(it->first) + "_" + mask_subset_name;
        EXPECT_TRUE(it->second->ReadFromMongoDB(GlobalEnv->gfs_, gfsvalid, opts_valid));
    }

    // check consistent of valid values loaded in gfsfull and gfsvalid
    for (auto it = subsetsfull.begin(); it != subsetsfull.end(); ++it) {
        SubsetPositions* full = it->second;
        SubsetPositions* valid = subsetsvalid.at(it->first);
        EXPECT_EQ(full->n_lyrs, newlyrs);
        EXPECT_EQ(full->n_lyrs, valid->n_lyrs);
        EXPECT_EQ(full->alloc_, valid->alloc_);
        EXPECT_EQ(full->n_cells, valid->n_cells);
        EXPECT_EQ(full->g_srow, valid->g_srow);
        EXPECT_EQ(full->g_erow, valid->g_erow);
        EXPECT_EQ(full->g_scol, valid->g_scol);
        EXPECT_EQ(full->g_ecol, valid->g_ecol);
        EXPECT_EQ(nullptr, full->data_);
        EXPECT_NE(nullptr, full->data2d_);
        EXPECT_EQ(nullptr, valid->data_);
        EXPECT_NE(nullptr, valid->data2d_);
        for (int i = 0; i < full->n_cells; i++) {
            EXPECT_EQ(full->local_pos_[i][0], valid->local_pos_[i][0]);
            EXPECT_EQ(full->local_pos_[i][1], valid->local_pos_[i][1]);
            EXPECT_EQ(full->local_posidx_[i], valid->local_posidx_[i]);
            for (int l = 0; l < newlyrs; l++) {
                EXPECT_DOUBLE_EQ(full->data2d_[i][l], valid->data2d_[i][l]);
            }
        }
    }

    delete maskrs4mongodata;
    delete maskrs4mongodata2;

    /** Output subset data to new single file **/
    EXPECT_TRUE(maskrs_->OutputSubsetToMongoDB(GlobalEnv->gfs_, mask_subset_name, ccgl::STRING_MAP(),
                    true, false, true)); // store fullsize data

    EXPECT_TRUE(maskrs_->OutputSubsetToMongoDB(GlobalEnv->gfs_, mask_subset_name, ccgl::STRING_MAP(),
                    false, false, true)); // store valid data only

    if (HasFailure()) { return; }

    string outgfsfile_full = "0_" + mask_subset_name;

    IntRaster* mongofull = IntRaster::Init(GlobalEnv->gfs_, outgfsfile_full.c_str(),
                                           true, maskrs_, true, -9999, opts_full);
    IntRaster* mongovalid = IntRaster::Init(GlobalEnv->gfs_, outgfsfile_full.c_str(),
                                            true, maskrs_, true, -9999, opts_valid);

    for (int k = 0; k < mongofull->GetValidNumber(); k++) {
        for (int l = 0; l < newlyrs; l++) {
            EXPECT_EQ(datafull[k][l], mongofull->GetValueByIndex(k, l + 1));
            EXPECT_EQ(mongofull->GetValueByIndex(k, l + 1),
                      mongovalid->GetValueByIndex(k, l + 1));
        }
    }
    delete mongofull;
    delete mongovalid;

#endif
    // release newdata
    for (auto it = newdata.begin(); it != newdata.end(); ++it) {
        Release2DArray(it->second);
    }
    newdata.clear();

    Release2DArray(datafull);
}
// Raster IO based on mask layer which has several subset
//   1. output raster data according to mask's subset
//   2. read subset data to combine an entire raster data
TEST_P(clsRasterData2DSplitMerge, SplitRaster) {
    // prepare mask data with user-specified groups
    EXPECT_FALSE(maskrsflt_->PositionsCalculated());
    map<int, int> new_group;
    new_group[1] = 1;
    new_group[2] = 2;
    new_group[3] = 3;
    new_group[4] = 1;
    EXPECT_TRUE(maskrsflt_->BuildSubSet(new_group));
    EXPECT_TRUE(maskrsflt_->PositionsCalculated());
    // read raster data with mask
    vector<string> filenames;
    filenames.emplace_back(GetParam()->raster_name1);
    filenames.emplace_back(GetParam()->raster_name2);
    filenames.emplace_back(GetParam()->raster_name3);
    int lyrs = CVT_INT(filenames.size());
    FltRaster* rs = FltRaster::Init(filenames, false, maskrsflt_, true);
    EXPECT_NE(nullptr, rs);
    int orglen;
    int orglyr;
    float** orgvalues;
    rs->Get2DRasterData(&orglen, &orglyr, &orgvalues);
    EXPECT_EQ(orglen, 20);
    EXPECT_EQ(orglyr, 3);
    EXPECT_NE(nullptr, orgvalues);

    map<int, SubsetPositions*>& subset = maskrsflt_->GetSubset();
    map<int, SubsetPositions*>& rs_subset = rs->GetSubset();
    EXPECT_FALSE(subset.empty());
    EXPECT_FALSE(rs_subset.empty());
    EXPECT_EQ(subset.size(), 3);
    EXPECT_EQ(rs_subset.size(), 2);

    /** Output subset to new files **/
    string outfile = ConcatFullName(Dstpath, corename, GetSuffix(GetParam()->raster_name1));
    EXPECT_TRUE(rs->OutputSubsetToFile(true, false, outfile));
    /** Check the output files **/
    map<int, float**> subarray = map<int, float**>();
    float** data1 = nullptr;
    Initialize2DArray(10, lyrs, data1, -9999.f);
    data1[0][0] = 1.1f;
    data1[1][0] = 4.4f;
    data1[2][0] = 5.5f;
    data1[3][0] = 10.f;
    data1[4][0] = 11.11f;
    data1[5][0] = 12.12f;
    data1[6][0] = 15.15f;
    data1[7][0] = 16.16f;
    data1[8][0] = 17.17f;
    data1[9][0] = 22.22f;

    data1[0][1] = 29.29f;
    data1[1][1] = 31.31f;
    data1[2][1] = 32.32f;
    data1[3][1] = -9999.f;
    data1[4][1] = 38.38f;
    data1[5][1] = 39.39f;
    data1[6][1] = -9999.f;
    data1[7][1] = -9999.f;
    data1[8][1] = 42.42f;
    data1[9][1] = 47.47f;

    data1[0][2] = -9999.f;
    data1[1][2] = 56.56f;
    data1[2][2] = 57.57f;
    data1[3][2] = 61.61f;
    data1[4][2] = 62.62f;
    data1[5][2] = -9999.f;
    data1[6][2] = 64.64f;
    data1[7][2] = -9999.f;
    data1[8][2] = 65.65f;
    data1[9][2] = -9999.f;

    subarray[1] = data1;

    float** data3 = nullptr;
    Initialize2DArray(6, lyrs, data3, -9999.f);
    data3[0][0] = 2.2f;
    data3[1][0] = 8.8f;
    data3[2][0] = 14.14f;
    data3[3][0] = 19.19f;
    data3[4][0] = 20.2f;
    data3[5][0] = 21.21f;

    data3[0][1] = -9999.f;
    data3[1][1] = 36.36f;
    data3[2][1] = 41.41f;
    data3[3][1] = 44.44f;
    data3[4][1] = 45.45f;
    data3[5][1] = 46.46f;

    data3[0][2] = 54.54f;
    data3[1][2] = 59.59f;
    data3[2][2] = 63.63f;
    data3[3][2] = 66.66f;
    data3[4][2] = 67.67f;
    data3[5][2] = -9999.f;
    subarray[3] = data3;

    float** datacombvalid = nullptr;
    Initialize2DArray(16, lyrs, datacombvalid, -9999.f);
    datacombvalid[0][0] = 1.1f;
    datacombvalid[1][0] = 2.2f;
    datacombvalid[2][0] = 4.4f;
    datacombvalid[3][0] = 5.5f;
    datacombvalid[4][0] = 8.8f;
    datacombvalid[5][0] = 10.f;
    datacombvalid[6][0] = 11.11f;
    datacombvalid[7][0] = 12.12f;
    datacombvalid[8][0] = 14.14f;
    datacombvalid[9][0] = 15.15f;
    datacombvalid[10][0] = 16.16f;
    datacombvalid[11][0] = 17.17f;
    datacombvalid[12][0] = 19.19f;
    datacombvalid[13][0] = 20.2f;
    datacombvalid[14][0] = 21.21f;
    datacombvalid[15][0] = 22.22f;

    datacombvalid[0][1] = 29.29f;
    datacombvalid[1][1] = -9999.f;
    datacombvalid[2][1] = 31.31f;
    datacombvalid[3][1] = 32.32f;
    datacombvalid[4][1] = 36.36f;
    datacombvalid[5][1] = -9999.f;
    datacombvalid[6][1] = 38.38f;
    datacombvalid[7][1] = 39.39f;
    datacombvalid[8][1] = 41.41f;
    datacombvalid[9][1] = -9999.f;
    datacombvalid[10][1] = -9999.f;
    datacombvalid[11][1] = 42.42f;
    datacombvalid[12][1] = 44.44f;
    datacombvalid[13][1] = 45.45f;
    datacombvalid[14][1] = 46.46f;
    datacombvalid[15][1] = 47.47f;

    datacombvalid[0][2] = -9999.f;
    datacombvalid[1][2] = 54.54f;
    datacombvalid[2][2] = 56.56f;
    datacombvalid[3][2] = 57.57f;
    datacombvalid[4][2] = 59.59f;
    datacombvalid[5][2] = 61.61f;
    datacombvalid[6][2] = 62.62f;
    datacombvalid[7][2] = -9999.f;
    datacombvalid[8][2] = 63.63f;
    datacombvalid[9][2] = 64.64f;
    datacombvalid[10][2] = -9999.f;
    datacombvalid[11][2] = 65.65f;
    datacombvalid[12][2] = 66.66f;
    datacombvalid[13][2] = 67.67f;
    datacombvalid[14][2] = -9999.f;
    datacombvalid[15][2] = -9999.f;

    for (auto it = rs_subset.begin(); it != rs_subset.end(); ++it) {
        vector<string> outfiles(lyrs);
        for (int ilyr = 0; ilyr < lyrs; ilyr++) {
            string outfilesub = PrefixCoreFileName(outfile, it->first);
            outfiles[ilyr] = AppendCoreFileName(outfilesub, ilyr + 1);
        }
        EXPECT_TRUE(FilesExist(outfiles));
        float** tmp = subarray.at(it->first);
        FltRaster* tmp_rs = FltRaster::Init(outfiles, true);
        EXPECT_FALSE(nullptr == tmp_rs);
        EXPECT_TRUE(tmp_rs->PositionsCalculated());
        int len;
        int lyr;
        float** validdata = nullptr;
        tmp_rs->Get2DRasterData(&len, &lyr, &validdata);
        EXPECT_EQ(lyr, lyrs);
        EXPECT_EQ(len, it->second->n_cells);
        for (int ki = 0; ki < len; ki++) {
            for (int kj = 0; kj < lyrs; kj++) {
                EXPECT_FLOAT_EQ(validdata[ki][kj], tmp[ki][kj]);
            }
        }
        delete tmp_rs;
    }
#ifdef USE_MONGODB
    // Store valid subset data to MongoDB
    string outsubname = rs->GetCoreName() + "_2d";
    EXPECT_TRUE(rs->OutputSubsetToMongoDB(GlobalEnv->gfs_, outsubname, ccgl::STRING_MAP(),
                    false, true, false)); // store valid data only
    // Combine raster data using mask's subset data
    for (auto it = subset.begin(); it != subset.end(); ++it) {
        string gfsvalid = itoa(it->first) + "_" + outsubname;
        if (!it->second->ReadFromMongoDB(GlobalEnv->gfs_, gfsvalid, ccgl::STRING_MAP())) {
            it->second->usable = false;
        }
    }
    string com_fname = rs->GetCoreName() + "_com_frommongo";
    string com_fname_real = "0_" + com_fname;
    EXPECT_TRUE(maskrsflt_->OutputToMongoDB(GlobalEnv->gfs_, com_fname,
                    ccgl::STRING_MAP(), true, false));

    if (HasFailure()) { return; }

    // read from MongoDB without mask data
    FltRaster* com_inrs_mongo2 = FltRaster::Init(GlobalEnv->gfs_, com_fname_real.c_str(),
                                                 true);
    EXPECT_NE(nullptr, com_inrs_mongo2);
    EXPECT_EQ(16, com_inrs_mongo2->GetCellNumber());
    for (int i = 0; i < 16; i++) {
        for (int l = 0; l < orglyr; l++) {
            EXPECT_FLOAT_EQ(datacombvalid[i][l], com_inrs_mongo2->GetValueByIndex(i, l + 1));
        }
    }
    delete com_inrs_mongo2;
#endif

    for (auto it = subarray.begin(); it != subarray.end(); ++it) {
        Release2DArray(it->second);
    }
    subarray.clear();

    Release2DArray(datacombvalid);

    delete rs;
}


#ifdef USE_GDAL
    INSTANTIATE_TEST_CASE_P(MultiLayers, clsRasterData2DSplitMerge,
                            Values(new InputRasterFiles(rs1_asc, rs2_asc, rs3_asc, mask_asc_file),
                                new InputRasterFiles(rs1_tif, rs2_tif, rs3_tif, mask_tif_file)),);
#else
    INSTANTIATE_TEST_CASE_P(MultiLayers, clsRasterData2DSplitMerge,
                            Values(new InputRasterFiles(rs1_asc, rs2_asc, rs3_asc, mask_asc_file)),);
#endif /* USE_GDAL */

} /* namespace */
