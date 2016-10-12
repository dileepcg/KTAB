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
// the fundamental KBase namespace
// -------------------------------------------------

#include "demomtch.h"

using KBase::PRNG;

namespace DemoMtch {
  using std::cout;
  using std::endl;
  using std::flush;
  using std::function;
  using std::get;
  using std::tuple;
  using std::vector;


  using KBase::KMatrix;
  using KBase::Actor;
  using KBase::Model;
  using KBase::Position;
  using KBase::State;
  using KBase::PCEModel;
  using KBase::VotingRule;
  using KBase::VPModel;
  using KBase::GHCSearch;
  using KBase::ReportingLevel;

  // Are these matchings equivalent?
  bool equivMtchPstn(const MtchPstn & mp1, const MtchPstn & mp2) {
    bool p = (mp1.numCat == mp2.numCat) && (mp1.numItm == mp2.numItm);
    if (p) {
      for (unsigned int i = 0; p && (i < mp1.numItm); i++) {
        p = p && (mp1.match[i] == mp2.match[i]);
      }
    }
    return p;
  }

  void showMtchPstn(const MtchPstn & mp) { 
    cout << "[MtchPstn ";
    for (auto m : mp.match) {
      cout << m << " ";
    }
    cout << "]";
    return;
  }

  bool stableMtchState(unsigned int iter, const State* s1) {
    bool earlyP = (0 == iter);
    auto ms1 = (const MtchState *)s1;
    const unsigned int numA = s1->model->numAct;
    unsigned int numC = 0;
    if (0 < iter) { // there was a previous state
      auto ms0 = ((const MtchState *)(s1->model->history[iter - 1]));
      for (unsigned int i = 0; i < numA; i++) {
        auto p0i = ((const MtchPstn*)(ms0->pstns[i]));
        auto p1i = ((const MtchPstn*)(ms1->pstns[i]));
        if (!equivMtchPstn(*p0i, *p1i)) {
          numC++;
        }
      }
    }

    printf("Number of changed matchings: %u \n", numC);

    return (!earlyP && (0 == numC));
  }

  // -------------------------------------------------

  MtchActor::MtchActor(string n, string d) : Actor(n, d){
    idNum = 0;
    vr = VotingRule::Proportional;
    pMod = PropModel::ExpUtil;
    sCap = 0;
    vals = vector<double>();
  }

  MtchActor::~MtchActor() {
    //
  }

  double MtchActor::vote(unsigned int est,unsigned int i, unsigned int j, const State* st) const {
	  unsigned int k = st->model->actrNdx(this);
	  auto uk = st->aUtil[est];
	  double uhki = uk(k, i);
	  double uhkj = uk(k, j);
	  const double vij = Model::vote(vr, sCap, uhki, uhkj);
	  return vij;
  }

  double MtchActor::vote(const Position * ap1, const Position * ap2) const {
    double u1 = posUtil(ap1);
    double u2 = posUtil(ap2);
    double v = Model::vote(vr, sCap, u1, u2);
    return v;
  }

  double MtchActor::posUtil(const Position * ap1) const  {
    auto p1 = ((const MtchPstn *)(ap1));
    const unsigned int n = vals.size();
    assert(n == p1->numItm);
    assert(n == p1->match.size());
    double v = 0;
    for (unsigned int i = 0; i < n; i++){
      if (idNum == p1->match[i]){
        v = v + vals[i];
      }
    }
    assert(0 <= v);
    assert(v <= 1);
    double u = 1.0 - (1 - v)*(1 - v); // adds risk-aversion, declining marginal utility, first few candies matter most, etc.
    return u;
  }

  void MtchActor::randomize(PRNG* rng, double minCap, double maxCap, unsigned int id, unsigned int numI) {
    idNum = id;
    sCap = rng->uniform(minCap, maxCap);
    // for fun, I do randomize their voting rules for this demo
    unsigned int vrNum = rng->uniform() % 3;
    switch (vrNum){
    case 0: vr = VotingRule::PropBin;      break;
    case 1: vr = VotingRule::Proportional; break;
    case 2: vr = VotingRule::PropCbc;      break;
    }

    // Weights are non-zero and sum to 1, but some much less desirable than others.
    // Thus, the minimum possible utility is 0 (get nothing), while the maximum is 1 (get everything)
    auto vij = [rng](unsigned int i, unsigned int j) {
      double a = rng->uniform(0, 1);
      double b = rng->uniform(0, 1);
      double v = (a + (b*b)) / 2;
      return v;
    };
    auto vm = KMatrix::map(vij, numI, 1);
    vm = vm / sum(vm);
    vals = vector<double>();
    for (auto v : vm){ vals.push_back(v); }
    return;
  }

