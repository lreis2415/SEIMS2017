#include "InterFlow_IKW.h"
#include "text.h"

// using namespace std;  // Avoid this statement! by lj.

InterFlow_IKW::InterFlow_IKW() :
    m_nCells(-1), m_dt(-1.0f), m_CellWidth(-1.0f), m_chWidth(nullptr),
    m_s0(nullptr), m_rootDepth(nullptr), m_ks(nullptr), m_landuseFactor(1.f),
    m_soilWtrSto(nullptr), m_porosity(nullptr), m_poreIndex(nullptr), m_fieldCapacity(nullptr),
    m_flowInIndex(nullptr), m_routingLayers(nullptr), m_nLayers(-1),
    m_q(nullptr), m_h(nullptr), m_sr(nullptr), m_streamLink(nullptr), m_hReturnFlow(nullptr) {
}

InterFlow_IKW::~InterFlow_IKW(void) {
    Release1DArray(m_h);
    Release1DArray(m_q);
    Release1DArray(m_hReturnFlow);
}

bool InterFlow_IKW::CheckInputData(void) {
    if (m_date <= 0) {
        throw ModelException(M_IKW_IF[0], "CheckInputData", "You have not set the Date variable.");
        return false;
    }

    if (m_nCells <= 0) {
        throw ModelException(M_IKW_IF[0], "CheckInputData", "The cell number of the input can not be less than zero.");
        return false;
    }

    if (m_dt <= 0) {
        throw ModelException(M_IKW_IF[0], "CheckInputData", "You have not set the TimeStep variable.");
        return false;
    }

    if (m_CellWidth <= 0) {
        throw ModelException(M_IKW_IF[0], "CheckInputData", "You have not set the CellWidth variable.");
        return false;
    }

    if (m_chWidth == nullptr) {
        throw ModelException(M_IKW_CH[0], "CheckInputData", "The parameter: CHWIDTH has not been set.");
    }

    if (m_flowInIndex == nullptr) {
        throw ModelException(M_IKW_IF[0], "CheckInputData", "The parameter: flow in index has not been set.");
    }
    if (m_routingLayers == nullptr) {
        throw ModelException(M_IKW_IF[0], "CheckInputData", "The parameter: routingLayers has not been set.");
    }

    if (m_s0 == nullptr) {
        throw ModelException(M_IKW_IF[0], "CheckInputData", "The parameter: slope has not been set.");
    }
    if (m_rootDepth == nullptr) {
        throw ModelException(M_IKW_IF[0], "CheckInputData", "The parameter: soil depth has not been set.");
    }
    if (m_ks == nullptr) {
        throw ModelException(M_IKW_IF[0], "CheckInputData", "The parameter: Conductivity has not been set.");
    }

    if (m_porosity == nullptr) {
        throw ModelException(M_IKW_IF[0], "CheckInputData", "The porosity can not be nullptr.");
    }
    if (m_poreIndex == nullptr) {
        throw ModelException(M_IKW_IF[0], "CheckInputData", "The pore index can not be nullptr.");
    }
    if (m_fieldCapacity == nullptr) {
        throw ModelException(M_IKW_IF[0], "CheckInputData", "The field capacity can not be nullptr.");
    }
    if (m_soilWtrSto == nullptr) {
        throw ModelException(M_IKW_IF[0], "CheckInputData", "The soil moistrue can not be nullptr.");
    }
    if (m_streamLink == nullptr) {
        throw ModelException(M_IKW_IF[0], "CheckInputData", "The STREAM_LINK can not be nullptr.");
    }

    if (m_sr == nullptr) {
        throw ModelException(M_IKW_IF[0], "CheckInputData", "The parameter D_SURU is not set.");
    }

    return true;
}

void InterFlow_IKW:: InitialOutputs() {
    if (m_nCells <= 0) {
        throw ModelException(M_IKW_IF[0], "InitialOutputs", "The cell number of the input can not be less than zero.");
    }

    if (m_q == nullptr) {
        CheckInputData();

        m_q = new float[m_nCells];
        m_h = new float[m_nCells];
        m_hReturnFlow = new float[m_nCells];
        for (int i = 0; i < m_nCells; ++i) {
            m_q[i] = 0.0f;
            m_h[i] = 0.f;
            m_hReturnFlow[i] = 0.f;
        }
    }
}

