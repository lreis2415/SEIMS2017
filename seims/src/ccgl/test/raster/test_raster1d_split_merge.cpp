/*!
 * \brief Test description
 *
 *        TEST CASE NAME (or TEST SUITE):
 *            clsRasterDataSplitMerge: Split and merge raster with mask
 *                                     by the subset feature of clsRasterData.
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
#include "../../src/utils_array.h"
#include "../../src/utils_string.h"
#include "../../src/utils_filesystem.h"
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
using testing::TestWithParam;
using testing::Values;

string Rspath = GetAppPath() + "./data/raster/";
string Dstpath = Rspath + "result/";

string maskname = "tinydemo_raster_r4c7";
string corename = "tinydemo_raster_r5c8";

string rs1_asc = Rspath + corename + ".asc";
string rs1_tif = Rspath + corename + ".tif";

string mask_asc_file = Rspath + maskname + ".asc";
string mask_tif_file = Rspath + maskname + ".tif";

struct InputRasterFiles {
public:
    InputRasterFiles(const string& maskf, const string& rsf) {
        mask_name = maskf.c_str();
        raster_name = rsf.c_str();
    }

    ~InputRasterFiles() { ; }

    const char* mask_name;
    const char* raster_name;
};

//Inside the test body, fixture constructor, SetUp(), and TearDown() you
//can refer to the test parameter by GetParam().  In this case, the test
//parameter is a factory function which we call in fixture's SetUp() to
//create and store an instance of clsRasterData<float>.
class clsRasterDataSplitMerge: public TestWithParam<InputRasterFiles *> {
public:
    clsRasterDataSplitMerge() : maskrs_(nullptr), maskrsflt_(nullptr) {
    }

    virtual ~clsRasterDataSplitMerge() { ; }

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

TEST_P(clsRasterDataSplitMerge, MaskLyrIO) {
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
    map<int, float*> newdata = map<int, float*>();
    float* data1 = nullptr;
    Initialize1DArray(newsub1->n_cells, data1, 1.f);
    data1[0] = 2008.f; // gidx: 0
    data1[1] = 11.f; // gidx: 2
    data1[2] = 9.f; // gidx: 3
    // data1[3] = 1.f; // gidx: 7
    // data1[4] = 1.f; // gidx: 8
    // data1[5] = 1.f; // gidx: 9
    // data1[6] = 1.f; // gidx: 12
    data1[7] = 2017.f; // gidx: 13
    data1[8] = 5.f; // gidx: 14
    data1[9] = 1.f; // gidx: 19
    EXPECT_TRUE(newsub1->SetData(newsub1->n_cells, data1));
    newdata[1] = data1;

    float* data2 = nullptr;
    Initialize1DArray(newsub2->n_cells, data2, 2.f);
    data2[0] = 2017.f; // gidx: 4
    data2[1] = 1.f; // gidx: 5
    data2[2] = 7.f; // gidx: 10
    // data2[3] = 2.f; // gidx: 15
    EXPECT_TRUE(newsub2->SetData(newsub2->n_cells, data2));
    newdata[2] = data2;

    float* data3 = nullptr;
    Initialize1DArray(newsub3->n_cells, data3, 3.f);
    data3[0] = 2019.f; // gidx: 1
    data3[1] = 2.f; // gidx: 6
    data3[2] = 18.f; // gidx: 11
    // data3[3] = 3.f; // gidx: 16
    // data3[4] = 3.f; // gidx: 17
    // data3[5] = 3.f; // gidx: 18
    EXPECT_TRUE(newsub3->SetData(newsub3->n_cells, data3));
    newdata[3] = data3;

    float* datafull = nullptr;
    Initialize1DArray(maskrs_->GetValidNumber(), datafull, -9999);
    datafull[0] = 2008.f;
    datafull[1] = 2019.f;
    datafull[2] = 11.f;
    datafull[3] = 9.f;
    datafull[4] = 2017.f;
    datafull[5] = 1.f;
    datafull[6] = 2.f;
    datafull[7] = 1.f;
    datafull[8] = 1.f;
    datafull[9] = 1.f;
    datafull[10] = 7.f;
    datafull[11] = 18.f;
    datafull[12] = 1.f;
    datafull[13] = 2017.f;
    datafull[14] = 5.f;
    datafull[15] = 2.f;
    datafull[16] = 3.f;
    datafull[17] = 3.f;
    datafull[18] = 3.f;
    datafull[19] = 1.f;

    /** Output subset to new files **/
    string outfile = Dstpath + maskrs_->GetCoreName() + "." + GetSuffix(GetParam()->mask_name);
    EXPECT_TRUE(maskrs_->OutputSubsetToFile(false, false, outfile));
    map<int, SubsetPositions*>& subsets = maskrs_->GetSubset();
    for (auto it = subsets.begin(); it != subsets.end(); ++it) {
        string outfilesub = PrefixCoreFileName(outfile, it->first);
        EXPECT_TRUE(FileExists(outfilesub));
        float* tmp = newdata.at(it->first);
        FltRaster* tmp_rs = FltRaster::Init(outfilesub, true);
        EXPECT_FALSE(nullptr == tmp_rs);
        EXPECT_TRUE(tmp_rs->PositionsCalculated());
        int len;
        float* validdata = nullptr;
        tmp_rs->GetRasterData(&len, &validdata);
        for (int k = 0; k < len; k++) {
            EXPECT_FLOAT_EQ(validdata[k], tmp[k]);
        }
        delete tmp_rs;
    }

    /** Output subset data to new single file **/
    EXPECT_TRUE(maskrs_->OutputSubsetToFile(false, true, outfile));
    string outfile_full = PrefixCoreFileName(outfile, 0);
    /* Same as:
    EXPECT_TRUE(maskrs_->OutputToFile(outfile_full, false));
    */

    EXPECT_TRUE(FileExists(outfile_full));
    FltRaster* tmp_rs = FltRaster::Init(outfile_full, true);
    EXPECT_FALSE(nullptr == tmp_rs);
    EXPECT_TRUE(tmp_rs->PositionsCalculated());
    int len;
    float* validdata = nullptr;
    tmp_rs->GetRasterData(&len, &validdata);
    EXPECT_EQ(len, maskrs_->GetCellNumber());
    EXPECT_NE(nullptr, validdata);
    for (int k = 0; k < len; k++) {
        EXPECT_FLOAT_EQ(validdata[k], datafull[k]);
    }
    delete tmp_rs;