  MtchPstn* MtchActor::rPos(unsigned int numI, unsigned int numC, PRNG * rng) {
    MtchPstn* p = new MtchPstn();
    p->numItm = numI;
    p->numCat = numC;
    p->match = VUI();
    for (unsigned int i = 0; i < numI; i++){
      unsigned int aID = rng->uniform() % numC;
      p->match.push_back(aID);
    }
    return p;
  }

  MtchActor* MtchActor::rAct(unsigned int numI, double minCap, double maxCap, PRNG* rng, unsigned int i) {
    string ni = "MActor-";
    ni.append(std::to_string(i));
    string di = "Random matching actor";
    MtchActor * ai = new MtchActor(ni, di);
    ai->randomize(rng, minCap, maxCap, i, numI);
    return ai;
  }

  // -------------------------------------------------
  MtchState::MtchState(Model* mod) : State(mod){
    //
  }

  MtchState::~MtchState() {
    //
  }
 
  tuple <KMatrix, VUI> MtchState::pDist(int persp) const {
    auto na = model->numAct;
    auto pcem = model->pcem;
    auto w = actrCaps();
    auto vr = VotingRule::Proportional;
    auto rl = KBase::ReportingLevel::Silent;
    auto vpm = VPModel::Linear;
    auto uij = KMatrix(na, na);

    if ((0 <= persp) && (persp < na)) {
      uij = aUtil[persp];
    }
    else if (-1 == persp) {
      for (unsigned int i = 0; i < na; i++) {
        for (unsigned int j = 0; j < na; j++) {
          auto ui = aUtil[i];
          uij(i, j) = aUtil[i](i, j);
        }
      }
    }
    else {
      cout << "SMPState::pDist: unrecognized perspective, " << persp << endl << flush;
      assert(false);
    }
    auto pd = Model::scalarPCE(na, na, w, uij, vr, vpm, pcem, rl);
    
    // TODO: test whether all positions are unique or not, see RPState::pDist for an example
    auto uNdx = VUI();
    for (unsigned int i = 0; i < na; i++) {
      uNdx.push_back(i);
    }
    return tuple< KMatrix, VUI> (pd, uNdx); 
  }


bool MtchState::equivNdx(unsigned int i, unsigned int j) const {
 assert (false); // TODO: finish this stub 
  return false;
}

  KMatrix MtchState::actrCaps() const {
    auto wFn = [this](unsigned int i, unsigned int j) {
      auto aj = ((MtchActor*)(model->actrs[j]));
      return aj->sCap;
    };
    auto w = KMatrix::map(wFn, 1, model->numAct);
    return w;
  }

  MtchModel* MtchModel::randomMS(unsigned int numA, unsigned int numI, VotingRule vr, MtchActor::PropModel pMod, uint64_t seed) {

    // The situation is a set of numA actors dividing a pile of numI sweets among themselves.
    // Hence, the categories are "for actor 0", "for actor 1", etc. so numCat = numAct
    const unsigned int numC = numA;
    const double minCap = 100;
    const double maxCap = 225;

    switch (pMod) {
    case MtchActor::PropModel::ExpUtil:
      cout << "Actors maximize expected utility, given positions of others" << endl;
      break;
    case MtchActor::PropModel::Probability:
      cout << "Actors maximize probability of adoption, given positions of others" << endl;
      break;
    case MtchActor::PropModel::AgreeUtil:
      cout << "Actors maximize expected utility of agreed-upon position, given positions of others" << endl;
      break;
    }

    // I will reset every actor to use the same voting rule, or else it can get very confusing.
    // With different voting rules, actor I might think that actor J's EU is x. Because
    // J uses a different voting rule, he might think his EU is lower, and might even
    // conclude that the best EU he can achieve is y, where y < x. From J's perspective,
    // he has improved the utility *he* expects under *his* beliefs - but that may or may
    // not look like an improvement to I under *his* beliefs.
    //
    // So I avoid this correct-but-confusing behavior by using the same VR for all.
    cout << "Using voting rule " << vr << endl;

    cout << "Randomly generated actors with random positions: " << endl;
    auto md0 = new MtchModel("", seed);
    md0->numItm = numI;
    md0->numCat = numC;
    auto st0 = new MtchState(md0);
    for (unsigned int i = 0; i < numA; i++){
      auto ai = MtchActor::rAct(numI, minCap, maxCap, md0->rng, i);
      ai->vr = vr;
      MtchPstn* pi = MtchActor::rPos(numI, numA, md0->rng);
      md0->addActor(ai);
      st0->addPstn(pi);

      printf("%2u: %s,  %s \n", i, ai->name.c_str(), ai->desc.c_str());
      printf("Scalar capability: %.2f \n", ai->sCap);
      printf("Voting rule: %s \n", KBase::vrName(ai->vr).c_str());
      printf("Values assigned to each sweet: \n");
      for (unsigned int j = 0; j < numI; j++) {
        printf(" %.4f ", ai->vals[j]);
      }
      cout << endl;
      showMtchPstn(*pi);
      cout << endl << endl;
    }

    md0->addState(st0);
    return md0;
  }


