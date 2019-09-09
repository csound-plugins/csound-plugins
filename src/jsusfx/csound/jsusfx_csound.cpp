/*
 * Copyright 2019 Eduardo Moguillansky
 *
 * Based on jsusfx for pd by Pascal Gauthier (Copyright 2014-2018)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * *distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/


#include <stdarg.h>
#include <fstream>
#include <ctype.h>
#include <limits>

#include "OpcodeBase.hpp"
#include "WDL/mutex.h"
#include "jsusfx.h"

#define REAPER_GET_INTERFACE(opaque) ((opaque) ? ((JsusFxCsound*)opaque) : nullptr)

#define PERFERR(p, fmt) (csound->PerfError(csound, &(p->h), fmt))
#define PERFERRF(p, fmt, ...) (csound->PerfError(csound, &(p->h), fmt, __VA_ARGS__))
#define INITERR(msg) csound->InitError(csound, msg)
#define INITERRF(fmt, ...) csound->InitError(csound, fmt, __VA_ARGS__)
#define MSG(msg) (csound->Message(csound, msg))
#define MSGF(fmt, ...) (csound->Message(csound, fmt, __VA_ARGS__))


using namespace std;

// The maximum of signal inlet/outlet; PD seems to have a limitation to 18 inlets ...
const int MAX_SIGNAL_PORT = 8;
const int MAX_BUF_LEN = MAX_SIGNAL_PORT * 128;
const int PATH_MAX_LEN = 1024;
const int MAX_SLIDERS = 64;

typedef uint32_t jsfxid;

#define register_deinit(csound, p, func) \
    csound->RegisterDeinitCallback(csound, p, (int32_t(*)(CSOUND*, void*))(func))


class JsusFxCsoundPath : public JsusFxPathLibrary_Basic {
    CSOUND *csound;

public:

    JsusFxCsoundPath(const char *_dataRoot, CSOUND *_csound) : JsusFxPathLibrary_Basic(_dataRoot) {
        csound = _csound;
    }    

    bool resolveImportPath(const string &importPath, const string &parentPath, string &resolvedPath) {
        const size_t pos = parentPath.rfind('/', '\\');
        if ( pos != string::npos )
            resolvedPath = parentPath.substr(0, pos + 1);
        
        if ( fileExists(resolvedPath + importPath) ) {
            resolvedPath = resolvedPath + importPath;
            return true;
        }
        return false;
    }

    bool resolveDataPath(const string &importPath, string &resolvedPath) {
        char result[PATH_MAX_LEN];
        const char *filename = importPath.c_str();
        char *resolved_name = csound->FindInputFile(csound, filename, "SSDIR");
        if(resolved_name == NULL) {
            return false;
        }
        csound->strNcpy(result, resolved_name, PATH_MAX_LEN-1);
        csound->Free(csound, resolved_name);
        resolvedPath = result;
        // resolvedPath += '/';
        // resolvedPath += importPath;
        return true;
    }
};

static EEL_F NSEEL_CGEN_CALL midisend(void *opaque, INT_PTR np, EEL_F **parms);

class JsusFxCsound : public JsusFx {
public:
    static const int kMidiBufferSize = 4096;
    
    uint8_t midiHead[kMidiBufferSize];
    uint8_t midiPreStream[4];
    int midiPre = 0, midiExpt = 0;
    uint8_t midiOutBuffer[kMidiBufferSize];
    int midiOutSize = 0;
    double scrapspace[128];
    
    // this is used to indicate if dsp is on (and midi parsing should be done)
    bool dspOn = 1;
    WDL_Mutex dspLock;

    JsusFxCsound(JsusFxPathLibrary &pathLibrary) : JsusFx(pathLibrary) {
        midi = &midiHead[0];
        NSEEL_addfunc_varparm("midisend",3,NSEEL_PProc_THIS,&midisend);
    }
    
    ~JsusFxCsound() {
    }

    // we pre-stream the byte messages since it might come in between dsp cycles
    void midiin(uint8_t b) {
        // midi is read in the dsp thread. Do accumulate stuff if you don't read it
        if ( ! dspOn )
            return;
        
        // in sysex stream, ignore everything
        if ( midiExpt == -1) {
            if ( b == 0xf0 ) {
                midiExpt = 0;
                return;
            }
        }
        
        // nothing is expected; new midi message
        if ( midiExpt == 0 ) {
            if ((b & 0xf0) == 0xf0) {
                midiExpt = -1;
                return;
            }
            
            midiPre = 1;
            midiPreStream[0] = b;
            
            switch(b & 0xf0) {
                case 0xc0:
                case 0xd0:
                    midiExpt = 2;
                    break;
                default:
                    midiExpt = 3;
            }
            return;
        }
    
        midiPreStream[midiPre++] = b;
        if ( midiPre >= midiExpt) {
            if ( midiSize + midiExpt >= kMidiBufferSize ) {
                printf("jsfx: midi buffer full\n");
                dspOn = false;
                return;
            }
        
            for(int i=0;i<midiExpt;i++) {
                // midi[midiSize++] = midiPreStream[i];
            }
            midiExpt = 0;
        }
    }
    
    void displayMsg(const char *fmt, ...) {
        char output[4096];
        va_list argptr;
        va_start(argptr, fmt);
        vsnprintf(output, 4095, fmt, argptr);
        va_end(argptr);

        printf("%s", output);
    }

    void displayError(const char *fmt, ...) {
        char output[4096];
        va_list argptr;
        va_start(argptr, fmt);
        vsnprintf(output, 4095, fmt, argptr);
        va_end(argptr);

        printf("error: %s", output);
    }

    void flushMidi() {
        midi = &midiHead[0];
        midiSize = 0;
    }
};

static EEL_F NSEEL_CGEN_CALL midisend(void *opaque, INT_PTR np, EEL_F **parms) {
    JsusFxCsound *ctx = REAPER_GET_INTERFACE(opaque);
    
    if ( JsusFxCsound::kMidiBufferSize <= ctx->midiOutSize + 3 ) {
        return 1;
    }

    ctx->midiOutBuffer[ctx->midiOutSize++] = *parms[1];
    if ( np >= 4 ) {
        ctx->midiOutBuffer[ctx->midiOutSize++] = *parms[2];
        ctx->midiOutBuffer[ctx->midiOutSize++] = *parms[3];
    } else {
        int v = *parms[2];
        ctx->midiOutBuffer[ctx->midiOutSize++] = v % 256;
        ctx->midiOutBuffer[ctx->midiOutSize++] = v / 256;
    }
    
    return 0;
}


/*
 * jsfx
 *
 * Full version
 *
 * ihandle jsfx_new "path" [, kid0, kparam0, ...]
 * a1, ... jsfx_audio ihandle, a1 [, a2, a3, ...]
 *
 * Simple version
 *
 * as many audio inputs as needed (ain1, ain2, ...)
 * as many audio outputs as needed after ihandle (does not need to match inputs)
 *
 * ihandle, aout1, ... jsfx "path", ain1, ... [, kid0, kparam0, ...]
 */

