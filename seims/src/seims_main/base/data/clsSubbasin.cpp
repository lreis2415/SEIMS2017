#include "clsSubbasin.h"

#include "text.h"
// #include "Logging.h"

Subbasin::Subbasin(const int id) :
    subbsn_id_(id), n_cells_(-1), cells_(nullptr), cell_area_(-1.),
    area_(-1.), pet_(-1.), perco_(-1.), pcp_(-1.),
    intercep_(-1.), intercep_et_(-1.), depression_et_(-1.), infil_(-1.),
    soil_et_(-1.), total_et_(-1.), net_perco_(-1.), runoff_(-1.), interflow_(-1.),
    soil_wtr_(-1.), net_pcp_(-1.),
    mean_tmp_(-1.), soil_tmp_(-1.), gwmax_(-1.), kg_(-1.),
    revap_coef_(-1.), base_ex_(-1.), qg_cvt_(-1.), slope_coef_(1.), slope_(-1.),
    revap_(-1.), gw_(-1.), deep_perco_(-1.), qg_(-1.), rg_(-1.),
    output_(false), revap_changed_(true) {
}

Subbasin::~Subbasin() {
    if (cells_ != nullptr) { Release1DArray(cells_); }
}

bool Subbasin::CheckInputSize(const int n) {
    if (n <= 0) {
        throw ModelException("Subbasin Class", "CheckInputSize",
                             "Input data for Subbasin is invalid. The size could not be less than zero.");
    }
    if (n_cells_ != n) {
        if (n_cells_ <= 0) {
            n_cells_ = n;
        } else {
            throw ModelException("Subbasin Class", "CheckInputSize",
                                 "All input data should have same size.");
        }
    }
    return true;
}

void Subbasin::SetCellList(const int n_cells, int* cells) {
    CheckInputSize(n_cells);
    cells_ = cells;
}

// Note: Since slope is calculated by drop/distance in TauDEM,
//		 the average should be calculated after doing atan(). By LJ, 2016-7-27
void Subbasin::SetSlope(FLTPT* slope) {
    FLTPT slope_sum = 0.;
#pragma omp parallel for reduction(+: slope_sum)
    for (int i = 0; i < n_cells_; i++) {
        slope_sum += atan(slope[cells_[i]]); // radian
    }
    slope_sum /= n_cells_;
    slope_ = tan(slope_sum); // to keep consistent with the slope unit in the whole model
}

//////////////////////////////////////////////////////////////////////////
//////////  clsSubbasins                           ///////////////////////
//////////////////////////////////////////////////////////////////////////
clsSubbasins::clsSubbasins(map<string, IntRaster*>& rs_int_map,
                           map<string, FloatRaster *>& rs_map, const int prefix_id): n_subbasins_(-1) {
    subbasin_ids_.clear();
    subbasin_objs_.clear();

    // read subbasin data
    int n_cells = -1;
    int* subbasin_data = nullptr;
    FLTPT cell_width = NODATA_VALUE;

    std::ostringstream oss;
    oss << prefix_id << "_" << VAR_SUBBSN[0];
    rs_int_map[GetUpper(oss.str())]->GetRasterData(&n_cells, &subbasin_data);
    cell_width = rs_int_map[GetUpper(oss.str())]->GetCellWidth();

    // valid cell indexes of each subbasin, key is subbasin ID, value is vector of cell's index
    map<int, vector<int> > cell_list_map;
    for (int i = 0; i < n_cells; i++) {
        int sub_id = subbasin_data[i];
        if (cell_list_map.find(sub_id) == cell_list_map.end()) {
#ifdef HAS_VARIADIC_TEMPLATES
            cell_list_map.emplace(sub_id, vector<int>());
#else
            cell_list_map.insert(make_pair(sub_id, vector<int>()));
#endif
        }
        cell_list_map[sub_id].emplace_back(i);
    }
    n_subbasins_ = CVT_INT(cell_list_map.size());
    for (auto it = cell_list_map.begin(); it != cell_list_map.end(); ++it) {
        // swap for saving memory, using shrink_to_fit() instead.
        vector<int>(it->second).swap(it->second);
        // (*it->second).shrink_to_fit();
        int sub_id = it->first;
        subbasin_ids_.emplace_back(sub_id);
        Subbasin* new_sub = new Subbasin(sub_id);
        int n_cells_tmp = CVT_INT(it->second.size());
        int* tmp = new int[n_cells_tmp];
        for (int j = 0; j < n_cells_tmp; j++)
            tmp[j] = it->second.at(j);
        new_sub->SetCellList(n_cells_tmp, tmp);
        new_sub->SetArea(cell_width * cell_width * n_cells_tmp);
        subbasin_objs_[sub_id] = new_sub;
    }
    vector<int>(subbasin_ids_).swap(subbasin_ids_);
    // m_subbasinIDs.shrink_to_fit();

    /// Set required parameters, e.g., slope
    FLTPT* slope_data = nullptr;
    oss.str("");
    oss << prefix_id << "_" << VAR_SLOPE[0];
    rs_map[GetUpper(oss.str())]->GetRasterData(&n_cells, &slope_data);
    SetSlopeCoefficient(slope_data);
}