  // -------------------------------------------------
  // JAH 20160711 added rng seed
  MtchModel::MtchModel(string d, uint64_t s, vector<bool> f) : Model(d, s, f) {
    numCat = 0;
    numItm = 0;
  }

  MtchModel::~MtchModel(){
    numCat = 0;
    numItm = 0;
  }

  // -------------------------------------------------
  void demoDivideSweets(uint64_t s ) {

   printf("Using PRNG seed: %020llu \n", s);
   auto rng = new PRNG();
   rng->setSeed(s);

    const unsigned int numI = 25;
    const unsigned int numA = 7;
    printf("Dividing %u sweets between %u actors \n", numI, numA);

    double minCap = 50;
    double maxCap = 100;

    cout << "Generate actors with random voting rules, values-of-sweets and positions (matchings)" << endl;
    auto as = vector<Actor*>();
    auto ps = vector<MtchPstn*>();
    for (unsigned int i = 0; i < numA; i++) {
      ps.push_back(MtchActor::rPos(numI, numA, rng));
      MtchActor* ai = MtchActor::rAct(numI, minCap, maxCap, rng, i);
      as.push_back(ai);


      printf("%2u: %s , %s \n", i, ai->name.c_str(), ai->desc.c_str());
      printf("Capability: %.2f \n", ai->sCap);
      cout << "Voting rule: " << ai->vr << endl;
      cout << "Valuation of each sweet: ";
      for (auto v : ai->vals) {
        printf("%5.3f  ", v);
      }
      cout << endl;
      cout << "Current position (who gets each sweet): ";
      for (auto m : ps[i]->match) {
        printf("%2i  ", m);
      }
      cout << endl << endl;
    }

    auto uFn1 = [as, ps](unsigned int i, unsigned int j) {
      auto ai = ((MtchActor*)(as[i]));
      double uij = ai->posUtil(ps[j]);
      return uij;
    };
    auto u = KMatrix::map(uFn1, numA, numA);

    cout << "Raw actor-pos util matrix" << endl;
    u.mPrintf(" %.4f ");
    cout << endl << flush;

    auto vfn = [as, ps](unsigned int k, unsigned int i, unsigned int j) {
      auto ak = ((MtchActor*)(as[k]));
      return ak->vote(ps[i], ps[j]);
    };


    auto c = Model::coalitions(vfn, as.size(), ps.size());
    cout << "Coalition strength matrix" << endl;
    c.mPrintf(" %+9.3f ");
    cout << endl << flush;

    auto vpm = VPModel::Linear;
    auto pcem = PCEModel::ConditionalPCM;

    const auto ppv = Model::probCE2(pcem, vpm, c);
    auto p = get<0>(ppv);
    auto pv = get<1>(ppv);

    cout << "Probability Opt_i > Opt_j" << endl;
    pv.mPrintf(" %.4f ");
    cout << endl;
    cout << "Probability Opt_i" << endl;
    p.mPrintf(" %.4f ");

    if (KBase::testProbCE) {
      //cout << "Testing probCE in demoDivideSweets ... " << flush;
      auto pv0 = Model::vProb(vpm, c);
      assert(KBase::norm(pv - pv0) < 1E-6);
      auto p0 = Model::probCE(pcem, pv0);
      assert(KBase::norm(p - p0) < 1E-6);
      //cout << "ok" << endl << flush;
    }

    cout << "Expected utility to actors: " << endl;
    (u*p).mPrintf(" %+8.3f ");
    cout << endl << flush;

    for (auto a : as) { delete a; }
    for (auto p : ps) { delete p; }
    return;
  }