// todo: MIDI

struct jsfx_handler {
    jsfxid id;
    JsusFxCsoundPath *path;
    JsusFxCsound *fx;
    char scriptpath[PATH_MAX_LEN];
    bool bypass;
    bool user_bypass;
    int pinIn, pinOut;  // number of channels defined in the script  
    float inbuf[MAX_BUF_LEN];
    float outbuf[MAX_BUF_LEN];
    float *in_chanptrs[MAX_SIGNAL_PORT];
    float *out_chanptrs[MAX_SIGNAL_PORT];
    int max_input_channels;
    int max_output_channels; // number of channels asked by the user
    jsfx_handler *nxt;
};

struct jsfx_globals {
    // this keeps a count and assigns a new value to each instance
    jsfxid id_count;
    jsfx_handler *handlers_list;
    CSOUND *csound;
};


// this function must be called after unregistering the handler in the global list
void destroy_handler(CSOUND *csound, jsfx_handler *handler) {
    delete handler->fx;
    delete handler->path;
    csound->Free(csound, handler);
}

void destroy_globals(CSOUND *csound, jsfx_globals *g);

#define JSFX_GLOBALS_VARNAME "__jsfx__globals__"

// can only be called once, only by get_globals
static jsfx_globals* create_globals(CSOUND *csound) {
    int err = csound->CreateGlobalVariable(csound, JSFX_GLOBALS_VARNAME, sizeof(jsfx_globals));
    if (err != 0) {
        MSG(Str("dict: failed to allocate globals"));
        return nullptr;
    };
    jsfx_globals *g = (jsfx_globals*)csound->QueryGlobalVariable(csound, JSFX_GLOBALS_VARNAME);
    g->id_count = 0;
    g->handlers_list = nullptr;
    g->csound = csound;
    csound->RegisterResetCallback(csound, (void*)g, (int32_t(*)(CSOUND*, void*))destroy_globals);
    return g;
}

// returns globals. creates them if not previously created
static inline jsfx_globals* get_globals(CSOUND *csound) {
    jsfx_globals *g = (jsfx_globals*)csound->QueryGlobalVariable(csound, JSFX_GLOBALS_VARNAME);
    if(LIKELY(g != nullptr)) return g;
    return create_globals(csound);
}

// to be called at reset time
void destroy_globals(CSOUND *csound, jsfx_globals *g) {
    jsfx_handler *h = g->handlers_list;
    jsfx_handler *nxt;
    while(h != nullptr) {
        nxt = h->nxt;
        destroy_handler(csound, h);
        h = nxt;
    }
    csound->DestroyGlobalVariable(csound, JSFX_GLOBALS_VARNAME);
}

string getFileName(const string &s) {
    char sep = '/';
    size_t i = s.rfind(sep, s.length());
    if (i != string::npos) {
        return s.substr(i+1, s.length() - i);
    }
    return s;
}

void jsfx_handler_describe(CSOUND *csound, jsfx_handler *x) {
    MSGF("<<< jsfx script %s : %s >>> \n", x->scriptpath, x->fx->desc);
    MSGF("    channels defined in script: in=%d, out=%d\n", x->fx->numInputs, x->fx->numOutputs);
    MSGF("    channels used:              in=%d, out=%d\n", x->pinIn, x->pinOut);
    for(int i=0;i<64;i++) {
        if(!(x->fx->sliders[i].exists))
            continue;
        JsusFx_Slider *s = &(x->fx->sliders[i]);
        if ( s->inc == 0 )
            MSGF("    slider%d: %g %g %s [%g] \n", i, s->min, s->max, s->desc, *(s->owner));
        else
            MSGF("    slider%d: %g %g (%g) %s [%g]\n", i, s->min, s->max, s->inc, s->desc, *(s->owner));
    }
}

static int _dumpvarsCallback(const char *name, EEL_F *val, void *ctx) {
    JsusFxCsound *fx = (JsusFxCsound *) ctx;
    int target;

    if ( sscanf(name, "slider%d", &target) ) {
        if ( target >= 0 && target < JsusFx::kMaxSliders ) {
            if ( ! fx->sliders[target].exists )
                return 1;
            else {
                if(fx->scrapspace[0] == 1) {
                    fx->displayMsg("\n");
                    fx->scrapspace[0] = 0;
                }
                fx->displayMsg("    ** %s --> %f (%s) \n", name, *val, fx->sliders[target].desc);
                return 1;
            }
        }
    }
    if (strlen(name) > 3 && strncmp("spl", name, 3)==0) {
        fx->displayMsg(" %s --> %f \t", name, *val);
        fx->scrapspace[0] = 1;
        return 1;
    }
    if(fx->scrapspace[0] == 1) {
        fx->displayMsg("\n");
        fx->scrapspace[0] = 0;
    }
    fx->displayMsg("    %s --> %f \n", name, *val);
    return 1;
}

void jsfx_dumpvars(JsusFxCsound *fx) {
    fx->displayMsg("\njsfx vars for: %s ========= \n", fx->desc);
    NSEEL_VM_enumallvars(fx->m_vm, _dumpvarsCallback, fx);
    // fx->dumpvars();
}

static int compile_handler(CSOUND *csound, int ksmps, jsfx_handler *x, char *newFile) {
    x->bypass = true;
    string filename = string(newFile);
    if ( newFile != NULL && newFile[0] != 0) {
        string result;
        x->path->resolveDataPath(string(filename), result);
        // find if the file exists with the .jsfx suffix
        if ( ! x->path->resolveDataPath(string(filename), result) ) {
            // maybe it isn't specified, try with the .jsfx
            filename += ".jsfx";
            if ( ! x->path->resolveDataPath(string(filename), result) )
                return INITERRF("compile_handler: unable to find script %s", newFile);
        }
        csound->strNcpy(x->scriptpath, result.c_str(), PATH_MAX_LEN-1);
        // strncpy(x->scriptpath, filename.c_str(), 1023);
    } else {
        if ( x->scriptpath[0] == 0 )
            return INITERR("compile_handler: no scriptfile");
    }
    x->fx->dspLock.Enter();
    if ( x->fx->compile(*(x->path), x->scriptpath, 0) ) {
        // int srate = static_cast<int>(*(x->fx->srate));
        // int samplesblock = static_cast<int>(*(x->fx->samplesblock));
        // printf("srate: %d, samplesblock: %d, float srate: %f\n", srate, samplesblock, *x->fx->srate);
        x->fx->prepare((int)csound->GetSr(csound), ksmps);
        x->bypass = false;
    } else {
        MSG("***** compile failed, bypassing *****\n");
        x->bypass = true;
    }
    x->fx->dspLock.Leave();
    return OK;
}

