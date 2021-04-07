/*!
 * \brief Grid layering class.
 * \author original, Junzhi Liu, 29-July-2012
 * \author Liangjun Zhu, since 21-July-2016
 *          lj - 28-Dec-2017 - Refactor as class.\n
 *          lj -  5-Mar-2018 - Use CCGL, and reformat code style.\n
 *          lj - 31-Mar-2021 - Rewrite most core parts and now support MFD-md algorithm.\n
 *          lj -  7-Apr-2021 - Since all spatial raster data is stored in float type in MongoDB,
 *                              for cross-platform compatible, both MongoDB and File mode use FloatRaster.\n
 * \description:
 *               The output list:
 *               1. X_FLOWOUT_INDEX_{FD}, X_FLOWIN_INDEX_{FD}
 *                     in which X is subbasinID (0 for the whole basin)
 *                              `FD` is the flow direction algorithm, include `D8`, `DINF`, and `MFDMD`
 *               2. X_ROUTING_LAYERS_UP_DOWN{_METHOD} and X_ROUTING_LAYERS_DOWN_UP{_METHOD}
 *                     in which `_METHOD` represent flow direction method, the empty denotes `D8`, others
 *                        include `_DINF` and `_MFDMD`
 *               3. X_FLOWOUT_FRACTION_{FD}, X_FLOWIN_FRACTION_{FD}
 *                     recording flow fractions of flow out and flow in cells,
 *                     specifically for multiple flow direction algorithms
 *
 */

#ifndef GRID_LAYERING_H
#define GRID_LAYERING_H

#include "data_raster.h"

using namespace ccgl;
using namespace data_raster;

#ifndef IntRaster
#define IntRaster   clsRasterData<int>
#endif
#ifndef FloatRaster
#define FloatRaster clsRasterData<float>
#endif
#ifndef FloatMaskedRaster
#define FloatMaskedRaster clsRasterData<float, int>
#endif
#ifndef FltMaskFltRaster
#define FltMaskFltRaster clsRasterData<float, float>
#endif

/*!
* \enum flowDirTypes
* \brief Algorithm of flow direction
*/
enum flowDirTypes {
    FD_D8 = 0,   /**< Default, D8 single flow direction */
    FD_Dinf = 1, /**< Dinf (Tarboton, 1997) */
    FD_MFDmd = 2 /**< MFD-md (Qin et al., 2007) */
};

const int fdccw[9] = {0, 1, 128, 64, 32, 16, 8, 4, 2};
const int drow[9] = {0, 0, -1, -1, -1, 0, 1, 1, 1};
const int dcol[9] = {0, 1, 1, 0, -1, -1, -1, 0, 1};

int find_flow_direction_index_ccw(const int fd);

int get_reversed_fdir(const int fd);

vector<int> uncompress_flow_directions(const int compressed_fd);

class GridLayering: Interface {
public:
#ifdef USE_MONGODB
    /*!
     * \brief Constructor using MongoDB.
     */
    GridLayering(int id, MongoGridFs* gfs, const char* out_dir);
#endif
    /*!
     * \brief Constructor using Raster file directly.
     */
    GridLayering(int id, const char* out_dir);
    ///< Destructor
    virtual ~GridLayering();
    /*!
     * \brief Execute workflow
     */
    bool Execute();
    /*!
     * \brief Load flow data
     */
    virtual bool LoadData() = 0;
    /*!
     * \brief Calculate index of valid cell according to mask raster data
     */
    void CalPositionIndex();
    /*!
     * \brief Get compressed reverse flow direction matrix, i.e., accumulate each cell's upstream direction
     *        e.g. cell (i, j) has three upstream source: (i, j+1), (i-1, j), (i+1, j)
     *             then the compressed reverse direction of cell (i, j) is 1 + 64 + 4 = 69
     */
    void GetReverseDirMatrix();
    /*!
     * \brief Count each cell's upstream number by bitwise AND operator
     *        e.g. cell (i, j) has a reversed direction value of 69, which stored as 1000101
     *        1000101 & 1 is True, and so as to 100, 1000000. So the upstream cell number is 3.
     */
    void CountFlowInCells();
    /*!
     * \brief Construct flow in indexes of each cells
     */
    bool BuildFlowInCellsArray();
    /*!
     * \brief Output flow in cells index data, both txt file and GridFS.
     */
    virtual bool OutputFlowIn();
    /*!
     * \brief Count each cell's downstream number by bitwise AND operator, CountFlowInCells
     */
    void CountFlowOutCells();
    /*!
     * \brief Construct flow out indexes of each cells
     */
    bool BuildFlowOutCellsArray();
    /*!
     * \brief Output flow out data, both txt file and GridFS.
     */
    virtual bool OutputFlowOut();
    /*!
     * \brief Build grid layers in Up-Down order from source
     */
    bool GridLayeringFromSource();
    /*!
     * \brief Build grid layers in Down-Up order from outlet
     */
    bool GridLayeringFromOutlet();
protected:
    /*!
     * \brief Build multiple flow out array
     */
    int BuildMultiFlowOutArray(float*& compressed_dir,
                               int*& connect_count, float*& p_output);
    /*!
     * \brief Ouput 2D array as txt file
     */
    bool Output2DimensionArrayTxt(const string& name, string& header, float* matrix, float* matrix2 = nullptr);
#ifdef USE_MONGODB
    /*!
     * \brief Output grid layering related data to MongoDB GridFS
     */
    bool OutputToMongodb(const char* name, int number, char* s);

