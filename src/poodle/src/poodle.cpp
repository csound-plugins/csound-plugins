#include "../../common/_common.h"
#include "../../common/_str_common.h"

/* ------------------------------------------------------------
author: "JOS, Revised by RM"
name: "zitarev"
version: "0.0"
Code generated with Faust 2.70.3 (https://faust.grame.fr)
Compilation options: -a /usr/share/faust/csound.cpp -lang cpp -i -ct 1 -es 1 -mcd 16 -mdd 1024 -mdy 33 -uim -double -ftz 0
------------------------------------------------------------ */

/************************************************************************
 IMPORTANT NOTE : this file contains two clearly delimited sections :
 the ARCHITECTURE section (in two parts) and the USER section. Each section
 is governed by its own copyright and license. Please check individually
 each section for license and copyright information.
 *************************************************************************/

/******************* BEGIN csound.cpp ****************/

/************************************************************************
 FAUST Architecture File
 Copyright (C) 2010-2019 V. Lazzarini and GRAME
 ---------------------------------------------------------------------
 This Architecture section is free software; you can redistribute it
 and/or modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 3 of
 the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; If not, see <http://www.gnu.org/licenses/>.

 EXCEPTION : As a special exception, you may create a larger work
 that contains this FAUST architecture section and distribute
 that work under terms of your choice, so long as this FAUST
 architecture section is not modified.

 ************************************************************************
 ************************************************************************/

//==============================================================================
//
//          CSOUND6 architecture file for FAUST
//          Y. Orlarey & V. Lazzarini
//
//          Usage : faust -uim -a csound.cpp <myfx>.dsp -o <myfx>.cpp
//                  g++ -O3 -DOPCODE_NAME=<myfx> -c <myfx>.cpp -o <myfx>.o
//                  ld -E --shared <myfx>.o -o <myfx>.so
//
//          History :
//          - 28/04/09 : first version
//          - 29/04/09 : dynamic allocation
//          - 10/07/14 : updated to Csound6 (Paul Batchelor)
//          - 29/03/20 : correct entry point
//
//==============================================================================

#include "csdl.h"                        /* CSOUND plugin API header */

// make sure we use csound floats
#define FAUSTFLOAT MYFLT

// we require macro declarations
#define FAUST_UIMACROS


/************************** BEGIN misc.h *******************************
FAUST Architecture File
Copyright (C) 2003-2022 GRAME, Centre National de Creation Musicale
---------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

EXCEPTION : As a special exception, you may create a larger work
that contains this FAUST architecture section and distribute
that work under terms of your choice, so long as this FAUST
architecture section is not modified.
***************************************************************************/

#ifndef __misc__
#define __misc__

#include <algorithm>
#include <cstdlib>
#include <string.h>

/************************** BEGIN meta.h *******************************
 FAUST Architecture File
 Copyright (C) 2003-2022 GRAME, Centre National de Creation Musicale
 ---------------------------------------------------------------------
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation; either version 2.1 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 EXCEPTION : As a special exception, you may create a larger work
 that contains this FAUST architecture section and distribute
 that work under terms of your choice, so long as this FAUST
 architecture section is not modified.
 ************************************************************************/

#ifndef __meta__
#define __meta__

/************************************************************************
 FAUST Architecture File
 Copyright (C) 2003-2022 GRAME, Centre National de Creation Musicale
 ---------------------------------------------------------------------
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation; either version 2.1 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 EXCEPTION : As a special exception, you may create a larger work
 that contains this FAUST architecture section and distribute
 that work under terms of your choice, so long as this FAUST
 architecture section is not modified.
 ***************************************************************************/

#ifndef __export__
#define __export__

// Version as a global string
#define FAUSTVERSION "2.70.3"

// Version as separated [major,minor,patch] values
#define FAUSTMAJORVERSION 2
#define FAUSTMINORVERSION 70
#define FAUSTPATCHVERSION 3

// Use FAUST_API for code that is part of the external API but is also compiled in faust and libfaust
// Use LIBFAUST_API for code that is compiled in faust and libfaust

#ifdef _WIN32
    #pragma warning (disable: 4251)
    #ifdef FAUST_EXE
        #define FAUST_API
        #define LIBFAUST_API
    #elif FAUST_LIB
        #define FAUST_API __declspec(dllexport)
        #define LIBFAUST_API __declspec(dllexport)
    #else
        #define FAUST_API
        #define LIBFAUST_API
    #endif
#else
    #ifdef FAUST_EXE
        #define FAUST_API
        #define LIBFAUST_API
    #else
        #define FAUST_API __attribute__((visibility("default")))
        #define LIBFAUST_API __attribute__((visibility("default")))
    #endif
#endif

#endif  // __export__

#endif  // __meta__
/**************************  END  meta.h **************************/

#endif  // __misc__

/**************************  END  misc.h **************************/

/************************** BEGIN dsp.h ********************************
 FAUST Architecture File
 Copyright (C) 2003-2022 GRAME, Centre National de Creation Musicale
 ---------------------------------------------------------------------
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation; either version 2.1 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 EXCEPTION : As a special exception, you may create a larger work
 that contains this FAUST architecture section and distribute
 that work under terms of your choice, so long as this FAUST
 architecture section is not modified.
 ************************************************************************/

#ifndef __dsp__
#define __dsp__

#include <cstdint>


struct FAUST_API UI;
struct FAUST_API Meta;


// #define FAUST_COMPILATION_OPIONS "-a /usr/share/faust/csound.cpp -lang cpp -i -ct 1 -es 1 -mcd 16 -mdd 1024 -mdy 33 -uim -double -ftz 0"
#define ZITAREV_INPUTS 2
#define ZITAREV_OUTPUTS 2
#define ZITAREV_CONTROLS 11

/**
 * DSP memory manager.
 */

struct FAUST_API dsp_memory_manager {

    virtual ~dsp_memory_manager() {}

    /**
     * Inform the Memory Manager with the number of expected memory zones.
     * @param count - the number of expected memory zones
     */
    virtual void begin(size_t /*count*/) {}

    /**
     * Give the Memory Manager information on a given memory zone.
     * @param size - the size in bytes of the memory zone
     * @param reads - the number of Read access to the zone used to compute one frame
     * @param writes - the number of Write access to the zone used to compute one frame
     */
    virtual void info(size_t /*size*/, size_t /*reads*/, size_t /*writes*/) {}

    /**
     * Inform the Memory Manager that all memory zones have been described,
     * to possibly start a 'compute the best allocation strategy' step.
     */
    virtual void end() {}

    /**
     * Allocate a memory zone.
     * @param size - the memory zone size in bytes
     */
    virtual void* allocate(size_t size) = 0;

    /**
     * Destroy a memory zone.
     * @param ptr - the memory zone pointer to be deallocated
     */
    virtual void destroy(void* ptr) = 0;

};

/**
* Signal processor definition.
*/

class FAUST_API dsp {

    public:

        dsp() {}
        virtual ~dsp() {}

        /* Return instance number of audio inputs */
        virtual int getNumInputs() = 0;

        /* Return instance number of audio outputs */
        virtual int getNumOutputs() = 0;

        /**
         * Trigger the ui_interface parameter with instance specific calls
         * to 'openTabBox', 'addButton', 'addVerticalSlider'... in order to build the UI.
         *
         * @param ui_interface - the user interface builder
         */
        virtual void buildUserInterface(UI* ui_interface) = 0;

        /* Return the sample rate currently used by the instance */
        virtual int getSampleRate() = 0;

        /**
         * Global init, calls the following methods:
         * - static class 'classInit': static tables initialization
         * - 'instanceInit': constants and instance state initialization
         *
         * @param sample_rate - the sampling rate in Hz
         */
        virtual void init(int sample_rate) = 0;

        /**
         * Init instance state
         *
         * @param sample_rate - the sampling rate in Hz
         */
        virtual void instanceInit(int sample_rate) = 0;

        /**
         * Init instance constant state
         *
         * @param sample_rate - the sampling rate in Hz
         */
        virtual void instanceConstants(int sample_rate) = 0;

        /* Init default control parameters values */
        virtual void instanceResetUserInterface() = 0;

        /* Init instance state (like delay lines...) but keep the control parameter values */
        virtual void instanceClear() = 0;

        /**
         * Return a clone of the instance.
         *
         * @return a copy of the instance on success, otherwise a null pointer.
         */
        virtual dsp* clone() = 0;

        /**
         * DSP instance computation, to be called with successive in/out audio buffers.
         *
         * @param count - the number of frames to compute
         * @param inputs - the input audio buffers as an array of non-interleaved FAUSTFLOAT samples (eiher float, double or quad)
         * @param outputs - the output audio buffers as an array of non-interleaved FAUSTFLOAT samples (eiher float, double or quad)
         *
         */
        virtual void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) = 0;

        /**
         * DSP instance computation: alternative method to be used by subclasses.
         *
         * @param date_usec - the timestamp in microsec given by audio driver.
         * @param count - the number of frames to compute
         * @param inputs - the input audio buffers as an array of non-interleaved FAUSTFLOAT samples (either float, double or quad)
         * @param outputs - the output audio buffers as an array of non-interleaved FAUSTFLOAT samples (either float, double or quad)
         *
         */
        virtual void compute(double /*date_usec*/, int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) { compute(count, inputs, outputs); }

};


// Denormal handling

#if defined (__SSE__)
#include <xmmintrin.h>
#endif

class FAUST_API ScopedNoDenormals {

    private:

        intptr_t fpsr = 0;

        void setFpStatusRegister(intptr_t fpsr_aux) noexcept
        {
        #if defined (__arm64__) || defined (__aarch64__)
            asm volatile("msr fpcr, %0" : : "ri" (fpsr_aux));
        #elif defined (__SSE__)
            // The volatile keyword here is needed to workaround a bug in AppleClang 13.0
            // which aggressively optimises away the variable otherwise
            volatile uint32_t fpsr_w = static_cast<uint32_t>(fpsr_aux);
            _mm_setcsr(fpsr_w);
        #endif
        }

        void getFpStatusRegister() noexcept
        {
        #if defined (__arm64__) || defined (__aarch64__)
            asm volatile("mrs %0, fpcr" : "=r" (fpsr));
        #elif defined (__SSE__)
            fpsr = static_cast<intptr_t>(_mm_getcsr());
        #endif
        }

    public:

        ScopedNoDenormals() noexcept
        {
        #if defined (__arm64__) || defined (__aarch64__)
            intptr_t mask = (1 << 24 /* FZ */);
        #elif defined (__SSE__)
        #if defined (__SSE2__)
            intptr_t mask = 0x8040;
        #else
            intptr_t mask = 0x8000;
        #endif
        #else
            intptr_t mask = 0x0000;
        #endif
            getFpStatusRegister();
            setFpStatusRegister(fpsr | mask);
        }

        ~ScopedNoDenormals() noexcept
        {
            setFpStatusRegister(fpsr);
        }

};

#define AVOIDDENORMALS ScopedNoDenormals ftz_scope;

#endif

/************************** END dsp.h **************************/
/************************** BEGIN DecoratorUI.h **************************
 FAUST Architecture File
Copyright (C) 2003-2022 GRAME, Centre National de Creation Musicale
---------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

EXCEPTION : As a special exception, you may create a larger work
that contains this FAUST architecture section and distribute
that work under terms of your choice, so long as this FAUST
architecture section is not modified.
*************************************************************************/

#ifndef Decorator_UI_H
#define Decorator_UI_H

/************************** BEGIN UI.h *****************************
 FAUST Architecture File
 Copyright (C) 2003-2022 GRAME, Centre National de Creation Musicale
 ---------------------------------------------------------------------
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation; either version 2.1 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 EXCEPTION : As a special exception, you may create a larger work
 that contains this FAUST architecture section and distribute
 that work under terms of your choice, so long as this FAUST
 architecture section is not modified.
 ********************************************************************/

#ifndef __UI_H__
#define __UI_H__


/*******************************************************************************
 * UI : Faust DSP User Interface
 * User Interface as expected by the buildUserInterface() method of a DSP.
 * This abstract class contains only the method that the Faust compiler can
 * generate to describe a DSP user interface.
 ******************************************************************************/

struct Soundfile;

template <typename REAL>
struct FAUST_API UIReal {

    UIReal() {}
    virtual ~UIReal() {}

    // -- widget's layouts

    virtual void openTabBox(const char* label) = 0;
    virtual void openHorizontalBox(const char* label) = 0;
    virtual void openVerticalBox(const char* label) = 0;
    virtual void closeBox() = 0;

    // -- active widgets

    virtual void addButton(const char* label, REAL* zone) = 0;
    virtual void addCheckButton(const char* label, REAL* zone) = 0;
    virtual void addVerticalSlider(const char* label, REAL* zone, REAL init, REAL min, REAL max, REAL step) = 0;
    virtual void addHorizontalSlider(const char* label, REAL* zone, REAL init, REAL min, REAL max, REAL step) = 0;
    virtual void addNumEntry(const char* label, REAL* zone, REAL init, REAL min, REAL max, REAL step) = 0;

    // -- passive widgets

    virtual void addHorizontalBargraph(const char* label, REAL* zone, REAL min, REAL max) = 0;
    virtual void addVerticalBargraph(const char* label, REAL* zone, REAL min, REAL max) = 0;

    // -- soundfiles

    virtual void addSoundfile(const char* label, const char* filename, Soundfile** sf_zone) = 0;

    // -- metadata declarations

    virtual void declare(REAL* /*zone*/, const char* /*key*/, const char* /*val*/) {}

    // To be used by LLVM client
    virtual int sizeOfFAUSTFLOAT() { return sizeof(FAUSTFLOAT); }
};

struct FAUST_API UI : public UIReal<FAUSTFLOAT> {
    UI() {}
    virtual ~UI() {}
};

#endif
/**************************  END  UI.h **************************/

#endif

/********************END ARCHITECTURE SECTION (part 1/2)****************/

/**************************BEGIN USER SECTION **************************/

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <math.h>

#ifndef FAUSTCLASS
#define FAUSTCLASS mydsp
#endif

#ifdef __APPLE__
#define exp10f __exp10f
#define exp10 __exp10
#endif

#if defined(_WIN32)
#define RESTRICT __restrict
#else
#define RESTRICT __restrict__
#endif

static inline double pow2f(double value) {
	return value * value;
}

#if USE_FMATH
  #include "../../common/fmath.hpp"
  #define expf fmath::exp

#else
  #define expf std::exp
#endif

#define ZR_EQ2LEVEL  0
#define ZR_EQ2FREQ   1
#define ZR_EQ1LEVEL  2
#define ZR_EQ1FREQ   3
#define ZR_DECAYMID  4
#define ZR_HFDAMP    5
#define ZR_DECAYLFX  6
#define ZR_DECAYLOW  7
#define ZR_DELAYMS   8
#define ZR_DRYWET    9
#define ZR_LEVEL     10

class zitarev_dsp : public dsp {

 public:

    FAUSTFLOAT params[ZITAREV_CONTROLS];

    int sr;
	double fConst3;
    double fConst1;
	double fConst4;
	double fRec13[2];
	double fRec12[2];
	int IOTA0;
	double fVec0[16384];
	int iConst6;
	double fVec1[16384];
	double fConst7;
	double fVec2[4096];
	int iConst8;
	double fRec10[2];
	double fConst10;
	double fRec17[2];
	double fRec16[2];
	double fVec3[16384];
	int iConst12;
	double fVec4[2048];
	int iConst13;
	double fRec14[2];
	double fConst15;
	double fRec21[2];
	double fRec20[2];
	double fVec5[16384];
	int iConst17;
	double fVec6[4096];
	int iConst18;
	double fRec18[2];
	double fConst20;
	double fRec25[2];
	double fRec24[2];
	double fVec7[16384];
	int iConst22;
	double fVec8[2048];
	int iConst23;
	double fRec22[2];
	double fConst25;
	double fRec29[2];
	double fRec28[2];
	double fVec9[32768];
	int iConst27;
	double fVec10[16384];
	double fVec11[4096];
	int iConst28;
	double fRec26[2];
	double fConst30;
	double fRec33[2];
	double fRec32[2];
	double fVec12[16384];
	int iConst32;
	double fVec13[4096];
	int iConst33;
	double fRec30[2];
	double fConst35;
    double fRec36[2];
	double fRec37[2];
	double fVec14[32768];
    double fVec15[4096];
	int iConst37;
	int iConst38;
    int iConst42;
    int iConst43;
	double fRec34[2];
	double fConst40;
	double fRec41[2];
	double fRec40[2];
	double fVec16[32768];
	double fVec17[2048];
	double fRec38[2];
	double fRec2[3];
	double fRec3[3];
	double fRec4[3];
	double fRec5[3];
	double fRec6[3];
	double fRec7[3];
	double fRec8[3];
	double fRec9[3];
	double fRec1[3];
	double fRec0[3];
	double fConst44;
	double fConst45;
	double fRec42[2];
	double fRec43[2];
	double fRec45[3];
	double fRec44[3];

 public:
	zitarev_dsp() {}

	virtual int getNumInputs() { return 2; }
	virtual int getNumOutputs() { return 2; }
	static void classInit(int sample_rate) {}