static inline int slider_check(CSOUND *csound, JsusFxCsound *fx, int id) {
    if ( id > MAX_SLIDERS || id < 0 )
        return INITERRF(Str("slider %d out of range"), id);
    if ( ! fx->sliders[id].exists ) {
        return INITERRF(Str("slider %d not assigned for this effect"), id);
    }
    return OK;
}

static inline int slider_set(JsusFxCsound *fx, int id, MYFLT value) {
    float fvalue = static_cast<float>(value);
    if (id >= 0)
        fx->moveSlider(id, fvalue, 0);
    else
        fx->moveSlider(-id, fvalue, 1);
    return OK;
}

/*
static void jsusfx_midiout(t_jsusfx *x) {
    if ( x->fx->midiOutSize >= JsusFxPd::kMidiBufferSize ) {
        post("jsusfx~: midiout buffer full");
    } else {
        for(int i=0;i<x->fx->midiOutSize;i++) {
            outlet_float(x->midiout, x->fx->midiOutBuffer[i]);
        }
    }
    x->fx->midiOutSize = 0;
}
*/


/* jsfx
 *
 * This is the simplest form of using a jsfx plugin
 *
 * ihandle, a1 [, a2, ...]  jsfx Spath, a1, [a2, ...] [, id0, kval0, id1, kval1, ...]
 *
 * Inputs:
 *   Spath          : path to script. Relative to the same
 *   a1, [a2, ...]  : input audio channels.
 *   idx, kvalx     : set slider `idx` to value `kvalx` (at each k-cycle)
 *
 *
 *
 */
struct t_jsfx {
    OPDS h;

    MYFLT *ihandle;
    void *args[128];

    // -----------------------------

    jsfx_handler *handler;

    // a pointer to the beginning of the input arguments of the opcode (starting with Spath)
    void **inargs;

    // a pointer to the beginning of the input arguments corresponding to the sliders
    // (after the audio args)
    MYFLT **sliderargs;

    // the number of sliders given as pairs (isliderid, kvalue)
    int num_sliders;

    // the number of audio inputs/outputs passed to the opcode
    int num_audio_inputs, num_audio_outputs;

    // number of input/output channels actually processed by the jsfx plugin
    int processed_inputs, processed_outputs;

    MYFLT slidervalues[64];

    // pointers to the input / output audio channels
    MYFLT *inchans[MAX_SIGNAL_PORT], *outchans[MAX_SIGNAL_PORT];
};


/*
 * get the signature of the opcode as ascii
 *
 * a: audio, i:init, k:kontrol, s:string, uppercase if it's an array
 *
 * args: a pointer to the beginning of the arguments passed to the opcode
 * numargs: the number of items in args
 * dest: signature will be put here. It should have been declared as: char dest[256]
 */
static int get_signature(CSOUND *csound, void **args, int numargs, char *dest) {
    CS_TYPE *cstype;
    ARRAYDAT *arr;
    for(int i=0; i < numargs; i++) {
        cstype = csound->GetTypeForArg(args[i]);
        char c = cstype->varTypeName[0];
        switch(c) {
        case 'S':
            dest[i] = 's';
            break;
        case 'c':
            dest[i] = 'i';
            break;
        case 'i':
        case 'k':
        case 'a':
            dest[i] = c;
            break;
        case '[':
            arr = static_cast<ARRAYDAT *>(args[i]);
            c = arr->arrayType->varTypeName[0];
            dest[i] = static_cast<char>(toupper(c));
            break;
        default:
            return INITERRF("Could not parse signature. idx %d = '%c'", i, c);
        }
    }
    dest[numargs] = '\0';
    return OK;
}

/*
 * return the number of audio input arguments
 */
int _get_num_audio_inputs(CSOUND *csound, t_jsfx *p) {
    char inputsig[1024];
    int numinargs = static_cast<int>(csound->GetInputArgCnt(p));
    if(NOTOK==get_signature(csound, p->inargs, numinargs, inputsig))
        return -1;
    // now count all the 'a's
    int cnt = 0;
    for(int i=0; i<numinargs; i++) {
        if(inputsig[i] == 'a')
            cnt += 1;
    }
    return cnt;
}

/*
 * Creates a new handler (returns NULL if failed)
 *
 * Spath: the path to the script
 * ksmps: the block size (needed to initialize internal buffers)
 * inchans / outchans: the max. number of input / output channels. Can be -1 to use the
 *      values defined in the script itself
 *
 * NB: we do not register te handler with the global handlers list, so id will be 0
 * when we return from here. To assign an id, call register_handler
 *
 */

jsfx_handler *make_handler(CSOUND *csound, STRINGDAT *Spath, int ksmps) {
    jsfx_handler *x = (jsfx_handler*)(csound->Malloc(csound, sizeof(jsfx_handler)));
    int numins = MAX_SIGNAL_PORT;
    int numouts = MAX_SIGNAL_PORT;
    printf("//////////////////// 1\n");
    x->id = 0;
    x->path = new JsusFxCsoundPath(string(".").c_str(), csound);
    x->fx = new JsusFxCsound(*(x->path));
    x->scriptpath[0] = 0;
    x->bypass = true;
    x->user_bypass = false;
    x->nxt = nullptr;
    printf("//////////////////// 10 \n");

    // default number of input / output channels
    x->pinIn = 2;
    x->pinOut = 2;

    // x->x_clock = clock_new(x, (t_method)jsusfx_midiout);

    x->max_input_channels = numins;
    x->max_output_channels = numouts;
    printf("//////////////////// 20 \n");

    csound->strNcpy(x->scriptpath, Spath->data, Spath->size);
    int ret = compile_handler(csound, ksmps, x, x->scriptpath);
    printf("//////////////////// 25\n");

    if(ret == NOTOK || x->bypass == true) {
        MSGF("Failed to compile script %s", x->scriptpath);
        // something went wrong with the compilation. bailout
        destroy_handler(csound, x);
        return nullptr;
    }
    printf("//////////////////// 30\n");

    // compile will permantly set the number of pins for this instance
    x->pinIn = x->fx->numInputs;
    x->pinOut = x->fx->numOutputs;

    MSGF("==== jsfx: script %s (%d inputs / %d outputs)\n",
         x->scriptpath, x->fx->numInputs, x->fx->numOutputs);


    if ( x->pinIn > x->max_input_channels ) {
        MSGF(Str("jsfx: The script defines %d input channels, but only %d will be used\n"),
             x->pinIn, x->max_input_channels);
        x->pinIn = x->max_input_channels;
    }
    if ( x->pinIn < 1 )
        x->pinIn = 1;
    if ( x->pinOut > x->max_output_channels ) {
        MSGF(Str("jsfx: The script defines %d output channels, but only %d will be used\n"),
             x->pinOut, x->max_output_channels);
        x->pinOut = x->max_output_channels;
    }
    if ( x->pinOut < 0 )
        x->pinOut = 0;

    jsfx_handler_describe(csound, x);

    // x->midiout = outlet_new(&x->x_obj, &s_float);
    return x;
}

/*
 * Add this handler to the global list of handlers, set its id
 * so it can be found later
 */