bool InterFlow_IKW::FlowInSoil(const int id) {
    //sum the upstream overland flow
    float qUp = 0.0f;
    for (int k = 1; k <= (int) m_flowInIndex[id][0]; ++k) {
        int flowInID = (int) m_flowInIndex[id][k];
        if (m_streamLink[id] > 0) {
            qUp += m_q[flowInID];
        }
    }

	float s0 = Max(m_s0[id], 0.01f);
    float flowWidth = m_CellWidth;
    // there is no land in this cell
    if (m_streamLink[id] > 0) {
        flowWidth -= m_chWidth[id];
        if (flowWidth <= 0) {
            m_q[id] = qUp;
            m_h[id] = 0.f;
        }
        //return;
    }

   	// adjust soil moisture
	for (int j = 0; j < (int)m_nSoilLyrs; j++) {
		//float s0 = m_s0[id];
		float soilVolumn = m_rootDepth[id][j] / 1000 * m_CellWidth * flowWidth / cos(atan(s0)); //m3
		m_soilWtrSto[id][j] += qUp * m_dt / soilVolumn;

		// the water exceeds the porosity is added to storage (return flow)
		if (m_soilWtrSto[id][j] > m_porosity[id][j]) {
			m_hReturnFlow[id] = (m_soilWtrSto[id][j] - m_porosity[id][j]) * m_rootDepth[id][j];
			m_sr[id] += m_hReturnFlow[id];
			m_soilWtrSto[id][j] = m_porosity[id][j];
		}

		// if soil moisture is below the field capacity, no interflow will be generated
		if (m_soilWtrSto[id][j] < m_fieldCapacity[id][j]) {
			m_q[id] = 0.f;
			m_h[id] = 0.f;
			//return;
		}

		// calculate effective hydraulic conductivity (mm/h -> m/s)
		//float k = m_ks[id]/1000/3600 * CalPow((m_soilMoistrue[id] - m_residual[id])/(m_porosity[id] - m_residual[id]), m_poreIndex[id]);
		float k = m_ks[id][j] / 1000 / 3600 * CalPow(m_soilWtrSto[id][j] / m_porosity[id][j], m_poreIndex[id][j]);
		// calculate interflow (m3/s)
		m_q[id] = m_landuseFactor * m_rootDepth[id][j] / 1000 * s0 * k * m_CellWidth;

		// available water
		float availableWater = (m_soilWtrSto[id][j] - m_fieldCapacity[id][j]) * soilVolumn;
		float interFlow = m_q[id] * (int)m_dt; // m3
		if (interFlow > availableWater) {
			m_q[id] = availableWater / (int)m_dt;
			interFlow = availableWater;
		}
		m_h[id] = 1000 * interFlow / (m_CellWidth * m_CellWidth);

		// adjust soil moisture
		m_soilWtrSto[id][j] -= interFlow / soilVolumn;
	}
	return true;
}

int InterFlow_IKW::Execute() {

    InitialOutputs();

    for (int iLayer = 0; iLayer < m_nLayers; ++iLayer) {
        // There are not any flow relationship within each routing layer.
        // So parallelization can be done here.
        int nCells = (int) m_routingLayers[iLayer][0];
        //SetOpenMPThread(2);
		int errCount = 0; //similar to SSR_DA, such that FlowInSoil(id) isn't called in omp loop
#pragma omp parallel for
        for (int iCell = 1; iCell <= nCells; ++iCell) {
            int id = (int) m_routingLayers[iLayer][iCell];
            if (!FlowInSoil(id)) errCount++;
        }
        if (errCount > 0) {
            throw ModelException(M_IKW_IF[0], "Execute:FlowInSoil",
                                 "Please check the error message for more information");
        }
    }
    return 0;
}

bool InterFlow_IKW::CheckInputSize(const char *key, int n) {
    if (n <= 0) {
        //StatusMsg("Input data for "+string(key) +" is invalid. The size could not be less than zero.");
        return false;
    }
    if (m_nCells != n) {
        if (m_nCells <= 0) { m_nCells = n; }
        else {
            //StatusMsg("Input data for "+string(key) +" is invalid. All the input data should have same size.");
            std::ostringstream oss;
            oss << "Input data for " + string(key) << " is invalid with size: " << n <<
                    ". The origin size is " << m_nCells << ".\n";
            throw ModelException(M_IKW_IF[0], "CheckInputSize", oss.str());
        }
    }

    return true;
}

