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
  aUtil = {}; // vector<KMatrix>();
  for (auto p : pstns) {
    assert(nullptr != p);
    delete p;
  }
  pstns = {}; // vector<Position*>();
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
double State::posProb(unsigned int i, const VUI & unq, const KMatrix & pdt) const {
  const unsigned int numA = model->numAct;
  auto nUnq = ((const unsigned int)(unq.size()));
  unsigned int k = numA + 1; // impossibly high
  for (unsigned int j1 = 0; j1 < nUnq; j1++) { // scan unique positions
    unsigned int j2 = unq[j1]; // get ordinary index of the position
    if (equivNdx(i, j2)) {
      k = j1;
    }
  }
  assert(k < numA);
  assert(1 == pdt.numC());
  assert(k < pdt.numR());
  assert(nUnq == pdt.numR());
  double pr = pdt(k, 0);
  return pr;
}

// return the turn-number of this state.
// 0 == initial state, and error if not in the model's history
unsigned int State::myTurn() const {
  int t = -1; // flag an impossible value
  assert(nullptr != model);
  auto hLen = ((const unsigned int)(model->history.size()));
  for (unsigned int i = 0; i < hLen; i++) { // cannot use range-for, as I need the value of 'i'
    State* si = model->history[i];
    assert(nullptr != si);
    if (this == si) {
      t = i;
    }
  }
  assert(0 <= t);
  return t;
}

void State::setUENdx() {
  /// Looking only at the positions in this state, return a vector of indices of unique positions.
  assert(0 == uIndices.size());
  assert(0 == eIndices.size());
  // Note that we have to lambda-bind 'this'. Otherwise, we'd need a 'static' function
  // to give to uIndices.
  auto efn = [this](unsigned int i, unsigned int j) {
    return equivNdx(i, j);
  };
  const unsigned int na = model->numAct;
  assert(Model::minNumActor <= na);
  assert(na <= Model::maxNumActor);
  auto ns = KBase::uiSeq(0, na - 1);
  auto uePair = KBase::ueIndices<unsigned int>(ns, efn);

  uIndices = get<0>(uePair);
  auto nu = ((const unsigned int)(uIndices.size()));
  assert(0 < nu);
  assert(nu <= na);

  eIndices = get<1>(uePair);
  auto ne = ((const unsigned int)(eIndices.size()));
  assert(na == ne);

  return;
}


void State::setAUtil(int perspH, ReportingLevel rl) {
  // we want to make sure that data is calculated at most once.
  // This is necessary because some utilities are very expensive to calculate,
  // it is easiest to be precise all the time.

  if (-1 == perspH) { // calculate them all at once
    assert(0 == aUtil.size());
    setAllAUtil(rl);
  }
  else { // we might get the perspectives of just a few actors
    const unsigned int na = model->numAct;
    assert(0 <= perspH); // -2 not OK
    assert(perspH < na);
    bool firstP = (0 == aUtil.size());
    bool firstForH = ((na == aUtil.size()) && (0 == aUtil[perspH].numR()) && (0 == aUtil[perspH].numC()));
    assert(firstP || firstForH);
    if (firstP) {
      aUtil.resize(na);
      for (unsigned int i = 0; i < na; i++) {
        aUtil[i] = KMatrix(0, 0);
      }
    }
    setOneAUtil(perspH, rl);
  }
  return;
}

void State::setOneAUtil(unsigned int perspH, ReportingLevel rl) {
  // TODO: make this non-dummy
  assert(false);
  return;
}

} // end of namespace

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