static int register_handler(CSOUND *csound, jsfx_handler *handler) {
    if(handler->id != 0)
        return INITERRF("handler was already registered with id: %u", handler->id);
    if(handler->nxt != nullptr)
        return INITERR("handler was already part of the list");
    jsfx_globals *globals = get_globals(csound);
    handler->id = ++(globals->id_count);
    if(handler->id <= 0)
        return INITERRF("register_handler: could not assign an id, got %u", handler->id);
    jsfx_handler *h = globals->handlers_list;
    if(h == nullptr) {
        globals->handlers_list = handler;
        return OK;
    }
    while(h->nxt != nullptr)
        h = h->nxt;
    // now h is the last handler
    h->nxt = handler;
    return OK;
}

jsfx_handler *find_handler(jsfx_globals *g, jsfxid id) {
    jsfx_handler *h = g->handlers_list;
    while(h != nullptr) {
        if(h->id == id)
            return h;
        h = h->nxt;
    }
    return nullptr;
}

/* Remove handler with given id from the global list.
 * Return the found handler, probably to be freed.
 */
jsfx_handler *unregister_handler(CSOUND *csound, jsfxid id) {
    jsfx_globals *globals = get_globals(csound);
    jsfx_handler *h = globals->handlers_list;
    jsfx_handler *prev = nullptr;
    if(h->id == id) {
        // the first one
        jsfx_handler *tmp = h;
        globals->handlers_list = nullptr;
        return tmp;
    }
    h = h->nxt;
    while(h != nullptr) {
        if(h->id == id) {
            // found it!
            if(prev != nullptr)
                prev->nxt = h->nxt;
            return h;
        }
        prev = h;
        h = h->nxt;
    }
    return nullptr;  // not found!
}

static int32_t jsfx_opcode_deinit(CSOUND *csound, t_jsfx *p) {
    if(p->handler->id != 0) {
        jsfx_handler *h = unregister_handler(csound, p->handler->id);
        if(h == nullptr) {
            MSG(Str("Error when attempting to unregister the given handler: handler not registered"));
            return NOTOK;
        }
    }
    destroy_handler(csound, p->handler);
    p->handler = nullptr;
    return OK;
}

static int32_t jsfx_opcode_init(CSOUND *csound, t_jsfx *p) {
    int outchans = p->num_audio_outputs = csound->GetOutputArgCnt(p) - 1;
    // Example: input args starts at idx 3 (ih does not count)
    // ih, a1, a2, a3 jsfx "path", a1, a2, a3
    p->inargs = &(p->args[outchans]);
    int inchans = _get_num_audio_inputs(csound, p);
    if(inchans < 0)
        return INITERR("could not get input signature");
    p->num_audio_inputs = inchans;
    int kparams = csound->GetInputArgCnt(p) - inchans - 1;
    if(kparams % 2 != 0)
        return INITERRF("params should be even, got %d", kparams);
    p->num_sliders = kparams / 2;
    p->sliderargs = (MYFLT **)&(p->inargs[1 + inchans]);
    STRINGDAT *Spath = (STRINGDAT *)p->inargs[0];
    int ksmps = p->h.insdshead->ksmps;
    p->handler = make_handler(csound, Spath, ksmps);
    if(p->handler == nullptr)
        return INITERRF("jsfx_init: Could not make handler for script %s", Spath->data);
    for(int paramidx=0; paramidx < p->num_sliders; paramidx++) {
        int paramid = static_cast<int>(*p->sliderargs[paramidx*2]);
        if(NOTOK == slider_check(csound, p->handler->fx, paramid)) {
            destroy_handler(csound, p->handler);
            return INITERRF("jsfx_init: slider %d does not exist\n", paramid);
        }
    }
    if(NOTOK == register_handler(csound, p->handler))
        return INITERR(Str("jsfx_init: Could not register handler"));
    *p->ihandle = p->handler->id;
    int numins = p->processed_inputs = min(p->num_audio_inputs, p->handler->pinIn);
    int numouts = p->processed_outputs = min(p->num_audio_outputs, p->handler->pinOut);

    if(numins != p->handler->pinIn || numouts != p->handler->pinOut)
        MSGF("jsfx: %s, processed audio streams: %d in / %d out\n",
             p->handler->scriptpath, p->processed_inputs, p->processed_outputs);

    for(int chan=0; chan < numins; chan++)
        p->inchans[chan] = (MYFLT *)(p->inargs[1+chan]);
    for(int chan=0; chan < numouts; chan++)
        p->outchans[chan] = (MYFLT *)(p->args[chan]);
    for(int i=0; i < 64; i++)
        p->slidervalues[i] = 0;
    register_deinit(csound, p, jsfx_opcode_deinit);
    return OK;
}


static int32_t jsfx_opcode_perf(CSOUND *csound, t_jsfx *p) {
    jsfx_handler *x = p->handler;

    MYFLT *outchan;
    MYFLT *inchan;

    int nsmps = p->h.insdshead->ksmps;

    // if processing is bypassed
    if ( (x->bypass || x->user_bypass) || x->fx->dspLock.TryEnter() ) {
        int maxchan = min(p->num_audio_inputs, p->num_audio_outputs);
        for(int chan=0; chan < maxchan; chan++) {
            outchan = (MYFLT *)p->args[chan];
            inchan = (MYFLT *)p->inargs[1+chan];
            for(int j=0; j < nsmps; j++)
                outchan[j] = inchan[j];
        }
        if(maxchan < p->num_audio_outputs) {
            for(int chan=maxchan; chan < p->num_audio_outputs; chan++) {
                outchan = (MYFLT *)p->args[chan];
                for(int j=0; j < nsmps; j++)
                    outchan[j] = 0;
            }
        }
        return OK;
    }

    // set sliders. each slider is a pair of args ksliderid, kslidervalue
    JsusFxCsound *fx = x->fx;
    for(int paramidx=0; paramidx < p->num_sliders; paramidx++) {
        int paramid = static_cast<int>(*p->sliderargs[paramidx*2]);
        MYFLT paramvalue = *p->sliderargs[paramidx*2+1];
        if(p->slidervalues[paramid] != paramvalue) {
            slider_set(fx, paramid, paramvalue);
            p->slidervalues[paramid] = paramvalue;
        }
    }

#ifdef USE_DOUBLE
    x->fx->process64((const MYFLT **)p->inchans, p->outchans, nsmps,
                     p->processed_inputs, p->processed_outputs);
#else
    x->fx->process((const MYFLT **)p->inchans, p->outchans, nsmps,
                   p->processed_inputs, p->processed_outputs);
#endif
    x->fx->dspLock.Leave();

    // if ( x->fx->midiOutSize != 0 )
    //    clock_delay(x->x_clock, 0);

    // x->fx->flushMidi();
    return OK;
}