	virtual void instanceConstants(int sample_rate) {
		sr = sample_rate;
		double fConst0 = std::min<double>(1.92e+05, std::max<double>(1.0, double(sr)));
		fConst1 = 6.283185307179586 / fConst0;
		double fConst2 = std::floor(0.174713 * fConst0 + 0.5);
		fConst3 = 6.907755278982138 * (fConst2 / fConst0);
		fConst4 = 3.141592653589793 / fConst0;
		double fConst5 = std::floor(0.022904 * fConst0 + 0.5);
		iConst6 = int(std::min<double>(8192.0, std::max<double>(0.0, fConst2 - fConst5)));
		fConst7 = 0.001 * fConst0;
		iConst8 = int(std::min<double>(2048.0, std::max<double>(0.0, fConst5 + -1.0)));
		double fConst9 = std::floor(0.153129 * fConst0 + 0.5);
		fConst10 = 6.907755278982138 * (fConst9 / fConst0);
		double fConst11 = std::floor(0.020346 * fConst0 + 0.5);
		iConst12 = int(std::min<double>(8192.0, std::max<double>(0.0, fConst9 - fConst11)));
		iConst13 = int(std::min<double>(1024.0, std::max<double>(0.0, fConst11 + -1.0)));
		double fConst14 = std::floor(0.127837 * fConst0 + 0.5);
		fConst15 = 6.907755278982138 * (fConst14 / fConst0);
		double fConst16 = std::floor(0.031604 * fConst0 + 0.5);
		iConst17 = int(std::min<double>(8192.0, std::max<double>(0.0, fConst14 - fConst16)));
		iConst18 = int(std::min<double>(2048.0, std::max<double>(0.0, fConst16 + -1.0)));
		double fConst19 = std::floor(0.125 * fConst0 + 0.5);
		fConst20 = 6.907755278982138 * (fConst19 / fConst0);
		double fConst21 = std::floor(0.013458 * fConst0 + 0.5);
		iConst22 = int(std::min<double>(8192.0, std::max<double>(0.0, fConst19 - fConst21)));
		iConst23 = int(std::min<double>(1024.0, std::max<double>(0.0, fConst21 + -1.0)));
		double fConst24 = std::floor(0.210389 * fConst0 + 0.5);
		fConst25 = 6.907755278982138 * (fConst24 / fConst0);
		double fConst26 = std::floor(0.024421 * fConst0 + 0.5);
		iConst27 = int(std::min<double>(16384.0, std::max<double>(0.0, fConst24 - fConst26)));
		iConst28 = int(std::min<double>(2048.0, std::max<double>(0.0, fConst26 + -1.0)));
		double fConst29 = std::floor(0.192303 * fConst0 + 0.5);
		fConst30 = 6.907755278982138 * (fConst29 / fConst0);
		double fConst31 = std::floor(0.029291 * fConst0 + 0.5);
		iConst32 = int(std::min<double>(8192.0, std::max<double>(0.0, fConst29 - fConst31)));
		iConst33 = int(std::min<double>(2048.0, std::max<double>(0.0, fConst31 + -1.0)));
		double fConst34 = std::floor(0.256891 * fConst0 + 0.5);
		fConst35 = 6.907755278982138 * (fConst34 / fConst0);
		double fConst36 = std::floor(0.027333 * fConst0 + 0.5);
		iConst37 = int(std::min<double>(16384.0, std::max<double>(0.0, fConst34 - fConst36)));
		iConst38 = int(std::min<double>(2048.0, std::max<double>(0.0, fConst36 + -1.0)));
		double fConst39 = std::floor(0.219991 * fConst0 + 0.5);
		fConst40 = 6.907755278982138 * (fConst39 / fConst0);
		double fConst41 = std::floor(0.019123 * fConst0 + 0.5);
		iConst42 = int(std::min<double>(16384.0, std::max<double>(0.0, fConst39 - fConst41)));
		iConst43 = int(std::min<double>(1024.0, std::max<double>(0.0, fConst41 + -1.0)));
		fConst44 = 44.1 / fConst0;
		fConst45 = 1.0 - fConst44;
	}

	virtual void instanceResetUserInterface() {
	    params[ZR_EQ2LEVEL] = FAUSTFLOAT(0.0);
		params[ZR_EQ2FREQ]  = FAUSTFLOAT(1.5e+03);
		params[ZR_EQ1LEVEL] = FAUSTFLOAT(0.0);
		params[ZR_EQ1FREQ]  = FAUSTFLOAT(315.0);
		params[ZR_DECAYMID] = FAUSTFLOAT(2.0);
		params[ZR_HFDAMP]   = FAUSTFLOAT(6e+03);
		params[ZR_DECAYLFX] = FAUSTFLOAT(2e+02);
		params[ZR_DECAYLOW] = FAUSTFLOAT(3.0);
		params[ZR_DELAYMS]  = FAUSTFLOAT(6e+01);
		params[ZR_DRYWET]   = FAUSTFLOAT(0.0);
		params[ZR_LEVEL]    = FAUSTFLOAT(-2e+01);
	}

	virtual void instanceClear() {
		for (int l0 = 0; l0 < 2; l0 = l0 + 1) {
			fRec13[l0] = 0.0;
		}
		for (int l1 = 0; l1 < 2; l1 = l1 + 1) {
			fRec12[l1] = 0.0;
		}
		IOTA0 = 0;
		for (int l2 = 0; l2 < 16384; l2 = l2 + 1) {
			fVec0[l2] = 0.0;
		}
		for (int l3 = 0; l3 < 16384; l3 = l3 + 1) {
			fVec1[l3] = 0.0;
		}
		for (int l4 = 0; l4 < 4096; l4 = l4 + 1) {
			fVec2[l4] = 0.0;
		}
		for (int l5 = 0; l5 < 2; l5 = l5 + 1) {
			fRec10[l5] = 0.0;
		}
		for (int l6 = 0; l6 < 2; l6 = l6 + 1) {
			fRec17[l6] = 0.0;
		}
		for (int l7 = 0; l7 < 2; l7 = l7 + 1) {
			fRec16[l7] = 0.0;
		}
		for (int l8 = 0; l8 < 16384; l8 = l8 + 1) {
			fVec3[l8] = 0.0;
		}
		for (int l9 = 0; l9 < 2048; l9 = l9 + 1) {
			fVec4[l9] = 0.0;
		}
		for (int l10 = 0; l10 < 2; l10 = l10 + 1) {
			fRec14[l10] = 0.0;
		}
		for (int l11 = 0; l11 < 2; l11 = l11 + 1) {
			fRec21[l11] = 0.0;
		}
		for (int l12 = 0; l12 < 2; l12 = l12 + 1) {
			fRec20[l12] = 0.0;
		}
		for (int l13 = 0; l13 < 16384; l13 = l13 + 1) {
			fVec5[l13] = 0.0;
		}
		for (int l14 = 0; l14 < 4096; l14 = l14 + 1) {
			fVec6[l14] = 0.0;
		}
		for (int l15 = 0; l15 < 2; l15 = l15 + 1) {
			fRec18[l15] = 0.0;
		}
		for (int l16 = 0; l16 < 2; l16 = l16 + 1) {
			fRec25[l16] = 0.0;
		}
		for (int l17 = 0; l17 < 2; l17 = l17 + 1) {
			fRec24[l17] = 0.0;
		}
		for (int l18 = 0; l18 < 16384; l18 = l18 + 1) {
			fVec7[l18] = 0.0;
		}
		for (int l19 = 0; l19 < 2048; l19 = l19 + 1) {
			fVec8[l19] = 0.0;
		}
		for (int l20 = 0; l20 < 2; l20 = l20 + 1) {
			fRec22[l20] = 0.0;
		}
		for (int l21 = 0; l21 < 2; l21 = l21 + 1) {
			fRec29[l21] = 0.0;
		}
		for (int l22 = 0; l22 < 2; l22 = l22 + 1) {
			fRec28[l22] = 0.0;
		}
		for (int l23 = 0; l23 < 32768; l23 = l23 + 1) {
			fVec9[l23] = 0.0;
		}
		for (int l24 = 0; l24 < 16384; l24 = l24 + 1) {
			fVec10[l24] = 0.0;
		}
		for (int l25 = 0; l25 < 4096; l25 = l25 + 1) {
			fVec11[l25] = 0.0;
		}
		for (int l26 = 0; l26 < 2; l26 = l26 + 1) {
			fRec26[l26] = 0.0;
		}
		for (int l27 = 0; l27 < 2; l27 = l27 + 1) {
			fRec33[l27] = 0.0;
		}
		for (int l28 = 0; l28 < 2; l28 = l28 + 1) {
			fRec32[l28] = 0.0;
		}
		for (int l29 = 0; l29 < 16384; l29 = l29 + 1) {
			fVec12[l29] = 0.0;
		}
		for (int l30 = 0; l30 < 4096; l30 = l30 + 1) {
			fVec13[l30] = 0.0;
		}
		for (int l31 = 0; l31 < 2; l31 = l31 + 1) {
			fRec30[l31] = 0.0;
		}
		for (int l32 = 0; l32 < 2; l32 = l32 + 1) {
			fRec37[l32] = 0.0;
		}
		for (int l33 = 0; l33 < 2; l33 = l33 + 1) {
			fRec36[l33] = 0.0;
		}
		for (int l34 = 0; l34 < 32768; l34 = l34 + 1) {
			fVec14[l34] = 0.0;
		}
		for (int l35 = 0; l35 < 4096; l35 = l35 + 1) {
			fVec15[l35] = 0.0;
		}
		for (int l36 = 0; l36 < 2; l36 = l36 + 1) {
			fRec34[l36] = 0.0;
		}
		for (int l37 = 0; l37 < 2; l37 = l37 + 1) {
			fRec41[l37] = 0.0;
		}
		for (int l38 = 0; l38 < 2; l38 = l38 + 1) {
			fRec40[l38] = 0.0;
		}
		for (int l39 = 0; l39 < 32768; l39 = l39 + 1) {
			fVec16[l39] = 0.0;
		}
		for (int l40 = 0; l40 < 2048; l40 = l40 + 1) {
			fVec17[l40] = 0.0;
		}
		for (int l41 = 0; l41 < 2; l41 = l41 + 1) {
			fRec38[l41] = 0.0;
		}
		for (int l42 = 0; l42 < 3; l42 = l42 + 1) {
			fRec2[l42] = 0.0;
		}
		for (int l43 = 0; l43 < 3; l43 = l43 + 1) {
			fRec3[l43] = 0.0;
		}
		for (int l44 = 0; l44 < 3; l44 = l44 + 1) {
			fRec4[l44] = 0.0;
		}
		for (int l45 = 0; l45 < 3; l45 = l45 + 1) {
			fRec5[l45] = 0.0;
		}
		for (int l46 = 0; l46 < 3; l46 = l46 + 1) {
			fRec6[l46] = 0.0;
		}
		for (int l47 = 0; l47 < 3; l47 = l47 + 1) {
			fRec7[l47] = 0.0;
		}
		for (int l48 = 0; l48 < 3; l48 = l48 + 1) {
			fRec8[l48] = 0.0;
		}
		for (int l49 = 0; l49 < 3; l49 = l49 + 1) {
			fRec9[l49] = 0.0;
		}
		for (int l50 = 0; l50 < 3; l50 = l50 + 1) {
			fRec1[l50] = 0.0;
		}
		for (int l51 = 0; l51 < 3; l51 = l51 + 1) {
			fRec0[l51] = 0.0;
		}
		for (int l52 = 0; l52 < 2; l52 = l52 + 1) {
			fRec42[l52] = 0.0;
		}
		for (int l53 = 0; l53 < 2; l53 = l53 + 1) {
			fRec43[l53] = 0.0;
		}
		for (int l54 = 0; l54 < 3; l54 = l54 + 1) {
			fRec45[l54] = 0.0;
		}
		for (int l55 = 0; l55 < 3; l55 = l55 + 1) {
			fRec44[l55] = 0.0;
		}
	}

	virtual void init(int sample_rate) {
		// classInit(sample_rate);
		instanceInit(sample_rate);
	}

	virtual void instanceInit(int sample_rate) {
		instanceConstants(sample_rate);
		instanceResetUserInterface();
		instanceClear();
	}

	virtual zitarev_dsp* clone() {
		return new zitarev_dsp();
	}

	virtual int getSampleRate() { return sr; }

    virtual void buildUserInterface(UI* ui_interface) {}