#ifdef USE_MONGODB
    /** Output subset data to MongoDB **/
    EXPECT_TRUE(maskrs_->OutputSubsetToMongoDB(GlobalEnv->gfs_, "", ccgl::STRING_MAP(),
                    true, false, false)); // store fullsize data

    EXPECT_TRUE(maskrs_->OutputSubsetToMongoDB(GlobalEnv->gfs_, "", ccgl::STRING_MAP(),
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
        string gfsfull = itoa(it->first) + "_" + maskrs_->GetCoreName();
        EXPECT_TRUE(it->second->ReadFromMongoDB(GlobalEnv->gfs_, gfsfull, opts_full));
    }
    for (auto it = subsetsvalid.begin(); it != subsetsvalid.end(); ++it) {
        string gfsvalid = itoa(it->first) + "_" + maskrs_->GetCoreName();
        EXPECT_TRUE(it->second->ReadFromMongoDB(GlobalEnv->gfs_, gfsvalid, opts_valid));
    }

    // check consistent of valid values loaded in gfsfull and gfsvalid
    for (auto it = subsetsfull.begin(); it != subsetsfull.end(); ++it) {
        SubsetPositions* full = it->second;
        SubsetPositions* valid = subsetsvalid.at(it->first);
        EXPECT_EQ(full->n_lyrs, valid->n_lyrs);
        EXPECT_EQ(full->alloc_, valid->alloc_);
        EXPECT_EQ(full->n_cells, valid->n_cells);
        EXPECT_EQ(full->g_srow, valid->g_srow);
        EXPECT_EQ(full->g_erow, valid->g_erow);
        EXPECT_EQ(full->g_scol, valid->g_scol);
        EXPECT_EQ(full->g_ecol, valid->g_ecol);
        EXPECT_NE(nullptr, full->data_);
        EXPECT_EQ(nullptr, full->data2d_);
        EXPECT_NE(nullptr, valid->data_);
        EXPECT_EQ(nullptr, valid->data2d_);
        for (int i = 0; i < full->n_cells; i++) {
            EXPECT_EQ(full->local_pos_[i][0], valid->local_pos_[i][0]);
            EXPECT_EQ(full->local_pos_[i][1], valid->local_pos_[i][1]);
            EXPECT_EQ(full->local_posidx_[i], valid->local_posidx_[i]);
            EXPECT_DOUBLE_EQ(full->data_[i], valid->data_[i]);
        }
    }

    delete maskrs4mongodata;
    delete maskrs4mongodata2;

    /** Output subset data to new single file **/
    EXPECT_TRUE(maskrs_->OutputSubsetToMongoDB(GlobalEnv->gfs_, "", ccgl::STRING_MAP(),
                    true, false, true)); // store fullsize data

    EXPECT_TRUE(maskrs_->OutputSubsetToMongoDB(GlobalEnv->gfs_, "", ccgl::STRING_MAP(),
                    false, false, true)); // store valid data only

    if (HasFailure()) { return; }

    string outgfsfile_full = "0_" + maskrs_->GetCoreName();

    IntRaster* mongofull = IntRaster::Init(GlobalEnv->gfs_, outgfsfile_full.c_str(),
                                           true, maskrs_, true, -9999, opts_full);
    IntRaster* mongovalid = IntRaster::Init(GlobalEnv->gfs_, outgfsfile_full.c_str(),
                                            true, maskrs_, true, -9999, opts_valid);

    for (int k = 0; k < mongofull->GetValidNumber(); k++) {
        EXPECT_EQ(datafull[k], mongofull->GetValueByIndex(k));
        EXPECT_EQ(mongofull->GetValueByIndex(k), mongovalid->GetValueByIndex(k));
    }
    delete mongofull;
    delete mongovalid;
#endif

    // release newdata
    for (auto it = newdata.begin(); it != newdata.end(); ++it) {
        Release1DArray(it->second);
    }
    newdata.clear();
    Release1DArray(datafull);
}

