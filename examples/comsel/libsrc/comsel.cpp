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
//
// define these:
//  tuple< KMatrix, VUI> CSState::pDist(int persp) const;
//  bool CSState::equivNdx(unsigned int i, unsigned int j) const;
//  void CSState::setAllAUtil(ReportingLevel rl);
//  CSState* CSState::doSUSN
//  CSState* CSState::stepSUSN
// --------------------------------------------

#include "smp.h"
#include "comsel.h"
#include "hcsearch.h"

namespace ComSelLib {
  using std::printf;
  using std::cout;
  using std::endl;
  using std::flush;
  using std::get;
  using std::string;

  using KBase::VBool;
  using KBase::printVUI;
  using KBase::VPModel;
  using KBase::PCEModel;

  using KBase::Model;

  using SMPLib::SMPModel;

  // --------------------------------------------
  VUI intToVB(unsigned int x, unsigned int n) {
    VUI vb = {};
    vb.resize(n);
    for (unsigned int i = 0; i < n; i++) {
      vb[i] = (x % 2);
      x = x / 2;
    }

    return vb;
  }


  unsigned int vbToInt(const VUI & vb) {
    auto n = ((const unsigned int)(vb.size()));
    unsigned int x = 0;
    for (unsigned int i = 0; i < n; i++) {
      x = 2 * x;
      if (1 == vb[n - (i + 1)]) {
        x = x + 1;
      }
    }
    return x;
  }

  // --------------------------------------------
  // JAH 20160711 added rng seed JAH 20160802 added sql flags
  CSModel::CSModel(unsigned int nd,  string d, uint64_t s, vector<bool> f) : Model(d, s, f) {

    assert(nd > 0);

    numDims = nd;
  }


  CSModel::~CSModel() {
    if (nullptr != actorCSPstnUtil) {
      delete actorCSPstnUtil;
      actorCSPstnUtil = nullptr;
    }
    if (nullptr != actorSpPstnUtil) {
      delete actorSpPstnUtil;
      actorSpPstnUtil = nullptr;
    }

  }


  bool CSModel::equivStates(const CSState * rs1, const CSState * rs2) {
    auto numA = ((const unsigned int)(rs1->pstns.size()));
    assert(numA == rs2->pstns.size());
    bool rslt = true;
    for (unsigned int i = 0; i < numA; i++) {
      auto m1 = ((MtchPstn*)(rs1->pstns[i]));
      auto m2 = ((MtchPstn*)(rs2->pstns[i]));
      bool same = (m1->match == m2->match);
      //cout << "Match on position " << i << "  " << same << endl << flush;
      rslt = rslt && same;
    }
    return rslt;
  }


  double CSModel::getActorCSPstnUtil(unsigned int ai, unsigned int pj) {
    if (nullptr == actorCSPstnUtil) {
      setActorSpPstnUtil();
      setActorCSPstnUtil();
    }
    assert(ai < actorCSPstnUtil->numR());
    assert(pj < actorCSPstnUtil->numC());
    double uij = (*actorCSPstnUtil)(ai, pj);
    return uij;
  }

  void CSModel::setActorSpPstnUtil() {
    assert(actorSpPstnUtil == nullptr);
    const unsigned int na = numAct;
    actorSpPstnUtil = new KMatrix(na, na);
    auto setuij = [this](unsigned int i, unsigned int j) {
      double uij = oneSpPstnUtil(i, j);
      (*actorSpPstnUtil)(i, j) = uij;
      return;
    };
    KMatrix::mapV(setuij, na, na);

    cout << "Actor spatial position utility matrix: " << endl;
    actorSpPstnUtil->mPrintf(" %5.3f");
    cout << endl << flush;

    return;
  }

  double CSModel::oneSpPstnUtil(unsigned int ai, unsigned int pj) const {
    const double bigR = 0.5; // moderatly risk-averse in spatial positions
    auto csai = ((const CSActor*)(actrs[ai]));
    auto csaj = ((const CSActor*)(actrs[pj]));
    const KMatrix spi = csai->vPos;
    const KMatrix spj = csaj->vPos;
    KMatrix vDiff = spi - spj;
    const auto vSal = csai->vSal;
    const double u = SMPModel::bvUtil(vDiff, vSal, bigR);
    return u;
  }