	virtual void compute(int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
		FAUSTFLOAT* input0 = inputs[0];
		FAUSTFLOAT* input1 = inputs[1];
		FAUSTFLOAT* output0 = outputs[0];
		FAUSTFLOAT* output1 = outputs[1];
		double fSlow0 = std::pow(1e+01, 0.05 * double(params[ZR_EQ2LEVEL]));
		double fSlow1 = double(params[ZR_EQ2FREQ]);
		double fSlow2 = fConst1 * (fSlow1 / std::sqrt(std::max<double>(0.0, fSlow0)));
		double fSlow3 = (1.0 - fSlow2) / (fSlow2 + 1.0);
		double fSlow4 = std::cos(fConst1 * fSlow1) * (fSlow3 + 1.0);
		double fSlow5 = std::pow(1e+01, 0.05 * double(params[ZR_EQ1LEVEL]));
		double fSlow6 = double(params[ZR_EQ1FREQ]);
		double fSlow7 = fConst1 * (fSlow6 / std::sqrt(std::max<double>(0.0, fSlow5)));
		double fSlow8 = (1.0 - fSlow7) / (fSlow7 + 1.0);
		double fSlow9 = std::cos(fConst1 * fSlow6) * (fSlow8 + 1.0);
		double fSlow10 = double(params[ZR_DECAYMID]);
		double fSlow11 = std::exp(-(fConst3 / fSlow10));
		double fSlow12 = pow2f(fSlow11);
		double fSlow13 = 1.0 - fSlow12;
		double fSlow14 = std::cos(fConst1 * double(params[ZR_HFDAMP]));
		double fSlow15 = 1.0 - fSlow14 * fSlow12;
		double fSlow16 = std::sqrt(std::max<double>(0.0, pow2f(fSlow15) / pow2f(fSlow13) + -1.0));
		double fSlow17 = fSlow15 / fSlow13;
		double fSlow18 = fSlow17 - fSlow16;
		double fSlow19 = 1.0 / std::tan(fConst4 * double(params[ZR_DECAYLFX]));
		double fSlow20 = 1.0 - fSlow19;
		double fSlow21 = 1.0 / (fSlow19 + 1.0);
		double fSlow22 = double(params[ZR_DECAYLOW]);
		double fSlow23 = std::exp(-(fConst3 / fSlow22)) / fSlow11 + -1.0;
		double fSlow24 = fSlow11 * (fSlow16 + (1.0 - fSlow17));
		int iSlow25 = int(std::min<double>(8192.0, std::max<double>(0.0, fConst7 * double(params[ZR_DELAYMS]))));
		double fSlow26 = std::exp(-(fConst10 / fSlow10));
		double fSlow27 = pow2f(fSlow26);
		double fSlow28 = 1.0 - fSlow27;
		double fSlow29 = 1.0 - fSlow27 * fSlow14;
		double fSlow30 = std::sqrt(std::max<double>(0.0, pow2f(fSlow29) / pow2f(fSlow28) + -1.0));
		double fSlow31 = fSlow29 / fSlow28;
		double fSlow32 = fSlow31 - fSlow30;
		double fSlow33 = std::exp(-(fConst10 / fSlow22)) / fSlow26 + -1.0;
		double fSlow34 = fSlow26 * (fSlow30 + (1.0 - fSlow31));
		double fSlow35 = std::exp(-(fConst15 / fSlow10));
		double fSlow36 = pow2f(fSlow35);
		double fSlow37 = 1.0 - fSlow36;
		double fSlow38 = 1.0 - fSlow14 * fSlow36;
		double fSlow39 = std::sqrt(std::max<double>(0.0, pow2f(fSlow38) / pow2f(fSlow37) + -1.0));
		double fSlow40 = fSlow38 / fSlow37;
		double fSlow41 = fSlow40 - fSlow39;
		double fSlow42 = std::exp(-(fConst15 / fSlow22)) / fSlow35 + -1.0;
		double fSlow43 = fSlow35 * (fSlow39 + (1.0 - fSlow40));
		double fSlow44 = std::exp(-(fConst20 / fSlow10));
		double fSlow45 = pow2f(fSlow44);
		double fSlow46 = 1.0 - fSlow45;
		double fSlow47 = 1.0 - fSlow14 * fSlow45;
		double fSlow48 = std::sqrt(std::max<double>(0.0, pow2f(fSlow47) / pow2f(fSlow46) + -1.0));
		double fSlow49 = fSlow47 / fSlow46;
		double fSlow50 = fSlow49 - fSlow48;
		double fSlow51 = std::exp(-(fConst20 / fSlow22)) / fSlow44 + -1.0;
		double fSlow52 = fSlow44 * (fSlow48 + (1.0 - fSlow49));
		double fSlow53 = std::exp(-(fConst25 / fSlow10));
		double fSlow54 = pow2f(fSlow53);
		double fSlow55 = 1.0 - fSlow54;
		double fSlow56 = 1.0 - fSlow14 * fSlow54;
		double fSlow57 = std::sqrt(std::max<double>(0.0, pow2f(fSlow56) / pow2f(fSlow55) + -1.0));
		double fSlow58 = fSlow56 / fSlow55;
		double fSlow59 = fSlow58 - fSlow57;
		double fSlow60 = std::exp(-(fConst25 / fSlow22)) / fSlow53 + -1.0;
		double fSlow61 = fSlow53 * (fSlow57 + (1.0 - fSlow58));
		double fSlow62 = std::exp(-(fConst30 / fSlow10));
		double fSlow63 = pow2f(fSlow62);
		double fSlow64 = 1.0 - fSlow63;
		double fSlow65 = 1.0 - fSlow14 * fSlow63;
		double fSlow66 = std::sqrt(std::max<double>(0.0, pow2f(fSlow65) / pow2f(fSlow64) + -1.0));
		double fSlow67 = fSlow65 / fSlow64;
		double fSlow68 = fSlow67 - fSlow66;
		double fSlow69 = std::exp(-(fConst30 / fSlow22)) / fSlow62 + -1.0;
		double fSlow70 = fSlow62 * (fSlow66 + (1.0 - fSlow67));
		double fSlow71 = std::exp(-(fConst35 / fSlow10));
		double fSlow72 = pow2f(fSlow71);
		double fSlow73 = 1.0 - fSlow72;
		double fSlow74 = 1.0 - fSlow14 * fSlow72;
		double fSlow75 = std::sqrt(std::max<double>(0.0, pow2f(fSlow74) / pow2f(fSlow73) + -1.0));
		double fSlow76 = fSlow74 / fSlow73;
		double fSlow77 = fSlow76 - fSlow75;
		double fSlow78 = std::exp(-(fConst35 / fSlow22)) / fSlow71 + -1.0;
		double fSlow79 = fSlow71 * (fSlow75 + (1.0 - fSlow76));
		double fSlow80 = std::exp(-(fConst40 / fSlow10));
		double fSlow81 = pow2f(fSlow80);
		double fSlow82 = 1.0 - fSlow81;
		double fSlow83 = 1.0 - fSlow14 * fSlow81;
		double fSlow84 = std::sqrt(std::max<double>(0.0, pow2f(fSlow83) / pow2f(fSlow82) + -1.0));
		double fSlow85 = fSlow83 / fSlow82;
		double fSlow86 = fSlow85 - fSlow84;
		double fSlow87 = std::exp(-(fConst40 / fSlow22)) / fSlow80 + -1.0;
		double fSlow88 = fSlow80 * (fSlow84 + (1.0 - fSlow85));
		double fSlow89 = fConst44 * double(params[ZR_DRYWET]);
		double fSlow90 = fConst44 * std::pow(1e+01, 0.05 * double(params[ZR_LEVEL]));
		for (int i0 = 0; i0 < count; i0 = i0 + 1) {
			double fTemp0 = fSlow4 * fRec0[1];
			double fTemp1 = fSlow9 * fRec1[1];
			fRec13[0] = -(fSlow21 * (fSlow20 * fRec13[1] - (fRec6[1] + fRec6[2])));
			fRec12[0] = fSlow24 * (fRec6[1] + fSlow23 * fRec13[0]) + fSlow18 * fRec12[1];
			fVec0[IOTA0 & 16383] = 0.35355339059327373 * fRec12[0] + 1e-20;
			double fTemp2 = double(input0[i0]);
			fVec1[IOTA0 & 16383] = fTemp2;
			double fTemp3 = 0.3 * fVec1[(IOTA0 - iSlow25) & 16383];
			double fTemp4 = fTemp3 + fVec0[(IOTA0 - iConst6) & 16383] - 0.6 * fRec10[1];
			fVec2[IOTA0 & 4095] = fTemp4;
			fRec10[0] = fVec2[(IOTA0 - iConst8) & 4095];
			double fRec11 = 0.6 * fTemp4;
			fRec17[0] = -(fSlow21 * (fSlow20 * fRec17[1] - (fRec2[1] + fRec2[2])));
			fRec16[0] = fSlow34 * (fRec2[1] + fSlow33 * fRec17[0]) + fSlow32 * fRec16[1];
			fVec3[IOTA0 & 16383] = 0.35355339059327373 * fRec16[0] + 1e-20;
			double fTemp5 = fVec3[(IOTA0 - iConst12) & 16383] + fTemp3 - 0.6 * fRec14[1];
			fVec4[IOTA0 & 2047] = fTemp5;
			fRec14[0] = fVec4[(IOTA0 - iConst13) & 2047];
			double fRec15 = 0.6 * fTemp5;
			double fTemp6 = fRec15 + fRec11;
			fRec21[0] = -(fSlow21 * (fSlow20 * fRec21[1] - (fRec4[1] + fRec4[2])));
			fRec20[0] = fSlow43 * (fRec4[1] + fSlow42 * fRec21[0]) + fSlow41 * fRec20[1];
			fVec5[IOTA0 & 16383] = 0.35355339059327373 * fRec20[0] + 1e-20;
			double fTemp7 = fVec5[(IOTA0 - iConst17) & 16383] - (fTemp3 + 0.6 * fRec18[1]);
			fVec6[IOTA0 & 4095] = fTemp7;
			fRec18[0] = fVec6[(IOTA0 - iConst18) & 4095];
			double fRec19 = 0.6 * fTemp7;
			fRec25[0] = -(fSlow21 * (fSlow20 * fRec25[1] - (fRec8[1] + fRec8[2])));
			fRec24[0] = fSlow52 * (fRec8[1] + fSlow51 * fRec25[0]) + fSlow50 * fRec24[1];
			fVec7[IOTA0 & 16383] = 0.35355339059327373 * fRec24[0] + 1e-20;
			double fTemp8 = fVec7[(IOTA0 - iConst22) & 16383] - (fTemp3 + 0.6 * fRec22[1]);
			fVec8[IOTA0 & 2047] = fTemp8;
			fRec22[0] = fVec8[(IOTA0 - iConst23) & 2047];
			double fRec23 = 0.6 * fTemp8;
			double fTemp9 = fRec23 + fRec19 + fTemp6;
			fRec29[0] = -(fSlow21 * (fSlow20 * fRec29[1] - (fRec3[1] + fRec3[2])));
			fRec28[0] = fSlow61 * (fRec3[1] + fSlow60 * fRec29[0]) + fSlow59 * fRec28[1];
			fVec9[IOTA0 & 32767] = 0.35355339059327373 * fRec28[0] + 1e-20;
			double fTemp10 = double(input1[i0]);
			fVec10[IOTA0 & 16383] = fTemp10;
			double fTemp11 = 0.3 * fVec10[(IOTA0 - iSlow25) & 16383];
			double fTemp12 = fTemp11 + 0.6 * fRec26[1] + fVec9[(IOTA0 - iConst27) & 32767];
			fVec11[IOTA0 & 4095] = fTemp12;
			fRec26[0] = fVec11[(IOTA0 - iConst28) & 4095];
			double fRec27 = -(0.6 * fTemp12);
			fRec33[0] = -(fSlow21 * (fSlow20 * fRec33[1] - (fRec7[1] + fRec7[2])));
			fRec32[0] = fSlow70 * (fRec7[1] + fSlow69 * fRec33[0]) + fSlow68 * fRec32[1];
			fVec12[IOTA0 & 16383] = 0.35355339059327373 * fRec32[0] + 1e-20;
			double fTemp13 = fVec12[(IOTA0 - iConst32) & 16383] + fTemp11 + 0.6 * fRec30[1];
			fVec13[IOTA0 & 4095] = fTemp13;
			fRec30[0] = fVec13[(IOTA0 - iConst33) & 4095];
			double fRec31 = -(0.6 * fTemp13);
			fRec37[0] = -(fSlow21 * (fSlow20 * fRec37[1] - (fRec5[1] + fRec5[2])));
			fRec36[0] = fSlow79 * (fRec5[1] + fSlow78 * fRec37[0]) + fSlow77 * fRec36[1];
			fVec14[IOTA0 & 32767] = 0.35355339059327373 * fRec36[0] + 1e-20;
			double fTemp14 = 0.6 * fRec34[1] + fVec14[(IOTA0 - iConst37) & 32767];
			fVec15[IOTA0 & 4095] = fTemp14 - fTemp11;
			fRec34[0] = fVec15[(IOTA0 - iConst38) & 4095];
			double fRec35 = 0.6 * (fTemp11 - fTemp14);
			fRec41[0] = -(fSlow21 * (fSlow20 * fRec41[1] - (fRec9[1] + fRec9[2])));
			fRec40[0] = fSlow88 * (fRec9[1] + fSlow87 * fRec41[0]) + fSlow86 * fRec40[1];
			fVec16[IOTA0 & 32767] = 0.35355339059327373 * fRec40[0] + 1e-20;
			double fTemp15 = 0.6 * fRec38[1] + fVec16[(IOTA0 - iConst42) & 32767];
			fVec17[IOTA0 & 2047] = fTemp15 - fTemp11;
			fRec38[0] = fVec17[(IOTA0 - iConst43) & 2047];
			double fRec39 = 0.6 * (fTemp11 - fTemp15);
			fRec2[0] = fRec38[1] + fRec34[1] + fRec30[1] + fRec26[1] + fRec22[1] + fRec18[1] + fRec10[1] + fRec14[1] + fRec39 + fRec35 + fRec31 + fRec27 + fTemp9;
			fRec3[0] = fRec22[1] + fRec18[1] + fRec10[1] + fRec14[1] + fTemp9 - (fRec38[1] + fRec34[1] + fRec30[1] + fRec26[1] + fRec39 + fRec35 + fRec27 + fRec31);
			double fTemp16 = fRec19 + fRec23;
			fRec4[0] = fRec30[1] + fRec26[1] + fRec10[1] + fRec14[1] + fRec31 + fRec27 + fTemp6 - (fRec38[1] + fRec34[1] + fRec22[1] + fRec18[1] + fRec39 + fRec35 + fTemp16);
			fRec5[0] = fRec38[1] + fRec34[1] + fRec10[1] + fRec14[1] + fRec39 + fRec35 + fTemp6 - (fRec30[1] + fRec26[1] + fRec22[1] + fRec18[1] + fRec31 + fRec27 + fTemp16);
			double fTemp17 = fRec11 + fRec23;
			double fTemp18 = fRec15 + fRec19;
			fRec6[0] = fRec34[1] + fRec26[1] + fRec18[1] + fRec14[1] + fRec35 + fRec27 + fTemp18 - (fRec38[1] + fRec30[1] + fRec22[1] + fRec10[1] + fRec39 + fRec31 + fTemp17);
			fRec7[0] = fRec38[1] + fRec30[1] + fRec18[1] + fRec14[1] + fRec39 + fRec31 + fTemp18 - (fRec34[1] + fRec26[1] + fRec22[1] + fRec10[1] + fRec35 + fRec27 + fTemp17);
			double fTemp19 = fRec11 + fRec19;
			double fTemp20 = fRec15 + fRec23;
			fRec8[0] = fRec38[1] + fRec26[1] + fRec22[1] + fRec14[1] + fRec39 + fRec27 + fTemp20 - (fRec34[1] + fRec30[1] + fRec18[1] + fRec10[1] + fRec35 + fRec31 + fTemp19);
			fRec9[0] = fRec34[1] + fRec30[1] + fRec22[1] + fRec14[1] + fRec35 + fRec31 + fTemp20 - (fRec38[1] + fRec26[1] + fRec18[1] + fRec10[1] + fRec39 + fRec27 + fTemp19);
			double fTemp21 = 0.37 * (fRec3[0] + fRec4[0]);
			double fTemp22 = fTemp21 + fTemp1;
			fRec1[0] = fTemp22 - fSlow8 * fRec1[2];
			double fTemp23 = fSlow8 * fRec1[0];
			double fTemp24 = fSlow5 * (fRec1[2] + fTemp23 - fTemp22);
			double fTemp25 = fTemp23 + fTemp21 + fRec1[2];
			fRec0[0] = 0.5 * (fTemp25 - fTemp1 + fTemp24) + fTemp0 - fSlow3 * fRec0[2];
			double fTemp26 = 0.5 * (fTemp25 + fTemp24 - fTemp1);
			double fTemp27 = fRec0[2] + fSlow3 * fRec0[0];
			fRec42[0] = fSlow89 + fConst45 * fRec42[1];
			double fTemp28 = fRec42[0] + 1.0;
			double fTemp29 = 1.0 - 0.5 * fTemp28;
			fRec43[0] = fSlow90 + fConst45 * fRec43[1];
			output0[i0] = FAUSTFLOAT(0.5 * fRec43[0] * (fTemp2 * fTemp28 + fTemp29 * (fTemp27 + fTemp26 + fSlow0 * (fTemp27 - (fTemp0 + fTemp26)) - fTemp0)));
			double fTemp30 = fSlow4 * fRec44[1];
			double fTemp31 = fSlow9 * fRec45[1];
			double fTemp32 = 0.37 * (fRec3[0] - fRec4[0]);
			double fTemp33 = fTemp32 + fTemp31;
			fRec45[0] = fTemp33 - fSlow8 * fRec45[2];
			double fTemp34 = fSlow8 * fRec45[0];
			double fTemp35 = fSlow5 * (fRec45[2] + fTemp34 - fTemp33);
			double fTemp36 = fTemp34 + fTemp32 + fRec45[2];
			fRec44[0] = 0.5 * (fTemp36 - fTemp31 + fTemp35) + fTemp30 - fSlow3 * fRec44[2];
			double fTemp37 = 0.5 * (fTemp36 + fTemp35 - fTemp31);
			double fTemp38 = fRec44[2] + fSlow3 * fRec44[0];
			output1[i0] = FAUSTFLOAT(0.5 * fRec43[0] * (fTemp10 * fTemp28 + fTemp29 * (fTemp38 + fTemp37 + fSlow0 * (fTemp38 - (fTemp30 + fTemp37)) - fTemp30)));
			fRec13[1] = fRec13[0];
			fRec12[1] = fRec12[0];
			IOTA0 = IOTA0 + 1;
			fRec10[1] = fRec10[0];
			fRec17[1] = fRec17[0];
			fRec16[1] = fRec16[0];
			fRec14[1] = fRec14[0];
			fRec21[1] = fRec21[0];
			fRec20[1] = fRec20[0];
			fRec18[1] = fRec18[0];
			fRec25[1] = fRec25[0];
			fRec24[1] = fRec24[0];
			fRec22[1] = fRec22[0];
			fRec29[1] = fRec29[0];
			fRec28[1] = fRec28[0];
			fRec26[1] = fRec26[0];
			fRec33[1] = fRec33[0];
			fRec32[1] = fRec32[0];
			fRec30[1] = fRec30[0];
			fRec37[1] = fRec37[0];
			fRec36[1] = fRec36[0];
			fRec34[1] = fRec34[0];
			fRec41[1] = fRec41[0];
			fRec40[1] = fRec40[0];
			fRec38[1] = fRec38[0];
			fRec2[2] = fRec2[1];
			fRec2[1] = fRec2[0];
			fRec3[2] = fRec3[1];
			fRec3[1] = fRec3[0];
			fRec4[2] = fRec4[1];
			fRec4[1] = fRec4[0];
			fRec5[2] = fRec5[1];
			fRec5[1] = fRec5[0];
			fRec6[2] = fRec6[1];
			fRec6[1] = fRec6[0];
			fRec7[2] = fRec7[1];
			fRec7[1] = fRec7[0];
			fRec8[2] = fRec8[1];
			fRec8[1] = fRec8[0];
			fRec9[2] = fRec9[1];
			fRec9[1] = fRec9[0];
			fRec1[2] = fRec1[1];
			fRec1[1] = fRec1[0];
			fRec0[2] = fRec0[1];
			fRec0[1] = fRec0[0];
			fRec42[1] = fRec42[0];
			fRec43[1] = fRec43[0];
			fRec45[2] = fRec45[1];
			fRec45[1] = fRec45[0];
			fRec44[2] = fRec44[1];
			fRec44[1] = fRec44[0];
		}
	}

};


/***************************END USER SECTION ***************************/


char *_params_list(const char* options[], char **cache) {
    if(*cache != NULL)
        return *cache;
    int numoptions = nullterminated_chararray_len(options, 256);
    if(numoptions < 0)
        return NULL;
    char *out = join_strings(options, ", ", numoptions);
    *cache = out;
    return out;
}

// ---------------------- zitarev ------------------------
//
// a1, a2 zitarev ain1, ain2 [, Skey_n, kvalue_n, ...]
// a1, a2 zitarev a1, a2, "drywet", 0.5, "eq1freq", 1500, "hfdump", 6000

struct ZITAREV {
    OPDS h;
    MYFLT* aout[ZITAREV_OUTPUTS];
    MYFLT* ain[ZITAREV_INPUTS];
    void* ctrls[22];                      // alternate Stringparam, kparamvalue
    zitarev_dsp* DSP;                     //
    AUXCH     dspmem;                     // aux memory allocated once to store the DSP object
    int ctrlindexes[11];
    int numargs;
};


static const char *zitarev_params[] = {
    "eq2level",
    "eq2freq",
    "eq1level",
    "eq1freq",
    "decaymid",
    "hfdamp",
    "decaylfx",
    "decaylow",
    "delayms",
    "drywet",
    "level",
    NULL
};
char* _zitarev_params_list = NULL;


static int _key_to_index(char *key, const char **options) {
    const char *param;
    // max. number of options: 256
    for(int i = 0; i < 256; i++) {
        param = options[i];
        if(param == NULL)
            break;
        if(!strcmp(key, param))
            return i;
    }
    return -1;
}


