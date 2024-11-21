#include "DataCenterMongoDB.h"
#include "text.h"
#include "Logging.h"

const int MAIN_DB_TABS_REQ_NUM = 6;
const char* MAIN_DB_TABS_REQ[] = {
    DB_TAB_FILE_IN, DB_TAB_FILE_OUT, DB_TAB_SITELIST,
    DB_TAB_PARAMETERS, DB_TAB_REACH, DB_TAB_SPATIAL
};

const int METEO_VARS_NUM = 6;
const char* METEO_VARS[] = {
    DataType_MeanTemperature, DataType_MaximumTemperature,
    DataType_MinimumTemperature, DataType_SolarRadiation,
    DataType_WindSpeed, DataType_RelativeAirMoisture
};

const int SOILWATER_VARS_NUM = 5;
const char* SOILWATER_VARS[] = {
    VAR_SOL_WPMM[0], VAR_SOL_AWC_AMOUNT[0], VAR_SOL_UL[0],
    VAR_SOL_SUMAWC[0], VAR_SOL_SUMSAT[0]
};

DataCenterMongoDB::DataCenterMongoDB(InputArgs* input_args, MongoClient* client,
                                     MongoGridFs* spatial_gfs_in, MongoGridFs* spatial_gfs_out,
                                     ModuleFactory* factory,
                                     const int subbasin_id /* = 0 */) :
    DataCenter(input_args, factory, subbasin_id), mongodb_ip_(input_args->host.c_str()),
    mongodb_port_(input_args->port),
    mongo_client_(client), main_database_(nullptr),
    spatial_gridfs_(spatial_gfs_in), spatial_gfs_out_(spatial_gfs_out) {
    double input_t = TimeCounting();
    //spatial_gridfs_ = new MongoGridFs(mongo_client_->GetGridFs(model_name_, DB_TAB_SPATIAL));
    //spatial_gfs_out_ = new MongoGridFs(mongo_client_->GetGridFs(model_name_, DB_TAB_OUT_SPATIAL));
    if (DataCenterMongoDB::GetFileInStringVector()) {
        input_ = SettingsInput::Init(file_in_strs_);
        if (nullptr == input_) {
            throw ModelException("DataCenterMongoDB", "Constructor", "Failed to initialize m_input!");
        }
    } else {
        throw ModelException("DataCenterMongoDB", "Constructor", "Failed to query FILE_IN!");
    }
    is_subbasin_conceptual = factory->isSubbasinConceptual(subbasin_id_);
    //is_lumped = factory->isLumped(); //TODO: check if lumped model is OK if commented. --wyj
    if (is_subbasin_conceptual) {
        CLOG(INFO, LOG_INIT) << "Process " << mpi_rank_ << ": Subbasin " << subbasin_id_ << " is conceptual";
    }
    else {
        CLOG(INFO, LOG_INIT) << "Process " << mpi_rank_ << ": Subbasin " << subbasin_id_ << " is physical";
    }

    outlet_id_ = DataCenterMongoDB::ReadIntParameterInDB(VAR_OUTLETID[0]);
    n_subbasins_ = DataCenterMongoDB::ReadIntParameterInDB(VAR_SUBBSNID_NUM[0]);
    //if(is_lumped && n_subbasins_ > 1) {
    //    throw ModelException("DataCenterMongoDB", "checkModelPreparedData", "isLumped=0 in preprocess.ini but model structure config is lumped!");
    //}

    if (outlet_id_ < 0 || n_subbasins_ < 0) {
        throw ModelException("DataCenterMongoDB", "Constructor", "Query subbasin number and outlet ID failed!");
    }
    if (DataCenterMongoDB::GetFileOutVector()) {
        // The start and end time of output items should be checked and updated here! -LJ. 09/28/2020
        UpdateOutputDate(input_->getStartTime(), input_->getEndTime());
        output_ = SettingsOutput::Init(n_subbasins_, outlet_id_, subbasin_id_, origin_out_items_,
                                       scenario_id_, calibration_id_, mpi_rank_, mpi_size_);
        if (nullptr == output_) {
            throw ModelException("DataCenterMongoDB", "Constructor", "Failed to initialize m_output!");
        }
    } else {
        throw ModelException("DataCenterMongoDB", "Constructor", "Failed to query FILE_OUT!");
    }
    /// Check the existence of all required and optional data
    if (!DataCenterMongoDB::CheckModelPreparedData()) {
        throw ModelException("DataCenterMongoDB", "checkModelPreparedData", "Model data has not been set up!");
    }
}