  // return the clm-vector of actors' expected utility for this particular committee
  KMatrix CSModel::oneCSPstnUtil(const VUI& vb) const {
    assert(actorSpPstnUtil != nullptr); // prerequisite data must be provided

    // TODO: change this to handle only unique positions
    assert(actorSpPstnUtil->numR() == numAct);
    assert(actorSpPstnUtil->numC() == numAct);

    assert(numAct == vb.size()); // must be correct size

    assert(1.0 < nonCommDivisor); // noncommittee members must have reduced strength, with same sign

    // vote_k(i:j), using the effective strengths for this committee
    auto vkij = [this, vb](unsigned int k, unsigned i, unsigned int j) {
      auto ak = (CSActor*)(actrs[k]);
      double sk = ak->sCap;
      switch (vb[k]) {
      case 0: // not on committee, reduce strength
        sk = sk / nonCommDivisor;
        break;
      case 1: // on committee, use full strength
        break;
      default:
        cout << "CSModel::oneCSPstnUtil - unrecognized match value: " << vb[k] << endl << flush;
        assert(false); // no way to recover from this programming error
        break;
      }
      auto v = Model::vote(ak->vr, sk, (*actorSpPstnUtil)(k, i), (*actorSpPstnUtil)(k, j));
      return v;
    };

    const KMatrix c = Model::coalitions(vkij, numAct, numAct); // coalitions for/against
    const KMatrix pv = Model::vProb(vpm, c); // prob of victory, square
    const KMatrix p = Model::probCE(pcem, pv); // prob of outcomes, column
    const KMatrix eu = (*actorSpPstnUtil) * p; // column of expected utilities to each actor
    return eu;
  }

  void CSModel::setActorCSPstnUtil() {
    assert(actorSpPstnUtil != nullptr); // prerequisite data must be provided
    assert(actorCSPstnUtil == nullptr);
    assert(nullptr != rng);
    auto numPos = ((unsigned int)(0.5 + exp2(numAct)));
    auto rawUij = KMatrix(numAct, numPos);
    for (unsigned int j = 0; j < numPos; j++) {
      const VUI vbj = intToVB(j, numAct);
      const KMatrix euj = oneCSPstnUtil(vbj);
      assert(numAct == euj.numR());
      assert(1 == euj.numC());
      for (unsigned int i = 0; i < numAct; i++) {
        rawUij(i, j) = euj(i, 0);
      }
    }
    auto uij = KBase::rescaleRows(rawUij, 0.0, 1.0); // von Neumann utility scale
    actorCSPstnUtil = new KMatrix(uij);
    return;
  }

  // --------------------------------------------
  CSState::CSState(CSModel * m) : State(m) {
    // nothing yet
  }


  CSState::~CSState() {
    // nothing yet
  }


  void CSState::show() const {
    const unsigned int numA = model->numAct;
    for (unsigned int i = 0; i < numA; i++) {
      auto pi = ((MtchPstn*)(pstns[i]));
      printf("Position %02u: ", i);
      printVUI(pi->match);
      cout << endl << flush;
    }
    auto pu = pDist(-1);
    KMatrix p = get<0>(pu);
    auto uNdx = get<1>(pu);
    printf("There are %zu unique positions \n", uNdx.size());
    for (unsigned int i1 = 0; i1 < uNdx.size(); i1++) {
      unsigned int i2 = uNdx[i1];
      printf("  %2u:  %.4f \n", i2, p(i1, 0));
    }
    cout << endl;
    return;
  }


  tuple< KMatrix, VUI> CSState::pDist(int persp) const {
    /// Calculate the probability distribution over states from this perspective

    // TODO: convert this to a single, commonly used setup function
    // It seems to require only that the Actor must have "vr" and "sCap" data members.

    const unsigned int numA = model->numAct;
    const unsigned int numP = numA; // for this demo, the number of positions is exactly the number of actors

    // get unique indices and their probability
    assert(0 < uIndices.size()); // should have been set with setUENdx();
    //auto uNdx2 = uniqueNdx(); // get the indices to unique positions

    auto numU = ((const unsigned int)(uIndices.size()));
    assert(numU <= numP); // might have dropped some duplicates

    cout << "Number of aUtils: " << aUtil.size() << endl << flush;

    const KMatrix u = aUtil[0]; // all have same beliefs in this demo

    auto uufn = [u, this](unsigned int i, unsigned int j1) {
      return u(i, uIndices[j1]);
    };

    auto uMat = KMatrix::map(uufn, numA, numU);
    assert(uMat.numR() == numA); // must include all actors
    assert(uMat.numC() == numU);

    // vote_k ( i : j )
    auto vkij = [this, uMat](unsigned int k, unsigned int i, unsigned int j) {
      auto ak = (CSActor*)(model->actrs[k]);
      auto v_kij = Model::vote(ak->vr, ak->sCap, uMat(k, i), uMat(k, j));
      return v_kij;
    };

    // the following uses exactly the values in the given euMat,
    // which may or may not be square
    const KMatrix c = Model::coalitions(vkij, uMat.numR(), uMat.numC());
    const KMatrix pv = Model::vProb(model->vpm, c); // square
    const KMatrix p = Model::probCE(model->pcem, pv); // column
    const KMatrix eu = uMat*p; // column

    assert(numA == eu.numR());
    assert(1 == eu.numC());

    return tuple <KMatrix, VUI>(p, uIndices);
  }

