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
// Demonstrate some very basic functionality in
// the DemoLeon namespace
// --------------------------------------------


#include "demoleon.h"

using std::cout;
using std::endl;
using std::flush;

using KBase::PRNG;
using KBase::KMatrix;
using KBase::Actor;
using KBase::Model;
using KBase::Position;
using KBase::State;

using KBase::PCEModel;
using KBase::VotingRule;
using KBase::VPModel;

namespace DemoLeon {
const double TolIFD = 1E-6;

LeonActor::LeonActor(string n, string d, LeonModel* em, unsigned int id) : Actor(n, d) {
  assert(nullptr != em);
  vr = VotingRule::Proportional;
  eMod = ((const LeonModel*)em);
  idNum = id;
  minS = 0;
  refS = 0.5;
  refU = 0.5;
  maxS = 1;
}


LeonActor::~LeonActor() {}


double LeonActor::vote(unsigned int est, unsigned int i, unsigned int j, const State* st) const {
  unsigned int k = st->model->actrNdx(this);
  auto uk = st->aUtil[est];
  double uhki = uk(k, i);
  double uhkj = uk(k, j);
  const double sCap = sum(vCap);
  const double vij = Model::vote(vr, sCap, uhki, uhkj);
  // as mentioned below, I calculate the vote the easy way.
  // I could compile the matrix of votes and re-use it.
  return vij;
}


double LeonActor::vote(const Position * ap1, const Position * ap2) const {
  auto p1 = ((const VctrPstn*)ap1);
  auto p2 = ((const VctrPstn*)ap2);

  // it rather inefficient to re-run the entire economic
  // model (yielding L+N utilities) separately for each actor.
  // Correct first, fast later (unless architecture prevents speed-up!)
  assert((0 < refU) && (refU < 1));
  assert((minS < refS) && (refS < maxS));
  const double u1 = posUtil(p1);
  const double u2 = posUtil(p2);
  const double sCap = sum(vCap);
  //assert (0 < sCap);
  const double v12 = Model::vote(vr, sCap, u1, u2);
  // for now, I do it the easy way.

  // More plausible, but tedious, would be to do a little sensitivity analysis
  // on each individual tax rate to see whether it, in context, is beneficial.
  // For example, suppose we want v(A:B) between two tax policies.
  // For each component i, define A'_i = A, but with i-th component replaced by B_i.
  // So da_i = u(A) - u(A'_i) is one assessment of the benefit of having A_i rather than B_i.
  // If the u(X)= dot(g,X), this would be just g_i * (A_i - B_i).
  //
  // Similarly, B'_i = B, but with i-th component replace by A_i.
  // Now db_i = u(B'_i) - u(B) is a second assessment of the benefit of having A_i rather than B_i.
  // Again, this would be  g_i * (A_i - B_i).
  //
  // di = (da_i + db_i)/2 = combined assessment of benefit of A_i over B_i
  //
  // v_i(A:B) = Model::vote(vr, vCap(i), +di/2, -di/2)
  //
  // Finally, set v(A:B) = sum_i of v_i(A:B)
  //
  // Perhaps an even better approach would be for other actors to look at the
  // capability to influence what the other actors care about. If Ai can influence X but not Y,
  // and Aj cares about Y but not X, then there is little which Ai can hold at risk to influence
  // Aj's actions on dimension Y. If they both care about and influence X, while Aj alone influences Y,
  // then the Nash Bargain may have Aj giving up something of Y to Ai in order to get more of X.
  // These cross-issue trades probably have to be addressed via a full multi-dimensional Nash Bargaining
  // solution; see the 'kutils' demo for Nash bargaining with 4D positions that takes into account per-component
  // salience and overall win-probability. This would seem to involve assessing each dimension as separately
  // as a 1D issue with salience and capability for each dimension -- then take the vector of outcomes
  // as the reference outcome from which to start cross-issue trades in Nash bargaining.
  //
  // This works best if one can assess utility separately along each dimension, as per the SMP, even
  // though the individual differences get combined into one weighted Euclidean distance in the SMP.
  // It is not so clear in this Leontief system, because of economic linkages, though the little
  // sensitivity analysis suggested above might suffice.

  return v12;
}



double LeonActor::posUtil(const Position * ap) const {
  auto tax = ((const VctrPstn*)ap);
  auto shares = eMod->vaShares(*tax, false);
  double si = shares(0, idNum);
  double u = shareToUtil(si);
  return u;
}

double LeonActor::shareToUtil(double gdpShare) const {
  double u = refU;
  if (gdpShare < refS)
    u = KBase::rescale(gdpShare, minS, refS, 0, refU);
  else
    u = KBase::rescale(gdpShare, refS, maxS, refU, 1);
  return u;
}


void LeonActor::randomize(PRNG* rng) {
  unsigned int numD = eMod->N;
  double sc = rng->uniform(20, 200);
  vCap = KMatrix::uniform(rng, numD, 1, 1.0, 5.0);
  vCap = (sc / sum(vCap)) * vCap;
  // utility scale is not random, but must be set from a run-matrix.
  return;
}

void LeonActor::setShareUtilScale(const KMatrix & runs) {
  // each row gives shares in [factor | sector] order, so it needs to know its column number
  refU = 0.5556;
  // induce risk-aversion:
  // worst plausible changes from the base-case could cost refU,
  // while best plausible imporovements can only gain 1-refU.
  // ratio of slopes = refU/(1-refU), e.g. 0.6/0.4 = 3/2 or (5/9)/(1-(5/9)) = 1.25
  //
  refS = runs(0, idNum); // by construction, row-0 is the zero-tax reference case
  minS = refS;
  maxS = refS;
  const unsigned int nr = runs.numR();
  for (unsigned int i = 1; i < nr; i++) {
    const double si = runs(i, idNum);
    minS = (si < minS) ? si : minS;
    maxS = (si > maxS) ? si : maxS;
  }

  assert((0 < minS) && (minS < maxS));
  const double sf = 1.001;
  minS = minS / sf;
  maxS = maxS*sf;
  assert((minS < refS) && (refS < maxS));

  double slopeRatio = 2.0;//1.5;
  // we want to have the slope steeper on the loss side than on the gain side,
  // sr > 1, compared to the reference share from zero-tax.
  // (refU - 0)/(refS-minS) = sr * (1 - refU)/(maxS - minS)
  // so ...
  refU = (slopeRatio*(refS - minS)) / ((maxS - refS) + (slopeRatio*(refS - minS)));
  return;
}


LeonState::LeonState(LeonModel * em) : State(em) {
  eMod = ((const LeonModel *)em); // avoids many type conversions later
}

LeonState::~LeonState() {
}


// TODO: see SMPState::pDist or RPState::pDist for a model which must be adapted to use vector-capabilities
tuple <KMatrix, VUI> LeonState::pDist(int persp) const {
  KMatrix pd;
  auto un = VUI();
  assert(false);
  return tuple <KMatrix, VUI>(pd, un);
}


bool LeonState::equivNdx(unsigned int i, unsigned int j) const {
  /// Compare two actual positions in the current state
  auto vpi = ((const VctrPstn *)(pstns[i]));
  auto vpj = ((const VctrPstn *)(pstns[j]));
  assert(vpi != nullptr);
  assert(vpj != nullptr);
  double diff = norm((*vpi) - (*vpj));
  auto lm = ((const LeonModel *)model);
  bool rslt = (diff < lm->posTol);
  return rslt;
}


LeonState* LeonState::stepSUSN() {
  setAUtil(-1, ReportingLevel::Medium);
  auto s2 = doSUSN(ReportingLevel::Silent);
  s2->step = [s2]() {
    return s2->stepSUSN();
  };
  return s2;
}


// i's estimate of the GDP from j's policy.
// This computes more than it needs to, so it is inefficient, but
// the time-cost is negligable compared to the rest of the processing.
double LeonState::estGDP(unsigned int i, unsigned int j) {
  auto lMod = (LeonModel*)model;
  assert(i < lMod->L + lMod->N);
  assert(j < lMod->L + lMod->N);

  auto delta = [](double x, double y) {
    const double d = 1E-10;
    return (2.0*fabs(x - y)) / (d + fabs(x) + fabs(y));
  };

  auto vpj = ((const VctrPstn *)(pstns[j]));
  KMatrix pj = KMatrix(*vpj);
  auto vs = lMod->vaShares(pj, false);


  double factorVA = 0.0;
  for (unsigned int k = 0; k < lMod->L; k++) { // sum VA over factors
    factorVA = factorVA + vs(0, k);
  }

  double sectorVA = 0.0;
  for (unsigned int k = 0; k < lMod->N; k++) { // sum VA over sectors
    sectorVA = sectorVA + vs(0, lMod->L + k);
  }


  double gdpij = 0.0;
  if (i < lMod->L) {
    gdpij = factorVA;
  }
  else {
    gdpij = sectorVA;
  }

  return gdpij;
}

LeonState* LeonState::doSUSN(ReportingLevel rl) const {
  using std::cout;
  using std::endl;
  using std::flush;
  using std::get;
  LeonState* s2 = nullptr;
  // TODO: filter out essentially-duplicate positions

  auto assertSimilar = [](const KMatrix & x, const KMatrix & y) {
    assert(KBase::maxAbs(x - y) < 1E-10);
    return;
  };


  const auto u = aUtil[0]; // all have same beliefs in this demo


  const unsigned int numA = model->numAct;
  assert(numA == eMod->actrs.size());
  auto vpm = VPModel::Linear;
  const unsigned int numP = pstns.size();
  assert(0 < numP);
  auto euMat = [rl, numA, numP, vpm, this](const KMatrix & uMat) {

    // again, I could do a complex vote, but I'll do the easy one.
    // BTW, be sure to lambda-bind uh *after* it is modified.
    auto vkij = [this, uMat](unsigned int k, unsigned int i, unsigned int j) {  // vote_k ( i : j )
      auto ak = (LeonActor*)(eMod->actrs[k]);
      auto ck = KBase::sum(ak->vCap);
      auto v_kij = Model::vote(ak->vr, ck, uMat(k, i), uMat(k, j));
      return v_kij;
    }; // end of vkij

    const auto c = Model::coalitions(vkij, numA, numP);
    const auto pv2 = Model::probCE2(model->pcem, model->vpm, c);
    const auto p = get<0>(pv2);
    const auto pv = get<1>(pv2);
    const auto eu = uMat*p;

    if (ReportingLevel::Low < rl) {
      cout << "Assessing EU from util matrix: " << endl;
      uMat.mPrintf(" %.6f ");
      cout << endl << flush;

      cout << "Coalition strength matrix" << endl;
      c.mPrintf(" %12.6f ");
      cout << endl << flush;

      cout << "Probability Opt_i > Opt_j" << endl;
      pv.mPrintf(" %.6f ");
      cout << endl << flush;

      cout << "Probability Opt_i" << endl;
      p.mPrintf(" %.6f ");
      cout << endl << flush;

      cout << "Expected utility to actors: " << endl;
      eu.mPrintf(" %.6f ");
      cout << endl << flush;
    }

    return eu;
  }; // end of euMat

  if (ReportingLevel::Low < rl) {
    printf("--------------------------------------- \n");
    printf("Assessing utility of actual state to all actors \n");
    for (unsigned int h = 0; h < numA; h++) {
      auto aPos = ((VctrPstn*)(pstns[h]));
      printf("Actual vector-position (possibly non-neutral) of actor %2u: ", h);
      trans(*aPos).mPrintf(" %+.6f ");
    }
    cout << endl << flush;
  }
  const auto eu0 = euMat(u);

  // end of setup?

  auto assessEU = [rl, this, u, assertSimilar, euMat](unsigned int h, const KMatrix & hPos) {
    // build the hypothetical utility matrix by modifying the h-column
    // of h's matrix (his expectation of the util to everyone else of changing his own position).
    const auto uh0 = aUtil[h];
    assertSimilar(u, uh0);  // all have same beliefs in this demo
    auto uh = uh0;
    bool normP = false;
    const double ifd = eMod->infsDegree(hPos);
    assert(ifd < TolIFD);
    auto fTax = eMod->makeFTax(hPos);
    auto shrs = eMod->vaShares(fTax, normP);  // row-vector (was using hPos, not fTax)
    for (unsigned int i = 0; i < eMod->numAct; i++) {
      auto ai = (LeonActor*)(eMod->actrs[i]);
      auto ui = ai->shareToUtil(shrs(0, i));
      uh(i, h) = ui; // utility to actor i of this hypothetical position by h
    }


    if (ReportingLevel::Low < rl) {
      printf("--------------------------------------- \n");
      printf("Assessing utility to %2i of hypo-pos: ", h);
      trans(hPos).mPrintf(" %+.6f ");
      cout << endl << flush;

      printf("Hypo-util minus base util: \n");
      (uh - uh0).mPrintf(" %+.4E ");
      cout << endl << flush;
    }

    const auto eu = euMat(uh);
    return eu(h, 0);
  }; // end of assessEU


  s2 = new LeonState((LeonModel *)model);

  for (unsigned int h = 0; h < numA; h++) {
    auto vhc = new KBase::VHCSearch();
    vhc->eval = [this, h, assessEU](const KMatrix & m1) {
      auto m2 = eMod->makeFTax(m1); // make it feasible
      return assessEU(h, m2);
    };
    vhc->nghbrs = KBase::VHCSearch::vn2;

    auto aPos = ((VctrPstn*)(pstns[h]));
    printf("---------------------------------------- \n");
    // JAH 20160811 changed to display actor names and not just id
    printf("Search for best next-position of actor %s\n", eMod->actrs[h]->name.c_str());
    //printf("Search for best next-position of actor %2i starting from ", h);
    //trans(*aPos).printf(" %+.6f ");
    cout << flush;
    auto rslt = vhc->run(*aPos,                       // p0
                         1000, 10, 1E-4,              // iterMax, stableMax, sTol
                         0.01, 0.618, 1.25, 1e-6,     // step0, shrink, stretch, minStep
                         ReportingLevel::Silent);
    // note that typical improvements in utility in the first round are on the order of 1E-1 or 1E-2.
    // Therefore, any improvement of less than 1/100th of that (below sTol = 1E-4) is considered "stable"

    double vBest = get<0>(rslt);
    KMatrix pBest = get<1>(rslt);
    unsigned int in = get<2>(rslt);
    unsigned int sn = get<3>(rslt);

    //cout << "pBest :    ";
    //trans(pBest).printf(" %+.6f ");


    delete vhc;
    vhc = nullptr;
    printf("Iter: %u  Stable: %u \n", in, sn);
    printf("Best value for %2u: %+.6f \n", h, vBest);
    cout << "Best point:    ";
    trans(pBest).mPrintf(" %+.6f ");
    KMatrix rBest = eMod->makeFTax(pBest);
    printf("Best rates for %2u: ", h);
    trans(rBest).mPrintf(" %+.6f ");

    VctrPstn * posBest = new VctrPstn(rBest);
    s2->pstns.push_back(posBest);

    double du = vBest - eu0(h, 0);
    printf("EU improvement for %2u of %+.4E \n", h, du);
    //printf("  vBest = %+.6f \n", vBest);
    //printf("  eu0(%i, 0) for %i = %+.6f \n", h, h, eu0(h,0));
    //cout << endl << flush;
    // Logically, du should always be non-negative, as VHC never returns a worse value than the starting point.
    //const double eps = 0; // 1E-5 ;
    assert(0 <= du);
  }

  auto p0 = ((VctrPstn*)(pstns[0]));
  KMatrix meanP = KMatrix(p0->numR(), p0->numC());
  for (unsigned int i = 0; i < numA; i++) {
    auto iPos = ((VctrPstn*)(pstns[i]));
    auto y = *iPos;
    meanP = meanP + y;
  }
  meanP = meanP / numA;

  auto acFn = [this](unsigned int i, unsigned int j) {
    const double nTol = 1E-8;
    const auto iPos = ((VctrPstn*)(pstns[i]));
    const auto y = *iPos;
    const auto jPos = ((VctrPstn*)(pstns[j]));
    const auto x = *jPos;
    double acij = lCorr(y, x);
    // avoid NAN by assigning zero in those cases
    if ((KBase::norm(x) < nTol) || (KBase::norm(y) < nTol)) {
      acij = 0.0;
    }
    return acij;
  };

  auto rcFn = [this, meanP](unsigned int i, unsigned int j) {
    const auto iPos = ((VctrPstn*)(pstns[i]));
    const auto y = *iPos;
    const auto jPos = ((VctrPstn*)(pstns[j]));
    const auto x = *jPos;
    double acij = lCorr(y - meanP, x - meanP);
    return acij;
  };

  KMatrix acMat = KMatrix::map(acFn, numA, numA);
  cout << endl << endl;
  cout << "Absolute correlation of policies" << endl;
  acMat.mPrintf(" %+0.4f ");

  cout << "Mean policy" << endl;
  trans(meanP).mPrintf(" %+0.4f ");

  cout << "Euclidean distance to mean policy: " << endl;
  for (unsigned int i = 0; i < numA; i++) {
    const auto iPos = ((VctrPstn*)(pstns[i]));
    const auto y = *iPos;
    printf("  %2u  %0.4f \n", i, KBase::norm(y - meanP));
  }

  KMatrix rcMat = KMatrix::map(rcFn, numA, numA);
  cout << "Correlation of policies relative to mean policy" << endl;
  rcMat.mPrintf(" %+0.4f ");



  assert(nullptr != s2);
  assert(numP == s2->pstns.size());
  assert(numA == s2->model->numAct);
  return s2;
}
// end of doSUSN

void LeonState::setAllAUtil(ReportingLevel rl) {
  using std::cout;
  using std::endl;
  using std::flush;
  unsigned int numA = model->numAct;
  auto eMod0 = (LeonModel*)model;
  auto uFn1 = [eMod0, this](unsigned int i, unsigned int j) {
    auto ai = ((LeonActor*)(eMod0->actrs[i]));
    double uij = ai->posUtil(pstns[j]);
    return uij;
  };
  auto u = KMatrix::map(uFn1, numA, numA);
  if (KBase::ReportingLevel::Low < rl) {
    cout << "Raw actor-pos util matrix" << endl;
    u.mPrintf(" %.4f ");
    cout << endl << flush;
    cout << flush;
  }

  // for the purposes of this demo, I consider each actor to know exactly what the others value.
  // They usually disagree on the likely consequences of a policy, as factors and sectors
  // use different economic models to estimate the consequences of a policy.
  // They usually value the consequences differently, as each factor and sector values only the GDP-share
  // they expect to get.
  // But they know what consequences the others expect, and how they will value those consequences,
  // even if they disagree on both facts and values.
  cout << "aUtil size: " << aUtil.size() << endl << flush;
  cout << flush;

  assert(0 == aUtil.size());
  for (unsigned int i = 0; i < numA; i++) {
    aUtil.push_back(u);
  }
  return;
}

// -------------------------------------------------

LeonModel::LeonModel(string d, uint64_t s, vector<bool> f) : Model(d, s, f) {
  // some arbitrary yet plausible default values
  maxSub = 0.50;
  maxTax = 1.00;
  posTol = 0.00001;
}

LeonModel::~LeonModel() {
  // nothing
}


double LeonModel::stateDist(const LeonState* s1, const LeonState* s2) {
  unsigned int n = s1->pstns.size();
  assert(n == s2->pstns.size());
  double dSum = 0;
  for (unsigned int i = 0; i < n; i++) {
    auto vp1i = ((const VctrPstn*)(s1->pstns[i]));
    auto vp2i = ((const VctrPstn*)(s2->pstns[i]));
    dSum = dSum + KBase::norm((*vp1i) - (*vp2i));
  }
  return dSum;
}




// Go back through the state history and print every actor's estimate of GDP from every other actor's policy-position
void LeonModel::printEstGDP() {
  for (unsigned int t = 0; t < history.size(); t++) {
    auto ls = (LeonState*)(history[t]);
    auto fn = [this, ls](unsigned int i, unsigned int j) {
      return ls->estGDP(i, j);
    };
    auto gdp = KMatrix::map(fn, numAct, numAct);
    printf("For turn %u, estimated GDP by actor (row) of positions (policy): \n", t);
    gdp.mPrintf("%8.2f ");
    cout << endl << flush;
  }

  return;
}

// L factors, M consumption groups, N sectors
tuple<KMatrix, KMatrix, KMatrix, KMatrix> LeonModel::makeBaseYear(unsigned int numF, unsigned int numCG, unsigned int numS, PRNG* rng) {
  using std::cout;
  using std::endl;
  using std::flush;

  using KBase::inv;
  using KBase::iMat;
  using KBase::norm;

  L = numF;
  M = numCG;
  N = numS;


  cout << endl;
  cout << "Build random but consistent base year data for I/O model." << endl;
  cout << "We follow the standard I/O layout" << endl;
  cout << "For 2 factors, 1 cons. group, and 4 sectors, it would be as follows:" << endl;
  cout << " T T T T C X" << endl;
  cout << " T T T T C X" << endl;
  cout << " T T T T C X" << endl;
  cout << " T T T T C X" << endl;
  cout << " V V V V" << endl;
  cout << " V V V V" << endl;
  cout << endl;

  cout << "Synthetic data has ";
  cout << L << " factors, " << M << " consumption groups, " << N << " industrial sectors" << endl;
  cout << " and one export sector (with constant elasticity demand)" << endl;

  assert(nullptr != rng);
  assert(L > 0);
  assert(M > 0);
  assert(N > 0);

  // These data are simple enough that they are clearly
  // correct-by-construction, so I do not check them
  // here. Both data and model get checked in makeIOModel.

  // Build a random transaction matrix.
  // Ensure that the value-added in each column is 1/2 to 2/3
  // of the total value in that column.
  auto trns = KMatrix::uniform(rng, N, N, 100.0, 999.0);
  auto rev = KMatrix::uniform(rng, L, N, 0.1, 1.0);
  for (unsigned int n = 0; n < N; n++) { // n-th column
    double cSum = 0.0;
    for (unsigned int j = 0; j < N; j++) { // j-th row of transactions
      cSum = cSum + trns(j, n);
    }
    double rnSum = 0.0;
    for (unsigned int l = 0; l < L; l++) { // el-th row of value-added
      rnSum = rnSum + rev(l, n);
    }
    double rnTrgt = cSum*rng->uniform(1.0, 2.0);
    for (unsigned int l = 0; l < L; l++) { // el-th row of value-added
      double rln = (rev(l, n) * rnTrgt) / rnSum;
      rev(l, n) = rln;
    }
  }

  // now we have to allocate all that value-added to each row of consumption and export.
  // get full column-sums, so we can see what slack exists on each row.
  auto sumClmT = vector<double>(N);
  auto sumClmR = vector<double>(N);
  for (unsigned int n = 0; n < N; n++) {
    double cs = 0.0;
    for (unsigned int i = 0; i < N; i++) {
      cs = cs + trns(i, n);
    }
    sumClmT[n] = cs;
    cs = 0.0;
    for (unsigned int l = 0; l < L; l++) {
      cs = cs + rev(l, n);
    }
    sumClmR[n] = cs;
  }
  auto sumRowT = vector<double>(N);
  for (unsigned int i = 0; i < N; i++) {
    double rs = 0.0;
    for (unsigned int j = 0; j < N; j++) {
      rs = rs + trns(i, j);
    }
    sumRowT[i] = rs;
  }

  // now, (sumClmT[i] + sumClmR[i]) - sumRowT[i] is amount to be allocated,
  // so it had better be positive. As value-added is 1/2 to 2/3 of the total
  // in every column, we'd expect this usually to be satisfied, but there
  // is always the chance that some unlucky row will come up short. Thus, we require
  // (sumClmT[i] + sumClmR[i]) - sumRowT[i] > 0.10 * sumRowT[i] > 0, and we will
  // achieve that by raising value added (i.e. sumClmR[i]) if necessary until
  // sumClmR[i] > 1.1*sumRowT[i] - sumClmT[i]
  auto f = vector<double>(N);
  for (unsigned int i = 0; i < N; i++) {
    f[i] = 1.0;
  }
  for (unsigned int i = 0; i < N; i++) {
    while (f[i] * sumClmR[i] < 1.1*sumRowT[i] - sumClmT[i]) {
      f[i] = 1.15 * f[i];
      printf("Raised f[%u] to %.3f \n", i, f[i]);
    }
    for (unsigned int l = 0; l < L; l++) {
      rev(l, i) = f[i] * rev(l, i);
    }
    sumClmR[i] = f[i] * sumClmR[i];
  }

  cout << "Transactions:" << endl;
  trns.mPrintf(" %7.1f ");
  cout << endl;

  cout << "Value-added revenue:" << endl;
  rev.mPrintf(" %7.1f ");
  cout << endl;

  auto xprt = KMatrix::uniform(rng, N, 1, 1.0, 100.0);
  auto cons = KMatrix(N, M);

  for (unsigned int i = 0; i < N; i++) {
    double rDef = sumClmT[i] + sumClmR[i] - sumRowT[i];
    double v = xprt(i, 0);
    double rSum = v;
    for (unsigned int m = 0; m < M; m++) {
      v = rng->uniform(1, 100);
      cons(i, m) = v;
      rSum = rSum + v;
    }
    xprt(i, 0) = xprt(i, 0)*(rDef / rSum);
    for (unsigned m = 0; m < M; m++) {
      cons(i, m) = cons(i, m)*(rDef / rSum);
    }
  }

  cout << "Cons:" << endl;
  cons.mPrintf(" %7.1f ");
  cout << endl;

  cout << "eXports:" << endl;
  xprt.mPrintf(" %7.1f ");
  cout << endl;

  auto rslt = tuple<KMatrix, KMatrix, KMatrix, KMatrix>(trns, rev, xprt, cons);
  return rslt;
}

// L factors, M consumption groups, N sectors
void LeonModel::makeIOModel(const KMatrix & trns, const KMatrix & rev, const KMatrix & xprt, const KMatrix & cons, PRNG* rng) {
  using std::cout;
  using std::endl;
  using std::flush;

  using KBase::inv;
  using KBase::iMat;
  using KBase::norm;

  cout << endl;
  cout << "Build I/O model from base-year data" << endl;

  x0 = xprt;

  assert(N == trns.numR());
  assert(N == trns.numC());

  assert(L == rev.numR());
  assert(N == rev.numC());

  assert(M == cons.numC());
  assert(N == cons.numR());

  assert(N == xprt.numR());
  assert(1 == xprt.numC());
  assert(L > 0);
  assert(M > 0);
  assert(N > 0);

  auto delta = [](double x, double y) {
    return (2 * fabs(x - y)) / (fabs(x) + fabs(y));
  };

  auto mDelta = [](const KMatrix & x, const KMatrix & y) {
    return (2 * norm(x - y)) / (norm(x) + norm(y));
  };


  // we assume each consumption group is driven by a Cobb-Douglas utility function,
  // which is easily inferred for each column. The budget constraints just add up
  // to the total value added, so we construct a random matrix to do that.
  double sxc = sum(xprt);
  auto zeta0 = KMatrix(N, M); // the CD coefficients
  auto sumConsClm = KMatrix(M, 1);
  double sCons = 0.0;
  for (unsigned int m = 0; m < M; m++) {
    double scc = 0.0;
    for (unsigned int i = 0; i < N; i++) {
      scc = scc + cons(i, m);
    }
    for (unsigned int i = 0; i < N; i++) {
      zeta0(i, m) = cons(i, m) / scc;
    }
    sumConsClm(m, 0) = scc;
    sCons = sCons + scc;
  }
  double sVA = 0.0;
  auto sumVARows = KMatrix(L, 1);
  for (unsigned int l = 0; l < L; l++) {
    double svr = 0.0;
    for (unsigned int j = 0; j < N; j++) {
      svr = svr + rev(l, j);
    }
    sumVARows(l, 0) = svr;
    sVA = sVA + svr;
  }


  auto sumVAClms = KMatrix(1, N);
  for (unsigned int j = 0; j < N; j++) {
    double svc = 0.0;
    for (unsigned int l = 0; l < L; l++) {
      svc = svc + rev(l, j);
    }
    sumVAClms(0, j) = svc;
  }

  cout << " check (export + cons = value added) ... " << flush;
  assert(fabs(sxc + sCons - sVA) < 0.001);
  cout << "ok" << endl;

  // Now, we have budgets of each consumption group as an Mx1 vector, B_i
  // and the revenue of each VA group an Lx1 vector, R_j, and we
  // need MxL expenditure matrix, E_ij, so that B_i = sum_j [ E_ij * R_j ]
  //
  // Note that because VA comes from exports as well, the total consumer
  // budget will add up to less than revenue.
  //
  // We can set random e_ij~U, and scale to get E_ij:
  // B_i = sum_j [ ( f_i * e_ij ) * R_j ]
  // = f_i * ( sum_j [ e_ij * R_j ] )
  // so
  // f_i = B_i / ( sum_j [ e_ij * R_j ] )
  auto eij = KMatrix::uniform(rng, M, L, 10.0, 100.0);

  auto expnd = KMatrix(M, L);
  for (unsigned int i = 0; i < M; i++) {
    double si = 0.0;
    for (unsigned int j = 0; j < L; j++) {
      si = si + eij(i, j)*sumVARows(j, 0);
    }
    double fi = sumConsClm(i, 0) / si;
    for (unsigned int j = 0; j < L; j++) {
      expnd(i, j) = fi * eij(i, j);
    }
  }

  cout << "sumVARows" << endl;
  sumVARows.mPrintf(" %.1f ");
  cout << endl;

  cout << "sumConsClm" << endl;
  sumConsClm.mPrintf(" %.1f ");
  cout << endl;

  cout << "expenditure matrix" << endl;
  expnd.mPrintf(" %.2f ");
  cout << endl;

  cout << "check (sumConsClm = expnd * sumVARows) ... " << flush;
  assert(mDelta(sumConsClm, expnd*sumVARows) < 1e-6);
  cout << "ok" << endl;

  double dpr = rng->uniform(0.08, 0.12);
  printf("Depreciation: %.3f \n", dpr);
  double grw = rng->uniform(0.02, 0.05);
  printf("Growth: %.3f \n", grw);

  eps = KMatrix::uniform(rng, N, 1, 2.0, 3.0);
  printf("export elasticities \n");
  eps.mPrintf(" %.2f ");
  cout << endl;

  auto capReq = KMatrix(N, N);
  for (unsigned int i = 0; i < N; i++) {
    for (unsigned int j = 0; j < N; j++) {
      double cr = rng->uniform(5.0, 10.0);
      cr = (cr*cr) / 5000.0; // usually tiny, sometimes quite significant
      capReq(i, j) = cr;
    }
  }

  printf("Capital Requirements, B \n");
  capReq.mPrintf(" %.4f ");
  cout << endl;

  // ------------------------------------------------------------
  // start checking that the expected identities hold,
  // i.e. that I got everything correct.
  // column matrix of row-sums
  auto qClm = KMatrix(N, 1);
  for (unsigned int i = 0; i < N; i++) {
    double qi = xprt(i, 0);
    for (unsigned int j = 0; j < N; j++) {
      double tij = trns(i, j);
      qi = qi + tij;
    }
    for (unsigned int k = 0; k < M; k++) {
      double cik = cons(i, k);
      qi = qi + cik;
    }
    qClm(i, 0) = qi;
  }

  printf("Column vector of total outputs (export+trans+cons) \n");
  qClm.mPrintf(" %.1f ");
  cout << endl;

  // row matrix of column-sums
  auto qRow = KMatrix(1, N);
  for (unsigned int j = 0; j < N; j++) {
    double qj = 0;
    for (unsigned int i = 0; i < N; i++) {
      double tij = trns(i, j);
      qj = qj + tij;
    }
    for (unsigned int h = 0; h < L; h++) {
      double rhj = rev(h, j);
      qj = qj + rhj;
    }
    qRow(0, j) = qj;
  }


  cout << "check row-sums == clm-sums ... " << flush;
  double tol = 0.001; // tolerance in matching row and column sums
  for (unsigned int n = 0; n < N; n++) {
    assert(delta(qClm(n, 0), qRow(0, n)) < tol);
  }
  cout << "ok" << endl;

  auto A = KMatrix(N, N);
  for (unsigned int j = 0; j < N; j++) {
    double qj = qRow(0, j);
    for (unsigned int i = 0; i < N; i++) {
      double aij = trns(i, j) / qj;
      A(i, j) = aij;
    }
  }
  cout << "A matrix:" << endl;
  A.mPrintf(" %.4f  ");
  cout << endl;


  rho = KMatrix(L, N);
  for (unsigned int j = 0; j < N; j++) {
    double qj = qRow(0, j);
    for (unsigned int h = 0; h < L; h++) {
      double rhj = rev(h, j) / qj;
      rho(h, j) = rhj;
    }
  }

  vas = KMatrix(1, N);
  for (unsigned int j = 0; j < N; j++) {
    double vj = 0.0;
    for (unsigned int h = 0; h < L; h++) {
      vj = vj + rho(h, j);
    }
    vas(0, j) = vj;
  }

  // ------------------------------------------
  cout << "Shares of GDP to VA factors (labor groups)" << endl;
  cout << " check budgetL == rho x qClm:" << endl;
  auto budgetL = rho * qClm;
  budgetL.mPrintf(" %.2f "); //  these are the VA to factors
  assert(mDelta(sumVARows, budgetL) < tol);
  cout << "ok" << endl;
  cout << endl;

  auto budgetS = KMatrix(1, N);
  for (unsigned int j = 0; j < N; j++) {
    double vs = qClm(j, 0) * vas(0, j);
    budgetS(0, j) = vs;
  }
  cout << "Shares of GDP to industry sectors (using Alpha, not Beta)" << endl;
  cout << "budgetS:" << endl;
  budgetS.mPrintf(" %.2f ");
  assert(mDelta(sumVAClms, budgetS) < tol);
  cout << "ok" << endl;
  cout << endl;

  cout << "GDP: " << endl;
  (vas*qClm).mPrintf(" %.2f ");
  cout << endl;


  auto budgetC = KMatrix(M, 1); // column-vector budget of each consumption category
  for (unsigned int k = 0; k < M; k++) {
    double bk = 0;
    for (unsigned int i = 0; i < N; i++) {
      double cik = cons(i, k);
      bk = bk + cik;
    }
    budgetC(k, 0) = bk;
  }
  cout << "budgetC" << endl << flush;
  budgetC.mPrintf(" %.4f ");
  assert(mDelta(budgetC, sumConsClm) < tol);
  cout << "ok" << endl;
  cout << endl;

  // because the initial prices are all 1, zeta_ik = theta_ik/P_i is
  // just theta_ik, which is C_ik/BC_k
  auto zeta = KMatrix(N, M);
  for (unsigned int k = 0; k < M; k++) {
    double bck = budgetC(k, 0);
    for (unsigned int i = 0; i < N; i++) {
      zeta(i, k) = cons(i, k) / bck;
    }
  }
  cout << "zeta" << endl << flush;
  zeta.mPrintf(" %.4f "); // OK
  cout << endl;

  cout << "cons" << endl << flush;
  (zeta * budgetC).mPrintf("%.4f  "); // OK
  cout << endl;

  cout << "expnd" << endl << flush;
  expnd.mPrintf(" %.4f ");
  cout << endl;

  cout << "budgetL" << endl << flush;
  budgetL.mPrintf(" %.4f ");
  cout << endl;

  cout << "check budgetC == expnd x budgetL" << endl << flush;
  (expnd*budgetL).mPrintf("%.4f  ");
  assert(mDelta(budgetC, expnd*budgetL) < tol);
  cout << "ok" << endl;
  cout << endl;

  auto alpha = A + (zeta * expnd * rho);
  cout << "alpha " << endl << flush;
  alpha.mPrintf(" %.4f ");
  for (auto a : alpha) {
    assert(0.0 < a);
  }
  cout << endl;

  auto id = iMat(N);
  aL = inv(id - alpha);

  cout << "check aL * X == qClm" << endl << flush;
  (aL*xprt).mPrintf(" %.4f ");
  assert(mDelta(aL*xprt, qClm) < tol);
  for (auto x : aL) {
    assert(0.0 < x);
  }
  cout << "ok" << endl;
  cout << endl;

  auto beta = alpha + (dpr + grw)*capReq;
  cout << "beta " << endl << flush;
  beta.mPrintf(" %.4f ");
  cout << endl;

  bL = inv(id - beta);
  auto betaQX = bL*xprt;
  cout << "check bL * X == betaQX" << endl << flush;
  betaQX.mPrintf(" %.4f ");
  for (auto x : bL) {
    assert(0.0 < x);
  }
  cout << "ok" << endl;
  cout << endl;

  auto budgetBS = KMatrix(1, N);
  for (unsigned int j = 0; j < N; j++) {
    double vbs = betaQX(j, 0) * vas(0, j);
    budgetBS(0, j) = vbs;;
  }
  cout << "Shares of GDP to industry sectors (using Beta, not Alpha)" << endl;
  cout << "budgetBS:" << endl;
  budgetBS.mPrintf(" %.2f ");
  cout << endl;

  // A more numerically stable expression for aL: I + a + a^2 + a^3 + ...
  // To give everything time to percolate through the entire economy,
  // up to a power of 5N seems to be enough usually to get
  // delta(pow,inv) below 0.005 on these matrices, but sometimes
  // 11N is necessary. I won't bother with it here.
  //
  // Note that if S(n) = I + a + a^2 + ... + a^n then
  // (I-a)^(-1) = S(n) + O(a^(n+1)) and we can
  // estimate the size of the remaining errors from
  // mean(a^(n+1)) compared to mean(S(n)).

  return;
}

// prepModel takes in a set of "real" data and sets up the IO model, and performs
// several checks to verify all the data is internally consistent; this replicates
// some of the functionality in makeBaseYear & makeIOModel
// numFac = L, numCon = M, numSec = N
void LeonModel::prepModel(unsigned int numFac, unsigned int numCon, unsigned int numSec,
  KMatrix & xprt, KMatrix & cons, KMatrix & elast, KMatrix & trns, KMatrix & rev, KMatrix & expnd, KMatrix & Bmat)
{
  using std::cout;
  using std::endl;
  using std::flush;

  using KBase::inv;
  using KBase::iMat;
  using KBase::norm;

  // check stuff first - compiler doesn't like the asserts for nullptr inequality(?), so comment out
  assert(numFac > 0);
  assert(numCon > 0);
  assert(numSec > 0);
  assert(xprt.numR() == numSec);
  assert(xprt.numC() == 1);
  assert(cons.numR() == numSec);
  assert(cons.numC() == numCon);
  assert(elast.numR() == numSec);
  assert(elast.numC() == 1);
  assert(trns.numR() == numSec);
  assert(trns.numC() == numSec);
  assert(rev.numR() == numFac);
  assert(rev.numC() == numSec);
  assert(expnd.numR() == numCon);
  assert(expnd.numC() == numFac);
  assert(Bmat.numR() == numSec);
  assert(Bmat.numC() == numSec);

  auto delta = [](double x, double y) {
    return (2 * fabs(x - y)) / (fabs(x) + fabs(y));
  };

  auto mDelta = [](const KMatrix & x, const KMatrix & y) {
    return (2 * norm(x - y)) / (norm(x) + norm(y));
  };

  // insert items into the model
  N = numSec;
  L = numFac;
  M = numCon;

  x0 = xprt;
  cout << "Base Year Export Demand:" << endl;
  x0.mPrintf(" %.4f ");
  cout << endl;

  cout << "Base Year Domestic Demand:" << endl;
  cons.mPrintf(" %.4f ");
  cout << endl;

  eps = elast;
  cout << "Export Elasticities:" << endl;
  eps.mPrintf(" %.4f ");
  cout << endl;

  cout << "Base Year Intra-sectoral Transactions:" << endl;
  trns.mPrintf(" %8.4f ");
  cout << endl;

  cout << "Factor Value-Added Revenue:" << endl;
  rev.mPrintf(" %8.4f ");
  cout << endl;

  // some calculations which we need for building and validating the IO model
  // much of this copied almost verbatim (vercodim?) from makeIOModel
  double sxc = sum(xprt);
  auto zeta0 = KMatrix(N, M); // the CD coefficients
  auto sumConsClms = KMatrix(M, 1);
  double sCons = 0.0;
  for (unsigned int m = 0; m < M; m++) {
    double scc = 0.0;
    for (unsigned int i = 0; i < N; i++) {
      scc = scc + cons(i, m);
    }
    for (unsigned int i = 0; i < N; i++) {
      zeta0(i, m) = cons(i, m) / scc;
    }
    sumConsClms(m, 0) = scc;
    sCons = sCons + scc;
  }

  double sVA = 0.0;
  auto sumVARows = KMatrix(L, 1);
  for (unsigned int l = 0; l < L; l++) {
    double svr = 0.0;
    for (unsigned int j = 0; j < N; j++) {
      svr = svr + rev(l, j);
    }
    sumVARows(l, 0) = svr;
    sVA = sVA + svr;
  }

  auto sumVAClms = KMatrix(1, N);
  for (unsigned int j = 0; j < N; j++) {
    double svc = 0.0;
    for (unsigned int l = 0; l < L; l++) {
      svc = svc + rev(l, j);
    }
    sumVAClms(0, j) = svc;
  }

  auto sumTRNSClms = KMatrix(1, N);
  for (unsigned int j = 0; j < N; j++) {
    double stc = 0.0;
    for (unsigned int l = 0; l < N; l++) {
      stc = stc + trns(l, j);
    }
    sumTRNSClms(0, j) = stc;
  }

  // now compute the rho & vas matrices
  auto qFromTransVA = sumVAClms + sumTRNSClms;
  rho = KMatrix(L,N);
  for (unsigned int l = 0; l < L; l++)
  {
    for (unsigned int j = 0; j < N; j++)
    {
        rho(l,j) = rev(l,j)/qFromTransVA(0,j);
    }
  }
  vas = KMatrix(1, N);
  for (unsigned int j = 0; j < N; j++) {
    double vj = 0.0;
    for (unsigned int h = 0; h < L; h++) {
      vj = vj + rho(h, j);
    }
    vas(0, j) = vj;
  }
  cout << "Factor Value-Add Shares" << endl;
  rho.mPrintf(" %0.4f ");
  cout << endl;
  cout << "Total Value-Add Shares" << endl;
  vas.mPrintf(" %0.4f ");
  cout << endl;

  cout << " check (export + cons = value added) ... " << flush;
  assert(fabs(sxc + sCons - sVA) < 0.001);
  cout << "ok" << endl;

  cout << "sumVARows" << endl;
  sumVARows.mPrintf(" %.4f ");
  cout << endl;

  cout << "sumConsClms" << endl;
  sumConsClms.mPrintf(" %.4f ");
  cout << endl;

  cout << "Expenditure Matrix" << endl;
  expnd.mPrintf(" %.4f ");
  cout << endl;

  cout << "check (sumConsClm = expnd * sumVARows) ... " << flush;
  auto expTSumVA = expnd*sumVARows;
  expTSumVA.mPrintf(" %0.4f ");
  assert(mDelta(sumConsClms, expTSumVA) < 1e-6);
  cout << "ok" << endl;

  // just display here the B matrix
  printf("Scaled Complete Capital Requirements, B \n");
  Bmat.mPrintf(" %.4f ");
  cout << endl;

  auto qClm = KMatrix(N, 1);
  for (unsigned int i = 0; i < N; i++) {
    double qi = xprt(i, 0);
    for (unsigned int j = 0; j < N; j++) {
      double tij = trns(i, j);
      qi = qi + tij;
    }
    for (unsigned int k = 0; k < M; k++) {
      double cik = cons(i, k);
      qi = qi + cik;
    }
    qClm(i, 0) = qi;
  }
  printf("Column vector of total outputs (export+trans+cons) \n");
  qClm.mPrintf(" %.4f ");
  cout << endl;

  // row matrix of column-sums
  auto qRow = KMatrix(1, N);
  for (unsigned int j = 0; j < N; j++) {
    double qj = 0;
    for (unsigned int i = 0; i < N; i++) {
      double tij = trns(i, j);
      qj = qj + tij;
    }
    for (unsigned int h = 0; h < L; h++) {
      double rhj = rev(h, j);
      qj = qj + rhj;
    }
    qRow(0, j) = qj;
  }
  cout << "check row-sums == clm-sums ... " << flush;
  double tol = 0.001; // tolerance in matching row and column sums
  for (unsigned int n = 0; n < N; n++) {
    assert(delta(qClm(n, 0), qRow(0, n)) < tol);
  }
  cout << "ok" << endl;

  auto A = KMatrix(N, N);
  for (unsigned int j = 0; j < N; j++) {
    double qj = qRow(0, j);
    for (unsigned int i = 0; i < N; i++) {
      double aij = trns(i, j) / qj;
      A(i, j) = aij;
    }
  }
  cout << "A matrix:" << endl;
  A.mPrintf(" %.4f ");
  cout << endl;

  // ------------------------------------------
  cout << "Shares of GDP to VA factors (labor groups)" << endl;
  cout << " check budgetL == rho x qClm:" << endl;
  auto budgetL = rho * qClm;
  budgetL.mPrintf(" %.4f "); //  these are the VA to factors
  assert(mDelta(sumVARows, budgetL) < tol);
  cout << "ok" << endl;
  cout << endl;

  auto budgetS = KMatrix(1, N);
  for (unsigned int j = 0; j < N; j++) {
    double vs = qClm(j, 0) * vas(0, j);
    budgetS(0, j) = vs;
  }
  cout << "Shares of GDP to industry sectors (using Alpha, not Beta)" << endl;
  cout << "budgetS:" << endl;
  budgetS.mPrintf(" %.4f ");
  assert(mDelta(sumVAClms, budgetS) < tol);
  cout << "ok" << endl;
  cout << endl;

  cout << "GDP:" << endl;
  (vas*qClm).mPrintf(" %.4f ");
  cout << endl;

  auto budgetC = KMatrix(M, 1); // column-vector budget of each consumption category
  for (unsigned int k = 0; k < M; k++) {
    double bk = 0;
    for (unsigned int i = 0; i < N; i++) {
      double cik = cons(i, k);
      bk = bk + cik;
    }
    budgetC(k, 0) = bk;
  }
  cout << "budgetC" << endl << flush;
  budgetC.mPrintf(" %.4f ");
  assert(mDelta(budgetC, sumConsClms) < tol);
  cout << "ok" << endl;
  cout << endl;

  // because the initial prices are all 1, zeta_ik = theta_ik/P_i is
  // just theta_ik, which is C_ik/BC_k
  auto zeta = KMatrix(N, M);
  for (unsigned int k = 0; k < M; k++) {
    double bck = budgetC(k, 0);
    for (unsigned int i = 0; i < N; i++) {
      zeta(i, k) = cons(i, k) / bck;
    }
  }
  cout << "zeta" << endl << flush;
  zeta.mPrintf(" %.4f "); // OK
  cout << endl;

  cout << "Domestic Demand computed from zeta*budgetC" << endl << flush;
  (zeta * budgetC).mPrintf(" %.4f "); // OK
  cout << endl;

  cout << "check budgetC == expnd x budgetL" << endl << flush;
  (expnd*budgetL).mPrintf(" %.4f ");
  assert(mDelta(budgetC, expnd*budgetL) < tol);
  cout << "ok" << endl;
  cout << endl;

  // compute & validate alpha, then invert & validate I-alpha
  auto alpha = A + (zeta * expnd * rho);
  cout << "alpha " << endl << flush;
  alpha.mPrintf(" %.4f ");
  for (auto a : alpha) {
    assert(0.0 < a);
  }
  cout << endl;

  auto id = iMat(N);
  aL = inv(id - alpha);
  cout << "check aL * X == qClm" << endl << flush;
  (aL*xprt).mPrintf(" %.4f ");
  assert(mDelta(aL*xprt, qClm) < tol);
  for (auto x : aL) {
    assert(0.0 < x);
  }
  cout << "ok" << endl;
  cout << endl;

  // compute beta, then invert & validate I-beta
  auto beta = alpha + Bmat;
  cout << "beta " << endl << flush;
  beta.mPrintf(" %.4f ");
  cout << endl;

  bL = inv(id - beta);
  auto betaQX = bL*xprt;
  cout << "check bL * X" << endl << flush;
  betaQX.mPrintf(" %.4f ");
  for (auto x : bL) {
    assert(0.0 < x);
  }
  cout << "ok" << endl;
  cout << endl;

  auto budgetBS = KMatrix(1, N);
  for (unsigned int j = 0; j < N; j++) {
    double vbs = betaQX(j, 0) * vas(0, j);
    budgetBS(0, j) = vbs;;
  }
  cout << "Shares of GDP to industry sectors (using Beta, not Alpha)" << endl;
  cout << "budgetBS:" << endl;
  budgetBS.mPrintf(" %.4f ");
  cout << endl;


  return;
}


// the usual CES demand function:  (q1/q0) = (p0/p1)^eps ,
// applied to each component where p0 = 1 and p1 = p0+tau
KMatrix LeonModel::xprtDemand(const KMatrix & tau) const {
  assert(sameShape(x0, tau));
  auto x1 = KMatrix(N, 1);
  double p0 = 1;
  for (unsigned int i = 0; i < N; i++) {
    double pi = p0 + tau(i, 0);
    assert(0.0 < pi);
    x1(i, 0) = pow(p0 / pi, eps(i, 0)) * x0(i, 0);
  }
  return x1;
}

KMatrix LeonModel::randomFTax(PRNG* rng) {
  using KBase::dot;
  KMatrix ftax = KMatrix(N, 1);

  auto makeRand = [this, rng]() {
    auto tax = KMatrix(N, 1);
    for (unsigned int i = 0; i < N; i++) {
      double ti = rng->uniform(-1, +1);
      if (ti < 0) {
        ti = ti*maxSub;
      }
      if (0 < ti) {
        ti = ti*maxTax;
      }
      tax(i, 0) = ti;
    }
    return tax;
  };

  auto clip = [this](const KMatrix & tm1) {
    auto tm2 = tm1;
    for (unsigned int i = 0; i < tm1.numR(); i++) {
      for (unsigned int j = 0; j < tm1.numC(); j++) {
        double t = tm1(i, j);
        t = (t < -maxSub) ? -maxSub : t;
        t = (maxTax < t) ? maxTax : t;
        tm2(i, j) = t;
      }
    }
    return tm2;
  };

  bool retry = true;
  while (retry) {
    retry = false;
    try {
      auto t1 = makeRand();
      // now tax is a bunch of random numbers, within [-maxSub, +maxTax] limits.
      // first, adjust to revenue neutrality assuming no demand-effects,
      // then, clip that back within allowable limits (esp make sure -1 < ti)
      auto t2 = clip(KBase::makePerp(t1, x0));
      // of course, that might ruin the revenu-neutrality, so repeat:
      auto t3 = clip(KBase::makePerp(t2, x0));
      // finally, adjust that including demand effects:
      ftax = makeFTax(t3);
    }
    catch (KBase::KException&) {
      retry = true;
    }
  }
  return ftax;
}

// this occaisonally fails to terminate, so we throw an exception
// in those cases. This generally indicates an unreasonably revenue-non-neutral
// initial tax, but search algorithms try crazy stuff: be prepared.
// One way to avoid trouble is to make sure the initial tax is
// within bounds and revenue neutral for the base case: dot(x0,tax) = 0.
// It usually takes 10-15 iterations, so 100 is generous.
KMatrix LeonModel::makeFTax(const KMatrix & tax) const {
  using std::cout;
  using std::endl;
  using std::flush;
  using KBase::ReportingLevel;
  using KBase::makePerp;
  auto srl = ReportingLevel::Silent;

  if (ReportingLevel::Silent < srl) {
    cout << "Raw Tax: ";
    trans(tax).mPrintf(" %+0.6f ");
    cout << endl;
    cout << flush;
  }


  unsigned int N = x0.numR();
  assert(1 == x0.numC());
  assert(sameShape(tax, x0));

  string keNeg = "LeonModel::makeFTax: subsidies at or over 100%";
  for (unsigned int i = 0; i < N; i++) {
    double pi = 1.0 + tax(i, 0);
    if (pi <= 0) {
      if (ReportingLevel::Silent < srl) {
        printf("%s\n", keNeg.c_str());
      }
      throw KBase::KException(keNeg);
    }
  }


  auto x1 = x0;
  string keItr = "LeonModel::makeFTax: iteration limit exceeded";
  const double tol = TolIFD / 10;
  const unsigned int iterMax = 100;
  unsigned int iter = 0;
  auto tau = tax;
  auto t2 = tax;
  double d = infsDegree(tau);
  //printf("First d in makeFTax: %.4E \n", d);
  //printf("Initial tax vector: ");
  //trans(tax).printf(" %+.6f ");
  const double shrink = 0.9;

  while (d > tol) {
    x1 = xprtDemand(tau);
    t2 = makePerp(tau, x1);
    tau = (t2 + tau) / 2;
    if (iter > (iterMax / 4)) {
      tau = shrink * tau;
    }
    d = infsDegree(tau);
    iter = iter + 1;
    if (ReportingLevel::Low < srl) {
      printf("%3u/%3u: %.3E \n", iter, iterMax, d);
    }
    if (iter > iterMax) {
      if (ReportingLevel::Silent < srl) {
        printf("%s\n", keItr.c_str());
      }
      throw KBase::KException(keItr);
    }
  }
  for (unsigned int i = 0; i < N; i++) {
    double pi = 1.0 + tau(i, 0);
    assert(0.0 < pi); // no negative prices
  }

  const double ifd = infsDegree(tau);
  assert(ifd <= tol); // one last check that it is A-OK
  return tau;
}

double LeonModel::infsDegree(const KMatrix & tax) const {
  double mAbs = 0.0;
  bool OK = true;
  for (unsigned int i = 0; i < N; i++) {
    double ti = tax(i, 0);
    if ((ti < 0) && (ti < -maxSub)) {
      mAbs = mAbs - (ti + maxSub); // e.g. ti = -0.7, maxSub = 0.5
      OK = false;
    }
    assert(0 <= mAbs);
    if ((0 < ti) && (maxTax < ti)) {
      mAbs = mAbs + (ti - maxTax);
      OK = false;
    }
    assert(0 <= mAbs);
  }
  if (OK) {
    auto xt = xprtDemand(tax);
    auto t2 = KBase::makePerp(tax, xt);
    mAbs = mAbs + KBase::maxAbs(t2 - tax);
  }
  return mAbs;
}

// Return row-vector of expected factor shares (e.g. L of them, e.g. labor), then expected sector shares (N of them).
// As they are estimated by different economic models, sum of factor VA will usually NOT match sum of sector VA
KMatrix  LeonModel::vaShares(const KMatrix & tax, bool normalizeSharesP) const {

  using KBase::sum;

  auto xt = xprtDemand(tax);

  assert(infsDegree(tax) < TolIFD); // make sure it is a feasible tax

  auto qA = aL * xt; // N-by-1 column vector
  auto budgetL = rho * qA;

  auto qB = bL * xt; // N-by-1 column vector
  auto budgetS = KMatrix(1, N);
  for (unsigned int j = 0; j < N; j++) {
    double vs = qB(j, 0) * vas(0, j);
    budgetS(0, j) = vs;
  }

  // note that the sums of factor and of sector VA's will
  // NOT be equal, as they are assessed by different models.
  auto fShares = KMatrix(1, L);
  auto sShares = KMatrix(1, N);

  for (unsigned int j = 0; j < L; j++) {
    double s = budgetL(j, 0);
    assert(0 < s);
    fShares(0, j) = s;
  }
  for (unsigned int j = 0; j < N; j++) {
    double s = budgetS(0, j);
    assert(0 < s);
    sShares(0, j) = s;
  }

  if (normalizeSharesP) {
    fShares = fShares / sum(fShares);
    sShares = sShares / sum(sShares);

  }

  return KBase::joinH(fShares, sShares); //  [factor | sector]  as promised
}


KMatrix LeonModel::monteCarloShares(unsigned int nRuns, PRNG* rng) {
  using std::cout;
  using std::endl;
  using std::flush;
  auto rl = KBase::ReportingLevel::Low;
  // each run is a row of unnormalized [factor | sector] shares
  // the first row is the base case of zero taxes (row 0 <--> tax 0)
  const bool normP = false;
  assert((0 <= maxSub) && (maxSub < 1));
  assert(0 <= maxTax);

  auto runs = KMatrix(nRuns, L + N);
  auto tau = KMatrix(N, 1); // zero taxes
  auto shr = vaShares(tau, normP);
  for (unsigned int j = 0; j < L + N; j++) {
    runs(0, j) = shr(0, j);
  }

  for (unsigned int i = 1; i < nRuns; i++) {
    tau = randomFTax(rng); // this occaisonally takes a long time
    tau = makeFTax(tau);
    double ifd = infsDegree(tau);
    assert(ifd < TolIFD);
    shr = vaShares(tau, normP);

    for (unsigned int j = 0; j < L + N; j++) {
      runs(i, j) = shr(0, j);
    }

    if (KBase::ReportingLevel::Medium <= rl) {
      printf("MC tax policy %4u \n", i);
      tau.mPrintf(" %+.4f ");
      cout << endl << flush;
      printf("MC shares %4u \n", i);
      shr.mPrintf(" %+.4f ");
      cout << endl << flush;
      for (unsigned int j = 0; j < L + N; j++) {
        if (L <= j) {
          // as they are in [factor |sector] order,
          // we have L factors to skip then N sectors to show
          printf("for MC tax policy %4u, actor %2u taxed %+.4f has share %+.4f \n",
                 i, j, tau(j - L, 0), shr(0, j));
          cout << flush;
        }
      }
    }
  }

  return runs;
}
// -------------------------------------------------

LeonModel* demoSetup(unsigned int numFctr, unsigned int numCGrp, unsigned int numSect, uint64_t s, PRNG* rng) {
  using std::cout;
  using std::endl;
  using std::flush;
  using std::get;

  printf("Setting up with PRNG seed:  %020llu \n", s);
  rng->setSeed(s);

  // because votes, and hence coalition strengths, cannot be computed simply as a function
  // of difference in utility (though influence is correlated with the difference), we
  // have to build the coalition strength matrix directly from votes.
  // Model::vProb(const KMatrix & c)

  //const unsigned int numFctr = 2;
  //const unsigned int numCGrp = 2;
  //const unsigned int numSect = 5;

  // if there are many sectors, it gets a bit unrealistic to assume that the impact on each is negotiated
  // separately. So we might reduce the dimensionality by assuming that there are K << N
  // attributes being taxed, which appear in the goods in a random mix.
  // That is, the policy is to tax/subsidize those attributes (e.g. embodied CO2), and
  // each good is affected differently by the policy.
  // Thus, generate K basis tax vectors, of N components each, and take the K policy
  // parameters as the weighting vectors (+/-) on those basis taxes.


  // JAH 20160830 changed to pass in the seed
  auto eMod0 = new LeonModel("", s);
  eMod0->stop = nullptr; // no stop-run method, for now

  const unsigned int maxIter = 50; // TODO: realistic limit
  double qf = 1000.0;



  eMod0->stop = [maxIter, qf](unsigned int iter, const State * s) {
    bool tooLong = (maxIter <= iter);
    bool quiet = false;
    if (1 < iter) {
      auto sf = [](unsigned int i1, unsigned int i2, double d12) {
        printf("sDist [%2i,%2i] = %.2E   ", i1, i2, d12);
        return;
      };

      auto s0 = ((const LeonState*)(s->model->history[0]));
      auto s1 = ((const LeonState*)(s->model->history[1]));
      auto d01 = LeonModel::stateDist(s0, s1);
      sf(0, 1, d01);

      auto sx = ((const LeonState*)(s->model->history[iter - 0]));
      auto sy = ((const LeonState*)(s->model->history[iter - 1]));
      auto dxy = LeonModel::stateDist(sx, sy);
      sf(iter - 0, iter - 1, dxy);

      quiet = (dxy < d01 / qf);
      if (quiet)
        printf("Quiet \n");
      else
        printf("Not Quiet \n");

      cout << endl << flush;
    }
    return (tooLong || quiet);
  };


  auto eSt0 = new LeonState(eMod0);
  eMod0->addState(eSt0);

  eSt0->step = nullptr; // no step method, for now
  //eSt0->step = [eSt0]() {return eSt0->stepSUSN(); };

  auto trxc = eMod0->makeBaseYear(numFctr, numCGrp, numSect, rng);

  auto trns = get<0>(trxc);
  auto rev = get<1>(trxc);
  auto xprt = get<2>(trxc);
  auto cons = get<3>(trxc);

  eMod0->makeIOModel(trns, rev, xprt, cons, rng);

  // determine the reference level, as well as upper and lower
  // bounds on economic gain/loss for these actors in this economy.
  unsigned int nRuns = 2500;
  cout << "Calibrate utilities via Monte Carlo of " << nRuns << " runs ... " << flush;
  const auto runs = eMod0->monteCarloShares(nRuns, rng); // row 0 is always 0 tax
  cout << "done" << endl << flush;

  cout << "EU State for Econ actors with vector capabilities" << endl;
  const unsigned int numA = numSect + numFctr;
  const unsigned int eDim = numSect;
  printf("Number of actors %u \n", numA);
  printf("Number of econ policy factors %u \n", eDim);
  // Note that if eDim < numA, then they do not have enough degrees of freedom to
  // precisely target benefits. If eDim > numA, then they do.


  {   // this block makes local all the temp vars used to build the state.

    // for this demo, assign consistent voting rule to the actors.
    // Note that because they have vector capabilities, not scalar,
    // we cannot do a simple scalar election, and the only way to
    // build the 'coalitions' matrix is by voting of the actors themselves.
    auto overallVR = VotingRule::Proportional;

    // now create those actors and display them
    auto es = vector<Actor*>();
    for (unsigned int i = 0; i < numA; i++) {
      string ni = "LeonActor-";
      ni.append(std::to_string(i));
      string di = "Random econ actor";
      auto ai = new LeonActor(ni, di, eMod0, i);
      ai->randomize(rng);;
      ai->vr = overallVR;
      ai->setShareUtilScale(runs);
      es.push_back(ai);
    }
    assert(numA == es.size());

    // now we give them positions that are slightly better for themselves
    // than the base case. this is just a random sampling.
    auto ps = vector<VctrPstn*>();
    for (unsigned int i = 0; i < numA; i++) {
      double maxAbs = 0.0; // with 0.0, all have zero-tax SQ as their initial position
      auto ai = (const LeonActor*)es[i];
      auto t0 = KMatrix::uniform(rng, eDim, 1, -maxAbs, +maxAbs);
      auto t1 = eMod0->makeFTax(t0);
      VctrPstn* ep = nullptr;
      const double minU = 0.0; // could be ai->refU, or with default, 0.6
      const double maxU = 1.0; // could be (1+ai->refU)/2, or with default, 0.8
      assert(0 < ai->refU);
      double ui = -1;
      while ((ui < minU) || (maxU < ui)) {
        if (nullptr != ep) {
          delete ep;
        }
        t0 = KMatrix::uniform(rng, eDim, 1, -maxAbs, +maxAbs);
        t1 = eMod0->makeFTax(t0);
        ep = new VctrPstn(t1);
        ui = ai->posUtil(ep);
      }
      ps.push_back(ep);
    }
    assert(numA == ps.size());

    for (unsigned int i = 0; i < numA; i++) {
      eMod0->addActor(es[i]);
      eSt0->addPstn(ps[i]);
    }
  }
  // end of local-var block

  for (unsigned int i = 0; i < numA; i++) {
    auto ai = (const LeonActor*)eMod0->actrs[i];
    auto pi = (const VctrPstn*)eSt0->pstns[i];
    printf("%2u: %s , %s \n", i, ai->name.c_str(), ai->desc.c_str());
    cout << "voting rule: " << ai->vr << endl;
    cout << "Pos vector: ";
    trans(*pi).mPrintf(" %+7.3f ");
    cout << "Cap vector: ";
    trans(ai->vCap).mPrintf(" %7.3f ");
    printf("minS: %.3f \n", ai->minS);
    printf("refS: %.3f \n", ai->refS);
    printf("maxS: %.3f \n", ai->maxS);
    cout << endl << flush;
  }

  eSt0->setAUtil(-1, KBase::ReportingLevel::Low);
  auto u = eSt0->aUtil[0];
  assert(numA == eSt0->model->numAct);

  auto vfn = [eMod0, eSt0](unsigned int k, unsigned int i, unsigned int j) {
    assert(j != i);
    auto ak = ((LeonActor*)(eMod0->actrs[k]));
    double vk = ak->vote(i, i, j, eSt0);
    return vk;
  };

  KMatrix c = Model::coalitions(vfn, eMod0->actrs.size(), eSt0->pstns.size());
  cout << "Coalition strength matrix" << endl;
  c.mPrintf(" %9.3f ");
  cout << endl << flush;

  auto vpm = VPModel::Linear;
  auto pcem = PCEModel::ConditionalPCM;

  const auto pv2 = Model::probCE2(pcem, vpm, c);
  const auto p = get<0>(pv2); // column
  const auto pv = get<1>(pv2); // square
  cout << "Probability Opt_i > Opt_j" << endl;
  pv.mPrintf(" %.4f ");
  cout << endl;
  cout << "Probability Opt_i" << endl;
  p.mPrintf(" %.4f ");
  cout << endl;
  auto eu0 = u*p;
  cout << "Expected utility to actors: " << endl;
  eu0.mPrintf(" %.4f ");
  cout << endl << flush;
  return eMod0;
} // end of demoSetup


void demoEUEcon(uint64_t s, unsigned int numF, unsigned int numG, unsigned int numS, PRNG* rng) {

  LeonModel * eMod0 = demoSetup(numF, numG, numS, s, rng);
  LeonState * eSt0 = ((LeonState *)(eMod0->history[0]));

  eSt0->aUtil = vector<KMatrix>(); // dropping any old ones
  eSt0->step = [eSt0]() {
    return eSt0->stepSUSN();
  };

  eMod0->run();

  cout << endl;
  cout << "Estimates of GDP over time" << endl;
  eMod0->printEstGDP();

  delete eMod0; // and all the actors in it
  eMod0 = nullptr;

  return;
}

void demoMaxEcon(uint64_t s, unsigned int numF, unsigned int numG, unsigned int numS, PRNG* rng) {
  using std::cout;
  using std::endl;
  using std::flush;
  using std::get;

  LeonModel * eMod0 = demoSetup(numF, numG, numS, s, rng);
  LeonState * eSt0 = ((LeonState *)(eMod0->history[0]));

  eSt0->aUtil = vector<KMatrix>(); // dropping any old ones
  eSt0->step = nullptr;

  auto sCap = KMatrix(eMod0->numAct, 1);
  for (unsigned int i = 0; i < eMod0->numAct; i++) {
    auto ai = (LeonActor*)(eMod0->actrs[i]);
    double si = sum(ai->vCap);
    sCap(i, 0) = si;
  }


  auto omegaFn = [eMod0, sCap](const KMatrix & m1) {
    auto m2 = eMod0->makeFTax(m1); // make it feasible
    auto shares = eMod0->vaShares(m2, false);
    double omega = 0;
    for (unsigned int i = 0; i < eMod0->numAct; i++) {
      auto ai = (LeonActor*)(eMod0->actrs[i]);
      double si = shares(0, i);
      double ui = ai->shareToUtil(si);
      omega = omega + ui*sCap(i, 0);
    }
    return omega;
  };

  auto reportFn = [eMod0](const KMatrix & m) {
    KMatrix r = eMod0->makeFTax(m);
    assert(eMod0->infsDegree(r) < TolIFD); // make sure it is a feasible tax
    printf("Rates: ");
    trans(r).mPrintf(" %+.6f ");
    return;
  };

  // TODO: do a VHCSearch here
  auto vhc = new KBase::VHCSearch();
  vhc->eval = omegaFn; // [] (const KMatrix & m1) { return 0.0;};
  vhc->nghbrs = KBase::VHCSearch::vn2;
  vhc->report = reportFn;

  auto rslt = vhc->run(KMatrix(numS, 1),            // p0
                       1000, 10, 1E-4,              // iterMax, stableMax, sTol
                       0.01, 0.618, 1.25, 1e-6,     // step0, shrink, stretch, minStep
                       ReportingLevel::Medium);
  // note that typical improvements in utility in the first round are on the order of 1E-1 or 1E-2.
  // Therefore, any improvement of less than 1/100th of that (below sTol = 1E-4) is considered "stable"

  double vBest = get<0>(rslt);
  KMatrix pBest = get<1>(rslt);
  unsigned int in = get<2>(rslt);
  unsigned int sn = get<3>(rslt);

  //cout << "pBest :    ";
  //trans(pBest).printf(" %+.6f ");


  delete vhc;
  vhc = nullptr;
  printf("Iter: %u  Stable: %u \n", in, sn);
  printf("Best value : %+.6f \n", vBest);
  cout << "Best point:    ";
  trans(pBest).mPrintf(" %+.6f ");
  KMatrix rBest = eMod0->makeFTax(pBest);
  printf("Best rates: ");
  trans(rBest).mPrintf(" %+.6f ");

  delete vhc;
  vhc = nullptr;


  cout << endl;
  cout << "Estimates of GDP over time" << endl;
  eMod0->printEstGDP();

  delete eMod0; // and all the actors in it
  eMod0 = nullptr;

  return;
}

// JAH 20160809 create a version of the Leon App that can
// run with real-ish data; this could eventually be extended
// to take in the setup data from csv
// note that this bypasses entirely demoSetup, makeBaseYear, and makeIOModel
void demoRealEcon(uint64_t s, PRNG* rng)
{
  using std::vector;
  using std::cout;
  using std::endl;
  using std::flush;
  using std::get;

  // setup Leon model - this is all copied from demoSetup
  auto eMod0 = new LeonModel("", s);
  eMod0->stop = nullptr; // no stop-run method, for now

  const unsigned int maxIter = 50; // TODO: realistic limit
  double qf = 1000.0;

  eMod0->stop = [maxIter, qf](unsigned int iter, const State * s) {
    bool tooLong = (maxIter <= iter);
    bool quiet = false;
    if (1 < iter) {
      auto sf = [](unsigned int i1, unsigned int i2, double d12) {
        printf("sDist [%2i,%2i] = %.2E   ", i1, i2, d12);
        return;
      };

      auto s0 = ((const LeonState*)(s->model->history[0]));
      auto s1 = ((const LeonState*)(s->model->history[1]));
      auto d01 = LeonModel::stateDist(s0, s1);
      sf(0, 1, d01);

      auto sx = ((const LeonState*)(s->model->history[iter - 0]));
      auto sy = ((const LeonState*)(s->model->history[iter - 1]));
      auto dxy = LeonModel::stateDist(sx, sy);
      sf(iter - 0, iter - 1, dxy);

      quiet = (dxy < d01 / qf);
      if (quiet)
        printf("Quiet \n");
      else
        printf("Not Quiet \n");

      cout << endl << flush;
    }
    return (tooLong || quiet);
  };

  auto eSt0 = new LeonState(eMod0);
  eMod0->addState(eSt0);
  eSt0->step = nullptr; // no step method, for now

  // SETUP THE DATA NOW - THIS IS THE NEW STUFF
  const unsigned int N = 9;
  const unsigned int M = 1;
  const unsigned int L = 3;

  // base year export by sector
  // this is from '[IO-USA-1981.xlsx]C+E'!F6:F14
  vector<double> xInput = {7.2451, 1.3015, 0.0228, 76.4894, 94.6275, 20.3296,
    23.5439, 27.6867, 19.9283};
  auto xprt = KMatrix::vecToKmat(xInput, N, 1);

  // base year domestic demand by sector
  // this is from '[IO-USA-1981.xlsx]C+E'!E6:E14
  vector<double> cInput = {15.5082, 9.5952, 303.6608, 295.1940, 365.1954, 122.5666,
    424.4459, 662.0114, 741.6478};
  auto cons = KMatrix::vecToKmat(cInput, N, M);

  // export demand price elasticities - different scenarios
  unsigned int epsscen = 1;
  KMatrix eps = KMatrix();  // must declare it here even if capscen == 0 because of scope
  switch (epsscen)
  {
    case 0: // simulated
      eps = KMatrix::uniform(rng, N, 1, 2.0, 3.0);
      break;
    case 1: // equal
      eps = KMatrix(N,1,2.5);
      break;
    case 2: // positively correlated with VA - most value-add linked with most elastic
    {
      vector<double> eInput = {3.000, 2.750, 2.625, 2.250, 2.500, 2.875, 2.375, 2.125, 2.000};
      eps = KMatrix::vecToKmat(eInput,N,1);
      break;
    }
    case 3: // negatively correlated with VA - most value-add linked with least elastic
    {
      vector<double> eInput = {2.000, 2.250, 2.375, 2.750, 2.500, 2.125, 2.625, 2.875, 3.000};
      eps = KMatrix::vecToKmat(eInput,N,1);
      break;
    }
  }
  // scenarios 2 & 3 calcs are from '[IO-USA-1981.xlsx]Trans-O'!V29:W37
  printf("Export Price Elasticities Generated with Scenario %u\n", epsscen);

  // base year transactions between all sectors
  // this is from '[IO-USA-1981.xlsx]Trans-L'!C2:K10
  vector<double> tInput = {41.7728, 0.0000, 0.4084, 4.6623, 95.9681, 0.0000,
    1.3071, 3.0326, 7.0988, 0.1770, 13.3697, 2.4507, 13.9868, 180.7771, 43.3691,
    0.0000, 0.0000, 2.3663, 1.9470, 9.8936, 0.8169, 7.4596, 7.8114, 15.7706,
    5.2285, 33.3587, 22.4796, 3.1861, 10.4283, 119.2672, 327.2914, 37.9409, 12.8136,
    5.2285, 2.0217, 42.5929, 28.8516, 6.1500, 26.1408, 59.6771, 330.3087, 51.7472,
    26.1427, 11.1196, 115.9473, 5.1331, 6.4174, 12.2535, 45.6902, 64.7227, 87.2310,
    41.1748, 22.2391, 65.0724, 7.2571, 3.2087, 32.2675, 48.4876, 50.2158, 10.3494,
    13.0714, 4.0435, 36.6772, 11.5052, 12.8349, 5.7183, 14.9193, 13.3909, 11.3351,
    41.1748, 147.5870, 62.7062, 3.8941, 4.8131, 38.8027, 39.1631, 49.0999, 24.6415,
    87.5781, 57.6196, 115.9473};
  auto trns = KMatrix::vecToKmat(tInput,N,N);

  // base year factor value-added
  // this is from '[IO-USA-1981.xlsx]Trans-L'!C11:K13
  vector<double> vInput = {32.7324, 100.9019, 69.5304, 127.6188, 98.2364, 105.2256,
    254.0997, 401.0373, 391.3663, 16.0422, 44.2612, 19.9704, 56.0749, 43.1645,
    51.5711, 145.4282, 209.2748, 204.2281, 24.5049, 55.1145, 80.8226, 187.4229,
    144.2715, 78.7763, 33.1342, 119.5358, 116.6532};
  auto rev = KMatrix::vecToKmat(vInput,L,N);


  // vector of expenditures by factor into consumption group(s)
  // this is from '[IO-USA-1981.xlsx]JAH'!C11:K13
  vector<double> eInput = {0.951327802789937, 0.875269217592669, 0.886106910793789};
  auto expnd = KMatrix::vecToKmat(eInput,M,L);

  // scaled (d+g)B matrix
  // this is from '[IO-USA-1981.xlsx]BCK'!C4:K12
  double regul = 0.02457; // this is from '[IO-USA-1981.xlsx]JAH'!H99
  vector<double> bInput = {0.0230, 0.0000, 0.0000, 0.0000, 0.0004, 0.0000, 0.0000,
    0.0000, 0.0000, 0.0003, 0.0022, 0.0001, 0.0003, 0.0008, 0.0005, 0.0001, 0.0001,
    0.0001, 0.0416, 0.0472, 0.0051, 0.0064, 0.0059, 0.0514, 0.0133, 0.2974, 0.0058,
    0.0808, 0.0419, 0.0165, 0.0482, 0.0301, 0.1242, 0.0284, 0.0299, 0.0209, 0.0009,
    0.0005, 0.0002, 0.0011, 0.0049, 0.0014, 0.0003, 0.0003, 0.0002, 0.0035, 0.0018,
    0.0007, 0.0022, 0.0016, 0.0084, 0.0012, 0.0013, 0.0009, 0.0145, 0.0075, 0.0030,
    0.0076, 0.0056, 0.0224, 0.0301, 0.0054, 0.0038, 0.0009, 0.0005, 0.0002, 0.0006,
    0.0004, 0.0014, 0.0003, 0.0003, 0.0002, 0.0000, 0.0000, 0.0000, 0.0005, 0.0002,
    0.0000, 0.0000, 0.0000, 0.0005};
  auto Bmat = regul*KMatrix::vecToKmat(bInput,N,N);

  // set up for scenarios of capacities
  // 0 = random, 1 = equal, 2 = self-weighted, 3 = input-weighted
  unsigned int capscen = 3;
  KMatrix caps = KMatrix();  // must declare it here even if capscen == 0 because of scope
  switch (capscen) {
  case 0:
    // just keep the randomizer code below
    break;
  case 1:
  {
    // every actor has the same capability in each dim (which are all summed anyway)
    caps = KMatrix(N + L, N, 1.0);
    cout << "Capabilities Matrix (Scen 1)" << endl;
    caps.mPrintf(" %0.3f ");
    cout << endl;
    break;
  }
  case 2:
  {
    // set up with cap = 1 for each sector in his own dim, and 1/N everywhere else
    // factors all have 1/N capability - not that this automatically underweights them
    // relative to the sectors
    auto eye = KMatrix(N, N, 1.0 / N);
    for (unsigned int r = 0; r < N; r++)
    {
      for (unsigned int c = 0; c < N; c++)
      {
        if (r == c)
        {
          eye(r, c) = 1.0;
        }
      }
    }
    caps = KBase::joinV(KMatrix(L, N, 1.0 / N), eye);
    cout << "Capabilities Matrix (Scen 2)" << endl;
    caps.mPrintf(" %0.3f ");
    cout << endl;
    break;
    // P.S. JAH 20160830 looking through the existing code, I determined that this
    // is wasted effort, as the vector of weights for each actor is simply summed
    // to a scalar number :-(; keeping this scenario for possible future use
  }
  case 3:
  {
    // JAH 20161003 weight each sector according to it's total VA
    // from '[IO-USA-1981.xlsx]Trans-O'!C26:N26
    vector<double> compVA = { 1537.2745, 750.0555, 923.6700, 62.8064, 205.5044,
      249.9203, 458.6496, 275.9301, 199.5496, 458.0236, 571.5965, 729.0196 };
    caps = KMatrix::vecToKmat(compVA, N + L, 1);
    cout << "Capabilities Matrix (Scen 3)" << endl;
    caps.mPrintf(" %0.3f ");
    cout << endl;
    break;
  }
  }

  // now prep it all
  eMod0->prepModel(L, M, N, xprt, cons, eps, trns, rev, expnd, Bmat);

  // factors and sectors - JAH 20161003 changed order of actors
  vector<string> actNames = { "HH Pay", "Other Pay", "Imports",
    "Agr", "Ming", "Const", "DurGood", "NDurGood", "Trans,C,U","W&R Trade", "Fin,Ins,RE", "Other Srv" };
  vector<string> actDescs = { "HH Pay", "Other Pay", "Imports", "Agriculture",
    "Mining", "Construction", "Durable goods manufacturing", "Nondurable goods manufacturing",
    "Transportation, communication and utilities", "Wholesale and retail trade",
    "Finance, insurance and real estate", "Other services" };

  // NOW BACK TO STUFF COPIED FROM demoSetup (with obvious edits)
  // determine the reference level, as well as upper and lower
  // bounds on economic gain/loss for these actors in this economy.
  unsigned int nRuns = 2500;
  cout << "Calibrate utilities via Monte Carlo of " << nRuns << " runs ... " << flush;
  const auto runs = eMod0->monteCarloShares(nRuns, rng); // row 0 is always 0 tax
  cout << "done" << endl << flush;

  cout << "EU State for Econ actors with vector capabilities" << endl;
  const unsigned int numA = N + L;
  const unsigned int eDim = N;
  printf("Number of actors %u \n", numA);
  printf("Number of econ policy factors %u \n", eDim);
  // Note that if eDim < numA, then they do not have enough degrees of freedom to
  // precisely target benefits. If eDim > numA, then they do.

  {   // this block makes local all the temp vars used to build the state.

    // for this demo, assign consistent voting rule to the actors.
    // Note that because they have vector capabilities, not scalar,
    // we cannot do a simple scalar election, and the only way to
    // build the 'coalitions' matrix is by voting of the actors themselves.
    auto overallVR = VotingRule::Proportional;

    // now create those actors and display them
    // JAH edited to use actor name & descs from the realish data
    auto es = vector<Actor*>();
    for (unsigned int i = 0; i < numA; i++) {
      auto ai = new LeonActor(actNames[i], actDescs[i], eMod0, i);
      // JAH 20160814 changed to allow scenarios
      switch (capscen) {
      case 0:
        ai->randomize(rng);
        break;
      default:
        // get a row from the scenario capabilities matrix and transpose to the column used by Actor
        ai->vCap = KBase::trans(caps.getRow(i));
        break;
      }
      ai->vr = overallVR;
      ai->setShareUtilScale(runs);
      es.push_back(ai);
    }
    assert(numA == es.size());

    // now we give them positions that are slightly better for themselves
    // than the base case. this is just a random sampling.
    auto ps = vector<VctrPstn*>();
    for (unsigned int i = 0; i < numA; i++) {
      double maxAbs = 0.0; // with 0.0, all have zero-tax SQ as their initial position
      auto ai = (const LeonActor*)es[i];
      auto t0 = KMatrix::uniform(rng, eDim, 1, -maxAbs, +maxAbs);
      auto t1 = eMod0->makeFTax(t0);
      VctrPstn* ep = nullptr;
      const double minU = 0.0; // could be ai->refU, or with default, 0.6
      const double maxU = 1.0; // could be (1+ai->refU)/2, or with default, 0.8
      assert(0 < ai->refU);
      double ui = -1;
      while ((ui < minU) || (maxU < ui)) {
        if (nullptr != ep) {
          delete ep;
        }
        t0 = KMatrix::uniform(rng, eDim, 1, -maxAbs, +maxAbs);
        t1 = eMod0->makeFTax(t0);
        ep = new VctrPstn(t1);
        ui = ai->posUtil(ep);
      }
      ps.push_back(ep);
    }
    assert(numA == ps.size());

    for (unsigned int i = 0; i < numA; i++) {
      eMod0->addActor(es[i]);
      eSt0->addPstn(ps[i]);
    }
  }
  // end of local-var block

  for (unsigned int i = 0; i < numA; i++) {
    auto ai = (const LeonActor*)eMod0->actrs[i];
    auto pi = (const VctrPstn*)eSt0->pstns[i];
    printf("%2u: %s , %s \n", i, ai->name.c_str(), ai->desc.c_str());
    cout << "voting rule: " << ai->vr << endl;
    cout << "Pos vector: ";
    trans(*pi).mPrintf(" %+7.3f ");
    cout << "Cap vector: ";
    trans(ai->vCap).mPrintf(" %7.3f ");
    printf("minS: %.3f \n", ai->minS);
    printf("refS: %.3f \n", ai->refS);
    printf("maxS: %.3f \n", ai->maxS);
    cout << endl << flush;
  }

  eSt0->setAUtil(-1, KBase::ReportingLevel::Low);
  auto u = eSt0->aUtil[0];
  assert(numA == eSt0->model->numAct);

  auto vfn = [eMod0, eSt0](unsigned int k, unsigned int i, unsigned int j) {
    assert(j != i);
    auto ak = ((LeonActor*)(eMod0->actrs[k]));
    double vk = ak->vote(i, i, j, eSt0);
    return vk;
  };

  KMatrix c = Model::coalitions(vfn, eMod0->actrs.size(), eSt0->pstns.size());
  cout << "Coalition strength matrix" << endl;
  c.mPrintf(" %9.3f ");
  cout << endl << flush;

  const auto vpm = VPModel::Linear;
  const auto pcem = PCEModel::ConditionalPCM;

  const auto pv2 = Model::probCE2(pcem, vpm, c);
  const auto p = get<0>(pv2);
  const auto pv = get<1>(pv2);

  cout << "Probability Opt_i > Opt_j" << endl;
  pv.mPrintf(" %.4f ");
  cout << endl;

  cout << "Probability Opt_i" << endl;
  p.mPrintf(" %.4f ");
  auto eu0 = u*p;
  cout << "Expected utility to actors: " << endl;
  eu0.mPrintf(" %.4f ");
  cout << endl << flush;

  // from here on out, the content is copied from demoEUEcon
  //LeonState * eSt0 = ((LeonState *)(eMod0->history[0]));

  eSt0->aUtil = vector<KMatrix>(); // dropping any old ones
  eSt0->step = [eSt0]() {
    return eSt0->stepSUSN();
  };

  eMod0->run();

  // JAH 2060814 want to display a matrix of final policies, as well as the final mean policy
  // this is predominantly for automatic extraction of results
  auto stfinal = eMod0->history[eMod0->history.size() - 1];
  printf("%u Iterations Completed\nFINAL POLICIES\n", eMod0->history.size() - 1);

  // compute the mean - I tried to get this as a method of the LeonModel and LeonState, but failed :-(
  auto p0 = ((VctrPstn*)(stfinal->pstns[0]));
  // want to display as row vectors, so might as well start of transposing
  KMatrix meanP = KMatrix(p0->numC(), p0->numR(), 0.0);
  for (unsigned int i = 0; i < eMod0->numAct; i++)
  {
    auto iPos = ((VctrPstn*)(stfinal->pstns[i]));
    auto y = KBase::trans(*iPos);
    meanP = meanP + y;
    y.mPrintf(" %+0.4f ");
  }
  meanP = meanP / numA;
  // talk
  printf("FINAL MEAN POLICY\n");
  meanP.mPrintf(" %+0.4f ");

  cout << endl;
  cout << "Estimates of GDP over time" << endl;
  eMod0->printEstGDP();

  delete eMod0; // and all the actors in it
  eMod0 = nullptr;

  return;
}

} // namespace