/*
 * jsfx_new
 *
 *     ihandle jsfx_new "path_to_script"
 *
 * This creates the instance and returns a handle which can be used by jsfx_setslider
 * to set slider values and jsfx_play to process a block of audio
 * Finally, slider values can be read with jsfx_getslider after having called jsfx_play
 * (this makes sense for sliders which have an output function, like gain reduction
 * in a compressor fx)
 *
 */

struct t_jsfx_new {
    OPDS h;

    MYFLT *ihandle;

    STRINGDAT *Spath;

    jsfx_handler *handler;
};


static int32_t jsfx_new_deinit(CSOUND *csound, t_jsfx_new *p) {
    if(p->handler->id != 0) {
        jsfx_handler *h = unregister_handler(csound, p->handler->id);
        if(h == nullptr) {
            MSG(Str("Error when attempting to unregister handler: handler was not registered"));
            return NOTOK;
        }
    }
    destroy_handler(csound, p->handler);
    p->handler = nullptr;
    return OK;
}


static int32_t jsfx_new_init(CSOUND *csound, t_jsfx_new *p) {
    STRINGDAT *Spath = p->Spath;
    int ksmps = p->h.insdshead->ksmps;
    jsfx_handler *handler = make_handler(csound, Spath, ksmps);
    if(NOTOK == register_handler(csound, handler))
        return INITERR(Str("Could not register handler"));
    if(handler->id <= 0) {
        return INITERRF(Str("Error assigning handle id, got %u"), handler->id);
    }
    *p->ihandle = handler->id;
    p->handler = handler;
    register_deinit(csound, p, jsfx_new_deinit);
    return OK;
}


// to be used: ACQUIRE_HANDLER(*p->ihandler)
#define ACQUIRE_HANDLER(myfltid)                     \
    jsfx_globals *_g = get_globals(csound);          \
    jsfxid _id = static_cast<jsfxid>(myfltid);           \
    p->handler = find_handler(_g, _id);              \
    if(p->handler == nullptr)                        \
        return csound->InitError(csound, "handler not found (id=%u)", _id); \

/*
 * jsfx_play
 *
 * a1 [, a2, a3, ...]  jsfx_play ihandle, a1, [a2, a3, ...]
 *
 * Process a block of audio. The handle must have previously been created by jsfx_new.
 * Slider values must have been previously set with jsfx_setslider.
 *
 */

// a1, ... jsfx_play ihandle, a1, ...

struct t_jsfx_play{
    OPDS h;
    MYFLT *outs[8];
    MYFLT *ihandle;
    MYFLT *ins[8];
    void *args[128];

    // ---------------------------
    jsfx_handler *handler;

    // the number of audio inputs/outputs passed to the opcode
    // for ex. the following case has 2 inputs and 3 outputs:
    //    a1, a2, a3 jsfx_play ih, a1, a2
    int num_audio_inputs;
    int num_audio_outputs;

    // the number of input/output channels actually processed each time
    // num channels = min(num. channels declared in the script, num. channels passed to the opcode)
    int processed_input_channels;
    int processed_output_channels;
};


static int32_t jsfx_play_init(CSOUND *csound, t_jsfx_play *p) {
    p->num_audio_outputs = csound->GetOutputArgCnt(p);
    p->num_audio_inputs = csound->GetInputArgCnt(p) - 1;
    ACQUIRE_HANDLER(*p->ihandle);
    p->processed_input_channels  = min(p->num_audio_inputs, p->handler->pinIn);
    p->processed_output_channels = min(p->num_audio_outputs, p->handler->pinOut);
    if(p->handler->pinIn != p->processed_input_channels)
        MSGF("The script declares %d inputs, jsfx_play is given %d inputs"
             ", so %d inputs will be processed\n",
             p->handler->pinIn, p->num_audio_inputs, p->processed_input_channels);
    if(p->handler->pinOut != p->processed_output_channels)
        MSGF("The script declares %d outputs, jsfx_play is given %d outputs"
             ", so %d outputs will be processed\n",
             p->handler->pinOut, p->num_audio_outputs, p->processed_output_channels);
    if(p->processed_input_channels <= 0)
        return INITERRF("Input channels should be >= 1, but got %d", p->processed_input_channels);
    if(p->processed_output_channels <= 0)
        return INITERRF("Output channels should be >= 1, but got %d", p->processed_output_channels);


    return OK;
}

#define DIRECT_MAPPING 1

static int32_t jsfx_play_perf(CSOUND *csound, t_jsfx_play *p) {
    jsfx_handler *x = p->handler;

    MYFLT *outchan, *inchan;

    int nsmps = p->h.insdshead->ksmps;

    // if processing is bypassed
    if ( (x->bypass || x->user_bypass) || x->fx->dspLock.TryEnter() ) {
        int maxchan = min(p->num_audio_inputs, p->num_audio_outputs);
        for(int chan=0; chan < maxchan; chan++) {
            outchan = (MYFLT *)p->outs[chan];
            inchan = (MYFLT *)p->ins[chan];
            for(int j=0; j < nsmps; j++)
                outchan[j] = inchan[j];
        }
        if(maxchan < p->num_audio_outputs) {
            for(int chan=maxchan; chan < p->num_audio_outputs; chan++) {
                outchan = (MYFLT *)p->args[chan];
                for(int j=0; j < nsmps; j++)
                    outchan[j] = 0;
            }
        }
        return OK;
    }

    int numins = p->processed_input_channels;
    int numouts = p->processed_output_channels;

    MYFLT *ins[MAX_SIGNAL_PORT], *outs[MAX_SIGNAL_PORT];
    for(int chan=0; chan < numins; chan++)
        ins[chan] = p->ins[chan];
    for(int chan=0; chan < numouts; chan++)
        outs[chan] = p->outs[chan];

#ifdef USE_DOUBLE
    x->fx->process64((const double **)ins, outs, nsmps, numins, numouts);
#else
    x->fx->process((const float **)ins, outs, nsmps, numins, numouts);
#endif
    x->fx->dspLock.Leave();
    return OK;
}

/*
 * jsfx_dump ihandle, ktrig
 *
 * dumps all variables whenever ktrig > 0 and ktrig > last ktrig
 * if ktrig == -1, dumps every time (to be used within an if statement)
 */

struct t_jsfx_dump  {
    OPDS h;

    MYFLT *ihandler;
    MYFLT *ktrig;

    jsfx_handler *handler;
    MYFLT lasttrig;
};


static int32_t jsfx_dump_init(CSOUND *csound, t_jsfx_dump *p) {
    ACQUIRE_HANDLER(*p->ihandler);
    p->lasttrig = 0;
    return OK;
}

static int32_t jsfx_dump_perf(CSOUND *csound, t_jsfx_dump *p) {
    IGN(csound);
    MYFLT ktrig = *p->ktrig;
    if(ktrig == -1 || (ktrig > 0 && ktrig > p->lasttrig))
        jsfx_dumpvars(p->handler->fx);
    p->lasttrig = ktrig;
    return OK;
}

