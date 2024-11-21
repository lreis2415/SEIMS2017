/*!
 * \brief IO of raster data or dataset based on mask layer.
 *
 * \author Liang-Jun Zhu, zlj(at)lreis.ac.cn
 * \remarks
 *     - 1. 2021-11-25 - lj - Rewrite as an stand-alone application inside CCGL
 *     - 2. 2022-04-29 - lj - Support multiple subsets, support IO of file and MongoDB
 *
 * \copyright 2017-2022. LREIS, IGSNRR, CAS
 *
 */

#if (defined _DEBUG) && (defined _MSC_VER) && (defined VLD)
#include "vld.h"
#endif /* Run Visual Leak Detector during Debug */

#include "mask_rasterio.h"

IOMODE StringToIOMode(const string& str) {
    string mode_str = GetUpper(str);
    if (mode_str == "DEC" || mode_str == "DECOMPOSE") { return DEC; }
    if (mode_str == "COM" || mode_str == "COMBINE") { return COM; }
    if (mode_str == "MASK&DEC" || mode_str == "MASKDEC") { return MASKDEC; }
    if (mode_str == "MASK") { return MASK; }
    return UNKNOWNMODE;
}

DATAFMT StringToDataFormat(const string& str) {
    string fmt_str = GetUpper(str);
    if (fmt_str == "SFILE" || fmt_str == "FILE") { return SFILE; }
    if (fmt_str == "GFS" || fmt_str == "GRIDFS") { return GFS; }
    return UNKNOWNFMT;
}

void Usage(const string& appname, const string& error_msg /* = std::string() */) {
    if (!error_msg.empty()) {
        cout << "FAILURE: " << error_msg << endl;
    }
    string corename = GetCoreFileName(appname);
    cout << "Simple Usage:\n    " << corename << " <configFile>\n\n";
    cout << "Two complete usage modes are supported:\n";
    cout << "1. " << corename <<
            " -in [<inFmt>] [<inFile>[,<inFile2>,...]] [<inGFSName>]"
            " [-mask [<maskFmt>] [<maskFile>] [<maskGFSName>]]"
            " [-reclass <reclassifyList>]"
            " [-mode <IOMode>]"
            " [-out [<outFmt>] [<outFile>] [<outGFSName>]"
            " [-outdatatype <outDataType>] [-default <defaultValue>] [-nodata <updatedNodata>]"
            " [-include_nodata <includeNoData>]"
            " [-mongo <host> <port> <DB> <GFS>]"
            " [-thread <threadsNum>]"
            " [-opts <options>]\n\n";
    cout << "2. " << corename << " -configfile <configFile> [-thread <threadsNum>]\n\n";

    cout << "\t<xxFmt> is data format for <in>, <out>, and <mask>, can be FILE or GFS.\n";
    cout << "\t<xxFile> and <xxFile2>... are full paths of raster, ASCII and GeoTIFF are recommended.\n";
    cout << "\t<xxGFSName> is the GridFS file name in MongoDB.\n";
    cout << "\t<reclassifyList> is the reclassified list, the format is origin1:new1,origin2:new2,.... "
            "The new value can be single value or multi values, e.g., 1.2 or 1.2|1.3|2.2 \n";
    cout << "\t<opts> is user-specified options, will be appended to metadata, e.g., paramtype=concpetual,xxx=yyy,...\n";
    cout << "\t<IOMode> is the operation type, can be MASK (default), DEC (Decompose), MASK&DEC, or COM (Combine).\n";
    cout << "\tIf -out is not specified, <in>_masked.tif will be generated.\n";
    cout << "\t<outDataType> is the data type of <out>, uint8, int8, uint16,"
            " int16, uint32, int32, float, and double are supported.\n";
    cout << "\t<defaultValue> is default value for nodata locations that covered by mask.\n";
    cout << "\t<updatedNodata> is updated nodata value.\n";
    cout << "\t<includeNoData> is used when output raster data into MongoDB, can be 1 or 0.\n";
    cout << "\t<threadsNum> is the number of thread used by OpenMP, which must be >= 1 (default).\n";
    cout << "\t-mongo specify the MongoDB configuration, including host, port, DB, and GFS.\n";
    cout << "\t<configFile> is a plain text file that defines all input parameters, the format is:\n";
    cout << "\t\t[-mode\t<IOMode>]";
    cout << "\t\t[-mongo\t<host>\t<port>\t<DB>\t<GFS>]\n";
    cout << "\t\t-mask\t[<maskFmt>\t][<maskFile>|<maskGFSName>]";
    cout << "\t\t[-in\t<inFmt>]\n";
    cout << "\t\t[-out\t<outFmt>]\n";
    cout << "\t\t[-outdatatype\t<outDataType>]\n";
    cout << "\t\t[-include_nodata\t<includeNoData>]\n";
    cout << "\t\t\"<in1>,<in2>,...;<out>;[<defaultValue>];[<updatedNodata>];[<outDataType>];"
            "[<reclassifyList>]\"\n";
    cout << "\t\t...\n\n";
}