DataCenterMongoDB::~DataCenterMongoDB() {
    CLOG(TRACE, LOG_RELEASE) << "Release DataCenterMongoDB...";
    /*if (spatial_gridfs_ != nullptr) {
        delete spatial_gridfs_;
        spatial_gridfs_ = nullptr;
    }
    if (spatial_gfs_out_ != nullptr) {
        delete spatial_gfs_out_;
        spatial_gfs_out_ = nullptr;
    }*/
    if (main_database_ != nullptr) {
        delete main_database_;
        main_database_ = nullptr;
    }
}

bool DataCenterMongoDB::CheckModelPreparedData() {
    /// 1. Check and get the main model database
    double input_t = TimeCounting();
    vector<string> existed_dbnames;
    mongo_client_->GetDatabaseNames(existed_dbnames);
    if (!ValueInVector(string(model_name_), existed_dbnames)) {
        LOG(ERROR) << "The main model is not existed: " << model_name_;
        return false;
    }
    LOG(INFO) << "DataCenterMongoDB::CheckModelPreparedData()1: " << TimeCounting() - input_t;
    main_database_ = new MongoDatabase(mongo_client_->GetDatabase(model_name_));
    /// 2. Check the existence of FILE_IN, FILE_OUT, PARAMETERS, REACHES, SITELIST, SPATIAL, etc
    vector<string> existed_main_db_tabs;
    main_database_->GetCollectionNames(existed_main_db_tabs);
    for (int i = 0; i < MAIN_DB_TABS_REQ_NUM; ++i) {
        if (!ValueInVector(string(MAIN_DB_TABS_REQ[i]), existed_main_db_tabs)) {
            LOG(ERROR) << "Table " << MAIN_DB_TABS_REQ[i] << " must be existed in " << model_name_;
            return false;
        }
    }

    LOG(INFO) << "DataCenterMongoDB::CheckModelPreparedData()2: " << TimeCounting() - input_t;
    /// 3. Read climate site information from Climate database
    clim_station_ = new InputStation(mongo_client_, input_->getDtHillslope(), input_->getDtChannel(), model_path_);
    ReadClimateSiteList();

    LOG(INFO) << "DataCenterMongoDB::CheckModelPreparedData()3: " << TimeCounting() - input_t;
    /// 4. Read initial parameters
    if (!ReadParametersInDB()) {
        return false;
    }
    LOG(INFO) << "DataCenterMongoDB::CheckModelPreparedData()4: " << TimeCounting() - input_t;
    DumpCaliParametersInDB();
    LOG(INFO) << "DataCenterMongoDB::CheckModelPreparedData()5: " << TimeCounting() - input_t;

    /// 5. Read Mask raster data
    std::ostringstream oss;
    oss << subbasin_id_ << "_" << VAR_SUBBSN[0]; // Tag_Mask[0];
    string mask_filename = GetUpper(oss.str());
    STRING_MAP opts;
    if (is_subbasin_conceptual) {
#ifdef HAS_VARIADIC_TEMPLATES
        opts.emplace(HEADER_RS_PARAM_ABSTRACTION_TYPE, PARAM_ABSTRACTION_TYPE_CONEPTUAL);
#else
        opts.insert(make_pair(HEADER_RS_PARAM_ABSTRACTION_TYPE, PARAM_ABSTRACTION_TYPE_CONEPTUAL));
#endif
    }
    LOG(INFO) << "DataCenterMongoDB::CheckModelPreparedData()6: " << TimeCounting() - input_t;
    mask_raster_ = IntRaster::Init(spatial_gridfs_, mask_filename.c_str(),
        false,nullptr,true,NODATA_VALUE,opts);
    assert(nullptr != mask_raster_);
    int v = mask_raster_->GetValidNumber();
#ifdef HAS_VARIADIC_TEMPLATES
    rs_int_map_.emplace(mask_filename, mask_raster_);
#else
    rs_int_map_.insert(make_pair(mask_filename, mask_raster_));
#endif

    if (is_subbasin_conceptual) {
#ifdef HAS_VARIADIC_TEMPLATES
        opts.emplace(HEADER_INC_NODATA, "FALSE");
#else
        opts.insert(make_pair(HEADER_INC_NODATA, "FALSE"));
#endif
    }
    LOG(INFO) << "DataCenterMongoDB::CheckModelPreparedData()7: " << TimeCounting() - input_t;
    /// 6. Constructor Subbasin data. Subbasin and slope data are required!
    oss.str("");
    oss << subbasin_id_ << "_" << VAR_SLOPE[0];
    LoadAdjustRasterData(VAR_SLOPE[0], GetUpper(oss.str()), false, &opts);
    oss.str("");
    oss << subbasin_id_ << "_" << VAR_CELL_AREA[0];
    LoadAdjustRasterData(VAR_CELL_AREA[0], GetUpper(oss.str()),false,&opts);
    subbasins_ = clsSubbasins::Init(rs_int_map_, rs_map_, subbasin_id_);
    assert(nullptr != subbasins_);
    LOG(INFO) << "DataCenterMongoDB::CheckModelPreparedData()8: " << TimeCounting() - input_t;

    /// 7. Read Reaches data, all reaches will be read for both MPI and OMP version
    if(n_subbasins_==1) {
        reaches_ = new clsReaches();
    } else {
        reaches_ = new clsReaches(mongo_client_, model_name_, DB_TAB_REACH, lyr_method_);
        reaches_->Update(init_params_, mask_raster_);
    }
    LOG(INFO) << "DataCenterMongoDB::CheckModelPreparedData()9: " << TimeCounting() - input_t;
    /// 8. Check if Scenario will be applied, Get scenario database if necessary
    if (ValueInVector(string(DB_TAB_SCENARIO), existed_main_db_tabs) && scenario_id_ >= 0) {
        bson_t* query = bson_new();
        scenario_dbname_ = QueryDatabaseName(query, DB_TAB_SCENARIO);
        if (!scenario_dbname_.empty()) {
            use_scenario_ = true;
            scenario_ = new Scenario(mongo_client_, scenario_dbname_, subbasin_id_, scenario_id_,
                                     input_->getStartTime(), input_->getEndTime());
            if (SetRasterForScenario()) {
                scenario_->setRasterForEachBMP();
            }
        }
    }
    LOG(INFO) << "DataCenterMongoDB::CheckModelPreparedData()10: " << TimeCounting() - input_t;
    return true;
}