/*
 * kvalue jsfx_getslider ihandle, ksliderid
 *
 * get the value of a slider (normaly an output slider)
 *
 */

struct t_jsfx_getslider {
    OPDS h;

    MYFLT *outval;

    MYFLT *ihandler;
    MYFLT *ksliderid;

    jsfx_handler *handler;
};


static int32_t jsfx_getslider_init(CSOUND *csound, t_jsfx_getslider *p) {
    ACQUIRE_HANDLER(*p->ihandler);
    return OK;
}

static int32_t jsfx_getslider_perf(CSOUND *csound, t_jsfx_getslider *p) {
    IGN(csound);
    int sliderid = (int)*p->ksliderid;
    if(NOTOK == slider_check(csound, p->handler->fx, sliderid))
        return NOTOK;
    *p->outval = static_cast<MYFLT>( p->handler->fx->sliders[sliderid].getValue() );
    return OK;
}


/*
 * jsfx_setslider ihandle, ksliderid, kvalue
 *
 * set the value of a slider
 *
 */

struct t_jsfx_setslider {
    OPDS h;
    MYFLT *ihandler;
    MYFLT *ksliderid;
    MYFLT *kvalue;

    MYFLT last;
    int lastslider;

    jsfx_handler *handler;
};


#define FL_UNSET -99.0

static int32_t jsfx_setslider_init(CSOUND *csound, t_jsfx_setslider *p) {
    ACQUIRE_HANDLER(*p->ihandler);
    p->last = FL_UNSET;
    p->lastslider = -1;
    return OK;
}

static int32_t jsfx_setslider_perf(CSOUND *csound, t_jsfx_setslider *p) {
    IGN(csound);
    int sliderid = (int)*p->ksliderid;
    MYFLT value = *p->kvalue;
    if(sliderid == p->lastslider) {
        if(p->last == value)
            return OK;
        p->last = value;
        return slider_set(p->handler->fx, sliderid, value);
    } else {
        if(NOTOK == slider_check(csound, p->handler->fx, sliderid))
            return NOTOK;
        p->lastslider = sliderid;
        p->last = value;
        return slider_set(p->handler->fx, sliderid, value);
    }
}

struct t_jsfx_setslider_many {
    OPDS h;
    MYFLT *ihandler;
    MYFLT *args[128];

    jsfx_handler *handler;
    MYFLT lastvalue[64];
    int sliderids[64];
    int numsliders;
};

static int32_t jsfx_setslider_many_init(CSOUND *csound, t_jsfx_setslider_many *p) {
    ACQUIRE_HANDLER(*p->ihandler);
    int numargs = csound->GetInputArgCnt(p) - 1;
    if(numargs % 2 != 0)
        return INITERRF("slider arguments should be even (got %d)."
                        " Signature: setslider ihandle, id0, kval0, id1, kval1, ...",
                        numargs);
    p->numsliders = numargs / 2;
    JsusFxCsound *fx = p->handler->fx;
    for(int i=0; i < p->numsliders; i++) {
        int sliderid = (int)(*p->args[i*2]);
        if(NOTOK == slider_check(csound, fx, sliderid))
            return INITERRF("Could not initialize slider %d", sliderid);
        p->lastvalue[sliderid] = FL_UNSET;
        p->sliderids[i] = sliderid;
    }
    return OK;
}

static int32_t jsfx_setslider_many_perf(CSOUND *csound, t_jsfx_setslider_many *p) {
    JsusFxCsound *fx = p->handler->fx;
    for(int i=0; i < p->numsliders; i++) {
        int sliderid = p->sliderids[i];
        MYFLT value = *p->args[i*2+1];
        if(p->lastvalue[sliderid] != value) {
            p->lastvalue[sliderid] = value;
            slider_set(fx, sliderid, value);
        }
    }
    return OK;
}


/* tubeharmonics
 *
 * port of tubeharmonics.jsfx, to test efficiency
 *
 * a1, a2 tubeharmonics a1, a2, keven=0.3, kodd=0.3, kfluct=0.1, kinputdb=0, koutputdb=0, kgaindb=0, klim=0.5
 * a1     tubeharmonics a1,     keven=0.3, kodd=0.3, kfluct=0.1, kinputdb=0, koutputdb=0, kgaindb=0, klim=0.5
 *
 *
 * slider1:0.3<0,1,0.001>Even Harmonics
 * slider2:0.3<0,1,0.001>Odd Harmonics
 * slider3:0.1<0,1,0.001>Fluctuation
 * slider4:0<-12,12,0.001>TS Input (dB)
 * slider5:0<-12,12,0.001>TS Output (dB)
 * slider6:0<-12,12,0.001>Output Gain (dB)
 */

static inline MYFLT rand(CSOUND *csound, int maxval, int *seed) {
    MYFLT rand1 = (MYFLT) (csound->Rand31(seed) - 1) / FL(2147483645.0);
    return rand1 * maxval;
}

struct t_tubeharmonics_stereo {
    OPDS h;
    MYFLT *out1;
    MYFLT *out2;
    MYFLT *a1, *a2;
    MYFLT *keven, *kodd, *kfluct, *kindb, *koutdb, *kgain;

    MYFLT seed0, seed1;
    MYFLT sc_y0, sc_y1, ka, kb, lim;
    MYFLT src_drve, src_y0, src_y1, src_abs0, ch0, ch1, src_abs1;
    MYFLT m00, m02, m04, m10, m12, m14;
    MYFLT dcf00, dcf01, dcf10, dcf11;
};

static int32_t tubeharmonics_stereo_init(CSOUND *csound, t_tubeharmonics_stereo *p) {
    if(*p->keven == -1)
        *p->keven = 0.3;
    if(*p->kodd == -1)
        *p->kodd = 0.3;
    if(*p->kfluct == -1)
        *p->kfluct = 0.1;

    int seed = csound->GetRandomSeedFromTime();
    MYFLT seed0 = rand(csound, 999, &seed);
    MYFLT seed1 = 0;
    while(seed1 == seed0)
        seed1 = rand(csound, 999, &seed);
    p->sc_y0 = p->sc_y1 = 1;
    p->ka = 0.97;
    p->kb = 1 - p->ka;
    p->lim = 0.5;
    p->seed0 = seed0;
    p->seed1 = seed1;

    p->src_drve = 0.3 * 4;
    p->src_y0 = 0;
    p->src_y1 = 0;
    p->src_abs0 = p->src_abs1 = 0;
    p->ch0 = p->ch1 = 0;
    p->m00 = p->m02 = p->m04 = 0;
    p->m10 = p->m12 = p->m14 = 0;
    p->dcf00 = p->dcf01 = p->dcf10 = p->dcf11 = 0;
    return OK;
}