static int zitarev_init(CSOUND *csound, ZITAREV *p) {
    if (p->dspmem.auxp == NULL)
        csound->AuxAlloc(csound, sizeof(zitarev_dsp), &p->dspmem);

    p->DSP = new (p->dspmem.auxp) zitarev_dsp;
    if (p->DSP == 0) return NOTOK;
    p->DSP->init((int)_GetLocalSr(csound, &(p->h)));

    int numargs = _GetInputArgCnt(csound, p) - 2;
    if(numargs % 2) {
        INITERRF("Expected even number of arguments, got %d\n", numargs);
        return NOTOK;
    }
    p->numargs = numargs;

    STRINGDAT *key;
    CS_TYPE *cstype;
    if(numargs > 0) {
        for(int i=0; i < numargs / 2; i++) {
            cstype = _GetTypeForArg(csound, p->ctrls[i*2]);
            if(cstype->varTypeName[0] != 'S') {
                INITERRF("Expected a string for arg %d, got %s\n", i + 2, cstype->varTypeName);
                return NOTOK;
            }
            key = (STRINGDAT*)(p->ctrls[i*2]);
            int paramindex = _key_to_index(key->data, zitarev_params);
            if(paramindex < 0) {
                INITERRF("Unknown parmeter %s. Possible parameters: %s", key->data, _params_list(zitarev_params, &_zitarev_params_list));
                return NOTOK;
            }
            p->ctrlindexes[i] = paramindex;
            cstype = _GetTypeForArg(csound, p->ctrls[i*2+1]);
            char typechar = cstype->varTypeName[0];
            if(typechar != 'c' && typechar != 'k' && typechar != 'i') {
                INITERRF("Value for key '%s' must be a scalar (a constant or an i- or k- var)"
                         ", got '%s'", key->data, cstype->varTypeName);
                return NOTOK;
            }
        }
    }
    return OK;
}


static int32_t zitarev_perf(CSOUND *csound, ZITAREV *p) {
    AVOIDDENORMALS;
    int numpairs = p->numargs / 2;
    zitarev_dsp *dsp = (zitarev_dsp *)(p->DSP);
    FAUSTFLOAT *slots = &(dsp->params[0]);
    for(int i = 0; i < numpairs; i++) {
        MYFLT value = *(MYFLT *)(p->ctrls[i * 2 + 1]);
        int index = p->ctrlindexes[i];
        slots[index] = value;
    }
    
    p->DSP->compute(_GetLocalKsmps(csound, &(p->h)), p->ain, p->aout);
    return OK;
}


// -------------------------------------------------------------

class fofcycle_dspSIG0 {

  private:

	int iVec0[2];
	int iRec0[2];

  public:

	int getNumInputs() {
		return 0;
	}
	int numOutputs() {
		return 1;
	}

	void instanceInit_dspSIG0(int sample_rate) {
		for (int l0 = 0; l0 < 2; l0 = l0 + 1) {
			iVec0[l0] = 0;
		}
		for (int l1 = 0; l1 < 2; l1 = l1 + 1) {
			iRec0[l1] = 0;
		}
	}

	void fill_dspSIG0(int count, float* table) {
		for (int i1 = 0; i1 < count; i1 = i1 + 1) {
			iVec0[0] = 1;
			iRec0[0] = (iRec0[1] + iVec0[1]) % 65536;
			table[i1] = std::sin(9.58738e-05f * float(iRec0[0]));
			iVec0[1] = iVec0[0];
			iRec0[1] = iRec0[0];
		}
	}

};

static fofcycle_dspSIG0* new_fofcycle_dspSIG0() { return (fofcycle_dspSIG0*)new fofcycle_dspSIG0(); }
static void delete_fofcycle_dspSIG0(fofcycle_dspSIG0* dsp) { delete dsp; }
static float fofcycle_ftbl0dspSIG0[65536];


static const char *fofcycle_params[] = {
    "vibfreq",
    "vibgain",
    "sustain",
    "bend",
    "vowel",
    "voicetype",
    "envattack",
    "outgain",
    NULL
};

char *_fofcycle_params_list = NULL;


#define    FOFCYCLE_VIBFREQ   0
#define    FOFCYCLE_VIBGAIN   1
#define    FOFCYCLE_SUSTAIN   2
#define    FOFCYCLE_BEND      3
#define    FOFCYCLE_VOWEL     4
#define    FOFCYCLE_VOICETYPE 5
#define    FOFCYCLE_ENVATTACK 6
#define    FOFCYCLE_OUTGAIN   7


class fofcycle_dsp : public dsp {
 public:
    FAUSTFLOAT param_gate;
    FAUSTFLOAT param_freq;
	FAUSTFLOAT param_gain;

    FAUSTFLOAT params[8];

    /*
    FAUSTFLOAT param_vibfreq;
	FAUSTFLOAT param_vibgain;
	FAUSTFLOAT param_sustain;
	FAUSTFLOAT param_bend;
	FAUSTFLOAT param_vowel;
	FAUSTFLOAT param_voicetype;
	FAUSTFLOAT param_envattack;
	FAUSTFLOAT param_outgain;
    */

    int fSampleRate;
	int iVec1[2];
	float fConst0;
	float fConst1;
	float fRec3[2];
	float fVec2[2];
	float fRec4[2];
	float fRec2[2];
	float fRec5[2];
	float fVec3[3];
	float fConst2;
	float fConst3;
	float fRec6[2];
	float fRec1[2];
	float fRec7[2];
	float fConst4;
	float fRec8[2];
	float fRec9[3];
	float fVec4[3];
	float fRec10[2];
	float fRec11[2];
	float fRec12[2];
	float fRec13[3];
	float fVec5[3];
	float fRec14[2];
	float fRec15[2];
	float fRec16[2];
	float fRec17[3];
	float fRec18[2];
	float fRec19[2];
	float fRec20[2];
	float fRec21[3];
	float fRec22[2];
	float fRec23[2];
	float fRec24[2];
	float fRec25[3];
	float fRec26[2];
	float fRec27[2];
	float fRec28[2];
	float fRec29[3];
	float fRec30[2];
	float fRec31[2];
	float fRec32[2];
	float fRec33[3];
	float fRec34[2];
	float fRec35[2];
	float fRec36[2];
	float fRec37[3];
	float fRec38[2];
	float fRec39[2];
	float fRec40[2];
	float fRec41[3];
	float fRec42[2];
	float fRec43[2];
	float fRec44[2];
	float fRec45[3];
	float fRec46[2];
	float fRec47[2];
	float fRec48[2];
	float fRec49[3];
	float fRec50[2];
	float fRec51[2];
	float fRec52[2];
	float fRec53[3];
	float fRec54[2];
	float fRec55[2];
	float fRec56[2];
	float fRec57[3];
	float fRec58[2];
	float fRec59[2];
	float fRec60[2];
	float fRec61[3];
	float fRec62[2];
	float fRec63[2];
	float fRec64[2];
	float fRec65[3];
	float fRec66[2];

 public:
	fofcycle_dsp() {
	}

	virtual int getNumInputs() { return 0; }
	virtual int getNumOutputs() { return 1; }

	static void classInit(int sample_rate) {
		fofcycle_dspSIG0* sig0 = new_fofcycle_dspSIG0();
		sig0->instanceInit_dspSIG0(sample_rate);
		sig0->fill_dspSIG0(65536, fofcycle_ftbl0dspSIG0);
		delete_fofcycle_dspSIG0(sig0);
	}

	virtual void instanceConstants(int sample_rate) {
		fSampleRate = sample_rate;
		fConst0 = std::min<float>(1.92e+05f, std::max<float>(1.0f, float(fSampleRate)));
		fConst1 = 1.0f / fConst0;
		fConst2 = 44.1f / fConst0;
		fConst3 = 1.0f - fConst2;
		fConst4 = 3.1415927f / fConst0;
	}

	virtual void instanceResetUserInterface() {
		param_gate = FAUSTFLOAT(0.0f);
		param_freq = 440;
		param_gain = FAUSTFLOAT(0.9f);

		params[FOFCYCLE_VIBFREQ] = 6.0f;
		params[FOFCYCLE_VIBGAIN] = 0.5f;
		params[FOFCYCLE_SUSTAIN] = 0.f;
		params[FOFCYCLE_BEND] = 0.f;
		params[FOFCYCLE_VOWEL] = 0.;
		params[FOFCYCLE_VOICETYPE] = 0.;
		params[FOFCYCLE_ENVATTACK] = 10;
		params[FOFCYCLE_OUTGAIN] = 0.5;
	}

	virtual void instanceClear() {
		for (int l2 = 0; l2 < 2; l2 = l2 + 1) {
			iVec1[l2] = 0;
		}
		for (int l3 = 0; l3 < 2; l3 = l3 + 1) {
			fRec3[l3] = 0.0f;
		}
		for (int l4 = 0; l4 < 2; l4 = l4 + 1) {
			fVec2[l4] = 0.0f;
		}
		for (int l5 = 0; l5 < 2; l5 = l5 + 1) {
			fRec4[l5] = 0.0f;
		}
		for (int l6 = 0; l6 < 2; l6 = l6 + 1) {
			fRec2[l6] = 0.0f;
		}
		for (int l7 = 0; l7 < 2; l7 = l7 + 1) {
			fRec5[l7] = 0.0f;
		}
		for (int l8 = 0; l8 < 3; l8 = l8 + 1) {
			fVec3[l8] = 0.0f;
		}
		for (int l9 = 0; l9 < 2; l9 = l9 + 1) {
			fRec6[l9] = 0.0f;
		}
		for (int l10 = 0; l10 < 2; l10 = l10 + 1) {
			fRec1[l10] = 0.0f;
		}
		for (int l11 = 0; l11 < 2; l11 = l11 + 1) {
			fRec7[l11] = 0.0f;
		}
		for (int l12 = 0; l12 < 2; l12 = l12 + 1) {
			fRec8[l12] = 0.0f;
		}
		for (int l13 = 0; l13 < 3; l13 = l13 + 1) {
			fRec9[l13] = 0.0f;
		}
		for (int l14 = 0; l14 < 3; l14 = l14 + 1) {
			fVec4[l14] = 0.0f;
		}
		for (int l15 = 0; l15 < 2; l15 = l15 + 1) {
			fRec10[l15] = 0.0f;
		}
		for (int l16 = 0; l16 < 2; l16 = l16 + 1) {
			fRec11[l16] = 0.0f;
		}
		for (int l17 = 0; l17 < 2; l17 = l17 + 1) {
			fRec12[l17] = 0.0f;
		}
		for (int l18 = 0; l18 < 3; l18 = l18 + 1) {
			fRec13[l18] = 0.0f;
		}
		for (int l19 = 0; l19 < 3; l19 = l19 + 1) {
			fVec5[l19] = 0.0f;
		}
		for (int l20 = 0; l20 < 2; l20 = l20 + 1) {
			fRec14[l20] = 0.0f;
		}
		for (int l21 = 0; l21 < 2; l21 = l21 + 1) {
			fRec15[l21] = 0.0f;
		}
		for (int l22 = 0; l22 < 2; l22 = l22 + 1) {
			fRec16[l22] = 0.0f;
		}
		for (int l23 = 0; l23 < 3; l23 = l23 + 1) {
			fRec17[l23] = 0.0f;
		}
		for (int l24 = 0; l24 < 2; l24 = l24 + 1) {
			fRec18[l24] = 0.0f;
		}
		for (int l25 = 0; l25 < 2; l25 = l25 + 1) {
			fRec19[l25] = 0.0f;
		}
		for (int l26 = 0; l26 < 2; l26 = l26 + 1) {
			fRec20[l26] = 0.0f;
		}
		for (int l27 = 0; l27 < 3; l27 = l27 + 1) {
			fRec21[l27] = 0.0f;
		}
		for (int l28 = 0; l28 < 2; l28 = l28 + 1) {
			fRec22[l28] = 0.0f;
		}
		for (int l29 = 0; l29 < 2; l29 = l29 + 1) {
			fRec23[l29] = 0.0f;
		}
		for (int l30 = 0; l30 < 2; l30 = l30 + 1) {
			fRec24[l30] = 0.0f;
		}
		for (int l31 = 0; l31 < 3; l31 = l31 + 1) {
			fRec25[l31] = 0.0f;
		}
		for (int l32 = 0; l32 < 2; l32 = l32 + 1) {
			fRec26[l32] = 0.0f;
		}
		for (int l33 = 0; l33 < 2; l33 = l33 + 1) {
			fRec27[l33] = 0.0f;
		}
		for (int l34 = 0; l34 < 2; l34 = l34 + 1) {
			fRec28[l34] = 0.0f;
		}
		for (int l35 = 0; l35 < 3; l35 = l35 + 1) {
			fRec29[l35] = 0.0f;
		}
		for (int l36 = 0; l36 < 2; l36 = l36 + 1) {
			fRec30[l36] = 0.0f;
		}
		for (int l37 = 0; l37 < 2; l37 = l37 + 1) {
			fRec31[l37] = 0.0f;
		}
		for (int l38 = 0; l38 < 2; l38 = l38 + 1) {
			fRec32[l38] = 0.0f;
		}
		for (int l39 = 0; l39 < 3; l39 = l39 + 1) {
			fRec33[l39] = 0.0f;
		}
		for (int l40 = 0; l40 < 2; l40 = l40 + 1) {
			fRec34[l40] = 0.0f;
		}
		for (int l41 = 0; l41 < 2; l41 = l41 + 1) {
			fRec35[l41] = 0.0f;
		}
		for (int l42 = 0; l42 < 2; l42 = l42 + 1) {
			fRec36[l42] = 0.0f;
		}
		for (int l43 = 0; l43 < 3; l43 = l43 + 1) {
			fRec37[l43] = 0.0f;
		}
		for (int l44 = 0; l44 < 2; l44 = l44 + 1) {
			fRec38[l44] = 0.0f;
		}
		for (int l45 = 0; l45 < 2; l45 = l45 + 1) {
			fRec39[l45] = 0.0f;
		}
		for (int l46 = 0; l46 < 2; l46 = l46 + 1) {
			fRec40[l46] = 0.0f;
		}
		for (int l47 = 0; l47 < 3; l47 = l47 + 1) {
			fRec41[l47] = 0.0f;
		}
		for (int l48 = 0; l48 < 2; l48 = l48 + 1) {
			fRec42[l48] = 0.0f;
		}
		for (int l49 = 0; l49 < 2; l49 = l49 + 1) {
			fRec43[l49] = 0.0f;
		}
		for (int l50 = 0; l50 < 2; l50 = l50 + 1) {
			fRec44[l50] = 0.0f;
		}
		for (int l51 = 0; l51 < 3; l51 = l51 + 1) {
			fRec45[l51] = 0.0f;
		}
		for (int l52 = 0; l52 < 2; l52 = l52 + 1) {
			fRec46[l52] = 0.0f;
		}
		for (int l53 = 0; l53 < 2; l53 = l53 + 1) {
			fRec47[l53] = 0.0f;
		}
		for (int l54 = 0; l54 < 2; l54 = l54 + 1) {
			fRec48[l54] = 0.0f;
		}
		for (int l55 = 0; l55 < 3; l55 = l55 + 1) {
			fRec49[l55] = 0.0f;
		}
		for (int l56 = 0; l56 < 2; l56 = l56 + 1) {
			fRec50[l56] = 0.0f;
		}
		for (int l57 = 0; l57 < 2; l57 = l57 + 1) {
			fRec51[l57] = 0.0f;
		}
		for (int l58 = 0; l58 < 2; l58 = l58 + 1) {
			fRec52[l58] = 0.0f;
		}
		for (int l59 = 0; l59 < 3; l59 = l59 + 1) {
			fRec53[l59] = 0.0f;
		}
		for (int l60 = 0; l60 < 2; l60 = l60 + 1) {
			fRec54[l60] = 0.0f;
		}
		for (int l61 = 0; l61 < 2; l61 = l61 + 1) {
			fRec55[l61] = 0.0f;
		}
		for (int l62 = 0; l62 < 2; l62 = l62 + 1) {
			fRec56[l62] = 0.0f;
		}
		for (int l63 = 0; l63 < 3; l63 = l63 + 1) {
			fRec57[l63] = 0.0f;
		}
		for (int l64 = 0; l64 < 2; l64 = l64 + 1) {
			fRec58[l64] = 0.0f;
		}
		for (int l65 = 0; l65 < 2; l65 = l65 + 1) {
			fRec59[l65] = 0.0f;
		}
		for (int l66 = 0; l66 < 2; l66 = l66 + 1) {
			fRec60[l66] = 0.0f;
		}
		for (int l67 = 0; l67 < 3; l67 = l67 + 1) {
			fRec61[l67] = 0.0f;
		}
		for (int l68 = 0; l68 < 2; l68 = l68 + 1) {
			fRec62[l68] = 0.0f;
		}
		for (int l69 = 0; l69 < 2; l69 = l69 + 1) {
			fRec63[l69] = 0.0f;
		}
		for (int l70 = 0; l70 < 2; l70 = l70 + 1) {
			fRec64[l70] = 0.0f;
		}
		for (int l71 = 0; l71 < 3; l71 = l71 + 1) {
			fRec65[l71] = 0.0f;
		}
		for (int l72 = 0; l72 < 2; l72 = l72 + 1) {
			fRec66[l72] = 0.0f;
		}
	}

	virtual void init(int sample_rate) {
		classInit(sample_rate);
		instanceInit(sample_rate);
	}

	virtual void instanceInit(int sample_rate) {
		instanceConstants(sample_rate);
		instanceResetUserInterface();
		instanceClear();
	}

	virtual fofcycle_dsp* clone() {
		return new fofcycle_dsp();
	}

	virtual int getSampleRate() {
		return fSampleRate;
	}

	virtual void buildUserInterface(UI* ui_interface) { IGN(ui_interface); }