  // Given the utility matrix, uMat, calculate the expected utility to each actor,
  // as a column-vector. Again, this is from the perspective of whoever developed uMat.
  KMatrix CSState::expUtilMat(KBase::ReportingLevel rl, unsigned int numA, unsigned int numP, const KMatrix & uMat) const {
    // BTW, be sure to lambda-bind uMat *after* it is modified.
    assert(uMat.numR() == numA); // must include all actors
    assert(uMat.numC() <= numP); // might have dropped some duplicates

    auto assertRange = [](const KMatrix& m, unsigned int i, unsigned int j) {
      // due to round-off error, we must have a tolerance factor
      const double tol = 1E-10;
      const double mij = m(i, j);
      if ((mij + tol < 0.0) || (1.0 + tol < mij)) {
        printf("%f  %i  %i  \n", mij, i, j);
        cout << flush;
      }
      assert(0.0 <= mij + tol);
      assert(mij <= 1.0 + tol);
      return;
    };

    // in Haskell, we could just using currying
    auto uRng = [assertRange, uMat](unsigned int i, unsigned int j) { assertRange(uMat, i, j); return; };
    KMatrix::mapV(uRng, uMat.numR(), uMat.numC());

    auto vkij = [this, uMat](unsigned int k, unsigned int i, unsigned int j) { // vote_k(i:j)
      auto ak = (CSActor*)(model->actrs[k]);
      auto v_kij = Model::vote(ak->vr, ak->sCap, uMat(k, i), uMat(k, j));
      return v_kij;
    };

    // the following uses exactly the values in the given euMat,
    // which may or may not be square
    const KMatrix c = Model::coalitions(vkij, uMat.numR(), uMat.numC());
    const KMatrix pv = Model::vProb(model->vpm, c); // square
    const KMatrix p = Model::probCE(model->pcem, pv); // column
    const KMatrix eu = uMat*p; // column

    assert(numA == eu.numR());
    assert(1 == eu.numC());
    // in Haskell, we could just using currying
    auto euRng = [assertRange, eu](unsigned int i, unsigned int j) { assertRange(eu, i, j); return; };
    KMatrix::mapV(euRng, eu.numR(), eu.numC());


    if (ReportingLevel::Low < rl) {
      printf("Util matrix is %i x %i \n", uMat.numR(), uMat.numC());
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
  };


  CSState* CSState::stepSUSN() {
    cout << endl << flush;
    cout << "State number " << model->history.size() - 1 << endl << flush;
    if ((0 == uIndices.size()) || (0 == eIndices.size())) {
      setUENdx();
    }
    setAUtil(-1, ReportingLevel::Silent);
    show();

    auto s2 = doSUSN(ReportingLevel::Silent);
    s2->step = [s2]() {
      return s2->stepSUSN();
    };
    cout << endl << flush;
    return s2;
  }


  CSState * CSState::doSUSN(ReportingLevel rl) const {
    // cout << "CSState::doSUSN not yet implemented" << endl; // TODO: finish this
    // assert(false);

    const unsigned int numA = model->numAct;
    assert(numA == model->actrs.size());

    auto numU = ((const unsigned int)(uIndices.size()));
    assert((0 < numU) && (numU <= numA));
    assert(numA == eIndices.size());

    const KMatrix u = aUtil[0]; // all have same beliefs in this demo

    auto numP = ((const unsigned int)(pstns.size()));


    // Given the utility matrix, uMat, calculate the expected utility to each actor,
    // as a column-vector. Again, this is from the perspective of whoever developed uMat.
    auto euMat = [rl, numA, numP, this](const KMatrix & uMat) { return expUtilMat(rl, numA, numP, uMat); };
    auto euState = euMat(u);
    cout << "Actor expected utilities: ";
    KBase::trans(euState).mPrintf("%6.4f, ");
    cout << endl << flush;

    if (ReportingLevel::Low < rl) {
      printf("--------------------------------------- \n");
      printf("Assessing utility of actual state to all actors \n");
      for (unsigned int h = 0; h < numA; h++) {
        cout << "not available" << endl;
      }
      cout << endl << flush;
      printf("Out of %u positions, %u were unique: ", numA, numU);
      cout << flush;
      for (auto i : uIndices) {
        printf("%2i ", i);
      }
      cout << endl;
      cout << flush;
    }


    auto uufn = [u, this](unsigned int i, unsigned int j1) { return u(i, uIndices[j1]); };
    auto uUnique = KMatrix::map(uufn, numA, numU);


    // Get expected-utility vector, one entry for each actor, in the current state.
    const KMatrix eu0 = euMat(uUnique); // 'u' with duplicates, 'uUnique' without duplicates

    CSState * s2 = new CSState((CSModel*)model);

    //s2->pstns = vector<KBase::Position*>();
    for (unsigned int h = 0; h < numA; h++) {
      s2->pstns.push_back(nullptr);
    }


    // TODO: clean up the nesting of lambda-functions.
    // need to create a hypothetical state and run setOneAUtil(h,Silent) on it
    //
    // The newPosFn does a generic hill climb to find the best next position for actor h,
    // and stores it in s2.
    // To do that, it defines three functions for evaluation, neighbors, and show:
    // efn, nfn, and sfn.
    auto newPosFn = [this, rl, euMat, u, eu0, s2](const unsigned int h) {
      s2->pstns[h] = nullptr;
      auto ph = ((const MtchPstn *)(pstns[h]));

      // Evaluate h's estimate of the expected utility, to h, of
      // advocating position mp. To do this, build a hypothetical utility matrix representing
      // h's estimates of the direct utilities to all other actors of h adopting this
      // Position. Do that by modifying the h-column of h's matrix.
      // Then compute the expected probability distribution, over h's hypothetical position
      // and everyone else's actual position. Finally, compute the expected utility to
      // each actor, given that distribution, and pick out the value for h's expected utility.
      // That is the expected value to h of adopting the position.
      auto efn = [this, euMat, rl, u, h](const MtchPstn & mph) {
        // This correctly handles duplicated/unique options
        // We modify the given euMat so that the h-column
        // corresponds to the given mph, but we need to prune duplicates as well.
        // This entails some type-juggling.
        const KMatrix uh0 = aUtil[h];
        assert(KBase::maxAbs(u - uh0) < 1E-10); // all have same beliefs in this demo
        const unsigned int nI = ((CSModel*)model)->numItm;
        if (mph.match.size() != nI) {
          cout << "Size of match object " << mph.match.size();
          cout << " does not match number of items " << nI << endl;
          cout << flush;
        }
        assert(mph.match.size() == nI);
        auto uh = uh0;
        for (unsigned int i = 0; i < model->numAct; i++) {
          auto ai = (CSActor*)(model->actrs[i]);
          double uih = ai->posUtil(&mph);
          uh(i, h) = uih; // utility to actor i of this hypothetical position by h
        }

        // 'uh' now has the correct h-column. Now we need to see how many options
        // are unique in the hypothetical state, and keep only those columns.
        // This entails juggling back and forth between the all current positions
        // and the one hypothetical position (mph at h).
        // Thus, the next call to euMat will consider only unique options.
        auto equivHNdx = [this, h, mph](const unsigned int i, const unsigned int j) {
          // this little function takes care of the different types needed to compare
          // dynamic pointers to positions (all but h) with a constant position (h itself).
          // In other words, the comparisons for index 'h' use the hypothetical mph, not pstns[h]
          bool rslt = false;
          auto mpi = ((const MtchPstn *)(pstns[i]));
          auto mpj = ((const MtchPstn *)(pstns[j]));
          assert(mpi != nullptr);
          assert(mpj != nullptr);
          if (i == j) {
            rslt = true; // Pi == Pj, always
          }
          else if (h == i) {
            rslt = (mph == (*mpj));
          }
          else if (h == j) {
            rslt = ((*mpi) == mph);
          }
          else {
            rslt = ((*mpi) == (*mpj));
          }
          return rslt;
        };

        auto ns = KBase::uiSeq(0, model->numAct - 1);
        const VUI uNdx = get<0>(KBase::ueIndices<unsigned int>(ns, equivHNdx));
        auto numU = ((const unsigned int)(uNdx.size()));
        auto hypUtil = KMatrix(model->numAct, numU);
        // we need now to go through 'uh', copying column J the first time
        // the J-th position is determined to be equivalent to something in the unique list
        for (unsigned int i = 0; i < model->numAct; i++) {
          for (unsigned int j1 = 0; j1 < numU; j1++) {
            unsigned int j2 = uNdx[j1];
            hypUtil(i, j1) = uh(i, j2); // hypothetical utility in column h
          }
        }

        if (false) {
          cout << "constructed hypUtil matrix:" << endl << flush;
          hypUtil.mPrintf(" %8.2f ");
          cout << endl << flush;
        }


        if (ReportingLevel::Low < rl) {
          printf("--------------------------------------- \n");
          printf("Assessing utility to %2i of hypo-pos: ", h);
          printVUI(mph.match);
          cout << endl << flush;
          printf("Hypo-util minus base util: \n");
          (uh - uh0).mPrintf(" %+.4E ");
          cout << endl << flush;
        }
        const KMatrix eu = euMat(hypUtil); // uh or hypUtil
        // BUG: If we use 'uh' here, it passes the (0 <= delta-EU) test, because
        // both hypothetical and actual are then calculated without dropping duplicates.
        // If we use 'hypUtil' here, it sometimes gets (delta-EU < 0), because
        // the hypothetical drops duplicates but the actual (computed elsewhere) does not.
        // FIX: fix  the 'elsewhere'
        const double euh = eu(h, 0);
        assert(0 < euh);
        return euh;
      }; // end of efn



      // return vector of neighboring committees
      auto nfn = [](const MtchPstn & mp0) { auto csVec = mp0.neighbors(2); return csVec; };

      // show some representation of this position on cout
      auto sfn = [](const MtchPstn & mp0) { printVUI(mp0.match); return; };

      auto ghc = new KBase::GHCSearch<MtchPstn>();
      ghc->eval = efn;
      ghc->nghbrs = nfn;
      ghc->show = sfn;

      auto rslt = ghc->run(*ph, // start from h's current positions
        ReportingLevel::Silent,
        100, // iter max
        3, 0.001); // stable-max, stable-tol

      if (ReportingLevel::Low < rl) {
        printf("---------------------------------------- \n");
        printf("Search for best next-position of actor %2i \n", h);
        //printf("Search for best next-position of actor %2i starting from ", h);
        //trans(*aPos).printf(" %+.6f ");
        cout << flush;
      }

      double vBest = get<0>(rslt);
      MtchPstn pBest = get<1>(rslt);
      unsigned int iterN = get<2>(rslt);
      unsigned int stblN = get<3>(rslt);

      delete ghc;
      ghc = nullptr;
      if (ReportingLevel::Medium < rl) {
        printf("Iter: %u  Stable: %u \n", iterN, stblN);
        printf("Best value for %2i: %+.6f \n", h, vBest);
        cout << "Best position:    " << endl;
        cout << "numCat: " << pBest.numCat << endl;
        cout << "numItm: " << pBest.numItm << endl;
        cout << "perm: ";
        printVUI(pBest.match);
        cout << endl << flush;
      }
      MtchPstn * posBest = new MtchPstn(pBest);
      s2->pstns[h] = posBest;
      // no need for mutex, as s2->pstns is the only shared var,
      // and each h is different.

      double du = vBest - eu0(h, 0); // (hypothetical, future) - (actual, current)
      if (ReportingLevel::Low < rl) {
        printf("EU improvement for %2i of %+.4E \n", h, du);
      }
      //printf("  vBest = %+.6f \n", vBest);
      //printf("  eu0(%i, 0) for %i = %+.6f \n", h, h, eu0(h,0));
      //cout << endl << flush;
      // Logically, du should always be non-negative, as GHC never returns a worse value than the starting point.
      // However, actors plan on the assumption that all others do not change - yet they do.
      const double eps = 0.05; // 0.025; // enough to avoid problems with round-off error
      assert(-eps <= du);
      return;
    }; // end of newPosFn

    const bool par = true; // parallel, asynch searches for new positions?

    auto ts = vector<std::thread>();
    // Each actor, h, finds the position which maximizes their EU in this situation.
    for (unsigned int h = 0; h < numA; h++) {
      if (par) { // launch all, concurrent
        ts.push_back(std::thread([newPosFn, h]() {
          newPosFn(h);
          return;
        }));
      }
      else { // do each, sequential
        newPosFn(h);
      }
    }

    if (par) { // now join them all before continuing
      for (auto& t : ts) {
        t.join();
      }
    }

    assert(nullptr != s2);
    assert(numP == s2->pstns.size());
    assert(numA == s2->model->numAct);
    for (auto p : s2->pstns) {
      assert(nullptr != p);
    }
    s2->setUENdx();

    return s2;
  }
  // end of doSUSN 


  CSState * CSState::doBCN(ReportingLevel rl) const {
    CSState * cs2 = nullptr;
    cout << "CSState::doBCN not yet implemented" << endl; // TODO: finish this
    assert(false);
    return cs2;
  }


  bool CSState::equivNdx(unsigned int i, unsigned int j) const {
    /// Compare two actual positions in the current state
    auto mpi = ((const MtchPstn *)(pstns[i]));
    auto mpj = ((const MtchPstn *)(pstns[j]));
    assert(mpi != nullptr);
    assert(mpj != nullptr);
    bool rslt = ((*mpi) == (*mpj));
    return rslt;
  }

  void CSState::setAllAUtil(ReportingLevel rl) {
    auto csm = ((CSModel*)model);
    const unsigned int na = csm->numAct;
    assert(Model::minNumActor <= na);
    assert(na <= Model::maxNumActor);
    assert(0 == aUtil.size());
    auto u = KMatrix(na, na);

    for (unsigned int i = 0; i < na; i++) {
      for (unsigned int j = 0; j < na; j++) {
        auto pj = ((const MtchPstn*)(pstns[j]));
        VUI vj = pj->match;
        unsigned int nj = vbToInt(vj);
        u(i, j) = csm->getActorCSPstnUtil(i, nj);
      }
    }

    // this sets the utility matrices to all be identical
    for (unsigned int i = 0; i < na; i++) { aUtil.push_back(u); }
    return;
  }

  // --------------------------------------------
  CSActor::CSActor(string n, string d, CSModel* csm) : Actor(n, d) {
    csMod = csm;
  }

  CSActor::~CSActor() {
    csMod = nullptr;
  }

  double CSActor::posUtil(const Position * ap1) const {
    //cout << "CSActor::posUtil - not yet implemented" << endl << flush; // TODO: complete this
    //assert(false);

    auto ai = csMod->actrNdx(this);
    auto rp = ((const MtchPstn *)ap1);
    auto nj = vbToInt(rp->match);
    double u0 = csMod->getActorCSPstnUtil(ai, nj);
    return u0;
  }


  double CSActor::vote(unsigned int est, unsigned int i, unsigned int j, const State* st) const {
    /// vote between the current positions to actors at positions p1 and p2 of this state

    unsigned int k = st->model->actrNdx(this);
    auto uk = st->aUtil[est];
    double uhki = uk(k, i);
    double uhkj = uk(k, j);
    const double vij = Model::vote(vr, sCap, uhki, uhkj);
    return vij;
  }

  double CSActor::vote(const Position* ap1, const Position* ap2) const {
    double u1 = posUtil(ap1);
    double u2 = posUtil(ap2);
    const double v12 = Model::vote(vr, sCap, u1, u2);
    return v12;
  }


  void CSActor::randomize(PRNG* rng, unsigned int nDim) {
    assert(nullptr != rng);
    assert(0 < nDim);

    sCap = exp(log(10.0)*rng->uniform(1.0, 2.0)); // 10 to 100, median at 31.6 
    vPos = VctrPstn(KMatrix::uniform(rng, nDim, 1, 0.01, 0.99));

    double totalSal = rng->uniform(0.8, 1.0);
    auto vsi = KMatrix::uniform(rng, nDim, 1, 0.90, 0.95);
    vSal = (totalSal / sum(vsi)) *  vsi;
    return;
  }


};
// end of namespace

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------

