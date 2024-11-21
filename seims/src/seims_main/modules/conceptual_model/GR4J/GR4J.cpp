#include "GR4J.h"
#include "text.h"

FLTPT sum(FLTPT* array, int size) {
    FLTPT res = 0;
    for (int i = 0; i < size; ++i) {
        res += array[i];
    }
    return res;
}

void GR4J::printSoilWater() {
    FLTPT sums[4];
    for (int j = 0; j < 4; ++j) {
        sums[j] = 0;
    }
    for (int i = 0; i < m_nCells; ++i) {
        for (int j = 0; j < 4; ++j) {
            sums[j] += m_soilWaterStorage[i][j];
        }
    }
    printf("    >> %f, %f, %f, %f\n", sums[0], sums[1], sums[2], sums[3]);
    fflush(stdout);
}

GR4J::GR4J() :
    N_SOIL_LAYERS(4),SOIL_PRODUCT_LAYER(0),SOIL_ROUTING_LAYER(1),SOIL_TEMP_LAYER(2),SOIL_GW_LAYER(3),
    m_nCells(-1), m_timeStep(-1), m_isInitialized(false),
    m_subbasins(nullptr), m_cellsMappingToSubbasinId(nullptr),
    //infiltration
    m_pcp(nullptr),m_NEPR_input(nullptr), m_soilThickness(nullptr), m_soilPorosity(nullptr), m_soilCapacity(nullptr),
    m_infil(nullptr), m_soilProfileWater(nullptr), m_soilWaterStorage(nullptr), m_pcpExcess(nullptr),m_netEvapCapacity(nullptr),
    //soil evaporation
    m_pet(nullptr), m_soilET(nullptr),
    //percolation
    m_GR4J_X2(nullptr), m_GR4J_X3(nullptr),
    //split and convolve
    m_GR4J_X4(nullptr), m_convEntering1(nullptr), m_convEntering2(nullptr)
{
    SetModuleName(CM_GR4J[0]);
}

void GR4J::InitialOutputs() {
    Initialize1DArray(m_nCells, m_soilET, 0.);
#pragma omp parallel for
    for (int i = 0; i < m_nCells; ++i) {
        m_soilET[i] = 0;
    }

    if (m_isInitialized) {
        return;
    }
    
    Initialize1DArray(m_nCells, m_pcpExcess, 0.);
    Initialize2DArray(m_nCells, N_SOIL_LAYERS, m_soilCapacity, 0.);
    Initialize1DArray(m_nCells, m_infil, 0.);
    Initialize1DArray(m_nCells, m_netEvapCapacity, 0.);
    Initialize1DArray(m_nCells, m_soilProfileWater, 0.);
    Initialize2DArray(m_nCells, N_SOIL_LAYERS, m_soilWaterStorage, 0.);
    Initialize1DArray(m_nCells, m_infil, 0.);
    Initialize1DArray(m_nCells, m_convEntering1, 0.);
    Initialize1DArray(m_nCells, m_convEntering2, 0.);


    if (m_NEPR_input != nullptr) {
#pragma omp parallel for
        for (int i = 0; i < m_nCells; i++) {
            m_pcp[i] = m_NEPR_input[i];
        }
    }

#pragma omp parallel for
    for (int i = 0; i < m_nCells; i++) {
        for (int j = 0; j < N_SOIL_LAYERS; ++j) {
            m_soilCapacity[i][j] = m_soilPorosity[i][j] * m_soilThickness[i][j];
            m_soilWaterStorage[i][j] = 0;
        }
    }

    m_unitHydro1.resize(m_nCells);
    m_unitHydro2.resize(m_nCells);
    m_convTransport1.resize(m_nCells);
    m_convTransport2.resize(m_nCells);
    InitUnitHydrograph(CONVOL_GR4J_1);
    InitUnitHydrograph(CONVOL_GR4J_2);

    m_isInitialized = true;
}