#ifdef USE_DOUBLE
// from https://www.gamedev.net/forums/topic/621589-extremely-fast-sin-approximation/
static inline double fast_sin(double x) {
    int k;
    double y, z;
    z  = x;
    z *= 0.3183098861837907;
    z += 6755399441055744.0;
    k  = *((int *) &z);
    z  = k;
    z *= 3.1415926535897932;
    x -= z;
    y  = x;
    y *= x;
    z  = 0.0073524681968701;
    z *= y;
    z -= 0.1652891139701474;
    z *= y;
    z += 0.9996919862959676;
    x *= z;
    k &= 1;
    k += k;
    z  = k;
    z *= x;
    x -= z;
    return x;
}
#else
float fast_sin(float floatx) {
    return (float)fast_sin((double)floatx);
}
#endif

static int32_t tubeharmonics_stereo_perf(CSOUND *csound, t_tubeharmonics_stereo *p) {
    MYFLT *a1 = p->a1;
    MYFLT *a2 = p->a2;
    MYFLT *out1 = p->out1;
    MYFLT *out2 = p->out2;

    MYFLT tgt_drve = *p->keven * 4;
    MYFLT drvo = *p->kodd * 9;
    MYFLT kr = *p->kfluct;
    MYFLT kabs = *p->kfluct * 10;
    MYFLT ingain = pow(2, *p->kindb / 6);
    MYFLT hgain = pow(2, *p->koutdb / 6);
    MYFLT trim = pow(2, *p->kgain / 6);
    MYFLT lim = p->lim;
    int samplesblock = p->h.insdshead->ksmps;
    // interpolate parameters

    MYFLT d_drve = (tgt_drve - p->src_drve)/samplesblock;
    MYFLT drve = p->src_drve;
    p->src_drve = tgt_drve;
    p->seed0 += 1;
    p->sc_y0 = sin(p->seed0 * p->sc_y0);

    MYFLT tgt_y0 = p->sc_y0 * kr;
    MYFLT d_y0 = (tgt_y0 - p->src_y0) / samplesblock;
    MYFLT y0 = p->src_y0;
    p->src_y0 = tgt_y0;
    p->seed1 += 1;
    p->sc_y1 = sin(p->seed1 * p->sc_y1);
    MYFLT tgt_y1 = p->sc_y1 * kr;
    MYFLT d_y1 = (tgt_y1 - p->src_y1) / samplesblock;
    MYFLT y1 = p->src_y1;
    p->src_y1 = tgt_y1;
    MYFLT tgt_abs0 = abs(p->ch0) * kabs;
    MYFLT d_abs0 = (tgt_abs0 - p->src_abs0) / samplesblock;
    MYFLT abs0 = p->src_abs0;
    p->src_abs0 = tgt_abs0;

    MYFLT tgt_abs1 = abs(p->ch1) * kabs;
    MYFLT d_abs1 = (tgt_abs1 - p->src_abs1)/samplesblock;
    MYFLT abs1 = p->src_abs1;
    p->src_abs1 = tgt_abs1;
    MYFLT ch0 = 1, ch1 = 1;
    MYFLT minflt = std::numeric_limits<MYFLT>::min();

    for(int n=0; n<samplesblock; n++) {
        ch0 = a1[n] * ingain;
        ch1 = a2[n] * ingain;

        //interpolate
        y0 += d_y0;
        y1 += d_y1;
        abs0 += d_abs0;
        abs1 += d_abs1;
        drve += d_drve;

        //set drive values
        MYFLT drve_rnd0 = drve-abs0;
        MYFLT drve_rnd1 = drve-abs1;
        MYFLT drvo_rnd0 = drvo-abs0-y0;
        MYFLT drvo_rnd1 = drvo-abs1-y1;

        //apply harmonics
        if(ch0 == 0)
            ch0 = minflt;
        if(ch1 == 0)
            ch1 = minflt;
        MYFLT h0 = fast_sin(ch0)/fast_sin(ch0*2)*drve_rnd0+(ch0-tan(ch0))*drvo_rnd0;
        MYFLT h1 = fast_sin(ch1)/fast_sin(ch1*2)*drve_rnd1+(ch1-tan(ch1))*drvo_rnd1;

        //dc filter i
        p->dcf00 = h0 * p->kb + p->dcf00 * p->ka;
        MYFLT dc00 = h0 - p->dcf00;
        p->dcf01 = h1 * p->kb + p->dcf01 * p->ka;
        MYFLT dc01 = h1 - p->dcf01;

        //limiter
        MYFLT lim0 = min(max(dc00*hgain, -lim), lim);
        MYFLT lim1 = min(max(dc01*hgain, -lim), lim);

        //fir filter
        MYFLT m01 = p->m00;
        MYFLT m03 = p->m02;
        MYFLT m05 = p->m04;
        p->m00 = lim0;
        p->m02 = 0.5 * (m01 + p->m00);
        p->m04 = 0.5 * (m03 + p->m02);
        MYFLT fir0 = 0.5 * (m05 + p->m04);
        MYFLT m11 = p->m10;
        MYFLT m13 = p->m12;
        MYFLT m15 = p->m14;
        p->m10 = lim1;
        p->m12 = 0.5 * (m11 + p->m10);
        p->m14 = 0.5 * (m13 + p->m12);
        MYFLT fir1 = 0.5 * (m15 + p->m14);

        //dc filter ii
        p->dcf10 = fir0 * p->kb + p->dcf10 * p->ka;
        MYFLT dc10 = fir0 - p->dcf10;
        p->dcf11 = fir1 * p->kb + p->dcf11 * p->ka;
        MYFLT dc11 = fir1 - p->dcf11;
        //sum

        out1[n] = (a1[n] + dc10) * trim;
        out2[n] = (a2[n] + dc11) * trim;
    }
    p->ch0 = ch0;
    p->ch1 = ch1;
    return OK;
}


struct t_tubeharmonics_mono {
    OPDS h;
    MYFLT *out1;
    MYFLT *a1;
    MYFLT *keven, *kodd, *kfluct, *kindb, *koutdb, *kgain;

    MYFLT seed0, seed1;
    MYFLT sc_y0, ka, kb, lim;
    MYFLT src_drve, src_y0, src_abs0, ch0;
    MYFLT m00, m02, m04;
    MYFLT dcf00, dcf01, dcf10, dcf11;
};

static int32_t tubeharmonics_mono_init(CSOUND *csound, t_tubeharmonics_mono *p) {
    if(*p->keven == -1)
        *p->keven = 0.3;
    if(*p->kodd == -1)
        *p->kodd = 0.3;
    if(*p->kfluct == -1)
        *p->kfluct = 0.1;
    int seed = csound->GetRandomSeedFromTime();
    MYFLT seed0 = rand(csound, 999, &seed);
    MYFLT seed1 = 0;
    while(seed1 == seed0)
        seed1 = rand(csound, 999, &seed);
    p->sc_y0 = 1;
    p->ka = 0.97;
    p->kb = 1 - p->ka;
    p->lim = 0.5;
    p->seed0 = seed0;
    p->seed1 = seed1;

    p->src_drve = 0.3 * 4;
    p->src_y0 = 0;
    p->src_abs0 = 0;
    p->ch0 = 0;
    p->m00 = p->m02 = p->m04 = 0;
    return OK;
}