bool parse_fmt_paths(const string& tag, const vector<string>& strs, DATAFMT& fmt, vector<string>& paths) {
    if (strs.empty()) { return false; }
    size_t tmpcount = strs.size();
    int path_idx = tmpcount > 1 ? 1 : 0;
    DATAFMT tmpfmt = StringToDataFormat(strs[0]);
    vector<string> file_paths = SplitString(strs[path_idx], ',');
    if (!StringMatch(tag, "OUT") && FilesExist(file_paths)) {
        // 1: {"/path/to/single_file.tif"},
        // 3: {"file", "/path/to/single_file.tif"}
        // 5: {"/path/to/file.tif;/path/to/file2.tif"}
        // 6: {"file", "/path/to/file.tif;/path/to/file2.tif"}
        // other: {"any_words", "path/to/file.tif"}
        fmt = SFILE;
    } else {
        if (tmpcount == 1) {
            if (tmpfmt == GFS || tmpfmt == SFILE) {
                // 7: {"file"} or {"gfs"}
                fmt = tmpfmt;
            }
            else if (file_paths.at(path_idx).find_first_of(SEP) != string::npos) {
                fmt = SFILE;
            }
            else {
                // 2: {"single_gfsname"}
                fmt = GFS; // we can only guess this is a GridFS filename!
            }
        }
        else {
            if (tmpfmt == GFS || tmpfmt == SFILE) { // 4: {"gfs", "single_gfsname"} or for "OUT" tag!
                fmt = tmpfmt;
            }
            else {
                cout << "Error: Illegal format for " << tag << "\n";
                return false;
            }
        }
    }
    for (auto it = file_paths.begin(); it != file_paths.end(); ++it) {
        paths.emplace_back(*it);
    }
    return true;
}

bool parse_key_values(string& kvstrs, map<vint, vector<double> >& kv) {
    if (kvstrs.empty()) { return false; }
    bool valid_kv = true;
    bool str2num_flag = false;
    vector<string> tmpkvlist = SplitString(kvstrs, ',');
    if (tmpkvlist.empty()) { return false; }
    for (auto it_kvlist = tmpkvlist.begin(); it_kvlist != tmpkvlist.end(); ++it_kvlist) {
        if ((*it_kvlist).empty()) { continue; }
        vector<string> tmpkv = SplitString(*it_kvlist, ':');
        if (tmpkv.size() != 2) { continue; }
        vint tmp_key = IsInt(tmpkv.at(0), str2num_flag);
        if (!str2num_flag) { continue; }
        vector<string> tmpvalues = SplitString(tmpkv.at(1), '|');
        vector<double> v;
        for (auto it = tmpvalues.begin(); it != tmpvalues.end(); ++it) {
            double tmp_value = IsDouble(*it, str2num_flag);
            if (!str2num_flag) { break; }
            v.emplace_back(tmp_value);
        }
        if (tmpvalues.size() == v.size()) {
#ifdef HAS_VARIADIC_TEMPLATES
            kv.emplace(tmp_key, v);
#else
            kv.insert(make_pair(tmp_key, v));
#endif
        } else {
            valid_kv = false;
        }
    }
    return valid_kv;
}

bool parse_key_values(string& kvstrs, STRING_MAP& res) {
    if (kvstrs.empty()) { return false; }
    bool str2num_flag = false;
    vector<string> kvlist = SplitString(kvstrs, ',');
    if (kvlist.empty()) { return false; }
    for (auto it_kvlist = kvlist.begin(); it_kvlist != kvlist.end(); ++it_kvlist) {
        if ((*it_kvlist).empty()) { continue; }
        vector<string> opt = SplitString(*it_kvlist, '=');
        if (opt.size() != 2) { continue; }
        UpdateStringMap(res, opt.at(0), opt.at(1));
    }
    return true;
}
/*!
 * \return
 *   0. Succeed
 *   1. Format error of input arguments
 *   2. File specified but not existed, including configuration file and input data files
 *   3. File content loaded failed or wrong format
 *
 */