	virtual void compute(int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
		IGN(inputs);
		FAUSTFLOAT* output0 = outputs[0];
		float fSlow0 = fConst1 * float(params[FOFCYCLE_VIBFREQ]);
		float fSlow1 = 0.1f * float(params[FOFCYCLE_VIBGAIN]);
		float fSlow2 = std::min<float>(1.0f, float(params[FOFCYCLE_SUSTAIN]) + float(param_gate));
		int iSlow3 = fSlow2 == 0.0f;
		float fSlow4 = std::pow(2.0f, 0.083333336f * float(params[FOFCYCLE_BEND]));
		float fSlow5 = float(param_freq);
		float fSlow6 = fConst1 * fSlow5;
		float fSlow7 = fConst2 * float(params[FOFCYCLE_VOWEL]);
		float fSlow8 = float(params[FOFCYCLE_VOICETYPE]);
		float fSlow9 = 5.0f * fSlow8;
		float fSlow10 = 5.0f * (1.0f - fSlow8);
		int iSlow11 = ((fSlow8 == 0.0f) ? 1 : ((fSlow8 == 3.0f) ? 1 : 0));
		int iSlow12 = iSlow11 >= 1;
		float fSlow13 = ((iSlow12) ? 174.61f : 82.41f);
		float fSlow14 = ((iSlow12) ? 1046.5f : 523.25f);
		float fSlow15 = fSlow14 - fSlow13;
		float fSlow16 = float(5 * iSlow11);
		int iSlow17 = iSlow11 == 0;
		int iSlow18 = fSlow8 != 2.0f;
		float fSlow19 = 2.0f * fSlow5;
		float fSlow20 = 0.001f * float(params[FOFCYCLE_ENVATTACK]);
		int iSlow21 = std::fabs(fSlow20) < 1.1920929e-07f;
		float fSlow22 = ((iSlow21) ? 0.0f : std::exp(-(fConst1 / ((iSlow21) ? 1.0f : fSlow20))));
		float fSlow23 = 75.0f * fSlow2 * float(param_gain) * (1.0f - fSlow22);
		float fSlow24 = float(params[FOFCYCLE_OUTGAIN]);
		for (int i0 = 0; i0 < count; i0 = i0 + 1) {
			iVec1[0] = 1;
			int iTemp0 = 1 - iVec1[1];
			float fTemp1 = ((iTemp0) ? 0.0f : fSlow0 + fRec3[1]);
			fRec3[0] = fTemp1 - std::floor(fTemp1);
			fVec2[0] = fSlow2;
			float fTemp2 = float((fSlow2 == fVec2[1]) | iSlow3);
			fRec4[0] = fSlow4 * (1.0f - 0.999f * fTemp2) + 0.999f * fTemp2 * fRec4[1];
			float fTemp3 = fRec4[0] * (fSlow1 * fofcycle_ftbl0dspSIG0[std::max<int>(0, std::min<int>(int(65536.0f * fRec3[0]), 65535))] + 1.0f);
			float fTemp4 = ((iTemp0) ? 0.0f : fRec2[1] + fSlow6 * fTemp3);
			fRec2[0] = fTemp4 - std::floor(fTemp4);
			float fTemp5 = float((fRec2[0] - fRec2[1]) < 0.0f);
			fRec5[0] = fTemp5 + fRec5[1] * float(fRec5[1] <= 2.0f);
			float fTemp6 = float(fRec5[0] == 3.0f) * fTemp5;
			fVec3[0] = fTemp6;
			int iTemp7 = iTemp0 | int(fVec3[1]);
			fRec6[0] = fSlow7 + fConst3 * fRec6[1];
			float fTemp8 = fSlow9 + fRec6[0];
			int iTemp9 = fTemp8 < 23.0f;
			int iTemp10 = fTemp8 < 24.0f;
			float fTemp11 = fSlow9 + fRec6[0] + -23.0f;
			int iTemp12 = fTemp8 < 22.0f;
			float fTemp13 = fSlow9 + fRec6[0] + -22.0f;
			int iTemp14 = fTemp8 < 21.0f;
			float fTemp15 = fSlow9 + fRec6[0] + -21.0f;
			int iTemp16 = fTemp8 < 2e+01f;
			float fTemp17 = fSlow9 + fRec6[0] + -2e+01f;
			int iTemp18 = fTemp8 < 19.0f;
			float fTemp19 = fSlow9 + fRec6[0] + -19.0f;
			int iTemp20 = fTemp8 < 18.0f;
			int iTemp21 = fTemp8 < 17.0f;
			int iTemp22 = fTemp8 < 16.0f;
			int iTemp23 = fTemp8 < 15.0f;
			int iTemp24 = fTemp8 < 14.0f;
			float fTemp25 = fSlow9 + fRec6[0] + -14.0f;
			int iTemp26 = fTemp8 < 13.0f;
			float fTemp27 = fSlow9 + fRec6[0] + -13.0f;
			int iTemp28 = fTemp8 < 12.0f;
			float fTemp29 = fSlow9 + fRec6[0] + -12.0f;
			int iTemp30 = fTemp8 < 11.0f;
			float fTemp31 = fSlow9 + fRec6[0] + -11.0f;
			int iTemp32 = fTemp8 < 1e+01f;
			float fTemp33 = fSlow9 + fRec6[0] + -1e+01f;
			float fTemp34 = 5e+01f * fTemp33;
			int iTemp35 = fTemp8 < 9.0f;
			float fTemp36 = fSlow9 + fRec6[0] + -9.0f;
			int iTemp37 = fTemp8 < 8.0f;
			float fTemp38 = fSlow9 + fRec6[0] + -8.0f;
			float fTemp39 = 5e+01f * fTemp38;
			int iTemp40 = fTemp8 < 7.0f;
			float fTemp41 = fSlow9 + fRec6[0] + -7.0f;
			int iTemp42 = fTemp8 < 6.0f;
			float fTemp43 = fSlow9 + fRec6[0] + -6.0f;
			int iTemp44 = fTemp8 < 5.0f;
			float fTemp45 = fRec6[0] - fSlow10;
			float fTemp46 = 3.5e+02f * fTemp45;
			int iTemp47 = fTemp8 < 4.0f;
			float fTemp48 = fSlow9 + fRec6[0] + -4.0f;
			float fTemp49 = fConst1 * ((iTemp9) ? ((iTemp12) ? ((iTemp14) ? ((iTemp16) ? ((iTemp18) ? ((iTemp20) ? ((iTemp21) ? ((iTemp22) ? ((iTemp23) ? ((iTemp24) ? ((iTemp26) ? ((iTemp28) ? ((iTemp30) ? ((iTemp32) ? ((iTemp35) ? ((iTemp37) ? ((iTemp40) ? ((iTemp42) ? ((iTemp44) ? ((iTemp47) ? 4.95e+03f : ((iTemp44) ? 4.95e+03f - 2.2e+03f * fTemp48 : 2.75e+03f)) : ((iTemp42) ? fTemp46 + 2.75e+03f : 3.1e+03f)) : ((iTemp40) ? 2.4e+02f * fTemp43 + 3.1e+03f : 3.34e+03f)) : ((iTemp37) ? 3.34e+03f - 4.4e+02f * fTemp41 : 2.9e+03f)) : ((iTemp35) ? fTemp39 + 2.9e+03f : 2.95e+03f)) : ((iTemp32) ? 4e+02f * fTemp36 + 2.95e+03f : 3.35e+03f)) : ((iTemp30) ? 3.35e+03f - fTemp34 : 3.3e+03f)) : ((iTemp28) ? 2.9e+02f * fTemp31 + 3.3e+03f : 3.59e+03f)) : ((iTemp26) ? 3.59e+03f - 2.9e+02f * fTemp29 : 3.3e+03f)) : ((iTemp24) ? 1e+02f * fTemp27 + 3.3e+03f : 3.4e+03f)) : ((iTemp23) ? 1.55e+03f * fTemp25 + 3.4e+03f : 4.95e+03f)) : 4.95e+03f) : 4.95e+03f) : 4.95e+03f) : 4.95e+03f) : ((iTemp16) ? 4.95e+03f - 1.7e+03f * fTemp19 : 3.25e+03f)) : ((iTemp14) ? 3.3e+02f * fTemp17 + 3.25e+03f : 3.58e+03f)) : ((iTemp12) ? 3.58e+03f - 4e+01f * fTemp15 : 3.54e+03f)) : ((iTemp9) ? 3.54e+03f - 5.4e+02f * fTemp13 : 3e+03f)) : ((iTemp10) ? 3e+02f * fTemp11 + 3e+03f : 3.3e+03f));
			float fTemp50 = ((iTemp7) ? 0.0f : fTemp49 + fRec1[1]);
			fRec1[0] = fTemp50 - std::floor(fTemp50);
			int iTemp51 = int(fTemp6);
			float fTemp52 = 2e+01f * fTemp17;
			float fTemp53 = fSlow9 + fRec6[0] + -18.0f;
			float fTemp54 = fSlow9 + fRec6[0] + -16.0f;
			float fTemp55 = 8e+01f * fTemp54;
			float fTemp56 = fSlow9 + fRec6[0] + -15.0f;
			float fTemp57 = 2e+01f * fTemp25;
			float fTemp58 = 2e+01f * fTemp33;
			float fTemp59 = 2e+01f * fTemp36;
			float fTemp60 = fSlow10 - fRec6[0];
			float fTemp61 = 1e+01f * fTemp60;
			int iTemp62 = fTemp8 < 3.0f;
			float fTemp63 = fSlow9 + fRec6[0] + -3.0f;
			int iTemp64 = fTemp8 < 2.0f;
			float fTemp65 = fSlow9 + fRec6[0] + -2.0f;
			int iTemp66 = fTemp8 < 1.0f;
			int iTemp67 = fTemp8 < 0.0f;
			float fTemp68 = ((iTemp9) ? ((iTemp12) ? ((iTemp14) ? ((iTemp16) ? ((iTemp18) ? ((iTemp20) ? ((iTemp21) ? ((iTemp22) ? ((iTemp23) ? ((iTemp24) ? ((iTemp26) ? ((iTemp28) ? ((iTemp30) ? ((iTemp32) ? ((iTemp35) ? ((iTemp37) ? ((iTemp40) ? ((iTemp42) ? ((iTemp44) ? ((iTemp47) ? ((iTemp62) ? ((iTemp64) ? ((iTemp66) ? ((iTemp67) ? 1.4e+02f : ((iTemp66) ? 6e+01f * fTemp8 + 1.4e+02f : 2e+02f)) : 2e+02f) : ((iTemp62) ? 2e+02f - 65.0f * fTemp65 : 135.0f)) : ((iTemp47) ? 65.0f * fTemp63 + 135.0f : 2e+02f)) : ((iTemp44) ? 2e+02f - 7e+01f * fTemp48 : 1.3e+02f)) : ((iTemp42) ? fTemp61 + 1.3e+02f : 1.2e+02f)) : 1.2e+02f) : 1.2e+02f) : 1.2e+02f) : ((iTemp32) ? fTemp59 + 1.2e+02f : 1.4e+02f)) : ((iTemp30) ? 1.4e+02f - fTemp58 : 1.2e+02f)) : 1.2e+02f) : 1.2e+02f) : 1.2e+02f) : ((iTemp23) ? fTemp57 + 1.2e+02f : 1.4e+02f)) : ((iTemp22) ? 6e+01f * fTemp56 + 1.4e+02f : 2e+02f)) : ((iTemp21) ? 2e+02f - fTemp55 : 1.2e+02f)) : 1.2e+02f) : ((iTemp18) ? 8e+01f * fTemp53 + 1.2e+02f : 2e+02f)) : ((iTemp16) ? 2e+02f - 6e+01f * fTemp19 : 1.4e+02f)) : ((iTemp14) ? 1.4e+02f - fTemp52 : 1.2e+02f)) : 1.2e+02f) : ((iTemp9) ? 15.0f * fTemp13 + 1.2e+02f : 135.0f)) : ((iTemp10) ? 135.0f - 15.0f * fTemp11 : 1.2e+02f));
			fRec7[0] = ((iTemp51) ? fTemp68 : fRec7[1]);
			float fTemp69 = expf(-(fConst4 * fRec7[0]));
			float fTemp70 = fSlow5 * fTemp3;
			float fTemp71 = fRec6[0] + fSlow16;
			int iTemp72 = fTemp71 >= 5.0f;
			int iTemp73 = fTemp71 >= 3.0f;
			int iTemp74 = fTemp71 >= 2.0f;
			int iTemp75 = fTemp71 >= 1.0f;
			int iTemp76 = fTemp71 >= 4.0f;
			int iTemp77 = fTemp71 >= 8.0f;
			int iTemp78 = fTemp71 >= 7.0f;
			int iTemp79 = fTemp71 >= 6.0f;
			float fTemp80 = ((iTemp72) ? ((iTemp77) ? 2.0f : ((iTemp78) ? 3.0f : ((iTemp79) ? 3.0f : 2.0f))) : ((iTemp73) ? ((iTemp76) ? 1.5f : 1.0f) : ((iTemp74) ? 1.25f : ((iTemp75) ? 1.25f : 1.0f))));
			float fTemp81 = fTemp80 + (((iTemp72) ? ((iTemp77) ? 12.0f : ((iTemp78) ? 12.0f : ((iTemp79) ? 12.0f : 15.0f))) : ((iTemp73) ? ((iTemp76) ? 4.0f : 1e+01f) : ((iTemp74) ? 2.5f : ((iTemp75) ? 2.5f : 1e+01f)))) - fTemp80) * ((fTemp70 <= fSlow13) ? 0.0f : ((fTemp70 >= fSlow14) ? 1.0f : (fTemp70 - fSlow13) / fSlow15));
			float fTemp82 = fTemp81 * fTemp68;
			fRec8[0] = ((iTemp51) ? fTemp82 : fRec8[1]);
			float fTemp83 = expf(-(fConst4 * fRec8[0]));
			fRec9[0] = fVec3[2] + fRec9[1] * (fTemp83 + fTemp69) - fTemp83 * fTemp69 * fRec9[2];
			float fTemp84 = float(fRec5[0] == 2.0f) * fTemp5;
			fVec4[0] = fTemp84;
			int iTemp85 = iTemp0 | int(fVec4[1]);
			float fTemp86 = ((iTemp85) ? 0.0f : fTemp49 + fRec10[1]);
			fRec10[0] = fTemp86 - std::floor(fTemp86);
			int iTemp87 = int(fTemp84);
			fRec11[0] = ((iTemp87) ? fTemp68 : fRec11[1]);
			float fTemp88 = expf(-(fConst4 * fRec11[0]));
			fRec12[0] = ((iTemp87) ? fTemp82 : fRec12[1]);
			float fTemp89 = expf(-(fConst4 * fRec12[0]));
			fRec13[0] = fVec4[2] + fRec13[1] * (fTemp89 + fTemp88) - fTemp89 * fTemp88 * fRec13[2];
			float fTemp90 = float(fRec5[0] == 1.0f) * fTemp5;
			fVec5[0] = fTemp90;
			int iTemp91 = iTemp0 | int(fVec5[1]);
			float fTemp92 = ((iTemp91) ? 0.0f : fRec14[1] + fTemp49);
			fRec14[0] = fTemp92 - std::floor(fTemp92);
			int iTemp93 = int(fTemp90);
			fRec15[0] = ((iTemp93) ? fTemp68 : fRec15[1]);
			float fTemp94 = expf(-(fConst4 * fRec15[0]));
			fRec16[0] = ((iTemp93) ? fTemp82 : fRec16[1]);
			float fTemp95 = expf(-(fConst4 * fRec16[0]));
			fRec17[0] = fVec5[2] + fRec17[1] * (fTemp95 + fTemp94) - fTemp95 * fTemp94 * fRec17[2];
			float fTemp96 = fSlow9 + fRec6[0] + -17.0f;
			float fTemp97 = ((iTemp28) ? 0.1f - 0.084151f * fTemp31 : 0.015849f);
			float fTemp98 = ((iTemp9) ? ((iTemp12) ? ((iTemp14) ? ((iTemp16) ? ((iTemp18) ? ((iTemp20) ? ((iTemp21) ? ((iTemp22) ? ((iTemp23) ? ((iTemp24) ? ((iTemp26) ? ((iTemp28) ? ((iTemp30) ? ((iTemp32) ? ((iTemp35) ? ((iTemp37) ? ((iTemp40) ? ((iTemp42) ? ((iTemp44) ? ((iTemp47) ? ((iTemp62) ? ((iTemp64) ? 0.001f : ((iTemp62) ? 0.000778f * fTemp65 + 0.001f : 0.001778f)) : ((iTemp47) ? 0.001778f - 0.001147f * fTemp63 : 0.000631f)) : ((iTemp44) ? 0.099369f * fTemp48 + 0.000631f : 0.1f)) : ((iTemp42) ? 0.025893f * fTemp45 + 0.1f : 0.125893f)) : ((iTemp40) ? 0.125893f - 0.086082f * fTemp43 : 0.039811f)) : ((iTemp37) ? 0.039811f - 0.029811f * fTemp41 : 0.01f)) : ((iTemp35) ? 0.005849f * fTemp38 + 0.01f : 0.015849f)) : ((iTemp32) ? 0.015849f - 0.00326f * fTemp36 : 0.012589f)) : ((iTemp30) ? 0.087411f * fTemp33 + 0.012589f : 0.1f)) : fTemp97) : ((iTemp26) ? 0.004104f * fTemp29 + 0.015849f : 0.019953f)) : 0.019953f) : ((iTemp23) ? 0.019953f - 0.016791f * fTemp25 : 0.003162f)) : ((iTemp22) ? 0.003162f - 0.001577f * fTemp56 : 0.001585f)) : ((iTemp21) ? 0.004725f * fTemp54 + 0.001585f : 0.00631f)) : ((iTemp20) ? 0.00631f - 0.003148f * fTemp96 : 0.003162f)) : ((iTemp18) ? 0.003162f - 0.002162f * fTemp53 : 0.001f)) : ((iTemp16) ? 0.078433f * fTemp19 + 0.001f : 0.079433f)) : ((iTemp14) ? 0.020567f * fTemp17 + 0.079433f : 0.1f)) : ((iTemp12) ? 0.1f - 0.068377f * fTemp15 : 0.031623f)) : ((iTemp9) ? 0.018496f * fTemp13 + 0.031623f : 0.050119f)) : 0.050119f);
			float fTemp99 = 0.00084f * (1e+03f - fTemp70) + 0.8f;
			float fTemp100 = 0.0036666666f * (4e+02f - fTemp70) + 3.0f;
			float fTemp101 = 1e+02f * fTemp11;
			float fTemp102 = fSlow9 + fRec6[0] + -1.0f;
			float fTemp103 = fConst1 * ((iTemp9) ? ((iTemp12) ? ((iTemp14) ? ((iTemp16) ? ((iTemp18) ? ((iTemp20) ? ((iTemp21) ? ((iTemp22) ? ((iTemp23) ? ((iTemp24) ? ((iTemp26) ? ((iTemp28) ? ((iTemp30) ? ((iTemp32) ? ((iTemp35) ? ((iTemp37) ? ((iTemp40) ? ((iTemp42) ? ((iTemp44) ? ((iTemp47) ? ((iTemp62) ? ((iTemp64) ? ((iTemp66) ? ((iTemp67) ? 3.5e+03f : ((iTemp66) ? 3.5e+03f - 2e+02f * fTemp8 : 3.3e+03f)) : ((iTemp64) ? 4e+02f * fTemp102 + 3.3e+03f : 3.7e+03f)) : ((iTemp62) ? 3.7e+03f - 2e+02f * fTemp65 : 3.5e+03f)) : 3.5e+03f) : ((iTemp44) ? 3.5e+03f - 1.05e+03f * fTemp48 : 2.45e+03f)) : ((iTemp42) ? fTemp46 + 2.45e+03f : 2.8e+03f)) : ((iTemp40) ? 2.5e+02f * fTemp43 + 2.8e+03f : 3.05e+03f)) : ((iTemp37) ? 3.05e+03f - 4.5e+02f * fTemp41 : 2.6e+03f)) : ((iTemp35) ? 75.0f * fTemp38 + 2.6e+03f : 2675.0f)) : ((iTemp32) ? 325.0f * fTemp36 + 2675.0f : 3e+03f)) : 3e+03f) : ((iTemp28) ? 3.5e+02f * fTemp31 + 3e+03f : 3.35e+03f)) : ((iTemp26) ? 3.35e+03f - 3.5e+02f * fTemp29 : 3e+03f)) : 3e+03f) : ((iTemp23) ? 9e+02f * fTemp25 + 3e+03f : 3.9e+03f)) : ((iTemp22) ? 3.9e+03f - 3e+02f * fTemp56 : 3.6e+03f)) : ((iTemp21) ? 3e+02f * fTemp54 + 3.6e+03f : 3.9e+03f)) : ((iTemp20) ? 3.9e+03f - 1e+02f * fTemp96 : 3.8e+03f)) : 3.8e+03f) : ((iTemp16) ? 3.8e+03f - 9e+02f * fTemp19 : 2.9e+03f)) : ((iTemp14) ? 3e+02f * fTemp17 + 2.9e+03f : 3.2e+03f)) : ((iTemp12) ? 5e+01f * fTemp15 + 3.2e+03f : 3.25e+03f)) : ((iTemp9) ? 3.25e+03f - 4.5e+02f * fTemp13 : 2.8e+03f)) : ((iTemp10) ? fTemp101 + 2.8e+03f : 2.9e+03f));
			float fTemp104 = ((iTemp7) ? 0.0f : fTemp103 + fRec18[1]);
			fRec18[0] = fTemp104 - std::floor(fTemp104);
			float fTemp105 = 1e+01f * fTemp13;
			float fTemp106 = 1e+01f * fTemp17;
			float fTemp107 = 5e+01f * fTemp19;
			float fTemp108 = 2e+01f * fTemp56;
			float fTemp109 = 1e+01f * fTemp33;
			float fTemp110 = 1e+01f * fTemp36;
			float fTemp111 = 6e+01f * fTemp48;
			float fTemp112 = 2e+01f * fTemp65;
			float fTemp113 = 2e+01f * fTemp8;
			float fTemp114 = ((iTemp9) ? ((iTemp12) ? ((iTemp14) ? ((iTemp16) ? ((iTemp18) ? ((iTemp20) ? ((iTemp21) ? ((iTemp22) ? ((iTemp23) ? ((iTemp24) ? ((iTemp26) ? ((iTemp28) ? ((iTemp30) ? ((iTemp32) ? ((iTemp35) ? ((iTemp37) ? ((iTemp40) ? ((iTemp42) ? ((iTemp44) ? ((iTemp47) ? ((iTemp62) ? ((iTemp64) ? ((iTemp66) ? ((iTemp67) ? 1.3e+02f : ((iTemp66) ? fTemp113 + 1.3e+02f : 1.5e+02f)) : 1.5e+02f) : ((iTemp62) ? 1.5e+02f - fTemp112 : 1.3e+02f)) : ((iTemp47) ? 5e+01f * fTemp63 + 1.3e+02f : 1.8e+02f)) : ((iTemp44) ? 1.8e+02f - fTemp111 : 1.2e+02f)) : 1.2e+02f) : 1.2e+02f) : 1.2e+02f) : 1.2e+02f) : ((iTemp32) ? fTemp110 + 1.2e+02f : 1.3e+02f)) : ((iTemp30) ? 1.3e+02f - fTemp109 : 1.2e+02f)) : 1.2e+02f) : 1.2e+02f) : 1.2e+02f) : ((iTemp23) ? 1e+01f * fTemp25 + 1.2e+02f : 1.3e+02f)) : ((iTemp22) ? fTemp108 + 1.3e+02f : 1.5e+02f)) : ((iTemp21) ? 1.5e+02f - 3e+01f * fTemp54 : 1.2e+02f)) : 1.2e+02f) : ((iTemp18) ? 6e+01f * fTemp53 + 1.2e+02f : 1.8e+02f)) : ((iTemp16) ? 1.8e+02f - fTemp107 : 1.3e+02f)) : ((iTemp14) ? 1.3e+02f - fTemp106 : 1.2e+02f)) : 1.2e+02f) : ((iTemp9) ? fTemp105 + 1.2e+02f : 1.3e+02f)) : ((iTemp10) ? 1.3e+02f - 1e+01f * fTemp11 : 1.2e+02f));
			fRec19[0] = ((iTemp51) ? fTemp114 : fRec19[1]);
			float fTemp115 = expf(-(fConst4 * fRec19[0]));
			float fTemp116 = fTemp81 * fTemp114;
			fRec20[0] = ((iTemp51) ? fTemp116 : fRec20[1]);
			float fTemp117 = expf(-(fConst4 * fRec20[0]));
			fRec21[0] = fVec3[2] + fRec21[1] * (fTemp117 + fTemp115) - fTemp117 * fTemp115 * fRec21[2];
			float fTemp118 = ((iTemp85) ? 0.0f : fTemp103 + fRec22[1]);
			fRec22[0] = fTemp118 - std::floor(fTemp118);
			fRec23[0] = ((iTemp87) ? fTemp114 : fRec23[1]);
			float fTemp119 = expf(-(fConst4 * fRec23[0]));
			fRec24[0] = ((iTemp87) ? fTemp116 : fRec24[1]);
			float fTemp120 = expf(-(fConst4 * fRec24[0]));
			fRec25[0] = fVec4[2] + fRec25[1] * (fTemp120 + fTemp119) - fTemp120 * fTemp119 * fRec25[2];
			float fTemp121 = ((iTemp91) ? 0.0f : fRec26[1] + fTemp103);
			fRec26[0] = fTemp121 - std::floor(fTemp121);
			fRec27[0] = ((iTemp93) ? fTemp114 : fRec27[1]);
			float fTemp122 = expf(-(fConst4 * fRec27[0]));
			fRec28[0] = ((iTemp93) ? fTemp116 : fRec28[1]);
			float fTemp123 = expf(-(fConst4 * fRec28[0]));
			fRec29[0] = fVec5[2] + fRec29[1] * (fTemp123 + fTemp122) - fTemp123 * fTemp122 * fRec29[2];
			float fTemp124 = ((iTemp20) ? 0.029314f * fTemp96 + 0.050119f : 0.079433f);
			float fTemp125 = ((iTemp9) ? ((iTemp12) ? ((iTemp14) ? ((iTemp16) ? ((iTemp18) ? ((iTemp20) ? ((iTemp21) ? ((iTemp22) ? ((iTemp23) ? ((iTemp24) ? ((iTemp26) ? ((iTemp28) ? ((iTemp30) ? ((iTemp32) ? ((iTemp35) ? ((iTemp37) ? ((iTemp40) ? ((iTemp42) ? ((iTemp44) ? ((iTemp47) ? ((iTemp62) ? ((iTemp64) ? ((iTemp66) ? ((iTemp67) ? 0.015849f : ((iTemp66) ? 0.001934f * fTemp8 + 0.015849f : 0.017783f)) : ((iTemp64) ? 0.017783f - 0.001934f * fTemp102 : 0.015849f)) : ((iTemp62) ? 0.023962f * fTemp65 + 0.015849f : 0.039811f)) : ((iTemp47) ? 0.039811f - 0.029811f * fTemp63 : 0.01f)) : ((iTemp44) ? 0.344813f * fTemp48 + 0.01f : 0.354813f)) : ((iTemp42) ? 0.103624f * fTemp60 + 0.354813f : 0.251189f)) : ((iTemp40) ? 0.251189f - 0.171756f * fTemp43 : 0.079433f)) : ((iTemp37) ? 0.020567f * fTemp41 + 0.079433f : 0.1f)) : ((iTemp35) ? 0.1f - 0.060189f * fTemp38 : 0.039811f)) : ((iTemp32) ? 0.023285f * fTemp36 + 0.039811f : 0.063096f)) : ((iTemp30) ? 0.036904f * fTemp33 + 0.063096f : 0.1f)) : fTemp97) : ((iTemp26) ? 0.063584f * fTemp29 + 0.015849f : 0.079433f)) : ((iTemp24) ? 0.079433f - 0.04781f * fTemp27 : 0.031623f)) : ((iTemp23) ? 0.068377f * fTemp25 + 0.031623f : 0.1f)) : ((iTemp22) ? 0.1f - 0.09f * fTemp56 : 0.01f)) : ((iTemp21) ? 0.040119f * fTemp54 + 0.01f : 0.050119f)) : fTemp124) : ((iTemp18) ? 0.079433f - 0.069433f * fTemp53 : 0.01f)) : ((iTemp16) ? 0.388107f * fTemp19 + 0.01f : 0.398107f)) : ((iTemp14) ? 0.398107f - 0.198581f * fTemp17 : 0.199526f)) : ((iTemp12) ? 0.199526f - 0.099526f * fTemp15 : 0.1f)) : ((iTemp9) ? 0.151189f * fTemp13 + 0.1f : 0.251189f)) : ((iTemp10) ? 0.251189f - 0.051663f * fTemp11 : 0.199526f));
			float fTemp126 = fConst1 * ((iTemp9) ? ((iTemp12) ? ((iTemp14) ? ((iTemp16) ? ((iTemp18) ? ((iTemp20) ? ((iTemp21) ? ((iTemp22) ? ((iTemp23) ? ((iTemp24) ? ((iTemp26) ? ((iTemp28) ? ((iTemp30) ? ((iTemp32) ? ((iTemp35) ? ((iTemp37) ? ((iTemp40) ? ((iTemp42) ? ((iTemp44) ? ((iTemp47) ? ((iTemp62) ? ((iTemp64) ? ((iTemp66) ? ((iTemp67) ? 2.8e+03f : ((iTemp66) ? 2.8e+03f - 1e+02f * fTemp8 : 2.7e+03f)) : 2.7e+03f) : ((iTemp62) ? 1.3e+02f * fTemp65 + 2.7e+03f : 2.83e+03f)) : ((iTemp47) ? 2.83e+03f - 3e+02f * fTemp63 : 2.53e+03f)) : ((iTemp44) ? 2.53e+03f - 2.8e+02f * fTemp48 : 2.25e+03f)) : ((iTemp42) ? 1.5e+02f * fTemp45 + 2.25e+03f : 2.4e+03f)) : ((iTemp40) ? 2e+02f * fTemp43 + 2.4e+03f : 2.6e+03f)) : ((iTemp37) ? 2.6e+03f - 2e+02f * fTemp41 : 2.4e+03f)) : 2.4e+03f) : ((iTemp32) ? 3.5e+02f * fTemp36 + 2.4e+03f : 2.75e+03f)) : ((iTemp30) ? 2.75e+03f - fTemp34 : 2.7e+03f)) : ((iTemp28) ? 2e+02f * fTemp31 + 2.7e+03f : 2.9e+03f)) : ((iTemp26) ? 2.9e+03f - 2e+02f * fTemp29 : 2.7e+03f)) : ((iTemp24) ? 5e+01f * fTemp27 + 2.7e+03f : 2.75e+03f)) : ((iTemp23) ? 1.5e+02f * fTemp25 + 2.75e+03f : 2.9e+03f)) : ((iTemp22) ? 2.9e+03f - 1e+02f * fTemp56 : 2.8e+03f)) : ((iTemp21) ? 1.5e+02f * fTemp54 + 2.8e+03f : 2.95e+03f)) : ((iTemp20) ? 2.95e+03f - 1.2e+02f * fTemp96 : 2.83e+03f)) : ((iTemp18) ? 2.83e+03f - 1.3e+02f * fTemp53 : 2.7e+03f)) : ((iTemp16) ? 2.7e+03f - fTemp107 : 2.65e+03f)) : ((iTemp14) ? 2.65e+03f - 5e+01f * fTemp17 : 2.6e+03f)) : ((iTemp12) ? 2e+02f * fTemp15 + 2.6e+03f : 2.8e+03f)) : ((iTemp9) ? 2.8e+03f - 2e+02f * fTemp13 : 2.6e+03f)) : ((iTemp10) ? fTemp101 + 2.6e+03f : 2.7e+03f));
			float fTemp127 = ((iTemp7) ? 0.0f : fTemp126 + fRec30[1]);
			fRec30[0] = fTemp127 - std::floor(fTemp127);
			float fTemp128 = ((iTemp9) ? ((iTemp12) ? ((iTemp14) ? ((iTemp16) ? ((iTemp18) ? ((iTemp20) ? ((iTemp21) ? ((iTemp22) ? ((iTemp23) ? ((iTemp24) ? ((iTemp26) ? ((iTemp28) ? ((iTemp30) ? ((iTemp32) ? ((iTemp35) ? ((iTemp37) ? ((iTemp40) ? ((iTemp42) ? ((iTemp44) ? ((iTemp47) ? ((iTemp62) ? ((iTemp64) ? 1.2e+02f : ((iTemp62) ? 1.2e+02f - fTemp112 : 1e+02f)) : ((iTemp47) ? 7e+01f * fTemp63 + 1e+02f : 1.7e+02f)) : ((iTemp44) ? 1.7e+02f - fTemp111 : 1.1e+02f)) : ((iTemp42) ? fTemp61 + 1.1e+02f : 1e+02f)) : 1e+02f) : 1e+02f) : 1e+02f) : ((iTemp32) ? fTemp59 + 1e+02f : 1.2e+02f)) : ((iTemp30) ? 1.2e+02f - fTemp58 : 1e+02f)) : 1e+02f) : 1e+02f) : 1e+02f) : ((iTemp23) ? fTemp57 + 1e+02f : 1.2e+02f)) : 1.2e+02f) : ((iTemp21) ? 1.2e+02f - 2e+01f * fTemp54 : 1e+02f)) : 1e+02f) : ((iTemp18) ? 7e+01f * fTemp53 + 1e+02f : 1.7e+02f)) : ((iTemp16) ? 1.7e+02f - fTemp107 : 1.2e+02f)) : ((iTemp14) ? 1.2e+02f - fTemp52 : 1e+02f)) : 1e+02f) : 1e+02f) : 1e+02f);
			fRec31[0] = ((iTemp51) ? fTemp128 : fRec31[1]);
			float fTemp129 = expf(-(fConst4 * fRec31[0]));
			float fTemp130 = fTemp81 * fTemp128;
			fRec32[0] = ((iTemp51) ? fTemp130 : fRec32[1]);
			float fTemp131 = expf(-(fConst4 * fRec32[0]));
			fRec33[0] = fVec3[2] + fRec33[1] * (fTemp131 + fTemp129) - fTemp131 * fTemp129 * fRec33[2];
			float fTemp132 = ((iTemp85) ? 0.0f : fTemp126 + fRec34[1]);
			fRec34[0] = fTemp132 - std::floor(fTemp132);
			fRec35[0] = ((iTemp87) ? fTemp128 : fRec35[1]);
			float fTemp133 = expf(-(fConst4 * fRec35[0]));
			fRec36[0] = ((iTemp87) ? fTemp130 : fRec36[1]);
			float fTemp134 = expf(-(fConst4 * fRec36[0]));
			fRec37[0] = fVec4[2] + fRec37[1] * (fTemp134 + fTemp133) - fTemp134 * fTemp133 * fRec37[2];
			float fTemp135 = ((iTemp91) ? 0.0f : fRec38[1] + fTemp126);
			fRec38[0] = fTemp135 - std::floor(fTemp135);
			fRec39[0] = ((iTemp93) ? fTemp128 : fRec39[1]);
			float fTemp136 = expf(-(fConst4 * fRec39[0]));
			fRec40[0] = ((iTemp93) ? fTemp130 : fRec40[1]);
			float fTemp137 = expf(-(fConst4 * fRec40[0]));
			fRec41[0] = fVec5[2] + fRec41[1] * (fTemp137 + fTemp136) - fTemp137 * fTemp136 * fRec41[2];
			float fTemp138 = ((iTemp9) ? ((iTemp12) ? ((iTemp14) ? ((iTemp16) ? ((iTemp18) ? ((iTemp20) ? ((iTemp21) ? ((iTemp22) ? ((iTemp23) ? ((iTemp24) ? ((iTemp26) ? ((iTemp28) ? ((iTemp30) ? ((iTemp32) ? ((iTemp35) ? ((iTemp37) ? ((iTemp40) ? ((iTemp42) ? ((iTemp44) ? ((iTemp47) ? ((iTemp62) ? ((iTemp64) ? ((iTemp66) ? ((iTemp67) ? 0.1f : ((iTemp66) ? 0.1f - 0.068377f * fTemp8 : 0.031623f)) : 0.031623f) : ((iTemp62) ? 0.126866f * fTemp65 + 0.031623f : 0.158489f)) : ((iTemp47) ? 0.158489f - 0.126866f * fTemp63 : 0.031623f)) : ((iTemp44) ? 0.32319f * fTemp48 + 0.031623f : 0.354813f)) : 0.354813f) : ((iTemp40) ? 0.354813f - 0.196324f * fTemp43 : 0.158489f)) : ((iTemp37) ? 0.158489f - 0.069364f * fTemp41 : 0.089125f)) : ((iTemp35) ? 0.089125f - 0.064006f * fTemp38 : 0.025119f)) : ((iTemp32) ? 0.045676f * fTemp36 + 0.025119f : 0.070795f)) : ((iTemp30) ? 0.055098f * fTemp33 + 0.070795f : 0.125893f)) : ((iTemp28) ? 0.125893f - 0.062797f * fTemp31 : 0.063096f)) : ((iTemp26) ? 0.063096f - 0.012977f * fTemp29 : 0.050119f)) : ((iTemp24) ? 0.020676f * fTemp27 + 0.050119f : 0.070795f)) : ((iTemp23) ? 0.070795f - 0.045676f * fTemp25 : 0.025119f)) : ((iTemp22) ? 0.152709f * fTemp56 + 0.025119f : 0.177828f)) : ((iTemp21) ? 0.177828f - 0.127709f * fTemp54 : 0.050119f)) : fTemp124) : ((iTemp18) ? 0.079433f - 0.06165f * fTemp53 : 0.017783f)) : ((iTemp16) ? 0.428901f * fTemp19 + 0.017783f : 0.446684f)) : ((iTemp14) ? 0.446684f - 0.195495f * fTemp17 : 0.251189f)) : ((iTemp12) ? 0.251189f - 0.125296f * fTemp15 : 0.125893f)) : ((iTemp9) ? 0.125296f * fTemp13 + 0.125893f : 0.251189f)) : ((iTemp10) ? 0.251189f - 0.109935f * fTemp11 : 0.141254f));
			float fTemp139 = ((iTemp9) ? ((iTemp12) ? ((iTemp14) ? ((iTemp16) ? ((iTemp18) ? ((iTemp20) ? ((iTemp21) ? ((iTemp22) ? ((iTemp23) ? ((iTemp24) ? ((iTemp26) ? ((iTemp28) ? ((iTemp30) ? ((iTemp32) ? ((iTemp35) ? ((iTemp37) ? ((iTemp40) ? ((iTemp42) ? ((iTemp44) ? ((iTemp47) ? ((iTemp62) ? ((iTemp64) ? ((iTemp66) ? ((iTemp67) ? 1.15e+03f : ((iTemp66) ? 4.5e+02f * fTemp8 + 1.15e+03f : 1.6e+03f)) : ((iTemp64) ? 1e+02f * fTemp102 + 1.6e+03f : 1.7e+03f)) : ((iTemp62) ? 1.7e+03f - 9e+02f * fTemp65 : 8e+02f)) : ((iTemp47) ? 8e+02f - 1e+02f * fTemp63 : 7e+02f)) : ((iTemp44) ? 3.4e+02f * fTemp48 + 7e+02f : 1.04e+03f)) : ((iTemp42) ? 5.8e+02f * fTemp45 + 1.04e+03f : 1.62e+03f)) : ((iTemp40) ? 1.3e+02f * fTemp43 + 1.62e+03f : 1.75e+03f)) : ((iTemp37) ? 1.75e+03f - 1e+03f * fTemp41 : 7.5e+02f)) : ((iTemp35) ? 7.5e+02f - 1.5e+02f * fTemp38 : 6e+02f)) : ((iTemp32) ? 5.2e+02f * fTemp36 + 6e+02f : 1.12e+03f)) : ((iTemp30) ? 6.8e+02f * fTemp33 + 1.12e+03f : 1.8e+03f)) : ((iTemp28) ? 5e+01f * fTemp31 + 1.8e+03f : 1.85e+03f)) : ((iTemp26) ? 1.85e+03f - 1.03e+03f * fTemp29 : 8.2e+02f)) : ((iTemp24) ? 8.2e+02f - 1.9e+02f * fTemp27 : 6.3e+02f)) : ((iTemp23) ? 5.2e+02f * fTemp25 + 6.3e+02f : 1.15e+03f)) : ((iTemp22) ? 8.5e+02f * fTemp56 + 1.15e+03f : 2e+03f)) : ((iTemp21) ? 1.4e+02f * fTemp54 + 2e+03f : 2.14e+03f)) : ((iTemp20) ? 2.14e+03f - 1.34e+03f * fTemp96 : 8e+02f)) : ((iTemp18) ? 8e+02f - 1e+02f * fTemp53 : 7e+02f)) : ((iTemp16) ? 3.8e+02f * fTemp19 + 7e+02f : 1.08e+03f)) : ((iTemp14) ? 6.2e+02f * fTemp17 + 1.08e+03f : 1.7e+03f)) : ((iTemp12) ? 1.7e+02f * fTemp15 + 1.7e+03f : 1.87e+03f)) : ((iTemp9) ? 1.87e+03f - 1.07e+03f * fTemp13 : 8e+02f)) : ((iTemp10) ? 8e+02f - 2e+02f * fTemp11 : 6e+02f));
			float fTemp140 = fSlow19 * fTemp3 + 3e+01f;
			float fTemp141 = fConst1 * ((iSlow18) ? (((fTemp139 >= 1.3e+03f) & (fTemp70 >= 2e+02f)) ? fTemp139 - 0.00095238094f * (fTemp70 + -2e+02f) * (fTemp139 + -1.3e+03f) : ((fTemp139 <= fTemp140) ? fTemp140 : fTemp139)) : fTemp139);
			float fTemp142 = ((iTemp7) ? 0.0f : fTemp141 + fRec42[1]);
			fRec42[0] = fTemp142 - std::floor(fTemp142);
			float fTemp143 = 1e+01f * fTemp48;
			float fTemp144 = 2e+01f * fTemp63;
			float fTemp145 = ((iTemp9) ? ((iTemp12) ? ((iTemp14) ? ((iTemp16) ? ((iTemp18) ? ((iTemp20) ? ((iTemp21) ? ((iTemp22) ? ((iTemp23) ? ((iTemp24) ? ((iTemp26) ? ((iTemp28) ? ((iTemp30) ? ((iTemp32) ? ((iTemp35) ? ((iTemp37) ? ((iTemp40) ? ((iTemp42) ? ((iTemp44) ? ((iTemp47) ? ((iTemp62) ? ((iTemp64) ? ((iTemp66) ? ((iTemp67) ? 9e+01f : ((iTemp66) ? 9e+01f - 1e+01f * fTemp8 : 8e+01f)) : ((iTemp64) ? 2e+01f * fTemp102 + 8e+01f : 1e+02f)) : ((iTemp62) ? 1e+02f - fTemp112 : 8e+01f)) : ((iTemp47) ? 8e+01f - fTemp144 : 6e+01f)) : ((iTemp44) ? fTemp143 + 6e+01f : 7e+01f)) : ((iTemp42) ? 1e+01f * fTemp45 + 7e+01f : 8e+01f)) : ((iTemp40) ? 1e+01f * fTemp43 + 8e+01f : 9e+01f)) : ((iTemp37) ? 9e+01f - 1e+01f * fTemp41 : 8e+01f)) : 8e+01f) : ((iTemp32) ? fTemp110 + 8e+01f : 9e+01f)) : ((iTemp30) ? 9e+01f - fTemp109 : 8e+01f)) : ((iTemp28) ? 1e+01f * fTemp31 + 8e+01f : 9e+01f)) : ((iTemp26) ? 9e+01f - 1e+01f * fTemp29 : 8e+01f)) : ((iTemp24) ? 8e+01f - 2e+01f * fTemp27 : 6e+01f)) : ((iTemp23) ? 3e+01f * fTemp25 + 6e+01f : 9e+01f)) : ((iTemp22) ? 1e+01f * fTemp56 + 9e+01f : 1e+02f)) : ((iTemp21) ? 1e+02f - 1e+01f * fTemp54 : 9e+01f)) : ((iTemp20) ? 9e+01f - 1e+01f * fTemp96 : 8e+01f)) : ((iTemp18) ? 8e+01f - 2e+01f * fTemp53 : 6e+01f)) : ((iTemp16) ? 3e+01f * fTemp19 + 6e+01f : 9e+01f)) : ((iTemp14) ? 9e+01f - fTemp106 : 8e+01f)) : ((iTemp12) ? 1e+01f * fTemp15 + 8e+01f : 9e+01f)) : ((iTemp9) ? 9e+01f - fTemp105 : 8e+01f)) : ((iTemp10) ? 8e+01f - 2e+01f * fTemp11 : 6e+01f));
			fRec43[0] = ((iTemp51) ? fTemp145 : fRec43[1]);
			float fTemp146 = expf(-(fConst4 * fRec43[0]));
			float fTemp147 = fTemp81 * fTemp145;
			fRec44[0] = ((iTemp51) ? fTemp147 : fRec44[1]);
			float fTemp148 = expf(-(fConst4 * fRec44[0]));
			fRec45[0] = fVec3[2] + fRec45[1] * (fTemp148 + fTemp146) - fTemp148 * fTemp146 * fRec45[2];
			float fTemp149 = ((iTemp85) ? 0.0f : fTemp141 + fRec46[1]);
			fRec46[0] = fTemp149 - std::floor(fTemp149);
			fRec47[0] = ((iTemp87) ? fTemp145 : fRec47[1]);
			float fTemp150 = expf(-(fConst4 * fRec47[0]));
			fRec48[0] = ((iTemp87) ? fTemp147 : fRec48[1]);
			float fTemp151 = expf(-(fConst4 * fRec48[0]));
			fRec49[0] = fVec4[2] + fRec49[1] * (fTemp151 + fTemp150) - fTemp151 * fTemp150 * fRec49[2];
			float fTemp152 = ((iTemp91) ? 0.0f : fRec50[1] + fTemp141);
			fRec50[0] = fTemp152 - std::floor(fTemp152);
			fRec51[0] = ((iTemp93) ? fTemp145 : fRec51[1]);
			float fTemp153 = expf(-(fConst4 * fRec51[0]));
			fRec52[0] = ((iTemp93) ? fTemp147 : fRec52[1]);
			float fTemp154 = expf(-(fConst4 * fRec52[0]));
			fRec53[0] = fVec5[2] + fRec53[1] * (fTemp154 + fTemp153) - fTemp154 * fTemp153 * fRec53[2];
			float fTemp155 = ((iTemp9) ? ((iTemp12) ? ((iTemp14) ? ((iTemp16) ? ((iTemp18) ? ((iTemp20) ? ((iTemp21) ? ((iTemp22) ? ((iTemp23) ? ((iTemp24) ? ((iTemp26) ? ((iTemp28) ? ((iTemp30) ? ((iTemp32) ? ((iTemp35) ? ((iTemp37) ? ((iTemp40) ? ((iTemp42) ? ((iTemp44) ? ((iTemp47) ? ((iTemp62) ? ((iTemp64) ? ((iTemp66) ? ((iTemp67) ? 0.630957f : ((iTemp66) ? 0.630957f - 0.567861f * fTemp8 : 0.063096f)) : ((iTemp64) ? 0.036904f * fTemp102 + 0.063096f : 0.1f)) : ((iTemp62) ? 0.254813f * fTemp65 + 0.1f : 0.354813f)) : ((iTemp47) ? 0.354813f - 0.103624f * fTemp63 : 0.251189f)) : ((iTemp44) ? 0.195495f * fTemp48 + 0.251189f : 0.446684f)) : ((iTemp42) ? 0.195495f * fTemp60 + 0.446684f : 0.251189f)) : ((iTemp40) ? 0.251189f - 0.219566f * fTemp43 : 0.031623f)) : ((iTemp37) ? 0.250215f * fTemp41 + 0.031623f : 0.281838f)) : ((iTemp35) ? 0.281838f - 0.181838f * fTemp38 : 0.1f)) : ((iTemp32) ? 0.401187f * fTemp36 + 0.1f : 0.501187f)) : ((iTemp30) ? 0.501187f - 0.301661f * fTemp33 : 0.199526f)) : ((iTemp28) ? 0.199526f - 0.13643f * fTemp31 : 0.063096f)) : ((iTemp26) ? 0.253132f * fTemp29 + 0.063096f : 0.316228f)) : ((iTemp24) ? 0.316228f - 0.216228f * fTemp27 : 0.1f)) : ((iTemp23) ? 0.401187f * fTemp25 + 0.1f : 0.501187f)) : ((iTemp22) ? 0.501187f - 0.401187f * fTemp56 : 0.1f)) : ((iTemp21) ? 0.151189f * fTemp54 + 0.1f : 0.251189f)) : ((iTemp20) ? 0.030649f * fTemp96 + 0.251189f : 0.281838f)) : ((iTemp18) ? 0.281838f - 0.123349f * fTemp53 : 0.158489f)) : ((iTemp16) ? 0.342698f * fTemp19 + 0.158489f : 0.501187f)) : ((iTemp14) ? 0.501187f - 0.301661f * fTemp17 : 0.199526f)) : ((iTemp12) ? 0.199526f - 0.021698f * fTemp15 : 0.177828f)) : ((iTemp9) ? 0.1384f * fTemp13 + 0.177828f : 0.316228f)) : ((iTemp10) ? 0.316228f - 0.216228f * fTemp11 : 0.1f));
			float fTemp156 = ((iTemp9) ? ((iTemp12) ? ((iTemp14) ? ((iTemp16) ? ((iTemp18) ? ((iTemp20) ? ((iTemp21) ? ((iTemp22) ? ((iTemp23) ? ((iTemp24) ? ((iTemp26) ? ((iTemp28) ? ((iTemp30) ? ((iTemp32) ? ((iTemp35) ? ((iTemp37) ? ((iTemp40) ? ((iTemp42) ? ((iTemp44) ? ((iTemp47) ? ((iTemp62) ? ((iTemp64) ? ((iTemp66) ? ((iTemp67) ? 8e+02f : ((iTemp66) ? 8e+02f - 4e+02f * fTemp8 : 4e+02f)) : ((iTemp64) ? 4e+02f - 5e+01f * fTemp102 : 3.5e+02f)) : ((iTemp62) ? 1e+02f * fTemp65 + 3.5e+02f : 4.5e+02f)) : ((iTemp47) ? 4.5e+02f - 125.0f * fTemp63 : 325.0f)) : ((iTemp44) ? 275.0f * fTemp48 + 325.0f : 6e+02f)) : ((iTemp42) ? 2e+02f * fTemp60 + 6e+02f : 4e+02f)) : ((iTemp40) ? 4e+02f - 1.5e+02f * fTemp43 : 2.5e+02f)) : ((iTemp37) ? 1.5e+02f * fTemp41 + 2.5e+02f : 4e+02f)) : ((iTemp35) ? 4e+02f - fTemp39 : 3.5e+02f)) : ((iTemp32) ? 3.1e+02f * fTemp36 + 3.5e+02f : 6.6e+02f)) : ((iTemp30) ? 6.6e+02f - 2.2e+02f * fTemp33 : 4.4e+02f)) : ((iTemp28) ? 4.4e+02f - 1.7e+02f * fTemp31 : 2.7e+02f)) : ((iTemp26) ? 1.6e+02f * fTemp29 + 2.7e+02f : 4.3e+02f)) : ((iTemp24) ? 4.3e+02f - 6e+01f * fTemp27 : 3.7e+02f)) : ((iTemp23) ? 4.3e+02f * fTemp25 + 3.7e+02f : 8e+02f)) : ((iTemp22) ? 8e+02f - 4.5e+02f * fTemp56 : 3.5e+02f)) : ((iTemp21) ? 3.5e+02f - fTemp55 : 2.7e+02f)) : ((iTemp20) ? 1.8e+02f * fTemp96 + 2.7e+02f : 4.5e+02f)) : ((iTemp18) ? 4.5e+02f - 125.0f * fTemp53 : 325.0f)) : ((iTemp16) ? 325.0f * (fTemp19 + 1.0f) : 6.5e+02f)) : ((iTemp14) ? 6.5e+02f - 2.5e+02f * fTemp17 : 4e+02f)) : ((iTemp12) ? 4e+02f - 1.1e+02f * fTemp15 : 2.9e+02f)) : ((iTemp9) ? 1.1e+02f * fTemp13 + 2.9e+02f : 4e+02f)) : ((iTemp10) ? 4e+02f - 5e+01f * fTemp11 : 3.5e+02f));
			float fTemp157 = fConst1 * ((fTemp156 <= fTemp70) ? fTemp70 : fTemp156);
			float fTemp158 = ((iTemp7) ? 0.0f : fTemp157 + fRec54[1]);
			fRec54[0] = fTemp158 - std::floor(fTemp158);
			float fTemp159 = ((iTemp9) ? ((iTemp12) ? ((iTemp14) ? ((iTemp16) ? ((iTemp18) ? ((iTemp20) ? ((iTemp21) ? ((iTemp22) ? ((iTemp23) ? ((iTemp24) ? ((iTemp26) ? ((iTemp28) ? ((iTemp30) ? ((iTemp32) ? ((iTemp35) ? ((iTemp37) ? ((iTemp40) ? ((iTemp42) ? ((iTemp44) ? ((iTemp47) ? ((iTemp62) ? ((iTemp64) ? ((iTemp66) ? ((iTemp67) ? 8e+01f : ((iTemp66) ? 8e+01f - fTemp113 : 6e+01f)) : ((iTemp64) ? 6e+01f - 1e+01f * fTemp102 : 5e+01f)) : ((iTemp62) ? fTemp112 + 5e+01f : 7e+01f)) : ((iTemp47) ? 7e+01f - fTemp144 : 5e+01f)) : ((iTemp44) ? fTemp143 + 5e+01f : 6e+01f)) : ((iTemp42) ? 2e+01f * fTemp60 + 6e+01f : 4e+01f)) : ((iTemp40) ? 2e+01f * fTemp43 + 4e+01f : 6e+01f)) : ((iTemp37) ? 6e+01f - 2e+01f * fTemp41 : 4e+01f)) : 4e+01f) : ((iTemp32) ? 4e+01f * (fTemp36 + 1.0f) : 8e+01f)) : ((iTemp30) ? 8e+01f - fTemp109 : 7e+01f)) : ((iTemp28) ? 7e+01f - 3e+01f * fTemp31 : 4e+01f)) : 4e+01f) : 4e+01f) : ((iTemp23) ? 4e+01f * (fTemp25 + 1.0f) : 8e+01f)) : ((iTemp22) ? 8e+01f - fTemp108 : 6e+01f)) : 6e+01f) : ((iTemp20) ? 6e+01f - 2e+01f * fTemp96 : 4e+01f)) : ((iTemp18) ? 1e+01f * fTemp53 + 4e+01f : 5e+01f)) : 5e+01f) : ((iTemp14) ? fTemp52 + 5e+01f : 7e+01f)) : ((iTemp12) ? 7e+01f - 3e+01f * fTemp15 : 4e+01f)) : ((iTemp9) ? 3e+01f * fTemp13 + 4e+01f : 7e+01f)) : ((iTemp10) ? 7e+01f - 3e+01f * fTemp11 : 4e+01f));
			fRec55[0] = ((iTemp51) ? fTemp159 : fRec55[1]);
			float fTemp160 = expf(-(fConst4 * fRec55[0]));
			float fTemp161 = fTemp81 * fTemp159;
			fRec56[0] = ((iTemp51) ? fTemp161 : fRec56[1]);
			float fTemp162 = expf(-(fConst4 * fRec56[0]));
			fRec57[0] = fVec3[2] + fRec57[1] * (fTemp162 + fTemp160) - fTemp162 * fTemp160 * fRec57[2];
			float fTemp163 = ((iTemp85) ? 0.0f : fTemp157 + fRec58[1]);
			fRec58[0] = fTemp163 - std::floor(fTemp163);
			fRec59[0] = ((iTemp87) ? fTemp159 : fRec59[1]);
			float fTemp164 = expf(-(fConst4 * fRec59[0]));
			fRec60[0] = ((iTemp87) ? fTemp161 : fRec60[1]);
			float fTemp165 = expf(-(fConst4 * fRec60[0]));
			fRec61[0] = fVec4[2] + fRec61[1] * (fTemp165 + fTemp164) - fTemp165 * fTemp164 * fRec61[2];
			float fTemp166 = ((iTemp91) ? 0.0f : fRec62[1] + fTemp157);
			fRec62[0] = fTemp166 - std::floor(fTemp166);
			fRec63[0] = ((iTemp93) ? fTemp159 : fRec63[1]);
			float fTemp167 = expf(-(fConst4 * fRec63[0]));
			fRec64[0] = ((iTemp93) ? fTemp161 : fRec64[1]);
			float fTemp168 = expf(-(fConst4 * fRec64[0]));
			fRec65[0] = fVec5[2] + fRec65[1] * (fTemp168 + fTemp167) - fTemp168 * fTemp167 * fRec65[2];
			fRec66[0] = fSlow23 + fSlow22 * fRec66[1];
			output0[i0] = FAUSTFLOAT(fSlow24 * fRec66[0] * (((iSlow17) ? fTemp100 : fTemp99) * (fRec65[0] * (1.0f - (fTemp167 + fTemp168 * (1.0f - fTemp167))) * fofcycle_ftbl0dspSIG0[std::max<int>(0, std::min<int>(int(65536.0f * fRec62[0]), 65535))] + fRec61[0] * (1.0f - (fTemp164 + fTemp165 * (1.0f - fTemp164))) * fofcycle_ftbl0dspSIG0[std::max<int>(0, std::min<int>(int(65536.0f * fRec58[0]), 65535))] + fRec57[0] * (1.0f - (fTemp160 + fTemp162 * (1.0f - fTemp160))) * fofcycle_ftbl0dspSIG0[std::max<int>(0, std::min<int>(int(65536.0f * fRec54[0]), 65535))]) + ((iSlow17) ? fTemp100 * fTemp155 : fTemp99 * fTemp155) * (fRec53[0] * (1.0f - (fTemp153 + fTemp154 * (1.0f - fTemp153))) * fofcycle_ftbl0dspSIG0[std::max<int>(0, std::min<int>(int(65536.0f * fRec50[0]), 65535))] + fRec49[0] * (1.0f - (fTemp150 + fTemp151 * (1.0f - fTemp150))) * fofcycle_ftbl0dspSIG0[std::max<int>(0, std::min<int>(int(65536.0f * fRec46[0]), 65535))] + fRec45[0] * (1.0f - (fTemp146 + fTemp148 * (1.0f - fTemp146))) * fofcycle_ftbl0dspSIG0[std::max<int>(0, std::min<int>(int(65536.0f * fRec42[0]), 65535))]) + ((iSlow17) ? fTemp100 * fTemp138 : fTemp99 * fTemp138) * (fRec41[0] * (1.0f - (fTemp136 + fTemp137 * (1.0f - fTemp136))) * fofcycle_ftbl0dspSIG0[std::max<int>(0, std::min<int>(int(65536.0f * fRec38[0]), 65535))] + fRec37[0] * (1.0f - (fTemp133 + fTemp134 * (1.0f - fTemp133))) * fofcycle_ftbl0dspSIG0[std::max<int>(0, std::min<int>(int(65536.0f * fRec34[0]), 65535))] + fRec33[0] * (1.0f - (fTemp129 + fTemp131 * (1.0f - fTemp129))) * fofcycle_ftbl0dspSIG0[std::max<int>(0, std::min<int>(int(65536.0f * fRec30[0]), 65535))]) + ((iSlow17) ? fTemp100 * fTemp125 : fTemp99 * fTemp125) * (fRec29[0] * (1.0f - (fTemp122 + fTemp123 * (1.0f - fTemp122))) * fofcycle_ftbl0dspSIG0[std::max<int>(0, std::min<int>(int(65536.0f * fRec26[0]), 65535))] + fRec25[0] * (1.0f - (fTemp119 + fTemp120 * (1.0f - fTemp119))) * fofcycle_ftbl0dspSIG0[std::max<int>(0, std::min<int>(int(65536.0f * fRec22[0]), 65535))] + fRec21[0] * (1.0f - (fTemp115 + fTemp117 * (1.0f - fTemp115))) * fofcycle_ftbl0dspSIG0[std::max<int>(0, std::min<int>(int(65536.0f * fRec18[0]), 65535))]) + ((iSlow17) ? fTemp100 * fTemp98 : fTemp99 * fTemp98) * (fRec17[0] * (1.0f - (fTemp94 + fTemp95 * (1.0f - fTemp94))) * fofcycle_ftbl0dspSIG0[std::max<int>(0, std::min<int>(int(65536.0f * fRec14[0]), 65535))] + fRec13[0] * (1.0f - (fTemp88 + fTemp89 * (1.0f - fTemp88))) * fofcycle_ftbl0dspSIG0[std::max<int>(0, std::min<int>(int(65536.0f * fRec10[0]), 65535))] + fRec9[0] * (1.0f - (fTemp69 + fTemp83 * (1.0f - fTemp69))) * fofcycle_ftbl0dspSIG0[std::max<int>(0, std::min<int>(int(65536.0f * fRec1[0]), 65535))])));
			iVec1[1] = iVec1[0];
			fRec3[1] = fRec3[0];
			fVec2[1] = fVec2[0];
			fRec4[1] = fRec4[0];
			fRec2[1] = fRec2[0];
			fRec5[1] = fRec5[0];
			fVec3[2] = fVec3[1];
			fVec3[1] = fVec3[0];
			fRec6[1] = fRec6[0];
			fRec1[1] = fRec1[0];
			fRec7[1] = fRec7[0];
			fRec8[1] = fRec8[0];
			fRec9[2] = fRec9[1];
			fRec9[1] = fRec9[0];
			fVec4[2] = fVec4[1];
			fVec4[1] = fVec4[0];
			fRec10[1] = fRec10[0];
			fRec11[1] = fRec11[0];
			fRec12[1] = fRec12[0];
			fRec13[2] = fRec13[1];
			fRec13[1] = fRec13[0];
			fVec5[2] = fVec5[1];
			fVec5[1] = fVec5[0];
			fRec14[1] = fRec14[0];
			fRec15[1] = fRec15[0];
			fRec16[1] = fRec16[0];
			fRec17[2] = fRec17[1];
			fRec17[1] = fRec17[0];
			fRec18[1] = fRec18[0];
			fRec19[1] = fRec19[0];
			fRec20[1] = fRec20[0];
			fRec21[2] = fRec21[1];
			fRec21[1] = fRec21[0];
			fRec22[1] = fRec22[0];
			fRec23[1] = fRec23[0];
			fRec24[1] = fRec24[0];
			fRec25[2] = fRec25[1];
			fRec25[1] = fRec25[0];
			fRec26[1] = fRec26[0];
			fRec27[1] = fRec27[0];
			fRec28[1] = fRec28[0];
			fRec29[2] = fRec29[1];
			fRec29[1] = fRec29[0];
			fRec30[1] = fRec30[0];
			fRec31[1] = fRec31[0];
			fRec32[1] = fRec32[0];
			fRec33[2] = fRec33[1];
			fRec33[1] = fRec33[0];
			fRec34[1] = fRec34[0];
			fRec35[1] = fRec35[0];
			fRec36[1] = fRec36[0];
			fRec37[2] = fRec37[1];
			fRec37[1] = fRec37[0];
			fRec38[1] = fRec38[0];
			fRec39[1] = fRec39[0];
			fRec40[1] = fRec40[0];
			fRec41[2] = fRec41[1];
			fRec41[1] = fRec41[0];
			fRec42[1] = fRec42[0];
			fRec43[1] = fRec43[0];
			fRec44[1] = fRec44[0];
			fRec45[2] = fRec45[1];
			fRec45[1] = fRec45[0];
			fRec46[1] = fRec46[0];
			fRec47[1] = fRec47[0];
			fRec48[1] = fRec48[0];
			fRec49[2] = fRec49[1];
			fRec49[1] = fRec49[0];
			fRec50[1] = fRec50[0];
			fRec51[1] = fRec51[0];
			fRec52[1] = fRec52[0];
			fRec53[2] = fRec53[1];
			fRec53[1] = fRec53[0];
			fRec54[1] = fRec54[0];
			fRec55[1] = fRec55[0];
			fRec56[1] = fRec56[0];
			fRec57[2] = fRec57[1];
			fRec57[1] = fRec57[0];
			fRec58[1] = fRec58[0];
			fRec59[1] = fRec59[0];
			fRec60[1] = fRec60[0];
			fRec61[2] = fRec61[1];
			fRec61[1] = fRec61[0];
			fRec62[1] = fRec62[0];
			fRec63[1] = fRec63[0];
			fRec64[1] = fRec64[0];
			fRec65[2] = fRec65[1];
			fRec65[1] = fRec65[0];
			fRec66[1] = fRec66[0];
		}
	}
};