clsSubbasins* clsSubbasins::Init(map<string, IntRaster*>& rs_int_map,
                                 map<string, FloatRaster*>& rs_map, const int prefix_id) {
    /// Mask raster data is prerequisite.
    std::ostringstream oss;
    oss << prefix_id << "_" << VAR_SUBBSN[0]; // mask, also subbasin
    string mask_file_name = GetUpper(oss.str());
    if (rs_int_map.find(mask_file_name) == rs_int_map.end()) {
        cout << "MASK data has not been loaded yet!" << endl;
        return nullptr;
    }
    /// Required rasters to initialize clsSubbasins.
    vector<string> rs_names;
    oss.str("");
    oss << prefix_id << "_" << VAR_SLOPE[0];
    rs_names.emplace_back(GetUpper(oss.str()));
	//oss.str("");
	//oss << prefix_id << "_" << VAR_REACH_DEPTH_SPATIAL;
	//rs_names.emplace_back(GetUpper(oss.str()));

    for (auto it = rs_names.begin(); it != rs_names.end(); ++it) {
        if (rs_map.find(*it) == rs_map.end()) {
            cout << *it << " data is required to build clsSubbasins!" << endl;
            return nullptr;
        }
    }
    return new clsSubbasins(rs_int_map, rs_map, prefix_id);
}

clsSubbasins::~clsSubbasins() {
    // CLOG(TRACE, LOG_RELEASE) << "Release subbasin class ...";
    StatusMessage("Release subbasin class ...");
    if (!subbasin_objs_.empty()) {
        for (auto iter = subbasin_objs_.begin(); iter != subbasin_objs_.end(); ++iter) {
            if (iter->second != nullptr) {
                delete iter->second;
                iter->second = nullptr;
            }
            // subbasin_objs_.erase(iter++);
        }
        subbasin_objs_.clear();
    }
}

FLTPT clsSubbasins::Subbasin2Basin(const string& key) {
    FLTPT temp = 0.;
    int total_count = 0;
    Subbasin* sub = nullptr;
    for (auto it = subbasin_ids_.begin(); it != subbasin_ids_.end(); ++it) {
        sub = subbasin_objs_[*it];
        int n_count = sub->GetCellCount();
        if (StringMatch(key, VAR_SLOPE[0])) {
            temp += atan(sub->GetSlope()) * n_count;
        } else if (StringMatch(key, VAR_PET[0])) {
            temp += sub->GetPet() * n_count;
        } else if (StringMatch(key, VAR_PERCO[0])) {
            temp += sub->GetPerco() * n_count;
        } else if (StringMatch(key, VAR_REVAP[0])) {
            temp += sub->GetEg() * n_count;
        } else if (StringMatch(key, VAR_PERDE[0])) {
            temp += sub->GetPerde() * n_count;
        } else if (StringMatch(key, VAR_RG[0])) {
            temp += sub->GetRg() * n_count;
        } else if (StringMatch(key, VAR_QG[0])) {
            temp += sub->GetQg();
        } else if (StringMatch(key, VAR_GW_Q[0])) {
            temp += sub->GetGw() * n_count;
        }

        total_count += n_count;
    }
    if (StringMatch(key, VAR_QG[0])) {
        return temp;
    } // basin sum
    if (StringMatch(key, VAR_SLOPE[0]))
        return tan(temp / total_count);
    return temp / total_count; // basin average
}

void clsSubbasins::SetSlopeCoefficient(FLTPT* rs_slope) {
    for (auto it = subbasin_objs_.begin(); it != subbasin_objs_.end(); ++it) {
        if (it->second->GetSlope() <= 0.) {
            it->second->SetSlope(rs_slope);
        }
    }
    FLTPT basin_slope = Subbasin2Basin(VAR_SLOPE[0]);
    for (auto it = subbasin_objs_.begin(); it != subbasin_objs_.end(); ++it) {
        if (basin_slope <= 0.) {
            cout << "WARNING: Mean slope of the whole basin is absent, the Groundwater modules"
                    " may be problematic, please contact the developers!" << endl;
            it->second->SetSlopeCoefofBasin(1.);
        } else {
            FLTPT slp_coef = it->second->GetSlope() / basin_slope;
            it->second->SetSlopeCoefofBasin(slp_coef);
        }
    }
}