  // ------------------------------------------------- 
  void demoMaxSupport(uint64_t s) {
    printf("Using PRNG seed: %020llu \n", s);
    auto rng = new PRNG();
    rng->setSeed(s);

    const unsigned int numA = 7;
    const unsigned int numI = 20;
    const unsigned int numC = numA;
    // because we are assigning sweets to children, the categories  are 
    // "for Alice", "for Bob", "for Carol", and so on.
    MtchGene* mg1 = new MtchGene();
    mg1->numCat = numC;
    mg1->numItm = numI;
    mg1->randomize(rng);
    cout << "Random matching of " << numI << " items to " << numA << " actors" << endl;
    cout << (*mg1) << endl; 
    delete mg1;
    mg1 = nullptr;

    // start: cut-n-paste creation and display code from above 

    // Suppose, for symmetry, that the seven actors value all items exactly the same,
    // two are Strong (X), and five are Weak (Y).
    // Condition A: 2X < 5Y, so the two strong cannot just take everything
    // Condition B: 2X+Y > 4Y, so a coalition of the strong and one defector can exclude the others.
    // Suppose Y=100. Then A&B imply 150 < X < 250.
    // The interesting values are likely to be the ends, the mid-point,
    // and the middles of the intervals on either side of it: {150, 175, 200, 225, 250}
    // The most strategically interesting are likely to be 175 and 225.
    // At 225, they barely need an ally to win: the weak party has little leverage.
    // At 175, they need an ally to win barely: the weak party has a lot of leverage.
    //
    // Note that, because the actors all want different things, the following can easily happen.
    // S0 and S1 both value the same items highly. W2, W3, and W4 do not want what S0 wants,
    // and S0 does not want what they want (or at least the conflict is less severe than with S1).
    // Clearly, X+2Y < X+3Y, so S0 and W2,W3,W4 can join forces to exclude everyone else, even S1.
    // S0 can give W2,W3,W4 what he does not want, and they give him what they do not want,
    // and everyone in the deal gets everything they do want. Log-rolling in action.

    double minCap = 100;
    double maxCap = 225;
    assert(2 * maxCap <= 5 * minCap);
    assert(2 * maxCap + minCap >= 4 * minCap);

    cout << "Generate actors with random voting rules, values-of-sweets and positions (matchings)" << endl;
    cout << "Utilities are normalized to [0,1] scale" << endl;
    cout << "minCap: " << minCap << endl;
    cout << "maxCap: " << maxCap << endl;
    cout << "Note: 2*maxCap          <= 5*minCap" << endl;
    cout << "      2*maxCap + minCap >= 4*minCap" << endl << flush;
    auto as = vector<Actor*>();
    auto ps = vector<MtchPstn*>();
    for (unsigned int i = 0; i < numA; i++){
      MtchPstn* pi = MtchActor::rPos(numI, numA, rng);
      ps.push_back(pi);
      auto ai = MtchActor::rAct(numI, minCap, maxCap, rng, i);
      as.push_back(ai);
    }

    if (false) {
      // set interesting strengths
      ((MtchActor*)as[0])->sCap = maxCap;
      ((MtchActor*)as[1])->sCap = maxCap;

      ((MtchActor*)as[2])->sCap = minCap;
      ((MtchActor*)as[3])->sCap = minCap;
      ((MtchActor*)as[4])->sCap = minCap;
      ((MtchActor*)as[5])->sCap = minCap;
      ((MtchActor*)as[6])->sCap = minCap;
    }

    for (unsigned int i = 0; i < numA; i++) {
      auto ai = (MtchActor*)as[i];
      printf("%2u: %s , %s \n", i, ai->name.c_str(), ai->desc.c_str());
      printf("Capability: %.2f \n", ai->sCap);
      cout << "Voting rule: " << ai->vr << endl;
      cout << "Valuation of each sweet: ";
      for (unsigned int j = 0; j < numI; j++){
        printf("%5.3f  ", ai->vals[j]);
      }
      cout << endl;
      cout << "Current position: ";
      showMtchPstn(*ps[i]);
      cout << endl << endl;
    }

    auto uFn1 = [as, ps](unsigned int i, unsigned int j) {
      auto ai = ((MtchActor*)(as[i]));
      MtchPstn* pj = ps[j];
      double uij = ai->posUtil(pj);
      return uij;
    };

    auto u = KMatrix::map(uFn1, numA, numA);
    cout << "Raw actor-pos util matrix" << endl;
    u.mPrintf(" %.4f ");
    cout << endl << flush;
    // end: cut-n-paste creation and display code from above

    auto zeta = [](const vector<Actor*> & as, const MtchPstn * p){
      double z = 0;
      for (auto a : as){
        auto ta = ((MtchActor*)a);
        double wu = (ta->sCap)*(ta->posUtil(p));
        z = z + wu;
      }
      return z;
    };


    for (unsigned int i = 0; i < ps.size(); i++){
      printf("zeta[%u] = %7.3f \n", i, zeta(as, ps[i]));
    }

    // Now we setup a GAOpt to look for the position which maximizes zeta.
    unsigned int gps = 20;
    printf("gpool: %u   \n", gps);
    auto gOpt = new KBase::GAOpt<MtchGene>(gps);

    gOpt->cross = [](const MtchGene* g1, const MtchGene* g2, PRNG* rng) {
      return g1->cross(g2, rng);
    };

    gOpt->mutate = [](const MtchGene* g1, PRNG* rng) {
      return g1->mutate(rng);
    };

    gOpt->eval = [numC, numI, as, zeta](const MtchGene* mg) {
      assert(numC == mg->numCat);
      assert(numI == mg->numItm);
      double z = zeta(as, mg);
      return z;
    };

    gOpt->showGene = [](const MtchGene* mg){ 
      cout << (*mg);
      return;
    };

    gOpt->equiv = [](const MtchGene* mg1, const MtchGene* mg2) {
      return mg1->equiv(mg2);
    };

    gOpt->makeGene = [numC, numI, as, ps](PRNG * rng) {
      MtchGene* m = new MtchGene();
      m->setState(as, ps);
      m->numCat = numC;
      m->numItm = numI;
      m->randomize(rng);
      return m;
    };

    cout << "Filling gpool ..." << endl;
    gOpt->fill(rng);
    cout << "Initial fully random gpool: " << endl;
    gOpt->show();
    cout << endl << endl << flush;

    // copy the best of a random lot
    // gOpt->sortPop();
    // auto mgPtr = get<1>(gOpt->getNth(0));
    // auto mg0 = MtchPstn(*mgPtr);



    unsigned int iter = 0;
    unsigned int sIter = 0;
    double cf = 2.0;
    double mf = 2.0;
    printf("crossFrac: %.2f  mutFrac: %.2f \n", cf, mf);
    auto srl = KBase::ReportingLevel::Low;
    gOpt->run(rng, cf, mf, 1000, 0.2, 50, srl, iter, sIter);

    cout << endl << endl << "Final gpool: " << endl;
    gOpt->show();
    cout << endl << endl << flush;
    delete gOpt;

    // try the same thing via GHC over MtchPstn
    auto ghc = new KBase::GHCSearch<MtchPstn>();

    auto eFn = [as, zeta](const MtchPstn mp) {
      double z = zeta(as, &mp);
      return z; };
    ghc->eval = eFn;

    unsigned int numVar = 2;
    function <vector<MtchPstn>(const MtchPstn)> nFn = [numVar](const MtchPstn mg) { return mg.neighbors(numVar); };
    ghc->nghbrs = nFn;

    ghc->show = showMtchPstn;

    // make a random starting point
    auto mgPtr = MtchActor::rPos(numI, numA, rng);
    ghc->run(*mgPtr, KBase::ReportingLevel::Medium, 100, 3, 0.001);
    delete mgPtr;

    delete ghc;
    for (auto a : as) { delete a; }
    for (auto p : ps) { delete p; }
    return;
  }




