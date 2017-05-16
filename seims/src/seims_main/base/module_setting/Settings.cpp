#include "Settings.h"

Settings::Settings(void) {
}

Settings::Settings(vector<vector<string> >& str2dvec) : m_Settings(str2dvec) {

}

Settings::Settings(vector<string>& str1dvec) {
    SetSettingTagStrings(str1dvec);
}

Settings::~Settings(void) {
    // There is no need to explicit release vector, by LJ
    //if (m_Settings.size() > 0) {
    //    m_Settings.clear();
    //    string2DArray(m_Settings).swap(m_Settings); /// added by LJ
    //}
}

string Settings::GetValue(const string& tag) {
    string res = "";
    if (m_Settings.size() > 0) {
        for (size_t idx = 0; idx < m_Settings.size(); idx++) {
            assert(m_Settings[idx].size() == 2);
            if (StringMatch(m_Settings[idx][0], tag)) {
                res = m_Settings[idx][1];
                break;
            }
        }
    } else {
        cout << "m_Settings is empty, please check and confirm!" << endl;
    }
    return res;
}

void Settings::SetSettingTagStrings(vector<string>& stringvector) {
    assert(stringvector.size() > 0);
    for (vector<string>::iterator iter = stringvector.begin(); iter != stringvector.end(); ++iter) {
        // parse the line into separate items
        vector<string> tokens = SplitString(*iter, '|');
        // is there anything in the token list?
        if (tokens.size() > 0) {
            for (size_t i = 0; i < tokens.size(); i++) {
                tokens[i] = trim(tokens[i]);
            }
            // is there anything in the first item?
            if (tokens[0].size() > 0) {
                // there is something to add so resize the header list to append it
                int sz = m_Settings.size(); // get the current number of rows
                m_Settings.resize(sz + 1);  // resize with one more row
                m_Settings[sz] = tokens;
            } // if there is nothing in the first item of the token list there is nothing to add to the header list
        }
    }
}

//bool Settings::LoadSettingsFromFile(string filename) {
//    m_settingFileName = filename;
//
//    bool bStatus = false;
//    ifstream myfile;
//    string line;
//    try {
//        // open the file
//        myfile.open(filename.c_str(), ios::in);
//        if (myfile.is_open()) {
//            while (!myfile.eof()) {
//                if (myfile.good()) {
//                    getline(myfile, line);
//                    line = trim(line);
//                    if ((line.size() > 0) && (line[0] != '#')) // ignore comments and empty lines
//                    {
//                        // parse the line into separate items
//                        vector <string> tokens = SplitString(line, '|');
//                        // is there anything in the token list?
//                        if (tokens.size() > 0) {
//                            for (size_t i = 0; i < tokens.size(); i++) {
//                                tokens[i] = trim(tokens[i]);
//                            }
//                            // is there anything in the first item?
//                            if (tokens[0].size() > 0) {
//                                // there is something to add so resize the header list to append it
//                                int sz = m_Settings.size(); // get the current number of rows
//                                m_Settings.resize(sz + 1);        // resize with one more row
//
//                                m_Settings[sz] = tokens;
//
//                                bStatus = true; // consider this a success
//                            } // if there is nothing in the first item of the token list there is nothing to add to the header list
//                        }
//                    }
//                }
//            }
//            bStatus = true;
//            myfile.close();
//        }
//    }
//    catch (...) {
//        myfile.close();
//        cout << "LoadSettingsFromFileOutFile, Load output setting file: " << string(filename) << " failed!" << endl;
//        throw;
//    }
//    return bStatus;
//}

void Settings::Dump(string fileName) {
}