    /*!
    * \brief Ouput 2D array as MongoDB-GridFS
    */
    bool OutputArrayAsGfs(const string& name, float* matrix);
    /*!
     * \brief Output grid layering as tiff file and MongoDB-GridFS
     */
    bool OutputGridLayering(const string& name, int datalength,
                            float* layer_grid, float* layer_cells);

    MongoGridFs* gfs_; ///< MongoDB-GridFS instance
#endif
    bool use_mongo_;         ///< Use MongoDB or file
    bool has_mask_;          ///< User-specific mask raster file
    flowDirTypes fdtype_;    ///< Flow direction model
    const char* output_dir_; ///< Output directory
    int subbasin_id_;        ///< Subbasin ID, 0 for entire basin
    int n_rows_;             ///< Rows
    int n_cols_;             ///< Cols
    float out_nodata_;       ///< Nodata value in output
    int n_valid_cells_;      ///< Valid Cells number
    int* pos_index_;         ///< Valid cell's index
    int** pos_rowcol_;       ///< Positions of valid cells, e.g., (row, col) coordinates
    FloatRaster* mask_;      ///< Mask raster data
    FltMaskFltRaster* flowdir_; ///< Flow direction raster data, e.g., `int` for D8
    float* flowdir_matrix_;     ///< Valid flow direction data, e.g., D8, compressed Dinf and MFD-md
    float* reverse_dir_;        ///< Compressed reversed direction
    int* flow_in_num_;          ///< Flow in cells number
    int flow_in_count_;         ///< All flow in counts from \a m_flowInNum
    /*!
     * \brief Stores flow in cells' indexes of each valid cells, which can be
     *          parsed as 2D array. Data length is flow_in_count_ + n_valid_cells_ + 1
     *        For example:
     *            53933 0 1 0 1 1 2 7 8 ...
     *            can be parsed as:
     *                The valid cell number is 53933
     *                ID    UpstreamCount     UpstreamID
     *                0          0
     *                1          1                0
     *                2          1                1
     *                3          2                7,8
     * \note The only reason to use float* rather than int* is that we use float
     *       to keep consistent in data IO of MongoDB.
     */
    float* flow_in_cells_;
    int* flow_out_num_;         ///< Flow out cells number
    int flow_out_count_;        ///< Flow out times
    float* flow_out_cells_;     ///< Indexes of each cell's flow out
    float* layers_updown_;      ///< the value of layering number from source, length is nRows * nCols
    float* layers_downup_;      ///< the value of layering number from outlet
    float* layer_cells_updown_; ///< store cell indexes in each layers, length is ValidNum + layerNum + 1
    float* layer_cells_downup_; ///< store cell indexes in each layers
    string flowdir_name_;       ///< Flow direction file name
    string mask_name_;          ///< Mask raster file name

    /** Output file names **/
    string flowin_index_name_;    ///< Flow in index
    string flowout_index_name_;   ///< Flow out index
    string layering_updown_name_; ///< Routing layers from sources
    string layering_downup_name_; ///< Routing layers from outlet
};

class GridLayeringD8: public GridLayering {
public:
#ifdef USE_MONGODB
    GridLayeringD8(int id, MongoGridFs* gfs, const char* out_dir);
#endif
    GridLayeringD8(int id, const char* in_file, const char* mask_file, const char* out_dir);

    ~GridLayeringD8();

    bool LoadData() OVERRIDE;
};


class GridLayeringDinf: public GridLayering {
public:
#ifdef USE_MONGODB
    GridLayeringDinf(int id, MongoGridFs* gfs, const char* out_dir);
#endif
    GridLayeringDinf(int id, const char* fd_file, const char* fraction_file,
                     const char* mask_file, const char* out_dir);

    ~GridLayeringDinf();

    bool LoadData() OVERRIDE;
    bool OutputFlowIn() OVERRIDE;
    bool OutputFlowOut() OVERRIDE;

private:
    string flowfrac_name_;             ///< Flow fraction raster file recording the fraction of first direction
    FltMaskFltRaster* flow_fraction_;  ///< Flow fraction of the first flow out direction
    float* flowfrac_matrix_;           ///< Flow fraction of the first flow out direction (valid cell number)
    float* flowin_fracs_;              ///< Flow in fraction
    float* flowout_fracs_;             ///< Flow fractions of each cell's flow in

    /** Output file names **/
    string flowin_frac_name_;  ///< Flow fraction of each flow in cell
    string flowout_frac_name_; ///< Flow fraction of each flow out cell
};

class GridLayeringMFDmd: public GridLayering {
public:
#ifdef USE_MONGODB
    GridLayeringMFDmd(int id, MongoGridFs* gfs, const char* out_dir);
#endif
    GridLayeringMFDmd(int id, const char* fd_file, const char* fraction_file,
                      const char* mask_file, const char* out_dir);

    ~GridLayeringMFDmd();

    bool LoadData() OVERRIDE;
    bool OutputFlowIn() OVERRIDE;
    bool OutputFlowOut() OVERRIDE;

private:
    string flowfrac_corename_;         ///< Core name of flow fraction raster files (multiple layer raster) in MongoDB
    vector<string> flowfrac_names_;    ///< Flow fraction raster files recording the fractions of each direction by ccw
    FltMaskFltRaster* flow_fraction_;  ///< Flow fraction of the first flow out direction
    float** flowfrac_matrix_;          ///< Flow fraction of the first flow out direction (valid cell number)
    float* flowin_fracs_;              ///< Flow in fraction
    float* flowout_fracs_;             ///< Flow fractions of each cell's flow in

    /** Output file names **/
    string flowin_frac_name_;  ///< Flow fraction of each flow in cell
    string flowout_frac_name_; ///< Flow fraction of each flow out cell
};
#endif /* GRID_LAYERING_H */