int main(const int argc, const char** argv) {
    IOMODE mode = MASK;
    int thread_num = 1;
    bool inc_nodata = true;

    bool use_mongo = false;
#ifdef USE_MONGODB
    string mongo_host;
    vint16_t mongo_port;
    string dbname;
    string gfsname;
    MongoClient* client = nullptr;
    MongoGridFs* gfs = nullptr;
#endif

    DATAFMT mask_fmt = UNKNOWNFMT;
    string mask_path;
    DATAFMT global_infmt = UNKNOWNFMT;
    vector<DATAFMT> in_fmts;
    vector<vector<string> > in_paths;
    DATAFMT global_outfmt = UNKNOWNFMT;
    vector<DATAFMT> out_fmts;
    vector<string> out_paths;
    double global_defaultv = NODATA_VALUE;
    bool global_updnodata = false;
    double global_nodata = NODATA_VALUE;
    vector<double> default_values;
    vector<bool> update_nodata;
    vector<double> nodata_values;
    RasterDataType global_outtype = RDT_Unknown;
    vector<RasterDataType> out_types;
    bool global_recls = false;
    vector<bool> reclass_data;
    vector<map<vint, vector<double> > > reclass_keyvalues;
    STRING_MAP opts; // Additional options, e.g., output data type

    if (argc < 2) {
        Usage(argv[0], "To run the program, "
              "use either a single configuration file and/or detail arguments as below.");
        return 1;
    }
    string config_path;
    vector<int> arg_sep;
    for (int idx = 1; idx < argc; idx++) {
        if (argv[idx] == nullptr || argv[idx][0] != '-') { continue; }
        if (StringMatch(argv[idx], "-configfile") && argc > idx && argv[idx + 1][0] != '-') {
            config_path = argv[idx + 1];
        }
        bool tmp_dbl_flag = false;
        IsDouble(argv[idx], tmp_dbl_flag);
        if (tmp_dbl_flag) { continue; }
        arg_sep.push_back(idx);
    }
    arg_sep.push_back(argc);

    map<string, vector<string> > key_args;
    if (arg_sep.empty()) {
        if (config_path.empty()) {
            if (argc >= 2 && argv[1][0] != '-') {
                config_path = argv[1];
            }
            else {
                Usage(argv[0], "Illegal inputs!");
                return 1;
            }
        }
    }
    else {
        for (auto arg_idx = arg_sep.begin(); arg_idx != arg_sep.end() - 1; ++arg_idx) {
            vector<string> tmpargs(argv + *arg_idx + 1, argv + *(arg_idx + 1));
            if (tmpargs.empty()) { continue; }
            string key(argv[*arg_idx]);
            if (StringMatch(key, "-configfile")) { continue; }
            key_args.insert(std::make_pair(GetUpper(key.substr(1, string::npos)), tmpargs));
        }
    }
    vector<string> config_strs;
    vector<vector<string> > io_strs;
    if (!config_path.empty()) {
        if (!FileExists(config_path)) {
            Usage(argv[0], "Configuration file specified but not existed!");
            return 2;
        }
        // Read configuration file
        if (!LoadPlainTextFile(config_path, config_strs)) {
            Usage(argv[0]);
            return 3;
        }
    }
    // Concatenate arguments from command line and configuration file
    for (auto it = config_strs.begin(); it != config_strs.end(); ++it) {
        string tmpstr = *it;
        TrimSpaces(tmpstr);
        if (tmpstr.empty()) { continue; }
        if (tmpstr[0] == '#') { continue; }
        if (tmpstr[0] == '-') { // configuration key and value that separated by space
            vector<string> tmpconfig = SplitString(tmpstr, '\t'); // Only support \t!!!
            if (tmpconfig.empty() || tmpconfig.size() < 2) {
                cout << tmpstr << " contains invalid configuration!" << endl;
                continue;
            }
            string tmpkey = GetUpper(tmpconfig[0].substr(1, string::npos));
            if (key_args.find(tmpkey) != key_args.end()) { continue; }
            key_args.insert(std::make_pair(tmpkey, vector<string>(tmpconfig.begin() + 1,
                                                                  tmpconfig.end())));
            continue;
        }
        vector<string> tmpconfig = SplitString(tmpstr, ';'); // data item configuration
        if (tmpconfig.empty()) {
            cout << tmpstr << " contains invalid data item!" << endl;
            continue;
        }
        io_strs.emplace_back(tmpconfig);
    }
    // Parse mixed arguments
    char* strend = nullptr;
    bool str2num_flag = false;
    for (auto itkv = key_args.begin(); itkv != key_args.end(); ++itkv) {
        if (itkv->first == "MODE") {
            mode = StringToIOMode(itkv->second.at(0));
            if (mode == UNKNOWNMODE) { return 1; }
        }
        else if (itkv->first == "IN" || itkv->first == "INPUT") {
            vector<string> tmppaths;
            if (!parse_fmt_paths("IN", itkv->second, global_infmt, tmppaths)) {
                return 2;
            }
            if (!tmppaths.empty()) {
                in_paths.emplace_back(tmppaths);
                in_fmts.emplace_back(global_infmt);
            }
        }
        else if (itkv->first == "MASK") {
            vector<string> tmppaths;
            if (!parse_fmt_paths("MASK", itkv->second, mask_fmt, tmppaths)
                && tmppaths.empty()) {
                return 2;
            }
            mask_path = tmppaths[0];
        }
        else if (itkv->first == "OUT" || itkv->first == "OUTPUT") {
            vector<string> tmppaths;
            if (parse_fmt_paths("OUT", itkv->second, global_outfmt, tmppaths)) {
                if (!tmppaths.empty()) {
                    out_paths.emplace_back(tmppaths[0]);
                    out_fmts.emplace_back(global_outfmt);
                }
            }
        }
        else if (itkv->first == "OUTDATATYPE") {
            global_outtype = StringToRasterDataType(itkv->second.at(0));
        }
        else if (itkv->first == "DEFAULT" || itkv->first == "DEFAULTVALUE") {
            global_defaultv = IsDouble(itkv->second.at(0), str2num_flag);
            if (!str2num_flag) { global_defaultv = NODATA_VALUE; }
        }
        else if (itkv->first == "NODATA" || itkv->first == "NODATAVALUE") {
            global_nodata = IsDouble(itkv->second.at(0), str2num_flag);
            global_updnodata = true;
            if (!str2num_flag) { global_nodata = NODATA_VALUE; }
        }
        else if (itkv->first == "RECLASS") {
            map<vint, vector<double> > tmp_recls = map<vint, vector<double> >();
            global_recls = parse_key_values(itkv->second.at(0), tmp_recls);
            reclass_keyvalues.emplace_back(tmp_recls);
        }
        else if (itkv->first == "MONGO" || itkv->first == "MONGODB") {
#ifdef USE_MONGODB
            if (itkv->second.size() < 4) {
                cout << "Warning: -mongo tag MUST have 4 arguments!\n";
                return 1;
            }
            mongo_host = itkv->second.at(0);
            mongo_port = static_cast<vint16_t>(strtol(itkv->second.at(1).c_str(),
                                                      &strend, 10));
            dbname = itkv->second.at(2);
            gfsname = itkv->second.at(3);
            client = MongoClient::Init(mongo_host.c_str(), mongo_port);
            if (nullptr == client) {
                cout << "Warning: Illegal arguments for MongoDB!\n";
            }
            else {
                gfs = client->GridFs(dbname, gfsname);
                if (nullptr == gfs) {
                    cout << "Warning: Get or create GridFS failed!\n";
                    use_mongo = false;
                }
                else { use_mongo = true; }
            }
#else
            cout << "Warning: MongoDB is not supported\n";
#endif
        }
        else if (itkv->first == "THREAD") {
            thread_num = CVT_INT(IsInt(itkv->second.at(0), str2num_flag));
            if (!str2num_flag) {
                cout << "Warning: Illegal thread number, use thread = 1 instead!\n";
                thread_num = 1;
            }
        }
        else if (itkv->first == "INCLUDE_NODATA") {
            inc_nodata = IsInt(itkv->second.at(0), str2num_flag) > 0;
            if (!str2num_flag) {
                cout << "Warning: Illegal flag for INCLUDE_NODATA, use 1 instead!\n";
                inc_nodata = true;
            }
        }
        else if (itkv->first == "OPTS") {
            parse_key_values(itkv->second.at(0), opts);
        }
        else {
            cout << "Warning: Unknown Tag that will be ignored: " << itkv->first << "\n";
        }
    }
    // Make sure in_paths, out_paths, default_values, nodata_values, and out_types have same size
    if (!use_mongo) {
        if (global_outfmt == GFS) { global_outfmt = SFILE; }
    }
    if (global_outfmt == UNKNOWNFMT) {
        global_outfmt = global_infmt;
    }
    if (!in_paths.empty()) {
        if (in_paths.size() != 1) { return 1; }
        if (out_paths.empty()) {
            if (in_paths[0].size() == 1) {
                out_paths.emplace_back(AppendCoreFileName(in_paths[0][0], "masked"));
            } else {
                string core_name = GetCoreFileName(in_paths[0][0]);
                string::size_type last_underline = core_name.find_last_of('_');
                if (last_underline == string::npos) {
                    last_underline = core_name.length();
                }
                out_paths.emplace_back(GetPathFromFullName(in_paths[0][0])
                                       + core_name.substr(0, last_underline) + "_masked."
                                       + GetSuffix(in_paths[0][0]));
            }
        }
        if (default_values.empty()) { default_values.emplace_back(global_defaultv); }
        if (update_nodata.empty()) { update_nodata.push_back(global_updnodata); }  // compatible with vs2010
        if (nodata_values.empty()) { nodata_values.emplace_back(global_nodata); }
        if (out_types.empty()) { out_types.emplace_back(global_outtype); }
        if (out_fmts.empty()) { out_fmts.emplace_back(global_outfmt); }
        else { out_fmts[0] = global_outfmt; }
        if (reclass_data.empty()) { reclass_data.push_back(global_recls); }
        if (reclass_keyvalues.empty()) { reclass_keyvalues.emplace_back(map<vint, vector<double> >()); }
    }
    else { // clear others
        if (!out_paths.empty()) { vector<string>().swap(out_paths); }
        if (!default_values.empty()) { vector<double>().swap(default_values); }
        if (!update_nodata.empty()) { vector<bool>().swap(update_nodata); }
        if (!nodata_values.empty()) { vector<double>().swap(nodata_values); }
        if (!out_types.empty()) { vector<RasterDataType>().swap(out_types); }
        if (!out_fmts.empty()) { vector<DATAFMT>().swap(out_fmts); }
        if (!reclass_data.empty()) { vector<bool>().swap(reclass_data); }
        if (!reclass_keyvalues.empty()) { vector<map<vint, vector<double> > >().swap(reclass_keyvalues); }
    }
    // Parse input and output settings in configuration file
    for (auto ioit = io_strs.begin(); ioit != io_strs.end(); ++ioit) {
        size_t item_count = (*ioit).size(); // item_count has been checked to be sure >=1
        int inidx = 0;
        int outidx = item_count >= 2 ? 1 : -1;
        int dvidx = item_count >= 3 ? 2 : -1;
        int nodataidx = item_count >= 4 ? 3 : -1;
        int outtypeidx = item_count >= 5 ? 4 : -1;
        int reclsidx = item_count >= 6 ? 5 : -1;

        if (item_count == 1 && (*ioit).at(0).empty()) { continue; }

        vector<string> tmpin = SplitString((*ioit).at(inidx), ','); // could be ""
        string tmpout = outidx > 0 ? (*ioit).at(outidx) : "";

        if (tmpin.size() == 1 && tmpin.at(0).empty() && tmpout.empty()) { continue; }

        if (global_infmt == UNKNOWNFMT && FilesExist(tmpin)) { global_infmt = SFILE; }
        if (global_infmt == UNKNOWNFMT && !tmpin.empty() && use_mongo) { global_infmt = GFS; }
        if (global_outfmt != GFS && use_mongo) { global_outfmt = GFS; }
        if (global_outfmt == UNKNOWNFMT) { global_outfmt = SFILE; }

        double tmp_defv = NODATA_VALUE;
        if (dvidx > 0 && !(*ioit).at(dvidx).empty()) {
            tmp_defv = IsDouble((*ioit).at(dvidx), str2num_flag);
            if (!str2num_flag) { tmp_defv = NODATA_VALUE; }
        }
        bool upd_nodata = false;
        double tmp_nodata = NODATA_VALUE;
        if (nodataidx > 0 && !(*ioit).at(nodataidx).empty()) {
            tmp_nodata = IsDouble((*ioit).at(nodataidx), str2num_flag);
            if (!str2num_flag) { tmp_nodata = NODATA_VALUE; }
            else { upd_nodata = true; }
        }
        RasterDataType tmpouttype = RDT_Unknown;
        if (outtypeidx > 0) {
            tmpouttype = StringToRasterDataType((*ioit).at(outtypeidx));
            if (tmpouttype == RDT_Unknown) { tmpouttype = global_outtype; }
        }
        bool do_recls = false;
        map<vint, vector<double> > tmp_recls = map<vint, vector<double> >();
        if (reclsidx > 0) {
            do_recls = parse_key_values((*ioit).at(reclsidx), tmp_recls);
        }
        if ((!FilesExist(tmpin) && do_recls)
            || (global_infmt == SFILE && FilesExist(tmpin))
            || global_infmt == GFS) {
            in_paths.emplace_back(tmpin);
            in_fmts.emplace_back(global_infmt);
            out_paths.emplace_back(tmpout);
            out_fmts.emplace_back(global_outfmt);
            default_values.emplace_back(tmp_defv);
            update_nodata.push_back(upd_nodata);  // compatible with vs2010
            nodata_values.emplace_back(tmp_nodata);
            out_types.emplace_back(tmpouttype);
            reclass_data.push_back(do_recls);
            reclass_keyvalues.emplace_back(tmp_recls);
        }
    }

#ifdef USE_GDAL
    GDALAllRegister();
#endif

    SetOpenMPThread(thread_num);

    // Load mask layer
    DblRaster* mask_layer = nullptr;
    if (!mask_path.empty()) {
        if (mask_fmt == SFILE && FileExists(mask_path)) {
            mask_layer = DblRaster::Init(mask_path);
            if (nullptr == mask_layer) { return 3; }
        } else if (mask_fmt == GFS && use_mongo) {
#ifdef USE_MONGODB
            mask_layer = DblRaster::Init(gfs, mask_path.c_str());
            if (nullptr == mask_layer) { return 3; }
#endif
        } else {
            // No mask layer
        }
    }

    // Load input raster
    bool output_all = false;
    bool output_subset = false;
    bool combine_subset = false;
    if (mode != MASK) {
        if (mask_layer != nullptr) { mask_layer->BuildSubSet(); }
        output_all = mode == MASKDEC;
        output_subset = mode == DEC || mode == MASKDEC;
        combine_subset = mode == COM;
    } else {
        output_all = true;
    }

    if (inc_nodata) {
        UpdateStringMap(opts, HEADER_INC_NODATA, "TRUE");
    } else {
        UpdateStringMap(opts, HEADER_INC_NODATA, "FALSE");
    }
    for (auto in_it = in_paths.begin(); in_it != in_paths.end(); ++in_it) {
        size_t in_idx = in_it - in_paths.begin();
        if (out_types.at(in_idx) == RDT_Unknown) {
            UpdateStringMap(opts, HEADER_RSOUT_DATATYPE, "DOUBLE");
            out_types.at(in_idx) = RDT_Double;
        } else {
            UpdateStringMap(opts, HEADER_RSOUT_DATATYPE, RasterDataTypeToString(out_types.at(in_idx)));
        }
        if (mode == MASK || mode == DEC || mode == MASKDEC) { // paths in *in_it are regarded as multiple layers
            DblRaster* rs = nullptr;
            if (in_fmts.at(in_idx) == SFILE && FilesExist(*in_it)) {
                rs = DblRaster::Init(*in_it,
                                     false,            // No need to calculate valid positions
                                     mask_layer, true, // Use entire extent of Mask
                                     default_values.at(in_idx));
                if (out_types.at(in_idx) != RDT_Unknown) { rs->SetOutDataType(out_types.at(in_idx)); }
            }
            else if (in_fmts.at(in_idx) == GFS && use_mongo) {
#ifdef USE_MONGODB
                rs = DblRaster::Init(gfs, (*in_it).at(0).c_str(),
                                     false,
                                     mask_layer, true,
                                     default_values.at(in_idx), opts);
#endif
            }
            else {
                continue;
            }
            if (nullptr == rs) { continue; }

            if (update_nodata.at(in_idx)) { rs->ReplaceNoData(nodata_values.at(in_idx)); }

            if (reclass_data.at(in_idx)) {
                rs->BuildSubSet();
            }

            if (out_fmts.at(in_idx) == SFILE) {
                if (reclass_data.at(in_idx)) {
                    if (output_subset) {
                        rs->OutputSubsetToFile(true, false,
                                               out_paths.at(in_idx), reclass_keyvalues.at(in_idx),
                                               default_values.at(in_idx));
                    }
                    if (output_all) {
                        rs->OutputSubsetToFile(true, true,
                                               out_paths.at(in_idx), reclass_keyvalues.at(in_idx),
                                               default_values.at(in_idx));
                    }
                } else {
                    if (output_subset) { rs->OutputToFile(out_paths.at(in_idx), false); }
                    if (output_all) { rs->OutputToFile(out_paths.at(in_idx), true); }
                }
            } else if (out_fmts.at(in_idx) == GFS && use_mongo) {
#ifdef USE_MONGODB
                if (reclass_data.at(in_idx)) {
                    if (output_subset) {
                        rs->OutputSubsetToMongoDB(gfs, out_paths.at(in_idx), opts,
                                                  inc_nodata, true, false,
                                                  reclass_keyvalues.at(in_idx),
                                                  default_values.at(in_idx));
                    }
                    if (output_all) {
                        rs->OutputSubsetToMongoDB(gfs, out_paths.at(in_idx), opts,
                                                  inc_nodata, true, true,
                                                  reclass_keyvalues.at(in_idx),
                                                  default_values.at(in_idx));
                    }
                } else {
                    if (output_subset) {
                        if (nullptr == mask_layer) {
                            rs->BuildSubSet();
                        }
                        rs->OutputSubsetToMongoDB(gfs, out_paths.at(in_idx), opts,
                                                  inc_nodata, true, false);
                    }
                    if (output_all) {
                        rs->OutputToMongoDB(gfs, "0_" + out_paths.at(in_idx), opts,
                                            inc_nodata, true);
                    }
                }
#endif
            } else {
                // Nothing to do
            }
            delete rs;
        } else if (mode == COM) {
            if (nullptr == mask_layer) {
                cout << "Error: The COMBINE mode MUST based on a mask layer!";
                return 1;
            }
            mask_layer->SetDefaultValue(default_values.at(in_idx));
            map<int, SubsetPositions*>& subset = mask_layer->GetSubset();
            for (auto it = subset.begin(); it != subset.end(); ++it) {
                it->second->usable = false;
            }
            for (auto inf_it = (*in_it).begin(); inf_it != (*in_it).end(); ++inf_it) {
                string tmpcname = GetCoreFileName(*inf_it);
                vector<string> cname_strs = SplitString(tmpcname, '_');
                if (cname_strs.empty()) { continue; }
                int subid = CVT_INT(IsInt(cname_strs[0], str2num_flag));
                if (!str2num_flag) { continue; }
                if (subset.find(subid) == subset.end()) { continue; }

                if (in_fmts.at(in_idx) == SFILE && FileExists(*inf_it)) {
                    DblRaster* tmpsubrs = nullptr;
                    tmpsubrs = DblRaster::Init(*inf_it, true);
                    if (nullptr == tmpsubrs) { continue; }
                    int tmpsublen;
                    double* tmpsubdata = nullptr;
                    tmpsubrs->GetRasterData(&tmpsublen, &tmpsubdata);
                    subset.at(subid)->SetData(tmpsublen, tmpsubdata);
                    delete tmpsubrs;
                }
                else if (in_fmts.at(in_idx) == GFS && use_mongo) {
#ifdef USE_MONGODB
                    subset.at(subid)->ReadFromMongoDB(gfs, *inf_it, opts);
#endif
                } else {
                    // Nothing to do
                }
            }
            mask_layer->SetOutDataType(out_types.at(in_idx));
            if (out_fmts.at(in_idx) == SFILE) {
                mask_layer->OutputToFile(out_paths.at(in_idx), false);
            }
            else if (out_fmts.at(in_idx) == GFS && use_mongo) {
#ifdef USE_MONGODB
                mask_layer->OutputToMongoDB(gfs, out_paths.at(in_idx), opts,
                                            inc_nodata, false);
#endif
            }
            else {
                // Nothing to do
            }
        } else {
            // Nothing to do
        }
    }
    delete mask_layer;
#ifdef USE_MONGODB
    if (use_mongo) {
        delete gfs;
        client->Destroy();
        delete client;
    }
#endif
    return 0;
}