string DataCenterMongoDB::QueryDatabaseName(bson_t* query, const char* tabname) {
    std::unique_ptr<MongoCollection>
            collection(new MongoCollection(mongo_client_->GetCollection(model_name_, tabname)));
    mongoc_cursor_t* cursor = collection->ExecuteQuery(query);
    const bson_t* doc;
    string dbname;
    while (mongoc_cursor_next(cursor, &doc)) {
        bson_iter_t iter;
        if (bson_iter_init(&iter, doc) && bson_iter_find(&iter, "DB")) {
            dbname = GetStringFromBsonIterator(&iter);
            break;
        }
        LOG(ERROR) << "The DB field does not exist in " << string(tabname);
    }
    bson_destroy(query);
    mongoc_cursor_destroy(cursor);
    return dbname;
}

bool DataCenterMongoDB::GetFileInStringVector() {
    if (file_in_strs_.empty()) {
        bson_t* b = bson_new();
        std::unique_ptr<MongoCollection>
                collection(new MongoCollection(mongo_client_->GetCollection(model_name_, DB_TAB_FILE_IN)));
        mongoc_cursor_t* cursor = collection->ExecuteQuery(b);
        bson_error_t err;
        if (mongoc_cursor_error(cursor, &err)) {
            LOG(ERROR) << "Nothing found in the collection: " << DB_TAB_FILE_IN << ".";
            return false;
        }
        bson_iter_t it;
        const bson_t* bson_table;
        while (mongoc_cursor_more(cursor) && mongoc_cursor_next(cursor, &bson_table)) {
            vector<string> tokens(2);
            if (bson_iter_init_find(&it, bson_table, Tag_ConfTag)) {
                tokens[0] = GetStringFromBsonIterator(&it);
            }
            if (bson_iter_init_find(&it, bson_table, Tag_ConfValue)) {
                tokens[1] = GetStringFromBsonIterator(&it);
            }
            if (StringMatch(tokens[0], Tag_Mode)) {
                model_mode_ = tokens[1];
            }
            size_t sz = file_in_strs_.size();                // get the current number of rows
            file_in_strs_.resize(sz + 1);                    // resize with one more row
            file_in_strs_[sz] = tokens[0] + "|" + tokens[1]; // keep the interface consistent
        }
        bson_destroy(b);
        mongoc_cursor_destroy(cursor);
    }
    if (!file_in_strs_.empty()) {
        for (auto it = file_in_strs_.begin(); it != file_in_strs_.end(); ++it) {
            CLOG(TRACE, LOG_INIT) << "FILE_IN Info: " << *it;
        }
    }
    return true;
}