GR4J::~GR4J() {
    Release1DArray(m_pcpExcess);
    Release1DArray(m_soilCapacity);
    Release1DArray(m_infil);
    Release1DArray(m_netEvapCapacity);
    Release1DArray(m_soilProfileWater);
    Release2DArray(m_soilWaterStorage);
    Release1DArray(m_infil);
    Release1DArray(m_convEntering1);
    Release1DArray(m_convEntering2);
}

void GR4J::SetValue(const char* key, int value) {
    string sk(key);
    if (StringMatch(sk, Tag_CellSize[0])) m_nCells = value;
    else if (StringMatch(sk, Tag_TimeStep[0])) m_timeStep = value;
    else if (StringMatch(sk, VAR_SUBBSNID_NUM[0])) m_nSubbasins = value;
    else if (StringMatch(sk, Tag_SubbasinId)) m_inputSubbsnId = value;
    else {
        throw ModelException(GetModuleName(), "SetValue",
                             "Integer Parameter " + sk + " does not exist.");
    }
}

void GR4J::Set1DData(const char* key, int n, int* data) {
    string sk(key);

    if (StringMatch(sk, VAR_SUBBSN[0])) m_cellsMappingToSubbasinId = data;
    else {
        throw ModelException(GetModuleName(), "Set1DData", "Parameter " + sk
                             + " does not exist in current module. Please contact the module developer.");
    }
}

void GR4J::Set1DData(const char* key, int n, FLTPT* data) {
    string sk(key);

    if (StringMatch(sk, VAR_PCP[0])) { m_pcp = data; }
    else if (StringMatch(sk, VAR_NEPR[0])) { m_NEPR_input = data; }
    else if (StringMatch(sk, VAR_PET[0])) { m_pet = data; }
    else if (StringMatch(sk, VAR_GR4J_X4[0])) { m_GR4J_X4 = data; }
    else {
        throw ModelException(GetModuleName(), "Set1DData", "Parameter " + sk
                             + " does not exist in current module. Please contact the module developer.");
    }
}

void GR4J::Set2DData(const char* key, int nrows, int ncols, FLTPT** data) {
    string sk(key);
    CheckInputSize2D(key, nrows, ncols, m_nCells, N_SOIL_LAYERS);
    if (StringMatch(sk, VAR_SOILTHICK[0]))
    {
        m_soilThickness = data;
    }
    else if (StringMatch(sk, VAR_POROST[0])) { m_soilPorosity = data; }
    else if (StringMatch(sk, VAR_GR4J_X2[0])) { m_GR4J_X2 = data; }
    else if (StringMatch(sk, VAR_GR4J_X3[0])) { m_GR4J_X3 = data; }
    else {
        throw ModelException(GetModuleName(), "Set2DData",
                             "Parameter " + sk + " does not exist.");
    }
}

bool GR4J::CheckInputSize(const char* key, int n) {
    return SimulationModule::CheckInputSize(key, n, m_nCells);
}

bool GR4J::CheckInputData(void) {
    if (m_nCells <= 0) {
        throw ModelException(GetModuleName(), "CheckInputData", "Input data is invalid. The size could not be less than zero.");
    }
    CHECK_POINTER(GetModuleName(), m_pcp);
    CHECK_POINTER(GetModuleName(), m_soilThickness);
    CHECK_POINTER(GetModuleName(), m_soilPorosity);
    for (int i = 0; i < m_nCells; ++i)
    {
        for (int j = 0; j < N_SOIL_LAYERS; ++j)
        {
            if (m_soilThickness[i][j] <= 0 || m_soilPorosity[i][j] <= 0) {
                throw ModelException(GetModuleName(), "InitialOutputs",
                    "Invalid input! m_soilPorosity or m_soilThickness is 0.");
            }
        }
        CHECK_POSITIVE(GetModuleName(), m_GR4J_X4[i]);
    }
    return true;
}

void GR4J::SetSubbasins(clsSubbasins* subbsns) {
    if (m_subbasins == nullptr) {
        m_subbasins = subbsns;
        m_subbasinIds = m_subbasins->GetSubbasinIDs();
    }
}

