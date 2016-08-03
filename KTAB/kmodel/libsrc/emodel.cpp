// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2015 King Abdullah Petroleum Studies and Research Center
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software
// and associated documentation files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom
// the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
// BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// --------------------------------------------

#include <assert.h>
#include <iostream>

#include "emodel.h"


namespace KBase {

using std::cout;
using std::endl;
using std::flush;
using std::get;
using std::tuple;

// --------------------------------------------
template <class PT>
// JAH 20160711 added rng seed
EModel<PT>::EModel(PRNG * r, string desc, uint64_t s, vector <bool> f) : Model(r, desc, s, f) {
    // nothing yet
}

template <class PT>
void EModel<PT>::setOptions() {
    assert(nullptr != enumOptions);
    assert(0 == theta.size());
    theta = enumOptions();
    return;
}



template <class PT>
EModel<PT>::~EModel() {
    for (auto t : theta) {
        delete t;
        t = nullptr;
    }
    theta = {};
}


template <class PT>
unsigned int EModel<PT>::numOptions() const {
    return theta.size();
}


template <class PT>
PT* EModel<PT>::nthOption(unsigned int i) const {
    assert(i < theta.size());
    PT* pi = theta[i];
    assert(nullptr != pi);
    return pi;
}



// --------------------------------------------
template <class PT>
EPosition<PT>::EPosition(EModel<PT>* m, int n) : Position() {
    assert(nullptr != m);
    eMod = m;
    assert(0 <= n);
    assert(n < eMod->numOptions());
    ndx = n;
}

template <class PT>
EPosition<PT>::~EPosition() {
    eMod = nullptr;
    ndx = -1;
}

// --------------------------------------------
template <class PT>
EState<PT>::EState(EModel<PT>* mod) : State(mod) {
    // nothing yet
}

template <class PT>
EState<PT>::~EState() {
    // nothing yet
}

template <class PT>
void EState<PT>::setAllAUtil(ReportingLevel rl) {
    // nothing yet
    return;
}

template <class PT>
void EState<PT>::setValues() {
    auto eMod = (EModel<PT>*) model;
    const unsigned int numAct = eMod->numAct;
    const unsigned int numOpt = eMod->theta.size();
    assert(numOpt == eMod->numOptions());
    assert(0 < numOpt);
    auto actorVals = KMatrix(numAct, numOpt);
    for (unsigned int j = 0; j < numOpt; j++) {
        vector<double> vp = actorVFn(j, this);
        assert(numAct == vp.size());
        for (unsigned int i = 0; i < numAct; i++) {
            actorVals(i, j) = vp[i];
        }
    }
    return;
}
// --------------------------------------------
// These functions do not need to used outside this file.
// They are just here to prompt the linker. Another approach is
// to #include this CPP file with the one single file that uses EModel<PT>.
/*
void emodelTest() {
  auto em01 = new EModel<unsigned int>(nullptr, "generic model");
  auto em02 = new EModel<tuple<unsigned int, unsigned int>>(nullptr, "generic model");
  delete em01;
  delete em02;
  return;
}
*/

} // end of namespace

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