static int32_t tubeharmonics_mono_perf(CSOUND *csound, t_tubeharmonics_mono *p) {
    MYFLT *a1 = p->a1;
    MYFLT *out1 = p->out1;

    MYFLT tgt_drve = *p->keven * 4;
    MYFLT drvo = *p->kodd * 9;
    MYFLT kr = *p->kfluct;
    MYFLT kabs = *p->kfluct * 10;
    MYFLT ingain = pow(2, *p->kindb / 6);
    MYFLT hgain = pow(2, *p->koutdb / 6);
    MYFLT trim = pow(2, *p->kgain / 6);
    int samplesblock = p->h.insdshead->ksmps;

    // interpolate parameters
    MYFLT d_drve = (tgt_drve - p->src_drve)/samplesblock;
    MYFLT drve = p->src_drve;
    p->src_drve = tgt_drve;
    p->seed0 += 1;
    p->sc_y0 = sin(p->seed0 * p->sc_y0);

    MYFLT tgt_y0 = p->sc_y0 * kr;
    MYFLT d_y0 = (tgt_y0 - p->src_y0) / samplesblock;
    MYFLT y0 = p->src_y0;
    p->src_y0 = tgt_y0;
    p->seed1 += 1;
    MYFLT tgt_abs0 = abs(p->ch0) * kabs;
    MYFLT d_abs0 = (tgt_abs0 - p->src_abs0) / samplesblock;
    MYFLT abs0 = p->src_abs0;
    p->src_abs0 = tgt_abs0;
    MYFLT ch0 = 0;

    for(int n=0; n<samplesblock; n++) {
        ch0 = a1[n] * ingain;

        //interpolate
        y0 += d_y0;
        abs0 += d_abs0;
        drve += d_drve;

        //set drive values
        MYFLT drve_rnd0 = drve-abs0;
        MYFLT drvo_rnd0 = drvo-abs0-y0;

        //apply harmonics
        if(ch0 == 0)
            ch0 = std::numeric_limits<MYFLT>::min();
        MYFLT h0 = fast_sin(ch0)/fast_sin(ch0*2)*drve_rnd0+(ch0-tan(ch0))*drvo_rnd0;

        //dc filter i
        p->dcf00 = h0 * p->kb + p->dcf00 * p->ka;
        MYFLT dc00 = h0 - p->dcf00;

        //limiter
        MYFLT lim0 = min(max(dc00*hgain, -p->lim), p->lim);

        //fir filter
        MYFLT m01 = p->m00;
        MYFLT m03 = p->m02;
        MYFLT m05 = p->m04;
        p->m00 = lim0;
        p->m02 = 0.5 * (m01 + p->m00);
        p->m04 = 0.5 * (m03 + p->m02);
        MYFLT fir0 = 0.5 * (m05 + p->m04);

        //dc filter ii
        p->dcf10 = fir0 * p->kb + p->dcf10 * p->ka;
        MYFLT dc10 = fir0 - p->dcf10;

        //sum
        out1[n] = (a1[n] + dc10) * trim;
    }
    p->ch0 = ch0;
    return OK;
}
// ----------------------------------------------------------------------------------------

#define S(x) sizeof(x)

extern "C" {
    static OENTRY oentries[] = {
        // aout jsfx Spath, ain, kparams...
        // a1 [, a2, ...] jsfx Spath, a1, [a2, ...], [id0, kval0, id1, kval1, ...]
        { (char*)"jsfx", S(t_jsfx), 0, 3, (char*)"i*", (char*)"S*",
          (SUBR)jsfx_opcode_init, (SUBR)jsfx_opcode_perf },

        // ihandle jsfx_new Spath
        { (char*)"jsfx_new", S(t_jsfx_new), 0, 1, (char*)"i", (char*)"S",
          (SUBR)jsfx_new_init, nullptr },

        // a1, [a2, ...] jsfx_play ihandle, a1, [a2, ...]
        { (char*)"jsfx_play", S(t_jsfx_play), 0, 3, (char*)"mmmmmmmm", (char*)"iM",
          (SUBR)jsfx_play_init, (SUBR)jsfx_play_perf },

        // jsfx_dump ihandle, ktrig
        { (char*)"jsfx_dump", S(t_jsfx_dump), 0, 3, (char*)"", (char*)"ik",
          (SUBR)jsfx_dump_init, (SUBR)jsfx_dump_perf },

        // kval jsfx_getslider ihandle, ksliderid
        { (char*)"jsfx_getslider", S(t_jsfx_getslider), 0, 3, (char*)"k", (char*)"ik",
          (SUBR)jsfx_getslider_init, (SUBR)jsfx_getslider_perf },

        // jsfx_setslider ihandle, kid, kval
        // { (char*)"jsfx_setslider", S(t_jsfx_setslider), 0, 3, (char*)"", (char*)"ikk",
        //  (SUBR)jsfx_setslider_init, (SUBR)jsfx_setslider_perf },

        // jsfx_setslider ihandle, id0, kval0 [, id1, kval1, ...]
        { (char*)"jsfx_setslider.many", S(t_jsfx_setslider_many), 0, 3, (char*)"", (char*)"iM",
          (SUBR)jsfx_setslider_many_init, (SUBR)jsfx_setslider_many_perf },

        // a1, a2 tubeharmonics a1, a2, keven, kodd, kfluct, kinputdb, koutputdb, kgain
        { (char*)"tubeharmonics.2", S(t_tubeharmonics_stereo), 0, 3, (char*)"aa", (char*)"aaJJJOOO",
          (SUBR)tubeharmonics_stereo_init, (SUBR)tubeharmonics_stereo_perf},

        { (char*)"tubeharmonics.1", S(t_tubeharmonics_mono), 0, 3, (char*)"a", (char*)"aJJJOOO",
          (SUBR)tubeharmonics_mono_init, (SUBR)tubeharmonics_mono_perf},
        // this signals end of loop
        { 0, 0, 0, 0, 0, 0, 0, 0}
    };

    PUBLIC int csoundModuleCreate(CSOUND *csound) {
        IGN(csound);
        return 0;
    }

    PUBLIC int csoundModuleInit(CSOUND *csound) {
        int status = 0;
        for(OENTRY *oentry = &oentries[0]; oentry->opname; oentry++) {
            status |= csound->AppendOpcode(csound, oentry->opname,
                                           oentry->dsblksiz, oentry->flags,
                                           oentry->thread,
                                           oentry->outypes, oentry->intypes,
                                           (int (*)(CSOUND*,void*)) oentry->iopadr,
                                           (int (*)(CSOUND*,void*)) oentry->kopadr,
                                           (int (*)(CSOUND*,void*)) oentry->aopadr);
        }
        return status;
    }

    PUBLIC int csoundModuleDestroy(CSOUND *csound) {
        IGN(csound);
        return 0;
    }
}