  void demoMtchSUSN(uint64_t s) {

    printf("Using PRNG seed: %020llu \n", s);
    auto rng = new PRNG();
    rng->setSeed(s);


    unsigned int numA = 6;  // number of actors
    unsigned int numC = numA;
    unsigned int numI = 17; // number of sweets being divied up

    const VotingRule vr = VotingRule::Proportional;

    auto md0 = MtchModel::randomMS(numA, numI, vr, MtchActor::PropModel::ExpUtil, s);
    for (unsigned int i = 0; i < numA; i++) {
      auto ai = (MtchActor*)(md0->actrs[i]);
      if (0 == (i % 3)) {
        ai->pMod = MtchActor::PropModel::Probability;
        // so they are 2/3 EU-maximizers and 1/3 P-maximizers
      }

      //ai->pMod = MtchActor::PropModel::ExpUtil;
      // As a matter of fact, they all P-Max (apologies to Gerry O'Driscoll).
    }


    auto st0 = ((MtchState*)(md0->history[0]));
    auto a0 = ((MtchActor*)(md0->actrs[0]));
    auto p00 = ((MtchPstn*)(st0->pstns[0]));

    assert(numI == p00->numItm);
    assert(numA == md0->numAct);
    assert(numC == p00->numCat);

    unsigned int maxIter = 20;
    md0->stop = [maxIter](unsigned int iter, const State * s) {
      bool stableP = stableMtchState(iter, s);
      bool tooLongP = (maxIter <= iter);
      return (stableP || tooLongP);
    };

    st0->step = [st0]() {return st0->stepSUSN(); };


    cout << "Demonstrate SUSN bargaining over division of sweets with " << numA << " actors and " << numI << " sweets" << endl;
    cout << "With non-Proportional voting, the actors generally do not stabilize their positions (within 10 turns, if ever)" << endl;
    cout << "When it does stabilze, the positions of utility-maximizers stabilize but do not converge, while" << endl;
    cout << "the positions of probability-maximizers do converge." << endl;

    st0->setAUtil(-1, ReportingLevel::Low);
    auto u = st0->aUtil[0]; // everyone got the same perspective, in this demo

    cout << "Util matrix for U(actor_r, pstn_c) in random initial state: " << endl;
    u.mPrintf(" %.4f ");

    auto pn = st0->pDist(-1);
    auto p = std::get<0>(pn);

    cout << "Probability of outcomes in random initial state: " << endl;
    p.mPrintf(" %.4f ");
    cout << endl << flush;

    cout << "Expected utility to actors in random initial state: " << endl;
    (u*p).mPrintf(" %.4f "); //TODO: may need to modify this to use only unique positions
    cout << endl << flush;
//    st0->setAUtil();

    md0->run();

    return;
  }

  void multiMtchSUSN(uint64_t s) {
    printf("Using PRNG seed: %020llu \n", s);
    unsigned int numTrial = 3;
    unsigned int numStbl = 0;
    for (unsigned int i = 1; i <= numTrial; i++) {
      if (oneMtchSUSN(s))
        numStbl++;
      printf("Stabilized: %u in %u / %u runs\n", numStbl, i, numTrial);
    }

    return;
  }