bool DataCenterMongoDB::GetFileOutVector() {
    if (!origin_out_items_.empty()) {
        return true;
    }
    bson_t* b = bson_new();
    std::unique_ptr<MongoCollection>
            collection(new MongoCollection(mongo_client_->GetCollection(model_name_, DB_TAB_FILE_OUT)));
    mongoc_cursor_t* cursor = collection->ExecuteQuery(b);
    bson_error_t err;
    if (mongoc_cursor_error(cursor, &err)) {
        LOG(ERROR) << "Nothing found in the collection: " << DB_TAB_FILE_OUT << ".";
        /// destroy
        bson_destroy(b);
        mongoc_cursor_destroy(cursor);
        return false;
    }
    bson_iter_t itertor;
    const bson_t* bson_table;
    while (mongoc_cursor_more(cursor) && mongoc_cursor_next(cursor, &bson_table)) {
        OrgOutItem tmp_output_item;
        if (bson_iter_init_find(&itertor, bson_table, Tag_OutputUSE)) {
            GetNumericFromBsonIterator(&itertor, tmp_output_item.use);
        }
        if (bson_iter_init_find(&itertor, bson_table, Tag_MODCLS)) {
            tmp_output_item.modCls = GetStringFromBsonIterator(&itertor);
        }
        if (bson_iter_init_find(&itertor, bson_table, Tag_OutputID)) {
            tmp_output_item.outputID = GetStringFromBsonIterator(&itertor);
        }
        if (bson_iter_init_find(&itertor, bson_table, Tag_OutputDESC)) {
            tmp_output_item.descprition = GetStringFromBsonIterator(&itertor);
        }
        if (bson_iter_init_find(&itertor, bson_table, Tag_FileName)) {
            tmp_output_item.outFileName = GetStringFromBsonIterator(&itertor);
        }
        if (bson_iter_init_find(&itertor, bson_table, Tag_AggType)) {
            tmp_output_item.aggType = GetStringFromBsonIterator(&itertor);
        }
        if (bson_iter_init_find(&itertor, bson_table, Tag_OutputUNIT)) {
            tmp_output_item.unit = GetStringFromBsonIterator(&itertor);
        }
        if (bson_iter_init_find(&itertor, bson_table, Tag_OutputSubbsn)) {
            tmp_output_item.subBsn = GetStringFromBsonIterator(&itertor);
        }
        if (bson_iter_init_find(&itertor, bson_table, Tag_StartTime)) {
            /// TODO: Currently we only accept "%d-%d-%d %d:%d:%d" for UTC TIME! -LJ. 09/28/2020
            tmp_output_item.sTimet = ConvertToTime(GetStringFromBsonIterator(&itertor),
                                                   "%d-%d-%d %d:%d:%d", true);
        }
        if (bson_iter_init_find(&itertor, bson_table, Tag_EndTime)) {
            tmp_output_item.eTimet = ConvertToTime(GetStringFromBsonIterator(&itertor),
                                                   "%d-%d-%d %d:%d:%d", true);
        }
        if (bson_iter_init_find(&itertor, bson_table, Tag_Interval)) {
            GetNumericFromBsonIterator(&itertor, tmp_output_item.interval);
        }
        if (bson_iter_init_find(&itertor, bson_table, Tag_IntervalUnit)) {
            tmp_output_item.intervalUnit = GetStringFromBsonIterator(&itertor);
        }
        if (tmp_output_item.use > 0) {
            origin_out_items_.emplace_back(tmp_output_item);
        }
    }
    vector<OrgOutItem>(origin_out_items_).swap(origin_out_items_);
    // m_OriginOutItems.shrink_to_fit();
    /// destroy
    bson_destroy(b);
    mongoc_cursor_destroy(cursor);
    return !origin_out_items_.empty();
}