void InterFlow_IKW::SetValue(const char *key, float data) {
    string sk(key);
    if (StringMatch(sk, Tag_HillSlopeTimeStep[0])) {
        m_dt = data;
    } else if (StringMatch(sk, Tag_CellWidth[0])) {
        m_CellWidth = data;
    } else if (StringMatch(sk, Tag_CellSize[0])) {
        m_nCells = (int) data;
    } else if (StringMatch(sk, VAR_KI[0])) {
        m_landuseFactor = data;
    } else {
        throw ModelException(M_IKW_IF[0], "SetSingleData", "Parameter " + sk
                             + " does not exist.");
    }

}

void InterFlow_IKW::Set1DData(const char *key, int n, float *data) {
    //check the input data
    CheckInputSize(key, n);
    string s(key);
    if (StringMatch(s, VAR_SLOPE[0])) {
        m_s0 = data;
    // } else if (StringMatch(s, VAR_SOILDEPTH[0])) {
    //     m_soilDepth = data;
    // } else if (StringMatch(s, VAR_FIELDCAP[0])) {
    //     m_fieldCapacity = data;
    // } else if (StringMatch(s, VAR_SOILDEPTH[0])) {
    //     m_rootDepth = data;
    // } else if (StringMatch(s, VAR_CONDUCT[0])) {
    //     m_ks = data;
    // } else if (StringMatch(s, VAR_POROST[0])) {
    //     m_porosity = data;
    // } else if (StringMatch(s, VAR_POREIDX[0])) {
    //     m_poreIndex = data;
    // } else if (StringMatch(s, VAR_SOL_ST[0])) {
    //     m_soilMoistrue = data;
    } else if (StringMatch(s, VAR_CHWIDTH[0])) {
        m_chWidth = data;
    } else if (StringMatch(s, VAR_SURU[0])) {
        m_sr = data;
    } else if (StringMatch(s, VAR_STREAM_LINK[0])) {
        m_streamLink = data;
    } else {
        throw ModelException(M_IKW_IF[0], "Set1DData", "Parameter " + s
                             + " does not exist.");
    }
}

void InterFlow_IKW::Get1DData(const char *key, int *n, float **data) {
    InitialOutputs();

    string sk(key);
    *n = m_nCells;
    if (StringMatch(sk, VAR_QSOIL[0])) {
        *data = m_q;
    } else if (StringMatch(sk, VAR_RETURNFLOW[0])) {
        *data = m_hReturnFlow;
    } else {
        throw ModelException(M_IKW_IF[0], "Get1DData", "Output " + sk
                             + " does not exist.");
    }
}

void InterFlow_IKW::Set2DData(const char *key, int nrows, int ncols, float **data) {
    //check the input data

    string sk(key);
    if (StringMatch(sk, Tag_ROUTING_LAYERS[0])) {
        m_nLayers = nrows;
        m_routingLayers = data;
    } else if (StringMatch(sk, Tag_FLOWIN_INDEX[0])) {
		CheckInputSize(key, nrows);
		m_flowInIndex = data;
	}
	else if (StringMatch(sk, VAR_SOILDEPTH[0])) {
		CheckInputSize(key, nrows);
		m_maxSoilLyrs = ncols;
		m_rootDepth = data;
    }
	else if (StringMatch(sk, VAR_SOL_ST[0])) {
		CheckInputSize(key, nrows);
		m_maxSoilLyrs = ncols;
		m_soilWtrSto = data;
	}
	else if (StringMatch(sk, VAR_FIELDCAP[0])) {
		CheckInputSize(key, nrows);
		m_maxSoilLyrs = ncols;
		m_fieldCapacity = data;
	}
	else if (StringMatch(sk, VAR_POROST[0])) {
		CheckInputSize(key, nrows);
		m_maxSoilLyrs = ncols;
		m_porosity = data;
	}
	else if (StringMatch(sk, VAR_POREIDX[0])) {
		CheckInputSize(key, nrows);
		m_maxSoilLyrs = ncols;
		m_poreIndex = data;
	}
	else if (StringMatch(sk, VAR_CONDUCT[0])) {
		CheckInputSize(key, nrows);
		m_maxSoilLyrs = ncols;
		m_ks = data;
	}
	else {
        throw ModelException(M_IKW_IF[0], "Set2DData", "Parameter " + sk
            + " does not exist. Please contact the module developer.");
    }
}