// aout fofcycle kgate, kfreq, kgain [, Sparam_n, kvalue_n, ...]

struct FOFCYCLE {
    OPDS h;
    MYFLT *aout[1];
    MYFLT *gate;
    MYFLT *freq;
    MYFLT *gain;
    void *ctrls[20];
    fofcycle_dsp *DSP;
    AUXCH dspmem;
    int ctrlindexes[10];
    int numargs;
};

/* Params

    gate
    freq
    gain

    slider0: vibfreq = 6 hz (1 - 10)
    slider1: vibgain = 0.5 (0-1)
    slider2: sustain = 0 (0-1)
    slider3: bend = 0 (-2, 2)
    slider4: vowel= 0 (0-4)
    slider5: voicetype = 0 (0-4)
    slider6: envattack = 10ms (0-100)zitare
    slider7: outgain: 0.5 (0-1)

 */


static int32_t fofcycle_init(CSOUND *csound, FOFCYCLE *p) {
    if(p->dspmem.auxp == NULL)
        csound->AuxAlloc(csound, sizeof(fofcycle_dsp), &p->dspmem);
    p->DSP = new (p->dspmem.auxp) fofcycle_dsp;
    if(p->DSP == 0) {
        INITERR("Memory allocation error");
        return NOTOK;
    }
    p->DSP->init((int)_GetLocalSr(csound, &(p->h)));
    int numargs = _GetInputArgCnt(csound, p) - 3;
    if(numargs % 2) {
        INITERRF("Expected even number of arguments, got %d\n", numargs);
        return NOTOK;
    }
    p->numargs = numargs;
    STRINGDAT *key;
    CS_TYPE *cstype;
    if(numargs > 0) {
        for(int i=0; i < numargs / 2; i++) {
            cstype = _GetTypeForArg(csound, p->ctrls[i*2]);
            if(cstype->varTypeName[0] != 'S') {
                INITERRF("Expected a string for arg %d, got %s\n", i + 2, cstype->varTypeName);
                return NOTOK;
            }
            key = (STRINGDAT*)(p->ctrls[i*2]);
            int paramindex = _key_to_index(key->data, fofcycle_params);
            if(paramindex < 0) {

                INITERRF("Unknown parmeter %s. Possible parameters: %s", key->data, _params_list(fofcycle_params, &_fofcycle_params_list));
                return NOTOK;
            }
            cstype = _GetTypeForArg(csound, p->ctrls[i*2+1]);
            char typechar = cstype->varTypeName[0];
            if(typechar != 'c' && typechar != 'k' && typechar != 'i') {
                INITERRF("Value for key '%s' must be a scalar (a constant or an i- or k- var)"
                         ", got '%s'", key->data, cstype->varTypeName);
                return NOTOK;
            }
            p->ctrlindexes[i] = paramindex;
        }
    }
    return OK;

}