int DataCenterMongoDB::ReadIntParameterInDB(const char* param_name) {
    bson_t* filter = BCON_NEW(PARAM_FLD_NAME, BCON_UTF8(param_name));
    CLOG(TRACE, LOG_INIT) << "Query for " << param_name << ": " << bson_as_json(filter, NULL);
    std::unique_ptr<MongoCollection>
            collection(new MongoCollection(mongo_client_->GetCollection(model_name_, DB_TAB_PARAMETERS)));
    mongoc_cursor_t* cursor = collection->ExecuteQuery(filter);
    bson_error_t err;
    if (mongoc_cursor_error(cursor, &err)) {
        LOG(ERROR) << "ReadIntParameterInDB: " << "Nothing found for " << param_name;
        /// destroy
        bson_destroy(filter);
        mongoc_cursor_destroy(cursor);
        return -9999;
    }
    bson_iter_t iter;
    const bson_t* bson_table;
    int num_tmp = -1;
    while (mongoc_cursor_next(cursor, &bson_table)) {
        if (bson_iter_init_find(&iter, bson_table, PARAM_FLD_VALUE)) {
            GetNumericFromBsonIterator(&iter, num_tmp);
        }
    }
    bson_destroy(filter);
    mongoc_cursor_destroy(cursor);
    return num_tmp;
}

void DataCenterMongoDB::ReadClimateSiteList() {
    double input_t = TimeCounting();
//    bson_t* query = bson_new();
//    BSON_APPEND_INT32(query, Tag_SubbasinId, subbasin_id_); // subbasin id
//    BSON_APPEND_UTF8(query, Tag_Mode, input_->getModelMode().c_str()); // mode

    bson_t* query = BCON_NEW(Tag_SubbasinId, BCON_INT32(subbasin_id_),
                             Tag_Mode, BCON_UTF8(input_->getModelMode().c_str()));
    CLOG(TRACE, LOG_INIT) << "ReadClimateSiteList: " << bson_as_json(query, NULL);
    std::unique_ptr<MongoCollection> collection(new MongoCollection(mongo_client_->GetCollection(model_name_,
                                                                        DB_TAB_SITELIST)));
    mongoc_cursor_t* cursor = collection->ExecuteQuery(query);

    const bson_t* doc;
    while (mongoc_cursor_next(cursor, &doc)) {
        bson_iter_t iter;
        if (bson_iter_init(&iter, doc) && bson_iter_find(&iter, MONG_SITELIST_DB)) {
            clim_dbname_ = GetStringFromBsonIterator(&iter);
        } else {
            throw ModelException("DataCenterMongoDB", "ReadClimateSiteList",
                                 "The DB field does not exist in SiteList table.");
        }
        string site_list;
        if (bson_iter_init(&iter, doc) && bson_iter_find(&iter, SITELIST_TABLE_M)) {
            site_list = GetStringFromBsonIterator(&iter);
            for (int i = 0; i < METEO_VARS_NUM; ++i) {
                clim_station_->ReadSitesData(clim_dbname_, site_list, METEO_VARS[i],
                                             input_->getStartTime(), input_->getEndTime(), input_->isStormMode(), input_->ifUseFileDB());
            }
        }

        if (bson_iter_init(&iter, doc) && bson_iter_find(&iter, SITELIST_TABLE_P)) {
            site_list = GetStringFromBsonIterator(&iter);
            clim_station_->ReadSitesData(clim_dbname_, site_list, DataType_Precipitation,
                                         input_->getStartTime(), input_->getEndTime(), input_->isStormMode(), input_->ifUseFileDB());
        }

        if (bson_iter_init(&iter, doc) && bson_iter_find(&iter, SITELIST_TABLE_PET)) {
            site_list = GetStringFromBsonIterator(&iter);
            clim_station_->ReadSitesData(clim_dbname_, site_list, DataType_PotentialEvapotranspiration,
                                         input_->getStartTime(), input_->getEndTime(), input_->isStormMode(), input_->ifUseFileDB());
        }
    }
    bson_destroy(query);
    mongoc_cursor_destroy(cursor);
}

