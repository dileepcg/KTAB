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

#include "kmodel.h"

namespace KBase {
using std::cout;
using std::endl;
using std::flush;
using std::get;
using std::tuple;
using KBase::PRNG;
using KBase::KMatrix;


State::State(Model * m) {
    clear();
    assert(nullptr != m);
    model = m;
}

State::~State() {
    clear();
}

void State::clear() {
    // We delete positions because they are part of the state.
    // Actors persist across states, so they are not deleted here.
    aUtil = vector<KMatrix>();
    for (auto p : pstns) {
        assert(nullptr != p);
        delete p;
    }
    pstns = vector<Position*>();
    step = nullptr;
}

void State::addPstn(Position* p) {
    assert(nullptr != p);
    pstns.push_back(p);
    return;
}


void State::randomizeUtils(double minU, double maxU, double uNoise) {
    auto rng = model->rng;
    unsigned int na = model->numAct;
    aUtil = vector<KMatrix>();
    auto u = KMatrix::uniform(rng, na, na, minU, maxU);
    for (unsigned int i = 0; i < na; i++) {
        auto un = KMatrix::uniform(rng, na, na, -uNoise, +uNoise);
        aUtil.push_back(u + un);
    }
    return;
}


VUI State::uniqueNdx() const {
    /// Looking only at the positions in this state, return a vector of indices of unique positions.
    // Note that we have to lambda-bind 'this'. Otherwise, we'd need a 'static' function
    // to give to uIndices.
    auto efn = [this](unsigned int i, unsigned int j) {
        return equivNdx(i,j);
    };

    auto ns = KBase::uiSeq(0, model->numAct - 1);
    auto uePair = KBase::ueIndices<unsigned int>(ns, efn);
    VUI uNdx1 = get<0>(uePair);
    VUI eNdx1 = get<1>(uePair);

    return uNdx1;
}

} // end of namespace

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
