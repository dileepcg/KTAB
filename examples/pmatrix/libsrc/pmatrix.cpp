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

#include "pmatrix.h" 

namespace PMatDemo {
using std::cout;
using std::endl;
using std::flush;
using std::get;
using std::tuple;

using KBase::PRNG;
using KBase::VotingRule;
using KBase::ReportingLevel;
using KBase::KMatrix;
using KBase::Model;
using KBase::EActor;
using KBase::EState;

// --------------------------------------------

string genName(const string & prefix, unsigned int i) {
  auto sBuff = KBase::newChars(prefix.size()+10);
  sprintf(sBuff, "%s-%02u", prefix.c_str(), i);
  auto n = string(sBuff);
  return n;
}

// --------------------------------------------

PMatrixModel* pmmCreation(uint64_t sd) {
  auto pmm = new PMatrixModel("Bob", sd);
  cout << "Created pmm named " << pmm->getScenarioName() << endl << flush;
  pmm->pcem = KBase::PCEModel::MarkovIPCM;
  auto rng = new PRNG(sd);
  const unsigned int numAct = 17;
  const unsigned int numOpt = 23;
  auto wMat = KMatrix::uniform(rng, 1, numAct, 10.0, 100.0);
  auto uMat = KMatrix::uniform(rng, numAct, numOpt, 0.0, 1.0);
  pmm->setWeights(wMat);
  pmm->setPMatrix(uMat);
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


PMatrixPos* pmpCreation(PMatrixModel* pmm) {
  auto pmp = new PMatrixPos(pmm, 7);
  cout << "Created pmp with index " << pmp->getIndex() << endl << flush;
  return pmp;
}

PMatrixState* pmsCreation(PMatrixModel * pmm) {
  auto pms = new PMatrixState(pmm);
  pmm->addState(pms);
  cout << "Added new state to PMatrixModel" << endl << flush;
  return pms;
}

// --------------------------------------------

PMatrixModel::PMatrixModel(string d, uint64_t s, vector<bool> vb) : EModel< unsigned int >(d, s, vb) {
  // nothing yet
}


PMatrixModel::~PMatrixModel() {
  // nothing yet
}


void PMatrixModel::setPMatrix(const KMatrix & pm0) {
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


void PMatrixModel::setWeights(const KMatrix & w0) {
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


void PMatrixModel::setActors(vector<string> names, vector<string> descriptions) {
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


KMatrix PMatrixModel::utilFromFP(const FittingParameters & fParams, double bigR) {
  using KBase::trim;

  // On some values, the utility returned was -0.
  // Not just a small number but something that +.20E prints as
  //  -0.00000000000000000000E+00
  // So I had to force all V and U to be at least epsilon
  const double epsilon = 1E-10;

  auto  maxVect = get<1>(fParams);
  auto  outcomes = get<2>(fParams);
  const unsigned int numAct = maxVect.size();
  assert(numAct == outcomes.numR());
  const unsigned int numScen = outcomes.numC();


  auto uMat = KMatrix(numAct, numScen, -1.0); // invalid values
  for (unsigned int i = 0; i<numAct; i++) {
    double rowMin = outcomes(i, 0);
    double rowMax = outcomes(i, 0);
    for (unsigned int j = 0; j<numScen; j++) {
      double oc = outcomes(i, j);
      if (oc < rowMin) {
        rowMin = oc;
      }
      if (oc > rowMax) {
        rowMax = oc;
      }
    }
    assert(rowMin < rowMax);
    for (unsigned int j = 0; j<numScen; j++) {
      double vij = -1.0; // invalid value
      if (maxVect[i]) {
        vij = (outcomes(i, j) - rowMin) / (rowMax - rowMin);
      }
      else {
        vij = (outcomes(i, j) - rowMax) / (rowMin - rowMax);
      }
      vij = trim(vij, -epsilon, 1.0 + epsilon, true);
      double uij = KBase::quadUfromV(vij, bigR);
      uMat(i, j) = trim(uij, epsilon, 1.0);
    }
  }
  return uMat;
}

tuple<double, KMatrix, KMatrix> PMatrixModel::minProbError(
    const FittingParameters & fParams,
    double bigR, double errWeight) {
  using KBase::hSlice;
  using KBase::vSlice;
  using KBase::VHCSearch;

  printf("Starting minimization with R = %+.3f and errWeight = %.2f \n",
         bigR, errWeight);
  cout << endl << flush;


  auto  aNames = get<0>(fParams);
  auto  maxVect = get<1>(fParams);
  auto  outcomes = get<2>(fParams);
  auto  caseWeights = get<3>(fParams);
  auto  probWeight = get<4>(fParams);
  auto  threshVal = get<5>(fParams);
  auto  overThresh = get<6>(fParams);

  const unsigned int numAct = aNames.size();
  assert(numAct == maxVect.size());
  assert(numAct == outcomes.numR());
  const unsigned int numScen = outcomes.numC();
  assert(numAct == caseWeights.numC());
  const unsigned int numCase = caseWeights.numR();
  assert(numCase == probWeight.numC());
  assert(numScen == probWeight.numR());
  assert(numCase == threshVal.size());
  assert(numCase == overThresh.size());

  // TODO: handle more than two cases
  assert(2 == numCase);

  // TODO: handle 'under' thresholds, not just 'over'
  assert(true == overThresh[0]);
  assert(true == overThresh[1]);

  auto wMat0 = KMatrix(1, numAct, 100.0);
  auto uMat = utilFromFP(fParams, bigR);

  auto wAdj1 = hSlice(caseWeights, 0);
  auto pSel1 = vSlice(probWeight, 0);
  double thresh1 = threshVal[0];

  auto wAdj2 = hSlice(caseWeights, 1);
  auto pSel2 = vSlice(probWeight, 1);
  double thresh2 = threshVal[1];


  assert(KBase::sameShape(wMat0, wAdj1));
  assert(KBase::sameShape(wMat0, wAdj2));


  auto vhc = new VHCSearch();
  auto eRL = ReportingLevel::Silent;
  auto rRL = ReportingLevel::Low;

  vhc->eval = [eRL, wMat0, uMat,
      wAdj1, pSel1, thresh1,
      wAdj2, pSel2, thresh2,
      errWeight]
      (const KMatrix & p) {
    auto c12 = probCost(p,
                        wMat0, uMat,
                        wAdj1, pSel1, thresh1,
                        wAdj2, pSel2, thresh2,
                        errWeight, eRL);

    const double eCost = get<0>(c12);
    // to minimize error cost, maximize 1-eCost.
    return 100.0*(1.0 - eCost);
  };

  vhc->nghbrs = VHCSearch::vn1; // vn2 takes 10 times as long, w/o improvement
  auto p0 = KMatrix(numAct, 1); // all zeros
  cout << "Initial point: ";
  trans(p0).mPrintf(" %+.4f ");
  cout << endl;
  auto rslt = vhc->run(p0,
                       2500, 10, 1E-5, // iMax, sMax, sTol
                       0.50, 0.618, 1.25, 1e-8, // step, shrink, grow, minStep
                       rRL);
  double vBest = get<0>(rslt);
  KMatrix pBest = get<1>(rslt);
  unsigned int in = get<2>(rslt);
  unsigned int sn = get<3>(rslt);
  delete vhc;
  vhc = nullptr;
  printf("Iter: %u  Stable: %u \n", in, sn);
  printf("Best value: %+.4f \n", vBest);
  cout << "Best point:    ";
  trans(pBest).mPrintf(" %+.4f ");
  cout << endl;

  // get more data: cost, w1, w2

  auto c12 = probCost(pBest,
                      wMat0, uMat,
                      wAdj1, pSel1, thresh1,
                      wAdj2, pSel2, thresh2,
                      errWeight, ReportingLevel::High);


  cout << endl;
  return c12;
}

tuple<double, KMatrix, KMatrix> PMatrixModel::probCost(const KMatrix& pnt,
                                                       const KMatrix& wMat, const KMatrix& uMat,
                                                       const KMatrix& wAdj1, const KMatrix& pSel1, double thresh1,
                                                       const KMatrix& wAdj2, const KMatrix& pSel2, double thresh2,
                                                       double errWeight, ReportingLevel rl) {
  assert(1 == wMat.numR());
  const unsigned int nAct = wMat.numC();
  const unsigned int nOpt = uMat.numC();

  assert(1 == pnt.numC());
  assert(nAct == pnt.numR());


  const auto vr = KBase::VotingRule::Proportional;
  const auto vpm = KBase::VPModel::Linear;
  const auto pcem = KBase::PCEModel::MarkovIPCM;

  // --------------------------------------------
  // the cost of the adjustment itself

  // adjustment factors, as column vector
  auto fVec = KMatrix::map([](double x) {
    return exp(x);
  }, pnt);

  // cost of adjustment
  auto cVec = KMatrix::map([](double f) {
    double s = f + (1.0 / f) - 2.0;
    return (s*s); // zero if f=1, positive otherwise
  },
  fVec);

  auto pCost = mean(cVec);

  auto sPlus = [](double x) {
    return (x>0.0 ? x : 0.0);
  };


  // Note that wMat, wAdj1, and wAdj2 are row-vectors.
  // wAdj1 and wAdj2 are 1 for all actors except those to be adjusted
  // in their respective cases.
  // To the contrary, pnt and f are column vectors.
  // Also, pDist1, pSel1, and pSel2 are column vectors.
  // pSel1 and pSel2 are 1 for the options to be counted in that case,
  // zero otherwise.
  // --------------------------------------------
  // cost of falling below threshold in case #1
  auto w1 = KMatrix(1, nAct);
  for (unsigned int i = 0; i<nAct; i++) {
    w1(0, i) = wMat(0, i)*fVec(i, 0)*wAdj1(0, i);
    //printf("w1[%2i] = %7.2f \n", i, w1(0,i));
    assert(0.0 <= w1(0, i));
  }
  auto pDist1 = Model::scalarPCE(nAct, nOpt, w1, uMat, vr, vpm, pcem, rl);
  auto err1 = sPlus(thresh1 - KBase::dot(pDist1, pSel1));
  double serr1 = err1 * err1;

  // --------------------------------------------
  // cost of falling below threshold in case #1
  auto w2 = KMatrix(1, nAct);
  for (unsigned int i = 0; i<nAct; i++) {
    w2(0, i) = wMat(0, i)*fVec(i, 0)*wAdj2(0, i);
    //printf("w2[%2i] = %7.2f \n", i, w2(0,i));
    assert(0.0 < w2(0, i));
  }
  auto pDist2 = Model::scalarPCE(nAct, nOpt, w2, uMat, vr, vpm, pcem, rl);
  auto err2 = sPlus(thresh2 - KBase::dot(pDist2, pSel2));
  double serr2 = err2 * err2;
  
  // count the larger error twice
  double err3 = (err1 > err2) ? err1 : err2;
  double serr3 = err3 * err3;
  
  const double totalCost = (pCost + ((serr1 + serr2 + serr3)*errWeight)) / (1.0 + errWeight);

  if (ReportingLevel::Silent < rl) {
    cout << "Point :";
    trans(pnt).mPrintf(" %+7.4f ");
    cout << endl;

    cout << "w1: ";
    w1.mPrintf(" %7.2f ");
    cout << endl;

    cout << "w2: ";
    w2.mPrintf(" %7.2f ");
    cout << endl;

    printf("Total cost= %.5f = [ %f + (%f^2 + %f^2 + %f^2)*%.2f ] / %.2f",
           totalCost, pCost, err1, err2, err3, errWeight, errWeight + 1.0);
    cout << endl << flush;
  }

  auto rslt = tuple<double, KMatrix, KMatrix>(totalCost, w1, w2);
  return rslt;
}
// --------------------------------------------

PMatrixPos::PMatrixPos(PMatrixModel* pm, int n) : EPosition< unsigned int >(pm, n) {
  // nothing yet
}

PMatrixPos::~PMatrixPos() {
  // nothing yet
}

// --------------------------------------------
PMatrixState::PMatrixState(PMatrixModel* pm) : EState<unsigned int>(pm) {
  // nothing else, yet
}


PMatrixState::~PMatrixState() {
  // nothing else, yet
}

EState<unsigned int>* PMatrixState::makeNewEState() const {
  PMatrixModel* pMod = (PMatrixModel*)model;
  PMatrixState* s2 = new PMatrixState(pMod);
  return s2;
}


VUI PMatrixState::similarPol(unsigned int ti, unsigned int nSim) const {
  auto pmm = (const PMatrixModel*)(eMod);
  auto uMat = pmm->getPolUtilMat();
  VUI sdk = powerWeightedSimilarity(uMat,  ti,  nSim);
  return sdk;
  /*
  const unsigned int numOpt = eMod->numOptions();
  VUI sp = {};
  sp.resize(numOpt);
  for (unsigned int i = 0; i < numOpt; i++) {
    sp[i] = i;
  }
  return sp;
  */
}

vector<double> PMatrixState::actorUtilVectFn(int h, int tj) const {
  // no difference in perspective for this demo
  const unsigned int na = eMod->numAct;
  const auto pMod = (const PMatrixModel*)eMod;
  const auto pMat = pMod->getPolUtilMat();
  assert(na == pMat.numR());
  assert(0 <= tj);
  assert(tj < pMat.numC());
  vector<double> rslt = {};
  rslt.resize(na);
  for (unsigned int i = 0; i<na; i++) {
    rslt[i] = pMat(i, tj);
  }
  return rslt;
}

// build the square matrix of position utilities.
// Row is actor i, Column is position of actor j,
// U(i,j) is utility to actor i of position of actor j.
// If there are duplicate positions, there will be duplicate columns.
void PMatrixState::setAllAUtil(ReportingLevel) {
  const unsigned int na = eMod->numAct;
  assert(Model::minNumActor <= na);
  assert(na <= Model::maxNumActor);
  aUtil = {};
  aUtil.resize(na);
  auto uMat = KMatrix(na, na); // they will all be the same in this demo
  for (unsigned int j = 0; j<na; j++) {
    unsigned int nj = posNdx(j);
    auto utilJ = actorUtilVectFn(-1, nj); // all have objective perspective
    for (unsigned int i = 0; i<na; i++) {
      uMat(i, j) = utilJ[i];
    }
  }
  for (unsigned int i = 0; i<na; i++) {
    aUtil[i] = uMat;
  }
  return;
}


}
// end of namespace


// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