bool DataCenterMongoDB::ReadParametersInDB() {
    bson_t* filter = bson_new();
    std::unique_ptr<MongoCollection>
            collection(new MongoCollection(mongo_client_->GetCollection(model_name_, DB_TAB_PARAMETERS)));
    mongoc_cursor_t* cursor = collection->ExecuteQuery(filter);

    bson_error_t err;
    const bson_t* info;
    if (mongoc_cursor_error(cursor, &err)) {
        LOG(ERROR) << "Nothing found in the collection: " << DB_TAB_PARAMETERS << ".";
        /// destroy
        bson_destroy(filter);
        mongoc_cursor_destroy(cursor);
        return false;
    }
    while (mongoc_cursor_more(cursor) && mongoc_cursor_next(cursor, &info)) {
        //ParamInfo<FLTPT>* p = new ParamInfo<FLTPT>();
        bson_iter_t iter;
        string name;
        string desc;
        string unit;
        string module;
        FLTPT value;
        string change;
        vector<FLTPT> impact;
        vector<vector<int>> impact_subbasins;
        FLTPT maximum = 0.;
        FLTPT minimum = 0.;
        bool isint = false;
        if (bson_iter_init_find(&iter, info, PARAM_FLD_NAME)) {
            name = GetUpper(GetStringFromBsonIterator(&iter));
        }
        if (bson_iter_init_find(&iter, info, PARAM_FLD_DESC)) {
            desc = GetStringFromBsonIterator(&iter);
        }
        if (bson_iter_init_find(&iter, info, PARAM_FLD_UNIT)) {
            unit = GetStringFromBsonIterator(&iter);
        }
        if (bson_iter_init_find(&iter, info, PARAM_FLD_MIDS)) {
            module = GetStringFromBsonIterator(&iter);
        }
        if (bson_iter_init_find(&iter, info, PARAM_FLD_VALUE)) {
            GetNumericFromBsonIterator(&iter, value);
        }
        if (bson_iter_init_find(&iter, info, PARAM_FLD_IMPACT)) {
            GetVectorFromBsonIter(&iter, impact);
        }
        if (bson_iter_init_find(&iter, info, PARAM_FLD_CHANGE)) {
            change = GetStringFromBsonIterator(&iter);
        }
        if (bson_iter_init_find(&iter, info, PARAM_FLD_MAX)) {
            GetNumericFromBsonIterator(&iter, maximum);
        }
        if (bson_iter_init_find(&iter, info, PARAM_FLD_MIN)) {
            GetNumericFromBsonIterator(&iter, minimum);
        }
        if (bson_iter_init_find(&iter, info, PARAM_FLD_DTYPE)) {
            isint = StringMatch(GetStringFromBsonIterator(&iter), "INT");
        }
        if (bson_iter_init_find(&iter, info, PARAM_CALI_VALUES) && calibration_id_ >= 0) {
            vector<vector<FLTPT>> cali_values;
            GetVectorVectorFromBsonIter(&iter, cali_values);
            if (calibration_id_ < CVT_INT(cali_values.size())) {
                impact = cali_values.at(calibration_id_);
            }
        }
        if (bson_iter_init_find(&iter, info, PARAM_IMPACT_SUBBASINS)) {
            GetVectorVectorFromBsonIter(&iter, impact_subbasins);
        }
        if (isint) {
            vector<int> impact_int;
            for (auto it = impact.begin(); it != impact.end(); ++it) {
                impact_int.push_back(CVT_INT(*it));
            }
            ParamInfo<int>* intp = new ParamInfo<int>(name, desc, unit, module, CVT_INT(value),
                                                      change, impact_int,impact_subbasins, CVT_INT(maximum),
                                                      CVT_INT(minimum), isint);
#ifdef HAS_VARIADIC_TEMPLATES
            if (!init_params_int_.emplace(name, intp).second) {
#else
            if (!init_params_int_.insert(make_pair(name, intp)).second) {
#endif
                LOG(ERROR) << "Load parameter: " << name << " failed!";
                return false;
            }
        }
        else {
            ParamInfo<FLTPT>* p = new ParamInfo<FLTPT>(name, desc, unit, module, value,
                                                       change, impact,impact_subbasins, maximum, minimum, isint);
#ifdef HAS_VARIADIC_TEMPLATES
            if (!init_params_.emplace(name, p).second) {
#else
            if (!init_params_.insert(make_pair(name, p)).second) {
#endif
                LOG(ERROR) << "Load parameter: " << name << " failed!";
                return false;
            }
            /// Special handling code for soil water capcity parameters
            /// e.g., SOL_AWC, SOL_UL, WILTINGPOINT. By ljzhu, 2018-1-11
            if (StringMatch(name, VAR_SW_CAP[0])) {
                for (int si = 0; si < SOILWATER_VARS_NUM; si++) {
                    ParamInfo<FLTPT>* tmpp = new ParamInfo<FLTPT>(*p);
                    tmpp->Name = SOILWATER_VARS[si];
#ifdef HAS_VARIADIC_TEMPLATES
                    init_params_.emplace(GetUpper(tmpp->Name), tmpp);
#else
                    init_params_.insert(make_pair(GetUpper(tmpp->Name), tmpp));
#endif
                }
            }
        }
    }
    bson_destroy(filter);
    mongoc_cursor_destroy(cursor);
    return true;
}

bool DataCenterMongoDB::ReadRasterData(const string& remote_filename, FloatRaster*& flt_rst, STRING_MAP* opts) {
    STRING_MAP opts_temp;
    if(opts==nullptr) {
        opts = &opts_temp;
    }
    UpdateStringMap(*opts, HEADER_INC_NODATA, "FALSE");
    FloatRaster* raster_data = FloatRaster::Init(spatial_gridfs_, remote_filename.c_str(),
                                                 true, mask_raster_, true,
                                                 NODATA_VALUE, *opts);
    if (nullptr == raster_data) { return false; }
    // When load from MongoDB failed (i.e., file not existed), the Initialized() will return false!
    if (!raster_data->Initialized()) {
        delete raster_data;
        return false;
    }
    /// using emplace() if possible or insert() to make sure the successful insertion.
#ifdef HAS_VARIADIC_TEMPLATES
    if (!rs_map_.emplace(remote_filename, raster_data).second) {
#else
    if (!rs_map_.insert(make_pair(remote_filename, raster_data)).second) {
#endif
        delete raster_data;
        return false;
    }
    flt_rst = raster_data;
    return true;
}

bool DataCenterMongoDB::ReadRasterData(const string& remote_filename, IntRaster*& int_rst, STRING_MAP* opts) {
    STRING_MAP opts_temp;
    if (opts == nullptr) {
        opts = &opts_temp;
    }
    UpdateStringMap(*opts, HEADER_INC_NODATA, "FALSE");
    IntRaster* raster_data = IntRaster::Init(spatial_gridfs_, remote_filename.c_str(),
                                             true, mask_raster_, true,
                                             NODATA_VALUE, *opts);
    if (nullptr == raster_data) { return false; }
    // When load from MongoDB failed (i.e., file not existed), the Initialized() will return false!
    if (!raster_data->Initialized()) {
        delete raster_data;
        return false;
    }
    /// using emplace() if possible or insert() to make sure the successful insertion.
#ifdef HAS_VARIADIC_TEMPLATES
    if (!rs_int_map_.emplace(remote_filename, raster_data).second) {
#else
    if (!rs_int_map_.insert(make_pair(remote_filename, raster_data)).second) {
#endif
        delete raster_data;
        return false;
    }
    int_rst = raster_data;
    return true;
}

void DataCenterMongoDB::ReadItpWeightData(const string& remote_filename, int& num, int& stations, FLTPT**& data, STRING_MAP* opts) {
    STRING_MAP opts_upd;
    opts_upd.insert(std::make_pair(HEADER_RS_PARAM_ABSTRACTION_TYPE, PARAM_ABSTRACTION_TYPE_PHYSICAL));
    if (opts==nullptr) {
        opts = &opts_upd;
    }
    ItpWeightData* weight_data = new ItpWeightData(spatial_gridfs_, remote_filename, *opts);
    if (!weight_data->Initialized()) {
        delete weight_data;
        data = nullptr;
        return;
    }
    FLTPT** tmpdata = nullptr;
    weight_data->GetWeightData2D(&num, &stations, &tmpdata);
    Initialize2DArray(num, stations, data, tmpdata);
    delete weight_data;
}

void DataCenterMongoDB::Read1DArrayData(const string& remote_filename, int& num, FLTPT*& data,
    STRING_MAP* opts) {
    char* databuf = nullptr;
    vint datalength;
    spatial_gridfs_->GetStreamData(remote_filename, databuf, datalength, nullptr, opts);
    if (nullptr == databuf) return;

    num = CVT_INT(datalength / sizeof(float));
    float *tmpdata = reinterpret_cast<float*>(databuf); // deprecate C-style: (float *) databuf;
    Initialize1DArray(num, data, tmpdata);
    delete[] tmpdata;
    databuf = nullptr;
}

void DataCenterMongoDB::Read1DArrayData(const string& remote_filename, int& num, int*& data,
    STRING_MAP* opts) {
    char* databuf = nullptr;
    vint datalength;
    spatial_gridfs_->GetStreamData(remote_filename, databuf, datalength, nullptr, opts);
    if (nullptr == databuf) return;

    num = CVT_INT(datalength / sizeof(float));
    float* tmpdata = reinterpret_cast<float*>(databuf); // deprecate C-style: (float *) databuf;
    Initialize1DArray(num, data, tmpdata);
    delete[] tmpdata;
    databuf = nullptr;
}

void DataCenterMongoDB::Read2DArrayData(const string& remote_filename, int& rows, int& cols, FLTPT**& data,
    STRING_MAP* opts) {
    char* databuf = nullptr;
    vint datalength;
    spatial_gridfs_->GetStreamData(remote_filename, databuf, datalength, nullptr, opts);
    if (nullptr == databuf) {
        data = nullptr;
        return;
    }
    float* float_values = reinterpret_cast<float*>(databuf); // deprecate C-style: (float *) databuf;
    if (!Initialize2DArray(float_values, rows, cols, data)) {
        data = nullptr;
    }
    Release1DArray(float_values);
    databuf = nullptr;
}

void DataCenterMongoDB::Read2DArrayData(const string& remote_filename, int& rows, int& cols, int**& data,
    STRING_MAP* opts) {
    char* databuf = nullptr;
    vint datalength;
    spatial_gridfs_->GetStreamData(remote_filename, databuf, datalength, nullptr, opts);
    if (nullptr == databuf) {
        data = nullptr;
        return;
    }
    float* float_values = reinterpret_cast<float*>(databuf); // deprecate C-style: (float *) databuf;
    if (!Initialize2DArray(float_values, rows, cols, data)) {
        data = nullptr;
    }
    Release1DArray(float_values);
    databuf = nullptr;
}

void DataCenterMongoDB::ReadIuhData(const string& remote_filename, int& n, FLTPT**& data, STRING_MAP* opts) {
    STRING_MAP opts_upd;
    opts_upd.insert(std::make_pair(HEADER_RS_PARAM_ABSTRACTION_TYPE, PARAM_ABSTRACTION_TYPE_PHYSICAL));
    if (opts == nullptr) {
        opts = &opts_upd;
    }
    char* databuf = nullptr;
    vint datalength;
    spatial_gridfs_->GetStreamData(remote_filename, databuf, datalength, nullptr, opts);
    if (nullptr == databuf) {
        data = nullptr;
        return;
    }
    float* float_values = reinterpret_cast<float*>(databuf); // deprecate C-style: (float *) databuf;
    // Previous code, which will cause memory leak.
    // n = CVT_INT(float_values[0]);
    // data = new FLTPT*[n];
    // int index = 1;
    // for (int i = 0; i < n; i++) {
    //     int n_sub = CVT_INT(float_values[index + 1] - float_values[index] + 3);
    //     data[i] = new FLTPT[n_sub];
    //
    //     data[i][0] = float_values[index];
    //     data[i][1] = float_values[index + 1];
    //     for (int j = 2; j < n_sub; j++) {
    //         data[i][j] = float_values[index + j];
    //     }
    //     index = index + n_sub;
    // }

    // Customize code according to Initialize2DArray. LJ, 2022-08-23
    int idx = 0;
    n = CVT_INT(float_values[idx++]);
    data = new(nothrow) FLTPT * [n];
    FLTPT* pool = nullptr;
    // Get actual data length of init_data, excluding the first element which is 'rows'
    int* cols = new int[n];
    int max_cols = -1;
    for (int i = 0; i < n; i++) {
        cols[i] = CVT_INT(float_values[idx + 1] - float_values[idx] + 3);
        idx += cols[i];
        if (cols[i] > max_cols) { max_cols = cols[i]; }
    }
    int length = idx - 1;
    // New a 1d array to store data
    Initialize1DArray(length, pool, float_values + 1);
    // Now point the row pointers to the appropriate positions in the data pool
    int pos = 0;
    for (int i = 0; i < n; ++i) {
        data[i] = pool + pos;
        pos += cols[i];
    }
    delete[] cols;

    Release1DArray(float_values);
    databuf = nullptr;
}

bool DataCenterMongoDB::SetRasterForScenario() {
    if (!use_scenario_) { return false; }
    if (nullptr == scenario_) { return false; }
    map<string, IntRaster*>& scene_rs_map = scenario_->getSceneRasterDataMap();
    if (scene_rs_map.empty()) { return false; }
    for (auto it = scene_rs_map.begin(); it != scene_rs_map.end(); ++it) {
        if (rs_int_map_.find(it->first) == rs_int_map_.end()) {
            if (!ReadRasterData(it->first, it->second)) { return false; }
        } else {
            it->second = rs_int_map_.at(it->first);
        }
    }
    return true;
}
