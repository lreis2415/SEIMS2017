/*!
 * \brief The main function should only be present once under the gtest framework.
 *
 * \version 1.0
 * \authors Liangjun Zhu, zlj(at)lreis.ac.cn; crazyzlj(at)gmail.com
 * \remarks 
 *     2017-12-02 - lj - Initial version.
 *     2019-11-06 - lj - Add global test environment to initialize input arguments.
 */
#if (defined _DEBUG) && (defined _MSC_VER) && (defined VLD)
#include "vld.h"
#endif /* Run Visual Leak Detector during Debug */
#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#pragma warning(disable : 4996) // 'function': was declared deprecated
#pragma warning(disable : 4127) // Use exception for catching assert
#endif
#include "gtest/gtest.h"
#ifdef USE_GDAL
#include <gdal.h>
#include <utility>
#endif

#include "test_global.h"
#include "../src/basic.h"
#include "../src/utils_filesystem.h"
#include "../src/utils_string.h"
#include "../src/db_mongoc.h"

using namespace ccgl;
using namespace utils_filesystem;

GlobalEnvironment* GlobalEnv;

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    // Parse the input arguments
    int i = 1;
    char* strend = nullptr;
    string mongo_host = "127.0.0.1";
    vint16_t mongo_port = 27017;
    while (argc > i) {
        if (utils_string::StringMatch(argv[i], "-host")) {
            i++;
            if (argc > i) {
                mongo_host = argv[i];
                i++;
            }
        }
        else if (utils_string::StringMatch(argv[i], "-port")) {
            i++;
            if (argc > i) {
                mongo_port = static_cast<vint16_t>(strtol(argv[i], &strend, 10));
                i++;
            }
        }
    }
#ifdef USE_MONGODB
    using namespace db_mongoc;
    MongoClient* client_ = MongoClient::Init(mongo_host.c_str(), mongo_port);
    MongoGridFs* gfs_ = new MongoGridFs(client_->GetGridFs("test", "spatial"));
    GlobalEnv = new GlobalEnvironment(client_, gfs_);
    ::testing::AddGlobalTestEnvironment(GlobalEnv);
#endif

    SetDefaultOpenMPThread();

#ifdef USE_GDAL
    GDALAllRegister(); // Register GDAL drivers!
#endif
    // Create new directory for outputs if not exists.
    string apppath = GetAppPath();
    string resultpath = apppath + "./data/raster/result";
    if (!DirectoryExists(resultpath)) { CleanDirectory(resultpath); }

#if (defined _DEBUG) && (defined _MSC_VER) && (defined VLD)
    // Get a checkpoint of the memory after Google Test has been initialized. (not finished yet! by lj)
    // Solving the memory leak problem caused by Google test in an imperfect way in MSVC with VLD.
    // refers: [1] https://stackoverflow.com/questions/12704543/memory-leak-when-using-google-test-on-windows
    //         [2] https://github.com/google/googletest/issues/624 (still unsolved by Google Test)
    //         [3] https://github.com/Tencent/rapidjson/blob/master/test/unittest/unittest.cpp
    _CrtMemState memoryState = { 0 };
    (void)memoryState;
    _CrtMemCheckpoint(&memoryState);
#endif /* Run Visual Leak Detector during Debug */

    int retval = RUN_ALL_TESTS();

#if (defined _DEBUG) && (defined _MSC_VER) && (defined VLD)
    // Check for leaks after tests have run
    // Current Google test constantly leak 2 blocks at exit
    _CrtMemDumpAllObjectsSince(&memoryState);
#endif /* Run Visual Leak Detector during Debug */

#ifdef USE_MONGODB
    //delete GlobalEnv;
    delete client_;
    delete gfs_;
#endif
    return retval;
}