void GR4J::Get1DData(const char* key, int* n, FLTPT** data) {
    string sk(key);
    if (StringMatch(sk, VAR_SURU[0])) { *data = m_pcpExcess; }
    else if (StringMatch(sk, VAR_AET_PLT[0])) { *data = m_soilET;}
    else if (StringMatch(sk, VAR_SOL_SW[0])) { *data = m_soilProfileWater;}
    else if (StringMatch(sk, VAR_SOET[0])) { *data = m_soilET;}
    else {
        throw ModelException(GetModuleName(), "Get1DData",
                             "Result " + sk +
                             " does not exist in current module. Please contact the module developer.");
    }
    *n = m_nCells;
}

void GR4J::Get2DData(const char* key, int* nRows, int* nCols, FLTPT*** data) {
    string sk(key);
    *nRows = m_nCells;
    *nCols = N_SOIL_LAYERS;
    if (StringMatch(sk, VAR_SOL_ST[0])) { *data = m_soilWaterStorage; }
    else {
        throw ModelException(GetModuleName(), "Get2DData", "Output " + sk
                             + " does not exist. Please contact the module developer.");
    }
}

void GR4J::CalculateSoilCapacity() {
#pragma omp parallel for
    for (int i = 0; i < m_nCells; i++) {
        for (int j = 0; j < N_SOIL_LAYERS; j++) {
            m_soilCapacity[i][j] = m_soilThickness[i][j] * m_soilPorosity[i][j];
        }
    }
}


void GR4J::Infiltration() {
#pragma omp parallel for
    for (int i = 0; i < m_nCells; i++) {
        m_netEvapCapacity[i] = 0;
        if (m_pcp[i]<m_pet[i]) {
            m_netEvapCapacity[i] = m_pet[i] - m_pcp[i];
            continue;
        }
        FLTPT netPCP = m_pcp[i] - m_pet[i];

        // Calculate X1
        ///Parameter X1 of GR4J model. X1 = SoilCapacity = Thickness * Porosity * (1-StoneFraction).
        ///StoneFraction is not considered in this implementation, assumed to be 0.
        FLTPT x1 = m_soilCapacity[i][SOIL_PRODUCT_LAYER];
        FLTPT infil = 0;
        if(x1>0) {
            FLTPT store = m_soilWaterStorage[i][SOIL_PRODUCT_LAYER]; // infiltration to soil layer 0 is all stored
            FLTPT sat = store / x1; // saturation
            FLTPT tmp = tanh(netPCP / x1);
            infil = x1 * (1.0 - (sat * sat)) * tmp / (1.0 + sat * tmp);
        }

        //output
        m_pcpExcess[i] += netPCP - infil;
        m_soilWaterStorage[i][SOIL_PRODUCT_LAYER] += infil;
        m_soilET[i] = m_pet[i]; // if AET = PET here, then SoilEvaporation will actually not happen.
    }
#ifdef PRINT_DEBUG
    FLTPT t1 = 0;
    FLTPT t2 = 0;
    FLTPT t3 = 0;
    FLTPT t4 = 0;
    for (int i = 0; i < m_nCells; i++) {
        t1 += m_pcp[i];
        t2 += m_pet[i];
        t3 += m_pcpExcess[i];
    }
    printf("[Infil] m_pcp=%f, m_pet=%f,  m_pcpExcess=%f\n",
        t1, t2, t3);
    printSoilWater();
#endif

}