  bool oneMtchSUSN(uint64_t s) {

    unsigned int numA = 6;  // number of actors
    unsigned int numC = numA;
    unsigned int numI = 17; // number of sweets being divied up

    const VotingRule vr = VotingRule::Proportional;

    auto md0 = MtchModel::randomMS(numA, numI, vr, MtchActor::PropModel::ExpUtil, s);
    for (unsigned int i = 0; i < numA; i++) {
      auto ai = (MtchActor*)(md0->actrs[i]);
      if (0 == (i % 3)) {
        ai->pMod = MtchActor::PropModel::Probability;
      }
    }
    // so they are 2/3 EU-maximizers and 1/3 P-maximizers

    auto st0 = ((MtchState*)(md0->history[0]));
    auto a0 = ((MtchActor*)(md0->actrs[0]));
    auto p00 = ((MtchPstn*)(st0->pstns[0]));

    assert(numI == p00->numItm);
    assert(numA == md0->numAct);
    assert(numC == p00->numCat);


    cout << "Demonstrate SUSN bargaining over division of sweets with " << numA << " actors and " << numI << " sweets" << endl;
    cout << "With non-Proportional voting, the actors generally do not stabilize their positions (within 10 turns, if ever)" << endl;
    cout << "When it does stabilze, the positions of utility-maximizers stabilize but do not converge, while" << endl;
    cout << "the positions of probability-maximizers do converge." << endl;

    st0->setAUtil(-1, ReportingLevel::Low);
    auto u = st0->aUtil[0]; // everyone gets the same perspective, in this demo

    cout << "Util matrix for U(actor_r, pstn_c) in random initial state: " << endl;
    u.mPrintf(" %.4f ");

    auto pn = st0->pDist(-1);
    auto p = std::get<0>(pn);

    cout << "Probability of outcomes in random initial state: " << endl;
    p.mPrintf(" %.4f ");
    cout << endl << flush;

    cout << "Expected utility to actors in random initial state: " << endl;
    (u*p).mPrintf(" %.4f "); // TODO: may need to modify this to use only unique positions
    cout << endl << flush;

    auto newPstns = vector<Position*>();
    bool changed = true;
    unsigned int iter = 0;
    unsigned int iterMax = 15;

    while (changed && (iter < iterMax)) { // rounds of the SUSN process
      newPstns = vector<Position*>();
      changed = false;
      cout << "Starting iteration " << iter << endl;
      for (unsigned int ih = 0; ih < numA; ih++) {
        MtchActor* ah = (MtchActor*)(md0->actrs[ih]);

        switch (ah->pMod) {
        case MtchActor::PropModel::ExpUtil:
          printf("maxEU search for actor %u ... \n", ih);
          break;
        case MtchActor::PropModel::Probability:
          printf("maxProb search for actor %u ... \n", ih);
          break;
        case MtchActor::PropModel::AgreeUtil:
          printf("maxAgU search for actor %u ... \n", ih);
          break;
        }

        auto evmp = ah->maxProbEUPstn(ah->pMod, st0);
        printf("Found %.4f at this matching: \n", get<0>(evmp));
        showMtchPstn(get<1>(evmp));
        cout << endl;
        double du = get<0>(evmp) -(u*p)(ih, 0);
        printf("Actual change in value:  %+.4f \n", du);
        cout << flush;

        // Expected improvements do not always occur, because other actors also change their positions.
        // In state 0, actor i searches for a position which is better for him, assuming
        // no one else changes their position. His expected delta-EU, delta-Prob, etc will be positive..
        // But when all actors do the same kind of planning in parallel, then all change position 
        // simultaneously his new utility, by his own analysis, may be worse than he expected,
        // and possibly even worse than his original position.
        // "Sometimes ya gotta run fast just to stay put."
        //
        // This is most likely to happen to weak players.

        MtchPstn* opi = (MtchPstn*)(st0->pstns[ih]);
        MtchPstn* npi = new MtchPstn(get<1>(evmp));
        newPstns.push_back(npi);

        printf("Old position %2u, %4u: ", ih, iter); showMtchPstn(*opi); cout << endl;
        printf("New position %2u, %4u: ", ih, iter); showMtchPstn(*npi); cout << endl;

        bool ei = equivMtchPstn(*npi, *opi);
        if (!ei) {
          cout << "Actor " << ih << " expects a strategic improvement" << endl;
        }
        cout << flush;

        cout << endl;
        changed = changed || !ei;
      }
      if (!changed) {
        cout << "No actor expects improvement" << endl;
      }

      cout << "Update state ..." << flush;
      for (unsigned int i = 0; i < numA; i++) {
        delete st0->pstns[i];
        st0->pstns[i] = newPstns[i];
      }

      // update the u_h_ij matrices
      st0->setAUtil(-1, ReportingLevel::Low);
      auto u2 = st0->aUtil[0]; // everyone got the same perspective

      cout << " done" << endl;

      cout << "Util matrix for U(actor_r, pstn_c): " << endl;
      u2.mPrintf(" %.4f ");

      auto pn2 = st0->pDist(-1);
      auto p2 = std::get<0>(pn2);

      cout << "Probability of outcomes: " << endl;
      p2.mPrintf(" %.4f ");
      cout << endl << flush;

      cout << "Expected utility to actors: " << endl;
      (u2*p2).mPrintf(" %.4f "); // TODO: may need to modify this to use only unique positions
      cout << endl << flush;


      iter++;
      cout << endl << flush;
    }
    return (iter < iterMax);
  }