static int32_t fofcycle_perf(CSOUND *csound, FOFCYCLE *p) {
    IGN(csound);
    int numpairs = p->numargs / 2;
    fofcycle_dsp *dsp = p->DSP;
    FAUSTFLOAT *slots = &(dsp->params[0]);
    for(int i = 0; i < numpairs; i++) {
        MYFLT value = *(MYFLT *)(p->ctrls[i * 2 + 1]);
        int index = p->ctrlindexes[i];
        slots[index] = value;
    }
    dsp->param_freq = *p->freq;
    dsp->param_gain = *p->gain;
    dsp->param_gate = *p->gate;
    p->DSP->compute(_GetLocalKsmps(csound, &(p->h)), NULL, p->aout);
    return OK;

}


// -------------------------------------------------------------


#ifdef CSOUNDAPI6
extern "C" {
    static OENTRY localops[] = {
        {(char*)"zitarev", sizeof(ZITAREV), 0, 3, (char*)"aa", (char*)"aa*", (SUBR)zitarev_init, (SUBR)zitarev_perf, NULL, NULL},
        {(char*)"fofcyclevoc", sizeof(FOFCYCLE), 0, 3, (char*)"a", (char*)"kkk*", (SUBR)fofcycle_init, (SUBR)fofcycle_perf, NULL, NULL}

    };
    LINKAGE
}

#else
extern "C" {
    static OENTRY localops[] = {
        {(char*)"zitarev", sizeof(ZITAREV), 0, (char*)"aa", (char*)"aa*", (SUBR)zitarev_init, (SUBR)zitarev_perf, NULL, NULL },
        {(char*)"fofcyclevoc", sizeof(FOFCYCLE), 0, (char*)"a", (char*)"kkk*", (SUBR)fofcycle_init, (SUBR)fofcycle_perf, NULL, NULL}

    };
    LINKAGE
}
#endif

// NB: The (char*) in front of the strings is to comply with the fact
// that in cpp literal c-strings are const char*, but OENTRY defines
// its fields as char*. The solution is to actually modify OENTRY
// to expect a const char*, since that is what it actually expects,
// but that will not probably happen, since that would mean to 
// modify the source of all OENTRY definitions
