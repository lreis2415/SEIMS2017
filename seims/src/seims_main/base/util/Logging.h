/*!
 * \file Logging.h
 * \brief Wapper class for easylogging++.h
 * 
 *  Reference:
 *    Copyright 2014 Stellar Development Foundation and contributors. Licensed
 *    under the ISC License. See the COPYING file at the top-level directory of
 *    this distribution or at http://opensource.org/licenses/ISC
 *
 * \author Liangjun Zhu
 * \date 19/08/2020
 */
#ifndef SEIMS_UTIL_LOGGING
#define SEIMS_UTIL_LOGGING

// macros="-DELPP_THREAD_SAFE -DELPP_FEATURE_CRASH_LOG"  ## Macros for library
// https://github.com/amrayn/easyloggingpp/blob/master/samples/STL/shared-static-libs/compile_shared.sh
#define ELPP_STL_LOGGING
#define ELPP_THREAD_SAFE
#define ELPP_FEATURE_CRASH_LOG
#define ELPP_DISABLE_DEFAULT_CRASH_HANDLING
#define ELPP_NO_DEFAULT_LOG_FILE
#define ELPP_LOGGING_FLAGS_FROM_ARG

#if defined(_MSC_VER) && (_MSC_VER <= 1600)
#pragma warning(disable: 4482)
#endif /* Ignore warnings of nonstandard extension used: enum 'xxx' used in qualified name */

// NOTE: Nothing else should include "easylogging++.h" directly,
//  include this file ("Logging.h") instead
#include "easylogging++.h"

// Define logger IDs
static const char LOG_DEFAULT[] = "default";
static const char LOG_TIMESPAN[] = "TIMESPAN";
static const char LOG_INIT[] = "INITIALIZE";
static const char LOG_OUTPUT[] = "OUTPUT";
static const char LOG_RELEASE[] = "RELEASE";


el::base::type::StoragePointer sharedLoggingRepository();

/*!
 * \class Logging
 * \ingroup util
 * \brief Logging swapper of easylogging++.h
 */
class Logging {
    static el::Configurations gDefaultConf;
public:
    static void init();
    static void setFmt(bool timestamps = true);
    static void setLoggingToFile(std::string const& filename);
    static void setLogLevel(el::Level level, const char* partition);
    static el::Level getLLfromString(std::string const& levelName);
    static el::Level getLogLevel(std::string const& partition);
    static std::string getStringFromLL(el::Level);
    static bool logDebug(std::string const& partition);
    static bool logTrace(std::string const& partition);
    static void rotate();
};

#endif  // SEIMS_UTIL_LOGGING