void GR4J::SoilEvaporation() {
//#pragma omp parallel for
    for (int i = 0; i < m_nCells; i++) {
        if(m_netEvapCapacity[i]<=0) {
            continue;
        }
        FLTPT maxStor = m_soilCapacity[i][SOIL_PRODUCT_LAYER]; //x1
        FLTPT stor = m_soilWaterStorage[i][SOIL_PRODUCT_LAYER];
        FLTPT sat = stor / maxStor;
        FLTPT tmp = tanh(m_netEvapCapacity[i] / maxStor);
        m_soilET[i] = stor * (2.0 - sat) * tmp / (1.0 + (1.0 - sat) * tmp);
        if(isnan(m_soilET[i]))
        {
            cout << "error" << endl;
        }
        m_soilWaterStorage[i][SOIL_PRODUCT_LAYER] -= m_soilET[i];
    }
#ifdef PRINT_DEBUG
    FLTPT t1 = 0;
    FLTPT t2 = 0;
    FLTPT t3 = 0;
    for (int i = 0; i < m_nCells; i++) {
       t1 += m_soilET[i];
    }
    printf("[Evap] Layer1-=m_soilET=%f\n", t1);
    printSoilWater();
#endif
}

void GR4J::Percolation(int fromSoilLayer, int toSoilLayer) {
#pragma omp parallel for
    for (int i = 0; i < m_nCells; i++) {
        FLTPT stor = m_soilWaterStorage[i][fromSoilLayer];
        FLTPT maxStor = m_soilCapacity[i][fromSoilLayer];
        if(maxStor<=0) {
            continue;
        }
        //TODO: In Raven, it is:
        //    rates[0]=stor*(1.0-CalPow(1.0+CalPow(4.0/9.0*Max(stor/max_stor,0.0),4),-0.25))/Options.timestep;
        FLTPT perc = stor * (1.0 - CalPow(1.0 + CalPow(4.0 / 9.0 * Max(stor / maxStor, 0.0), 4), -0.25));

        m_soilWaterStorage[i][fromSoilLayer] -= perc;
        m_soilWaterStorage[i][toSoilLayer] += perc;
    }

#ifdef PRINT_DEBUG
    printf("[Perc]\n");
    printSoilWater();
#endif
}

void GR4J::PercolationExch(int fromSoilLayer, int toSoilLayer) {
#pragma omp parallel for
    for (int i = 0; i < m_nCells; i++) {
        FLTPT stor = m_soilWaterStorage[i][fromSoilLayer];
        FLTPT x2 = m_GR4J_X2[i][fromSoilLayer];
        FLTPT x3 = m_GR4J_X3[i][fromSoilLayer];

        //note - x2 can be negative (exports) or positive (imports/baseflow)
        FLTPT perc = -x2 * CalPow(Max(Min(stor / x3, 1.0), 0.0), 3.5);

        m_soilWaterStorage[i][fromSoilLayer] -= perc;
        m_soilWaterStorage[i][toSoilLayer] += perc;
    }
#ifdef PRINT_DEBUG
    printf("[PercExch]\n");
    printSoilWater();
#endif
}

void GR4J::PercolationExch2(int fromSoilLayer, int toSoilLayer) {
    // TODO: difference between exch exch2? Is Raven Exch2 correct?
#pragma omp parallel for
    for (int i = 0; i < m_nCells; i++) {
        FLTPT stor = m_soilWaterStorage[i][SOIL_ROUTING_LAYER];
        FLTPT x2 = m_GR4J_X2[i][SOIL_ROUTING_LAYER];
        FLTPT x3 = m_GR4J_X3[i][SOIL_ROUTING_LAYER];

        //note - x2 can be negative (exports) or positive (imports/baseflow)
        FLTPT perc = -x2 * CalPow(Max(Min(stor / x3, 1.0), 0.0), 3.5);
        m_soilWaterStorage[i][fromSoilLayer] -= perc;
        m_soilWaterStorage[i][toSoilLayer] += perc;
    }
#ifdef PRINT_DEBUG
    printf("[PercExch2]\n");
    printSoilWater();
#endif
}


void GR4J::Flush(FLTPT* fromStore, int toSoilLayer) {
#pragma omp parallel for
    for (int i = 0; i < m_nCells; i++) {
        m_soilWaterStorage[i][toSoilLayer] += fromStore[i];
        fromStore[i] = 0;
    }
#ifdef PRINT_DEBUG
    printf("[Flush]\n");
    printSoilWater();
#endif
}