  tuple<double, MtchPstn> MtchActor::maxProbEUPstn(PropModel pm, const MtchState * mst) const {
    // Note that this requires & assumes that aUtil be already setup

    using KBase::ReportingLevel;
 
    const VPModel vpm = mst->model->vpm;
    const PCEModel pcem = mst->model->pcem;

    const unsigned int numA = mst->model->numAct;
    unsigned int ih = mst->model->actrNdx(this);
    const auto uh = mst->aUtil[ih];
    const auto w = mst->actrCaps();

    //auto wFn = [st](unsigned int i, unsigned int j) {
    //  auto aj = ((MtchActor*)(st->model->actrs[j]));
    //  return aj->sCap;
    //};
    //const KMatrix w = KMatrix::map(wFn, 1, numA);

    auto utilH = [mst, uh, ih, numA](const MtchPstn* ph) {
      auto u = uh; // copy
      for (unsigned int i = 0; i < numA; i++) {
        auto ai = ((MtchActor*)(mst->model->actrs[i]));
        double uih = ai->posUtil(ph);
        u(i, ih) = uih;
      }
      return u;
    };


    // Note that, for demo purposes, each actor assess the expected utility or the
    // probability-of-adoptions of their proposal under the assumption that everyone
    // uses the same voting rule as do they.
    auto assessProbEU = [numA, utilH, w, ih, pm, vpm, pcem, this](const MtchPstn  ph) { 
      auto u = utilH(&ph);
      auto p = Model::scalarPCE(numA, numA, w, u, vr, vpm, pcem, ReportingLevel::Silent);
      auto eu = u*p;
      double peu = 0;
      double noAgreementPenalty = 0.333;
      switch (pm) {
      case MtchActor::PropModel::ExpUtil:
        peu = eu(ih, 0); // expected utility maximizer
        break;
      case MtchActor::PropModel::Probability:
        peu = p(ih, 0); // probability maximizer
        break;
      case MtchActor::PropModel::AgreeUtil:
        peu = p(ih, 0)*u(ih, ih) - (1 - p(ih, 0))*noAgreementPenalty;
        break;
      }
      return peu;
    };


    auto ghc = KBase::GHCSearch<MtchPstn>();
    ghc.eval = assessProbEU;
    ghc.nghbrs = [](MtchPstn mp) { return mp.neighbors(2); };
    ghc.show = showMtchPstn;

    auto r0 = ghc.run(*((MtchPstn*)(mst->pstns[ih])), KBase::ReportingLevel::Silent, 100, 1, 0.001);

    double bestEU = get<0>(r0);
    MtchPstn bestMP = get<1>(r0);
    // iteration number and stability count currently ignored
    //unsigned int iNum = get<2>(r0);
    //unsigned int sNum = get<3>(r0);

    auto r1 = tuple<double, MtchPstn>(bestEU, bestMP);
    return r1;
  }


  MtchState * MtchState::stepSUSN() {
    if (0 == aUtil.size()) {
      setAUtil(-1, ReportingLevel::Medium);
    }
    auto s2 = doSUSN(ReportingLevel::Medium);
    s2->step = [s2]() {return s2->stepSUSN(); };
    return s2;
  }




  void MtchState::setAllAUtil(ReportingLevel rl) {
    unsigned int numA = model->numAct;

    auto uFn = [this](unsigned int i, unsigned int j) {
      auto ai = ((MtchActor*)(model->actrs[i]));
      auto pj = ((MtchPstn*)(pstns[j]));
      double uij = ai->posUtil(pj);
      return uij;
    };
    auto u = KMatrix::map(uFn, numA, numA);

    aUtil = vector<KMatrix>();

    for (unsigned int h = 0; h < numA; h++) {
      aUtil.push_back(u); // everyone gets the same perspective
    }
    return;
  }