// Raster IO based on mask layer which has several subset
//   1. output raster data according to mask's subset
//   2. read subset data to combine an entire raster data
TEST_P(clsRasterDataSplitMerge, SplitRaster) {
    // prepare mask data with user-specified groups
    EXPECT_FALSE(maskrsflt_->PositionsCalculated());
    map<int, int> new_group;
    new_group[1] = 1;
    new_group[2] = 2;
    new_group[3] = 3;
    new_group[4] = 1;
    EXPECT_TRUE(maskrsflt_->BuildSubSet(new_group));
    EXPECT_TRUE(maskrsflt_->PositionsCalculated());
    // read raster data in float type with mask
    FltRaster* rs = FltRaster::Init(GetParam()->raster_name,
                                    false, maskrsflt_, true);
    EXPECT_NE(nullptr, rs);
    int orglen;
    float* orgvalues;
    rs->GetRasterData(&orglen, &orgvalues);
    EXPECT_EQ(orglen, 20);
    EXPECT_NE(nullptr, orgvalues);

    map<int, SubsetPositions*>& subset = maskrsflt_->GetSubset();
    map<int, SubsetPositions*>& rs_subset = rs->GetSubset();
    EXPECT_FALSE(subset.empty());
    EXPECT_FALSE(rs_subset.empty());
    EXPECT_EQ(subset.size(), 3);
    EXPECT_EQ(rs_subset.size(), 2);

    /** Output subset to new files **/
    string outfile = ConcatFullName(Dstpath, corename, GetSuffix(GetParam()->raster_name));
    EXPECT_TRUE(rs->OutputSubsetToFile(true, false, outfile));
    /** Check the output files **/
    map<int, float*> subarray = map<int, float*>();
    float* data1 = nullptr;
    Initialize1DArray(10, data1, -9999.f);
    data1[0] = 1.1f;
    data1[1] = 4.4f;
    data1[2] = 5.5f;
    data1[3] = 10.f;
    data1[4] = 11.11f;
    data1[5] = 12.12f;
    data1[6] = 15.15f;
    data1[7] = 16.16f;
    data1[8] = 17.17f;
    data1[9] = 22.22f;
    subarray[1] = data1;

    float* data3 = nullptr;
    Initialize1DArray(6, data3, -9999.f);
    data3[0] = 2.2f;
    data3[1] = 8.8f;
    data3[2] = 14.14f;
    data3[3] = 19.19f;
    data3[4] =20.2f;
    data3[5] = 21.21f;
    subarray[3] = data3;

    float* datacom = nullptr;
    Initialize1DArray(20, datacom, -9999.f);
    datacom[0] = 1.1f;
    datacom[1] = 2.2f;
    datacom[2] = 4.4f;
    datacom[3] = 5.5f;
    datacom[4] = -9999.f;
    datacom[5] = -9999.f;
    datacom[6] = 8.8f;
    datacom[7] = 10.f;
    datacom[8] = 11.11f;
    datacom[9] = 12.12f;
    datacom[10] = -9999.f;
    datacom[11] = 14.14f;
    datacom[12] = 15.15f;
    datacom[13] = 16.16f;
    datacom[14] = 17.17f;
    datacom[15] = -9999.f;
    datacom[16] = 19.19f;
    datacom[17] = 20.2f;
    datacom[18] = 21.21f;
    datacom[19] = 22.22f;
    for (int i = 0; i < 20; i++) {
        EXPECT_EQ(orgvalues[i], datacom[i]);
    }

    float* datacomvalid = nullptr;
    Initialize1DArray(16, datacomvalid, -9999.f);
    datacomvalid[0] = 1.1f;
    datacomvalid[1] = 2.2f;
    datacomvalid[2] = 4.4f;
    datacomvalid[3] = 5.5f;
    datacomvalid[4] = 8.8f;
    datacomvalid[5] = 10.f;
    datacomvalid[6] = 11.11f;
    datacomvalid[7] = 12.12f;
    datacomvalid[8] = 14.14f;
    datacomvalid[9] = 15.15f;
    datacomvalid[10] = 16.16f;
    datacomvalid[11] = 17.17f;
    datacomvalid[12] = 19.19f;
    datacomvalid[13] = 20.2f;
    datacomvalid[14] = 21.21f;
    datacomvalid[15] = 22.22f;

    vector<string> outfiles;
    for (auto it = rs_subset.begin(); it != rs_subset.end(); ++it) {
        string outfilesub = PrefixCoreFileName(outfile, it->first);
        outfiles.push_back(outfilesub);
        EXPECT_TRUE(FileExists(outfilesub));
        float* tmp = subarray.at(it->first);
        FltRaster* tmp_rs = FltRaster::Init(outfilesub, true);
        EXPECT_FALSE(nullptr == tmp_rs);
        EXPECT_TRUE(tmp_rs->PositionsCalculated());
        int len;
        float* validdata = nullptr;
        tmp_rs->GetRasterData(&len, &validdata);
        for (int k = 0; k < len; k++) {
            EXPECT_FLOAT_EQ(validdata[k], tmp[k]);
        }
        delete tmp_rs;
    }
    // Combine raster data using mask's subset data
    for (auto it = subset.begin(); it != subset.end(); ++it) {
        string outfilesub = PrefixCoreFileName(outfile, it->first);
        if (!FileExists(outfilesub)) {
            it->second->usable = false;
            continue;
        }
        FltRaster* tmpsub = FltRaster::Init(outfilesub, true);
        int len;
        float* validdata = nullptr;
        tmpsub->GetRasterData(&len, &validdata);
        it->second->SetData(len, validdata);
        delete tmpsub;
    }
    string combined_file = AppendCoreFileName(outfile, "combined");
    maskrsflt_->OutputToFile(combined_file, false);
    FltRaster* rs_comb = FltRaster::Init(PrefixCoreFileName(combined_file, 0), true);
    EXPECT_NE(nullptr, rs_comb);
    int comblen;
    float* combvalues;
    rs_comb->GetRasterData(&comblen, &combvalues);
    EXPECT_EQ(comblen, 16);
    EXPECT_NE(nullptr, combvalues);
    for (int i = 0; i < 16; i++) {
        EXPECT_EQ(combvalues[i], datacomvalid[i]);
    }
    delete rs_comb;

#ifdef USE_MONGODB
    // Store valid subset data to MongoDB
    EXPECT_TRUE(rs->OutputSubsetToMongoDB(GlobalEnv->gfs_, "", ccgl::STRING_MAP(),
                    false, true, false)); // store valid data only
    // Combine raster data using mask's subset data
    for (auto it = subset.begin(); it != subset.end(); ++it) {
        string gfsvalid = itoa(it->first) + "_" + rs->GetCoreName();
        if (!it->second->ReadFromMongoDB(GlobalEnv->gfs_, gfsvalid, ccgl::STRING_MAP())) {
            it->second->usable = false;
        }
    }
    string com_fname = rs->GetCoreName() + "_com_frommongo";
    string com_fname_real = "0_" + com_fname;
    EXPECT_TRUE(maskrsflt_->OutputToMongoDB(GlobalEnv->gfs_, com_fname, ccgl::STRING_MAP(),
                    true, false));

    if (HasFailure()) { return; }

    // read from MongoDB with mask data
    FltRaster* com_inrs_mongo = FltRaster::Init(GlobalEnv->gfs_, com_fname_real.c_str(),
                                                false, maskrsflt_, true);
    EXPECT_NE(nullptr, com_inrs_mongo);
    EXPECT_EQ(20, com_inrs_mongo->GetCellNumber());
    for (int i = 0; i < 20; i++) {
        EXPECT_FLOAT_EQ(datacom[i], com_inrs_mongo->GetValueByIndex(i));
    }
    delete com_inrs_mongo;
    // read from MongoDB without mask data
    FltRaster* com_inrs_mongo2 = FltRaster::Init(GlobalEnv->gfs_, com_fname_real.c_str(),
                                                 true);
    EXPECT_NE(nullptr, com_inrs_mongo2);
    EXPECT_EQ(16, com_inrs_mongo2->GetCellNumber());
    for (int i = 0; i < 16; i++) {
        EXPECT_FLOAT_EQ(datacomvalid[i], com_inrs_mongo2->GetValueByIndex(i));
    }
    delete com_inrs_mongo2;
#endif

    for (auto it = subarray.begin(); it != subarray.end(); ++it) {
        Release1DArray(it->second);
    }
    subarray.clear();

    Release1DArray(datacom);
    Release1DArray(datacomvalid);
    delete rs;
}


#ifdef USE_GDAL
INSTANTIATE_TEST_CASE_P(SingleLayer, clsRasterDataSplitMerge,
                        Values(new InputRasterFiles(mask_asc_file, rs1_asc),
                            new InputRasterFiles(mask_tif_file, rs1_tif)),);
#else
INSTANTIATE_TEST_CASE_P(SingleLayer, clsRasterDataSplitMerge,
                        Values(new InputRasterFiles(mask_asc_file, rs1_asc)),);
#endif /* USE_GDAL */

} /* namespace */