void GR4J::Flush(int fromSoilLayer, FLTPT* toStore) {
#pragma omp parallel for
    for (int i = 0; i < m_nCells; i++) {
        toStore[i] += m_soilWaterStorage[i][fromSoilLayer];
        m_soilWaterStorage[i][fromSoilLayer] = 0;
    }
#ifdef PRINT_DEBUG
    printf("[Flush]\n");
    printSoilWater();
#endif
}

//Split RAVEN_DEFAULT TEMP_STORE CONVOLUTION[0] CONVOLUTION[1] 0.9
void GR4J::Split() {
#pragma omp parallel for
    for (int i = 0; i < m_nCells; i++) {
        m_convEntering1[i] = m_soilWaterStorage[i][SOIL_TEMP_LAYER] * 0.9;
        m_convEntering2[i] = m_soilWaterStorage[i][SOIL_TEMP_LAYER] * 0.1;
        m_soilWaterStorage[i][SOIL_TEMP_LAYER] = 0;
    }
}


void GR4J::Convolve(ConvoleType t) {

    vector<vector<FLTPT>>* unitHydro = nullptr;
    FLTPT* convEntering = nullptr;
    vector<vector<FLTPT>>* convTransport = nullptr;

    int toSoilLayer = 0;
    if (t == CONVOL_GR4J_1) {
        unitHydro = &m_unitHydro1;
        convEntering = m_convEntering1;
        convTransport = &m_convTransport1;
        toSoilLayer = SOIL_ROUTING_LAYER;
    }
    else if (t == CONVOL_GR4J_2) {
        unitHydro = &m_unitHydro2;
        convEntering = m_convEntering2;
        convTransport = &m_convTransport2;
        toSoilLayer = SOIL_TEMP_LAYER;
    }
    else {
        throw ModelException(GetModuleName(), "Convolve", "unknown convolution type " + ValueToString(t));
    }

    //FLTPT t1 = 0;
    //FLTPT t3 = 0;
    //for (int i = 0; i < m_nCells; i++) {
    //    t1 += convEntering[i];
    //    for (int j = 0; j < unitHydro->at(i).size(); ++j) {
    //        t3 += convTransport->at(i).at(j);
    //    }
    //}
    //printf("[Conv] Entering=%f, Transport=%f", t1, t3);
#pragma omp parallel for
    for (int i = 0; i < m_nCells; i++) {
        // moving entering water into transport water array
        convTransport->at(i).at(0) = convEntering[i];
        // moving every transport water element to the out soil layer (conceptual store),
        // with a proportion corresponding to the unit hydrograph
        for (int n = 0; n < unitHydro->at(i).size(); n++) {
            m_soilWaterStorage[i][toSoilLayer] += convTransport->at(i).at(n) * unitHydro->at(i).at(n);
        }
        // moving every transport water element to the next index (next timestep).
        for (int n = unitHydro->at(i).size() - 1; n > 0; n--) {
            convTransport->at(i).at(n) = convTransport->at(i).at(n-1);
        }
        convTransport->at(i).at(0) = 0;  // redundant, but clear
    }
    //FLTPT t2 = 0;
    //for (int i = 0; i < m_nCells; i++) {
    //    printf("%d:%f\n",i, m_soilWaterStorage[i][toSoilLayer]);
    //    t2 += m_soilWaterStorage[i][toSoilLayer];
    //}
    //printf("toLayer%d=%f\n", toSoilLayer, t2);
    convEntering = nullptr;
    convTransport = nullptr;
}

