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
// -------------------------------------------------
//

#include "rplib2.h"
#include <algorithm>

namespace RfrmPri2 {
// namespace to hold everything related to the
// "priority of reforms" CDMP, using EState<unsigned int>.
// Note that KBase has no access.
using std::cout;
using std::endl;
using std::flush;
using std::get;
using std::tuple;

using KBase::PRNG;
using KBase::TDI;
using KBase::VUI;
using KBase::KMatrix;

using KBase::VotingRule;
using KBase::ReportingLevel;
using KBase::Model;
using KBase::EActor;
using KBase::EState;

// --------------------------------------------

string genName(const string & prefix, unsigned int i) {
  auto sBuff = KBase::newChars(prefix.size() + 10);
  sprintf(sBuff, "%s-%02u", prefix.c_str(), i);
  auto n = string(sBuff);
  return n;
}

// --------------------------------------------

RP2Model* pmmCreation(uint64_t sd) {
  auto pmm = new RP2Model("Bob", sd);
  cout << "Created pmm named " << pmm->getScenarioName() << endl << flush;
  pmm->pcem = KBase::PCEModel::MarkovIPCM;
  auto rng = new PRNG(sd);
  const unsigned int numAct = 17;
  const unsigned int numOpt = 23;
  auto wMat = KMatrix::uniform(rng, 1, numAct, 10.0, 100.0);
  auto uMat = KMatrix::uniform(rng, numAct, numOpt, 0.0, 1.0);
  pmm->setWeights(wMat);
  pmm->setRP2(uMat);
  assert(numOpt == pmm->numOptions());
  vector<string> names = {};
  vector<string> desc = {};
  for (unsigned int i = 0; i < numAct; i++) {
    names.push_back(genName("Actor", i));
    desc.push_back(genName("Description", i));
  }
  pmm->setActors(names, desc);
  cout << "Added " << numAct << " actors " << endl << flush;
  cout << "Configured " << pmm->getScenarioName() << endl << flush;
  return pmm;
}
RP2Pos* pmpCreation(RP2Model* pmm) {
  auto pmp = new RP2Pos(pmm, 7);
  cout << "Created pmp with index " << pmp->getIndex() << endl << flush;
  return pmp;
}
RP2State* pmsCreation(RP2Model * pmm) {
  auto pms = new RP2State(pmm);
  pmm->addState(pms);
  cout << "Added new state to RP2Model" << endl << flush;
  return pms;
}


double rawValPos(unsigned int ai, const VUI &pstn,
                 const KMatrix& riVal, const vector <double>& aCap, const KMatrix& govCost,
                 double govBudget, double obFactor, const vector<double>& prob) {
  assert(0.0 < govBudget);
  assert(govBudget < sum(govCost));
  assert(0.0 < obFactor);
  assert(obFactor < 1.0);

  double costSoFar = 0;
  double vip = 0.0;
  for (unsigned int j = 0; j < pstn.size(); j++)
  {
    unsigned int rj = pstn[j];
    double cj = govCost(0, rj);
    double vij = prob[j] * riVal(ai, rj);
    if (govBudget < costSoFar + cj)
    {
      vij = vij * obFactor;
    }
    vip = vip + vij;
    costSoFar = costSoFar + cj;
  }
  return vip;
}


void initScen(uint64_t sd) {

  using KBase::quadUfromV;
  auto rng = new PRNG(sd);
  const unsigned int numA = 40;
  const unsigned int numItm = 7;

  const auto govCost = KMatrix::uniform(rng, 1, numItm, 25, 100); //col-vec
  const double govBudget = 0.625 * sum(govCost); // cannot afford everything
  const double obFactor = 0.125;

  // The 'riVals' matrix shows the value to the actor (row) of each reform item (clm),
  // not the utility of entire positions (which are permutations of items)
  const KMatrix riVal = KMatrix::uniform(rng, numA, numItm, 10, 100);

  const double pDecline = 0.8750;

  vector<double> aCap = {};
  for (unsigned int i = 0; i < numA; i++)
  {
    double ac = rng->uniform(2.0, 4.0); // [4.0, 16.0] with median at 9.0
    ac = rng->uniform(17.0, 18.0)*ac*ac;
    aCap.push_back(ac);
  }


  vector<double> prob = {};
  double pj = 1.0;
  printf("pDecline factor: %.3f \n", pDecline);
  printf("obFactor: %.3f \n", obFactor);
  // rate of decline has little effect on the results.
  for (unsigned int j = 0; j < numItm; j++) {
    prob.push_back(pj);
    pj = pj * pDecline;
  }

  cout << "Computing positions ... " << endl;
  vector<VUI> positions; // list of all positions
  VUI pstn;
  // build the first permutation: 1,2,3,...
  for (unsigned int i = 0; i < numItm; i++) {
    pstn.push_back(i);
  }
  positions.push_back(pstn);
  while (next_permutation(pstn.begin(), pstn.end())) {
    positions.push_back(pstn);
  }
  const unsigned int numPos = positions.size();
  cout << "For " << numItm << " reform items there are ";
  cout << numPos << " positions" << endl;


  cout << "Building position utility matrix ... " << flush;
  auto rpValFn = [positions, riVal, aCap, govCost, govBudget, obFactor, prob](unsigned int ai, unsigned int pj) {
    VUI pstn = positions[pj];
    double rvp = rawValPos(ai, pstn, riVal, aCap, govCost, govBudget, obFactor, prob);
    return rvp;
  };

  const auto rpRawVal = KMatrix::map(rpValFn, numA, numPos);

  const auto rpNormVal = KBase::rescaleRows(rpRawVal, 0.0, 1.0);

  const double bigR = 0.5;
  auto curve = [bigR](double v) {
    return quadUfromV(v, bigR);
  };

  const auto rpUtil = KMatrix::map(curve, rpNormVal);
  cout << "done." << endl << flush;

  // This is an attempt to define a domain-independent measure of similarity,
  // by looking at the difference in outcomes to actors.
  // Notice that if we sort the columns by difference from #i,
  // those with small differences in outcome might do it by very different
  // means, so that columns from all over the matrix are placed near i.
  auto diffFn = [](unsigned int i, unsigned int j, const KMatrix& uMat) {
    const auto colI = KBase::vSlice(uMat, i);
    const auto colJ = KBase::vSlice(uMat, j);
    for (unsigned int k=0;k<colI.numR(); k++){
    }
    double dij = KBase::norm(colI - colJ);
    return dij;
  };

  auto diffVUI = [&rpUtil, numPos, diffFn](unsigned int n, unsigned int nSim) {
    vector<TDI> vdk = {};
    for (unsigned int k = 0; k < numPos; k++) {
      double dkn = diffFn(k, n, rpUtil);
      auto dk = TDI(dkn, k);
      vdk.push_back(dk);
    }
    auto tupleLess = [](TDI t1, TDI t2) {
      double d1 = get<0>(t1);
      double d2 = get<0>(t2);
      return (d1 < d2);
    };
    std::sort(vdk.begin(), vdk.end(), tupleLess);
    vector<TDI> sdk = {};
    for (unsigned int i = 0; ((i < nSim) && (i < numPos)); i++) {
      sdk.push_back(vdk[i]);
    }
    return sdk;
  };

  // The permutations with outcomes most similar to the given
  // one are those which make the most minor changes
  // (somewhat similar to a vector hill climbing search where the
  // points in a nearby neighborhood make small changes in objective).
  // So looking at a few dozen (out of thousands) might be enough to
  // get significant variation to keep the search progressing.
  unsigned int numClose = 50;
  unsigned int rand1 = rng->uniform(0, numPos);
  auto rslt1 = diffVUI(rand1, numClose);
  
  
  cout << "Permutations with outcomes most similar to " << rand1 << endl;
  for (unsigned int i = 0; i < rslt1.size(); i++) {
    auto dp = rslt1[i];
    double d = get<0>(dp);
    unsigned int p = get<1>(dp);
    auto posP = positions[p];
    printf("%2u: %4u  %.4f  ", i, p, d);
    KBase::printVUI(posP);
    cout << endl;
  }

  delete rng;
  rng = nullptr;
  return;
}

// --------------------------------------------

RP2Model::RP2Model(string d, uint64_t s, vector<bool> vb) : EModel< unsigned int >(d, s, vb) {
  // nothing yet
}


RP2Model::~RP2Model() {
  // nothing yet
}


void RP2Model::setRP2(const KMatrix & pm0) {
  const unsigned int nr = pm0.numR();
  const unsigned int nc = pm0.numC();
  if (0 < numAct) {
    assert(nr == numAct);
  }
  else {
    numAct = nr;
  }

  if (0 < numOptions()) {
    assert(nc == numOptions());
  }
  else {
    theta.resize(nc); // was size zero
    for (unsigned int i = 0; i < nc; i++) {
      theta[i] = i;
    }
  }

  assert(minNumActor <= numAct);
  assert(numAct <= maxNumActor);

  assert(minNumOptions <= numOptions());

  for (auto u : pm0) {
    assert(0.0 <= u);
    assert(u <= 1.0);
  }

  // if all OK, set it
  polUtilMat = pm0;

  return;
}


void RP2Model::setWeights(const KMatrix & w0) {
  const unsigned int nr = w0.numR();
  const unsigned int nc = w0.numC();

  assert(1 == nr);

  if (0 < numAct) {
    assert(nc == numAct);
  }
  else {
    numAct = nc;
  }

  for (auto w : w0) {
    assert(0.0 <= w);
  }
  assert(minNumActor <= numAct);
  assert(numAct <= maxNumActor);

  // if it is OK, set it
  wghtVect = w0;
  return;
}


void RP2Model::setActors(vector<string> names, vector<string> descriptions) {
  const unsigned int na = numAct;
  numAct = 0;

  assert(0 < na);
  assert(na == names.size());
  assert(na == descriptions.size());
  assert(na == wghtVect.numC());
  assert(na == polUtilMat.numR());

  for (unsigned int i = 0; i < na; i++) {
    auto ai = new EActor<unsigned int>(this, names[i], descriptions[i]);
    ai->vr = VotingRule::Proportional;
    ai->sCap = wghtVect(0, i);
    addActor(ai);
  }
  assert(na == numAct);
  return;
}


// --------------------------------------------

RP2Pos::RP2Pos(RP2Model* pm, int n) : EPosition< unsigned int >(pm, n) {
  // nothing yet
}

RP2Pos::~RP2Pos() {
  // nothing yet
}

// --------------------------------------------
RP2State::RP2State(RP2Model* pm) : EState<unsigned int>(pm) {
  // nothing else, yet
}


RP2State::~RP2State() {
  // nothing else, yet
}


VUI RP2State::similarPol(unsigned int ti, unsigned int nSim) const {
  auto rp2m = (const RP2Model*)(eMod);
  auto uMat = rp2m->getPolUtilMat();
  VUI sdk = powerWeightedSimilarity(uMat,  ti,  nSim);
  return sdk;
}


EState<unsigned int>* RP2State::makeNewEState() const {
  RP2Model* pMod = (RP2Model*)model;
  RP2State* s2 = new RP2State(pMod);
  return s2;
}


vector<double> RP2State::actorUtilVectFn(int h, int tj) const {
  // no difference in perspective for this demo
  const unsigned int na = eMod->numAct;
  const auto pMod = (const RP2Model*)eMod;
  const auto pMat = pMod->getPolUtilMat();
  assert(na == pMat.numR());
  assert(0 <= tj);
  assert(tj < pMat.numC());
  vector<double> rslt = {};
  rslt.resize(na);
  for (unsigned int i = 0; i < na; i++) {
    rslt[i] = pMat(i, tj);
  }
  return rslt;
}

// build the square matrix of position utilities.
// Row is actor i, Column is position of actor j,
// U(i,j) is utility to actor i of position of actor j.
// If there are duplicate positions, there will be duplicate columns.
void RP2State::setAllAUtil(ReportingLevel) {
  const unsigned int na = eMod->numAct;
  assert(Model::minNumActor <= na);
  assert(na <= Model::maxNumActor);
  aUtil = {};
  aUtil.resize(na);
  auto uMat = KMatrix(na, na); // they will all be the same in this demo
  for (unsigned int j = 0; j < na; j++) {
    unsigned int nj = posNdx(j);
    auto utilJ = actorUtilVectFn(-1, nj); // all have objective perspective
    for (unsigned int i = 0; i < na; i++) {
      uMat(i, j) = utilJ[i];
    }
  }
  for (unsigned int i = 0; i < na; i++) {
    aUtil[i] = uMat;
  }
  return;
}

}; // end of namespace


// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