  MtchState * MtchState::doSUSN(ReportingLevel rl) const {

    MtchState * s2 = new MtchState(model);

    const unsigned int numA = model->numAct;
    for (unsigned int ih = 0; ih < numA; ih++) {
      MtchActor* ah = (MtchActor*)(model->actrs[ih]);
      if (ReportingLevel::Low < rl) {
        switch (ah->pMod) {
        case MtchActor::PropModel::ExpUtil:
          printf("maxEU search for actor %u ... \n", ih);
          break;
        case MtchActor::PropModel::Probability:
          printf("maxProb search for actor %u ... \n", ih);
          break;
        case MtchActor::PropModel::AgreeUtil:
          printf("maxAgU search for actor %u ... \n", ih);
          break;
        }
      }
      auto evmp = ah->maxProbEUPstn(ah->pMod, this); // all the real action is in this function

      if (ReportingLevel::Low < rl) {
        printf("Found %.4f at this matching: \n", get<0>(evmp));
        showMtchPstn(get<1>(evmp));
        cout << endl;
      }

      // Expected improvements do not always occur, because other actors also change their positions.
      // In state 0, actor i searches for a position which is better for him, assuming
      // no one else changes their position. His expected delta-EU, delta-Prob, etc will be positive.
      //
      // But if all actors do the same kind of planning in parallel and all change position simultaneously,
      // then his new utility, by his own analysis, may be worse than he expected, and possibly even
      // worse than his original position: "Sometimes ya gotta run fast just to stay put."
      //
      // This is most likely to happen to weak players.

      MtchPstn* oldPi = (MtchPstn*)(pstns[ih]);
      MtchPstn* newPi = new MtchPstn(get<1>(evmp));
      s2->addPstn(newPi);

      if (ReportingLevel::Low < rl) {
        printf("Old position %2u: ", ih); showMtchPstn(*oldPi); cout << endl;
        printf("New position %2u: ", ih); showMtchPstn(*newPi); cout << endl;
      }
    } // end of loop over ih, actors


    if (ReportingLevel::Low < rl) {
      s2->setAUtil(-1, ReportingLevel::Silent);
      auto u2 = s2->aUtil[0]; // they all have the same aUtil matrix, in this demo.

      auto pn2 = pDist(-1); // objective perspective
      auto p2 = std::get<0>(pn2);
      cout << "Util matrix for U(actor_r, pstn_c) in new state: " << endl;
      u2.mPrintf(" %.4f ");

      cout << "Probability of outcomes in new state: " << endl;
      p2.mPrintf(" %.4f ");
      cout << endl << flush;

      cout << "Expected utility to actors in new state: " << endl;
      (u2*p2).mPrintf(" %.4f "); // TODO: may need to modify this to use only unique positions
      cout << endl << flush;
    }

    return s2;
  }




  MtchState * MtchState::stepBCN() {
    setAUtil(-1, ReportingLevel::Medium);
    auto s2 = doBCN(ReportingLevel::Medium);
    s2->step = [s2]() {return s2->stepBCN(); };
    return s2;
  }

  MtchState * MtchState::doBCN(ReportingLevel rl) const  {
    MtchState * s2 = nullptr;
    assert(false);
    return s2;
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
  bool dosP = false;
  bool maxSupP = false;
  bool mtchSUSNP = true; // debugging with Visual Studio

  auto showHelp = []() {
    printf("\n");
    printf("Usage: specify one or more of these options\n");
    printf("--help            print this message\n");
    printf("--dos             division of sweets\n");
    printf("--maxSup          max support in division of sweets\n");
    printf("--mtchSUSN        SUSN bargaining over division of sweets\n");
    printf("--seed <n>        set a 64bit seed\n");
    printf("                  0 means truly random\n");
    printf("                  default: %020llu \n", dSeed);
  };

  // tmp args
  // seed = 0;


  if (ac > 1) {
    for (int i = 1; i < ac; i++) {
      if (strcmp(av[i], "--seed") == 0) {
        i++;
        seed = std::stoull(av[i]);
      }
      else if (strcmp(av[i], "--maxSup") == 0) {
        maxSupP = true;
      }
      else if (strcmp(av[i], "--mtchSUSN") == 0) {
        mtchSUSNP = true;
      }
      else if (strcmp(av[i], "--dos") == 0) {
        dosP = true;
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

  if (!run){
    showHelp();
    return 0;
  }

  PRNG * rng = new PRNG();
  seed = rng->setSeed(seed); // 0 == get a random number
  printf("Using PRNG seed:  %020llu \n", seed);
  printf("Same seed in hex:   0x%016llX \n", seed);

  // note that we reset the seed every time, so that in case something 
  // goes wrong, we need not scroll back too far to find the
  // seed required to reproduce the bug.  
  if (dosP) {
    cout << "-----------------------------------" << endl;
    DemoMtch::demoDivideSweets(seed);
  }
  if (maxSupP) {
    cout << "-----------------------------------" << endl;
    DemoMtch::demoMaxSupport(seed);
  }
  if (mtchSUSNP) {
    cout << "-----------------------------------" << endl;
    DemoMtch::demoMtchSUSN(seed);
  }
  cout << "-----------------------------------" << endl;

  delete rng;
  KBase::displayProgramEnd(sTime);
  return 0;
}


// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
