/*
  Copyright (C) 2018 Rory Walsh

  You should have received a copy of the GNU Lesser General Public
  License along with Csound; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/

#include <plugin.h>
#include <string>
#include <modload.h>
#include <iomanip>
#include <fstream>
#include <iostream>
#include "json.hpp"
#include <algorithm>

#include <vector>

#include <numeric>

using json = nlohmann::json;


//https://stackoverflow.com/questions/4643512/replace-substring-with-another-substring-c
void replaceAll(std::string &s, const std::string &search, const std::string &replace) {
    for (size_t pos = 0; ; pos += replace.length()) {
        // Locate the substring to replace
        pos = s.find(search, pos);
        if (pos == std::string::npos) break;
        // Replace by erasing and inserting
        s.erase(pos, search.length());
        s.insert(pos, replace);
    }
}


struct channelStateSave : csnd::Plugin<1, 1>
{
    int init() {
        writeDataToDisk();
        return OK;
    }

    int kperf() {
        writeDataToDisk();
        return OK;
    }

    void writeDataToDisk() {
        json j;

        controlChannelInfo_s* csoundChanList;
        int numberOfChannels = csound->get_csound()->ListChannels(csound->get_csound(), &csoundChanList);

        for (int i = 0; i < numberOfChannels; i++) {
            const float min = csoundChanList[i].hints.min;
            const float max = (csoundChanList[i].hints.max == 0 ? 1 : csoundChanList[i].hints.max);
            const float defaultValue = csoundChanList[i].hints.dflt;
            std::string name;

            MYFLT* value;
            char* chString;

            if (csound->get_csound()->GetChannelPtr(csound->get_csound(), &value, csoundChanList[i].name,
                                                  CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL) == CSOUND_SUCCESS) {
                j[csoundChanList[i].name] = *value;
            }

            if (csound->get_csound()->GetChannelPtr(csound->get_csound(), &value, csoundChanList[i].name,
                                                  CSOUND_STRING_CHANNEL | CSOUND_OUTPUT_CHANNEL) == CSOUND_SUCCESS) {
                chString = ((STRINGDAT*)value)->data;
                std::string s(chString);
                replaceAll(s, "\\\\", "/");
                j[csoundChanList[i].name] = std::string(s);
            }
        }

        std::string filename(inargs.str_data(0).data);
        replaceAll(filename, "\\\\", "/");
        std::ofstream file;
        file.open(filename);
        if (file.is_open() == false)
            outargs[0] = 0;
        else
            outargs[0] = 1;

        file << std::setw(4) << j << std::endl;
        file.close();
        csound->message(j.dump());
    }
};


struct channelStateRecall : csnd::Plugin<1, 2> {
    int init() {
        readDataFromDisk();
        return OK;
    }

    int kperf() {
        readDataFromDisk();
        return OK;
    }

    void readDataFromDisk() {
        json j;
        std::string filename(inargs.str_data(0).data);
        std::vector<std::string> ignoreStrings;

        if(inargs[1] != 0) {
            csnd::Vector<STRINGDAT>& in = inargs.vector_data<STRINGDAT>(1);
            for (int i = 0; i < in.len(); i++) {
                ignoreStrings.push_back(std::string(in[i].data));
            }
        }

        std::ifstream file(filename);
        if (file.fail()) {
            csound->message("Unable to open file");
            outargs[0] = 0;
            return;
        }

        j << file;
        MYFLT* value;

        for (json::iterator it = j.begin(); it != j.end(); ++it) {
            bool ignore = false;
            std::string channelName = it.key();

            for (int i = 0; i < ignoreStrings.size(); i++) {
                if (channelName == ignoreStrings[i])
                    ignore = true;
            }

            if (ignore == false) {
                if (it.value().is_number_float()) {
                    if (csound->get_csound()->GetChannelPtr(csound->get_csound(), &value, channelName.c_str(),
                                                          CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL) == CSOUND_SUCCESS) {
                        *value = it.value();
                    }
                }
                else if (it.value().is_string()) {
                    if (csound->get_csound()->GetChannelPtr(csound->get_csound(), &value, channelName.c_str(),
                                                          CSOUND_STRING_CHANNEL | CSOUND_OUTPUT_CHANNEL) == CSOUND_SUCCESS) {
                        std::string string = it.value();
                        ((STRINGDAT*)value)->data = csound->strdup((char*)string.c_str());
                    }
                }
            }
        }
        outargs[0] = 1;
        file.close();
        csound->message(j.dump());
    }
};


// expseg type opcode with trigger mechanism
struct TrigExpseg : csnd::Plugin<1, 64> {
    int init() {
        int argCnt = 1;
        samplingRate = csound->sr();
        playEnv = 0;
        counter = 0;
        outargs[0] = inargs[1];
        segment = 0;
        outValue = inargs[1];

        while (argCnt < in_count()) {
            if (argCnt % 2 == 0)
                durations.push_back (inargs[argCnt]*samplingRate);
            else {
                if (inargs[argCnt] <= 0.0) {
                    csound->message ("iVal is 0");
                    return NOTOK;
                }
                values.push_back (inargs[argCnt]);
            }
            argCnt++;
        }

        //values.push_back(inargs[argCnt-1]);

        incr = pow (values[1] / values[0], 1 / (durations[0]));

        return OK;
    }

    int kperf() {
        outargs[0] = envGenerator (nsmps);
        return OK;
    }


    int aperf() {
        for (int i = offset; i < nsmps; i++)
            outargs (0)[i] = envGenerator (1);

        return OK;
    }

    MYFLT envGenerator (int sampIncr) {
        // trigger envelope
        if (inargs[0] == 1) {
            incr = pow(values[1] / values[0], 1 / (durations[0]));
            outValue = inargs[1];
            playEnv = 1;
        }

        if (playEnv == 1 && segment < durations.size()) {
            if (counter < durations[segment]) {
                outValue *= incr;
                counter += sampIncr;
            }
            else {
                segment++;
                counter = 0;
                if(segment < durations.size())
                    incr = pow (values[segment + 1] / values[segment], 1 / (durations[segment]));
            }
        }
        else {
            playEnv = 0;
            counter = 0;
            segment = 0;
            outValue = values[values.size() - 1];
        }
        return outValue;
    }

    int samplingRate, playEnv, counter, segment;
    MYFLT outValue, incr;
    std::vector<MYFLT> values;
    std::vector<MYFLT> durations;
};


// linseg type opcode with trigger mechanism
struct TrigLinseg : csnd::Plugin<1, 64> {
    int init() {
        int argCnt = 1;
        totalLength = 0;
        samplingRate = csound->sr();
        playEnv = 0;
        counter = 0;
        outargs[0] = inargs[1];
        segment = 0;
        outValue = 0;

        while (argCnt < in_count()) {
            if (argCnt % 2 == 0)
                durations.push_back (inargs[argCnt]*samplingRate);
            else
                values.push_back (inargs[argCnt]);

            argCnt++;
        }

        //values.push_back(inargs[argCnt - 1]);

        incr = (values[1] - values[0]) / durations[0];
            totalLength = std::accumulate (durations.begin(), durations.end(), 0);
            return OK;
        }

    int kperf() {
        outargs[0] = envGenerator (nsmps);
        return OK;
    }


    int aperf() {
        for (int i = offset; i < nsmps; i++)
            outargs (0)[i] = envGenerator (1);

        return OK;
    }

    MYFLT envGenerator (int sampIncr) {
        // trigger envelope
        if (inargs[0] == 1)
        {
            incr = (values[1] - values[0]) / durations[0];
            outValue = inargs[1];
            playEnv = 1;
        }


        if (playEnv == 1 && segment < durations.size()) {
            if (counter < durations[segment]) {
                outValue += incr;
                counter += sampIncr;
            }
            else {
                segment++;
                counter = 0;
                if (segment < durations.size())
                    incr = (values[segment + 1] - values[segment]) / durations[segment];
            }
        }
        else {
            playEnv = 0;
            counter = 0;
                segment = 0;
            outValue = values[values.size() - 1];
        }

        return outValue;
    }

    int samplingRate, playEnv, counter, totalLength, segment;
    MYFLT outValue, incr;
    std::vector<MYFLT> values;
    std::vector<MYFLT> durations;
};


////////////////////////////////////////////////////


void csnd::on_load (Csound* csound)
{
    csnd::plugin<channelStateSave> (csound, "channelStateSave.i", "i", "S", csnd::thread::i);
    csnd::plugin<channelStateSave>(csound, "channelStateSave.k", "k", "S", csnd::thread::k);
    csnd::plugin<channelStateRecall> (csound, "channelStateRecall.i", "i", "S", csnd::thread::i);
    csnd::plugin<channelStateRecall> (csound, "channelStateRecall.k", "k", "SO", csnd::thread::k);
    csnd::plugin<channelStateRecall>(csound, "channelStateRecall.k", "k", "SS[]", csnd::thread::k);

    csnd::plugin<TrigExpseg> (csound, "trigExpseg.aa", "a", "km", csnd::thread::ia);
    csnd::plugin<TrigExpseg> (csound, "trigExpseg.kk", "k", "km", csnd::thread::ik);

    csnd::plugin<TrigLinseg> (csound, "trigLinseg.aa", "a", "km", csnd::thread::ia);
    csnd::plugin<TrigLinseg> (csound, "trigLinseg.kk", "k", "km", csnd::thread::ik);

}