int main(int ac, char **av) {
  using std::cout;
  using std::endl;
  using std::flush;
  using KBase::dSeed;

  auto sTime = KBase::displayProgramStart();
  uint64_t seed = dSeed;
  bool run = true;
  bool euEconP = false;
  bool maxEconP = false;
  bool rlEconP = false;

  auto showHelp = []() {
    printf("\n");
    printf("Usage: specify one or more of these options\n");
    printf("--help            print this message\n");
    printf("--euEcon          exp. util. of IO econ model\n");
    printf("--maxEcon         max support of IO econ model\n");
    printf("--rlEcon          same as euEcon but with the synthesized data\n");
    printf("--seed <n>        set a 64bit seed, in decimal\n");
    printf("                  0 means truly random\n");
    printf("                  default: %020llu \n", dSeed);
  };

  // tmp args
  //seed = 0;
  //maxEconP = true;


  if (ac > 1) {
    for (int i = 1; i < ac; i++) {
      if (strcmp(av[i], "--seed") == 0) {
        i++;
        seed = std::stoull(av[i]);
      }
      else if (strcmp(av[i], "--euEcon") == 0) {
        euEconP = true;
      }
      else if (strcmp(av[i], "--maxEcon") == 0) {
        maxEconP = true;
      }
      else if (strcmp(av[i], "--rlEcon") == 0) {
        rlEconP = true;
      }
      else if (strcmp(av[i], "--help") == 0) {
        run = false;
      }
      else {
        run = false;
        printf("Unrecognized argument %s\n", av[i]);
      }
    }
  }

  if (!run) {
    showHelp();
    return 0;
  }

  printf("Given PRNG seed:  %020llu \n", seed);
  PRNG * rng = new PRNG();
  seed = rng->setSeed(seed); // 0 == get a random number

  printf("Using PRNG seed:  %020llu \n", seed);
  printf("Same seed in hex:   0x%016llX \n", seed);

  // don't understand why these were set to L = M = 5 and N = 10; the article had L = 3, M = 2, N = 5
  const unsigned int numF = 3; // 5; // 2;
  const unsigned int numG = 2; //5; // 2;
  const unsigned int numS = 5; //10; //5;

  // note that we reset the seed every time, so that in case something
  // goes wrong, we need not scroll back too far to find the
  // seed required to reproduce the bug.

  // JAH 20160811; Even if someone runs leonApp with the maxEcon flag,
  // both euEcon and maxEcon are run. I'm unsure what maxEcon really
  // does vis-a-vis euEcon, but this seems like unintended behavior to me.
  if (euEconP) {
    cout << "E----------------------------------" << endl;
    DemoLeon::demoEUEcon(seed, numF, numG, numS, rng);
  }
  if (maxEconP) {
    cout << "M----------------------------------" << endl;
    DemoLeon::demoMaxEcon(seed, numF, numG, numS, rng);
  }
  if (rlEconP)
  {
    cout << "R----------------------------------" << endl;
    DemoLeon::demoRealEcon(seed, rng);
  }
  cout << "-----------------------------------" << endl;

  delete rng;
  KBase::displayProgramEnd(sTime);
  return 0;
}


// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