void GR4J::InitUnitHydrograph(ConvoleType t) {

    vector<vector<FLTPT>>* unitHydro = nullptr;
    vector<vector<FLTPT>>* convTransport = nullptr;

    FLTPT tstep = 1; // days
#pragma omp parallel for
    for (int i = 0; i < m_nCells; i++) {
        FLTPT maxTime = 0;
        FLTPT x4 = m_GR4J_X4[i];

        if (t == CONVOL_GR4J_1) {
            maxTime = x4;
            convTransport = &m_convTransport1;
            unitHydro = &m_unitHydro1;
        }
        else if (t == CONVOL_GR4J_2) {
            maxTime = 2 * x4;
            convTransport = &m_convTransport2;
            unitHydro = &m_unitHydro2;
        }
        else {
            throw ModelException(GetModuleName(), "GenerateUnitHydrograph", "unknown convolution type " + ValueToString(t));
        }

        int N = ceil(maxTime / tstep);

        if (N == 0) { N = 1; }
        if (N > 50) {
            throw ModelException(GetModuleName(), "GenerateUnitHydrograph", "unit hydrograph duration for convolution too long");
        }

        FLTPT sum = 0.0;
        for (int n = 0; n < N; ++n) {
            FLTPT h = 0.0;
            if (t == CONVOL_GR4J_1) {
                h = Min(CalPow((n+1)*tstep/x4,2.5), 1.0) - Min(CalPow(n*tstep/x4,2.5), 1.0);
            }
            else if (t == CONVOL_GR4J_2) {
                h = GR4J_SH2((n + 1) * tstep, x4) - GR4J_SH2(n * tstep, x4);
            }
            unitHydro->at(i).push_back(h);
            convTransport->at(i).push_back(0);
            sum += h;
        }
        if (sum == 0.0) {
            throw ModelException(GetModuleName(), "GenerateUnitHydrograph", "bad unit hydrograph constructed");
        }
        for (int n = 0; n < N; ++n) { unitHydro->at(i).at(n) /= sum; }
    }
    convTransport = nullptr;
    unitHydro = nullptr;
}

FLTPT GR4J::GR4J_SH2(const FLTPT& t, const FLTPT& x4) {
    if (t / x4 < 1.0) {
        return 0.5 * CalPow(t / x4, 2.5);
    }
    else if (t / x4 < 2.0) {
        return 1.0 - 0.5 * CalPow(2 - t / x4, 2.5);
    }
    else {
        return 1.0;
    }
}

void GR4J::Baseflow(int fromSoilLayer) {
    FLTPT t1 = 0;
#pragma omp parallel for
    for (int i = 0; i < m_nCells; i++) {
        FLTPT x3 = m_GR4J_X3[i][fromSoilLayer];
        FLTPT stor = m_soilWaterStorage[i][fromSoilLayer];
        FLTPT q = stor * (1.0 - CalPow(1.0 + CalPow(Max(stor/x3, 0.0), 4), -0.25));
        t1 += q;
        m_pcpExcess[i] += q;
        m_soilWaterStorage[i][fromSoilLayer] -= q;
    }
#ifdef PRINT_DEBUG
    printf("[Baseflow] delta q=%f, m_pcpExcess=%f\n", t1, sum(m_pcpExcess,m_nCells));
    printSoilWater();
#endif
}


int GR4J::Execute() {
    InitialOutputs();
    CheckInputData();

    Infiltration();
    SoilEvaporation();
    Percolation(SOIL_PRODUCT_LAYER, SOIL_TEMP_LAYER);
    Flush(m_pcpExcess, SOIL_TEMP_LAYER);
    Split();
    Convolve(CONVOL_GR4J_1);
    Convolve(CONVOL_GR4J_2);
    PercolationExch(SOIL_ROUTING_LAYER, SOIL_GW_LAYER);
    PercolationExch2(SOIL_TEMP_LAYER, SOIL_GW_LAYER);
    Flush(SOIL_TEMP_LAYER, m_pcpExcess);
    Baseflow(SOIL_ROUTING_LAYER);
    for (int i = 0; i < m_nCells; ++i) {
        m_soilProfileWater[i] = 0;
        for (int j = 0; j < N_SOIL_LAYERS - 1; ++j) {
            m_soilProfileWater[i] += NonNeg(m_soilWaterStorage[i][j]);
            if(isnan(m_soilWaterStorage[i][j]))
            {
                cout << "[" << i << "," << j << "] " << m_soilWaterStorage[i][j] << endl;
            }
        }
    }
    return 0;
}
