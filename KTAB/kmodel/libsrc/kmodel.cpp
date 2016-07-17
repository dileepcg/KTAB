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
 
#include <time.h>
#include "kmodel.h"

//#ifdef WIN32
//#define localtime_r(_Time, _Tm) localtime_s(_Tm, _Time)
//#endif
namespace KBase {

using std::cout;
using std::endl;
using std::flush;
using std::get;
using std::tuple;

// --------------------------------------------
// JAH 20160711 added seed
Model::Model(PRNG * r, string desc, uint64_t s) {
    history = vector<State*>();
    actrs = vector<Actor*>();
    numAct = 0;
    stop = nullptr;
    assert(nullptr != r);
    rng = r;

    // Record the UTC time so it can be used as the default scenario name
    std::chrono::time_point<std::chrono::system_clock> st;
    st = std::chrono::system_clock::now();
    std::time_t start_time = std::chrono::system_clock::to_time_t(st);
	 
	 
	/*tm localTime;
	localtime_r(&start_time, &localTime);*/

	const std::chrono::duration<double> tse = st.time_since_epoch();
	std::chrono::seconds::rep milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(tse).count() % 1000;


	auto utcBuffId = newChars(500);
	auto hshCode = newChars(100);
    if (0 == desc.length()) {
        auto utcBuff = newChars(200);
		
        std::strftime(utcBuff, 150, "Scenario-UTC-%Y-%m-%u-%H%M-%S", gmtime(&start_time));
        cout << "No scenario description provided to Model::Model, " << endl;
        cout << "generating default name from UTC start time." << endl << flush;
        scenName = utcBuff;
	    // Scenario Id Generation  include the microsecond
		 
		sprintf(utcBuffId, "%s_%u", utcBuff, milliseconds);
	 
        delete utcBuff;
        utcBuff = nullptr;
		
    }
    else
    {
        scenName = desc;
		sprintf(utcBuffId, "%s_%u", desc.c_str(), milliseconds);
	}
	//get the hash
	uint64_t scenIdhash = (std::hash < std::string> () (utcBuffId))   ;
	sprintf(hshCode, "0x%032llX", scenIdhash);
	scenId = hshCode;
	delete hshCode;
	hshCode = nullptr;
	delete utcBuffId;
	utcBuffId = nullptr;

    // BPW 20160717
    // (a) record a reproducible seed even we were given '0'
    // (b) make sure that the given value really is the initial seed
    // (c) at this point, the PRNG* is almost irrelevant, except that someone
    // might, in the future, provide an object which was a new subclass of PRNG*
    if (0 == s) {
      rng->setSeed(0); // random and irreproducible
      s = rng->uniform(); // random and irreproducible 64-bits
    }
    rng->setSeed(s);

    // JAH 20160711 save the seed for the rng
    rngSeed = s;

    cout << "Scenario assigned name: -|" << scenName.c_str() << "|-" << endl << flush;
}


Model::~Model() {
    while (0 < history.size()) {
        State* s = history[history.size() - 1];
        delete s;
        history.pop_back();
    }

    while (0 < actrs.size()) {
        Actor * a = actrs[actrs.size() - 1];
        delete a;
        actrs.pop_back();
    }
    numAct = 0;
    rng = nullptr;
}


void Model::run() {
    assert(1 == history.size());
    State* s0 = history[0];
    bool done = false;
    unsigned int iter = 0;
 
    while (!done) {
        assert(nullptr != s0);
        assert(nullptr != s0->step);
        iter++;
        cout << "Starting Model::run iteration " << iter << endl;
        auto s1 = s0->step();
        addState(s1);
        done = stop(iter, s1);
        s0 = s1;
    }
    return;
}

string vpmName(const VPModel& vpm) {
    string s="";
    switch (vpm) {
    case  VPModel::Linear:
        s = "Linear";
        break;
    case VPModel::Square:
        s = "Square";
        break;
    case VPModel::Quartic:
        s = "Quartic";
        break;
    case VPModel::Binary:
        s = "Binary";
        break;
    default:
        cout << "VPMName: unrecognized VPModel" << endl;
        assert(false);
        break;
    }
    return s;
}

ostream& operator<< (ostream& os, const VPModel& vpm) {
    os << vpmName(vpm);
    return os;
}

unsigned int Model::addActor(Actor* a) {
    assert(nullptr != a);
    actrs.push_back(a);
    numAct = ((unsigned int) (actrs.size()));
    return numAct;
}


unsigned int Model::addState(State* s) {
    assert(nullptr != s);
    assert(this == s->model);
    s->model = this;
    history.push_back(s);
    auto hs = ((const unsigned int) (history.size()));
    return hs;
}


KMatrix Model::bigRfromProb(const KMatrix & p, BigRRange rr) {
    double pMin = 1.0;
    double pMax = 0.0;
    for (double pi : p) {
        assert(0.0 <= pi);
        pMin = (pi < pMin) ? pi : pMin;
        pMax = (pi > pMax) ? pi : pMax;
    }

    const double pTol = 1E-8;
    assert(fabs(1 - KBase::sum(p)) < pTol);

    function<double(unsigned int, unsigned int)> rfn = nullptr;
    switch (rr) {
    case BigRRange::Min:
        rfn = [pMin, pMax, p](unsigned int i, unsigned int j) {
            return (p(i, j) - pMin) / (pMax - pMin);
        };
        break;
    case BigRRange::Mid:
        rfn = [pMin, pMax, p](unsigned int i, unsigned int j) {
            return (3 * p(i, j) - (pMax + 2 * pMin)) / (2 * (pMax - pMin));
        };
        break;
    case BigRRange::Max:
        rfn = [pMin, pMax, p](unsigned int i, unsigned int j) {
            return (2 * p(i, j) - (pMax + pMin)) / (pMax - pMin);
        };
        break;
    }
    auto rMat = KMatrix::map(rfn, p.numR(), p.numC());
    return rMat;
}


// returns h's estimate of i's risk attitude, using the risk-adjustment-rule
double Model::estNRA(double rh, double  ri, BigRAdjust ra) {
    double rhi = 0.0;
    switch (ra) {
    case BigRAdjust::FullRA:
        rhi = ri;
        break;
    case BigRAdjust::TwoThirdsRA:
        rhi = (rh + (2.0*ri)) / 3.0;
        break;
    case BigRAdjust::HalfRA:
        rhi = (rh + ri) / 2;
        break;
    case BigRAdjust::OneThirdRA:
        rhi = ((2.0*rh) + ri) / 3.0;
        break;
    case BigRAdjust::NoRA:
        rhi = rh;
        break;
    }
    return rhi;
}

int Model::actrNdx(const Actor* a) const {
    int ai = -1;
    for (unsigned int i = 0; i < numAct; i++) {
        if (a == actrs[i]) {
            ai = i;
        }
    }
    return ai; // negative iff this "actor" was not found
}

double Model::nProd(double x, double y) {
    if ((0.0 < x) && (0.0 < y)) {
        return (x * y);
    }
    if ((x < 0.0) && (y < 0.0)) {
        return (x + y);
    }
    return ((x < y) ? x : y);
}

string vrName(const VotingRule& vr) {
    string vrn = "";
    switch (vr) {
    case VotingRule::Binary:
        vrn = "Binary";
        break;
    case VotingRule::PropBin:
        vrn = "PropBin";
        break;
    case VotingRule::Proportional:
        vrn = "Prop";
        break;
    case VotingRule::PropCbc:
        vrn = "PropCbc";
        break;
    case VotingRule::Cubic:
        vrn = "Cubic";
        break;
    default:
        throw KException("vrName - Unrecognized VotingRule");
        break;
    }
    return vrn;
}

ostream& operator<< (ostream& os, const VotingRule& vr) {
    os << vrName(vr);
    return os;
}


string stmName(const StateTransMode& stm) {
    string stmName = "";
    switch (stm) {
    case StateTransMode::DeterminsticSTM:
        stmName = "DeterminsticSTM";
        break;
    case StateTransMode::StochasticSTM:
        stmName = "StochasticSTM";
        break;
    default:
        throw KException("stmName - Unrecognized StateTransMode");
        break;
    }
    return stmName;
}


ostream& operator<< (ostream& os, const StateTransMode& stm) {
    os << stmName(stm);
    return os;
}


// At this point, the LISP keyword 'defmacro' should leap to mind, again.
string pcmName(const PCEModel& pcm) {
    string s="";
    switch (pcm) {
    case PCEModel::MarkovPCM:
        s = "MarkovPCM";
        break;
    case PCEModel::ConditionalPCM:
        s = "ConditionalPCM";
        break;
    default:
        throw KException("pcmName - Unrecognized PCEModel");
        break;

    }
    return s;
}

ostream& operator<< (ostream& os, const PCEModel& pcm) {
    os << pcmName(pcm);
    return os;
}


string tpcName(const ThirdPartyCommit& tpc) {
    string tpcn="";
    switch (tpc) {
    case ThirdPartyCommit::FullCommit:
        tpcn = "FullCommit";
        break;
    case ThirdPartyCommit::SemiCommit:
        tpcn = "SemiCommit";
        break;
    case ThirdPartyCommit::NoCommit:
        tpcn = "NoCommit";
        break;

    default:
        throw KException("tpcName - Unrecognized ThirdPartyCommit");
        break;

    }

    return tpcn;
}

ostream& operator<< (ostream& os, const ThirdPartyCommit& tpc) {
    os << tpcName(tpc);
    return os;
}

string bigRRName(const BigRRange & rRng) {
    string s = "";
    switch (rRng) {
    case BigRRange::Min:
        s = "Min";
        break;
    case BigRRange::Mid:
        s = "Mid";
        break;
    case BigRRange::Max:
        s = "Max";
        break;
    default:
        cout << "bigRRName:: unknown BigRRange" << endl;
        assert (false);
        break;
    }
    return s;
}

ostream& operator << (ostream& os, const BigRRange& rRng) {
    os << bigRRName(rRng);
    return os;
}

string bigRAName(const BigRAdjust & rAdj) {
    string s = "";
    switch (rAdj) {
    case BigRAdjust::FullRA:
        s = "FullRA";
        break;
    case BigRAdjust::TwoThirdsRA:
        s = "TwoThirdsRA";
        break;
    case BigRAdjust::HalfRA:
        s = "HalfRA";
        break;
    case BigRAdjust::OneThirdRA:
        s = "OneThirdRA";
        break;
    case BigRAdjust::NoRA:
        s = "NoRA";
        break;
    default:
        cout << "bigRAName: unrecognized BigRAdjust" << endl;
        assert(false);
        break;
    }
    return s;
}
ostream& operator << (ostream& os, const BigRAdjust& rAdj) {
    os << bigRAName(rAdj);
    return os;
}



double Model::vote(VotingRule vr, double wi, double uij, double uik) {
    if (wi <= 0.0 ) { // you can make it really small (10E-10), but never zero or below.
        throw KException("Model::vote - non-positive voting weight");
    }
    double v = 0.0;
    const double du = uij - uik;

    double rBin = 0; // binary response
    const double sTol = 1E-8;
    rBin = du / sTol;
    rBin = (rBin > +1) ? +1 : rBin;
    rBin = (rBin < -1) ? -1 : rBin;

    double rProp = du; // proportional response
    double rCubic = du * du * du; // cubic reponse

    // the following weights determine how much the hybrids deviate from proportional
    const double rbp = 0.2;
    // rbp = 0.2 makes LHS slope and RHS slope of equal size (0.8 each), and twice the center jump (0.4)

    const double rpc = 0.5;

    switch (vr) {
    case VotingRule::Binary:
        v = wi * rBin;
        break;

    case VotingRule::PropBin:
        v = wi * ((1 - rbp)*rProp + rbp*rBin);
        break;

    case VotingRule::Proportional:
        v = wi * rProp;
        break;

    case VotingRule::PropCbc:
        v = wi * ((1 - rpc)*rProp + rpc*rCubic);
        break;

    case VotingRule::Cubic:
        v = wi * rCubic;
        break;

    default:
        throw KException("Model::vote - Unrecognized VotingRule");
        break;
    }
    return v;
}

tuple<double, double> Model::vProb(VPModel vpm, const double s1, const double s2) {
    const double tol = 1E-8;
    const double minX = 1E-6;
    double x1 = 0;
    double x2 = 0;
    switch (vpm) {
    case VPModel::Linear:
        x1 = s1;
        x2 = s2;
        break;
    case VPModel::Square:
        x1 = KBase::sqr(s1);
        x2 = KBase::sqr(s2);
        break;
    case VPModel::Quartic:
        x1 = KBase::qrtc(s1);
        x2 = KBase::qrtc(s2);
        break;
    case VPModel::Binary:
    {
        // this is setup so that 10% or more advantage either way gives a guaranteed
        // result. As with binary voting, it is necessary to have interpolation between
        // to avoid weird round-off effects.
        const double thresh = 1.10;
        if (s1 >= thresh*s2) {
            x1 = 1.0;
            x2 = minX;
        }
        else if (s2 >= thresh*s1) {
            x1 = minX;
            x2 = 1.0;
        }
        else { // less than the threshold difference
            double r12 = s1 / (s1 + s2);
            // We now need a linear rescaling so that
            // when s1/s2 = t, or r12 = t/(t+1), p12 = 1, and
            // when s2/s1 = t, or r12 = 1/(1+t), p12 =0.
            // We can work out (a,b) so that
            // a*(t/(t+1)) + b = 1, and
            // a*(1/(1+t)) + b = 0, then
            // verify that a*(1/(1+1))+b = 1/2.
            //
            double p12 = (r12*(thresh + 1.0) - 1.0) / (thresh - 1.0);
            x1 = p12;
            x2 = 1 - p12;
        }
    }
    break;
    }
    double p1 = x1 / (x1 + x2);
    double p2 = x2 / (x1 + x2);
    assert(0 <= p1);
    assert(0 <= p2);
    assert(fabs(p1 + p2 - 1.0) < tol);

    return tuple<double, double>(p1, p2);
}

// note that while the C_ij can be any arbitrary positive matrix
// with C_kk = 0, the p_ij matrix has the symmetry pij + pji = 1
// (and hence pkk = 1/2).
KMatrix Model::vProb(VPModel vpm, const KMatrix & c) {
    unsigned int numOpt = c.numR();
    assert(numOpt == c.numC());
    auto p = KMatrix(numOpt, numOpt);
    for (unsigned int i = 0; i < numOpt; i++) {
        for (unsigned int j = 0; j < i; j++) {
            double cij = c(i, j);
            assert(0 <= cij);
            double cji = c(j, i);
            assert(0 <= cji);
            assert((0 < cij) || (0 < cji));
            auto ppr = vProb(vpm, cij, cji);
            p(i, j) = get<0>(ppr); // set the lower left  probability: if Linear, cij / (cij + cji)
            p(j, i) = get<1>(ppr); // set the upper right probability: if Linear, cji / (cij + cji)
        }
        p(i, i) = 0.5; // set the diagonal probability
    }
    return p;
}

KMatrix Model::coalitions(function<double(unsigned int ak, unsigned int pi, unsigned int pj)> vfn,
                          unsigned int numAct, unsigned int numOpt) {
    // if several actors occupy the same position, then numAct > numOpt
    const double minC = 1E-8;
    auto c = KMatrix(numOpt, numOpt);
    for (unsigned int i = 0; i < numOpt; i++) {
        for (unsigned int j = 0; j < i; j++) {
            // scan only lower-left
            double cij = minC;
            double cji = minC;
            for (unsigned int k = 0; k < numAct; k++) {
                double vkij = vfn(k, i, j);
                if (vkij > 0) {
                    cij = cij + vkij;
                }
                if (vkij < 0) {
                    cji = cji - vkij;
                }
            }
            c(i, j) = cij;  // set the lower left coalition
            c(j, i) = cji;  // set the upper right coalition

        }
        c(i, i) = minC; // set the diagonal coalition
    }
    return c;
}

// these are assumed to be unique options.
// returns a square matrix.
KMatrix Model::vProb(VotingRule vr, VPModel vpm, const KMatrix & w, const KMatrix & u) {
    // u_ij is utility to actor i of the position advocated by actor j
    unsigned int numAct = u.numR();
    unsigned int numOpt = u.numC();
    // w_j is row-vector of actor weights, for simple voting
    assert(numAct == w.numC()); // require 1-to-1 matching of actors and strengths
    assert(1 == w.numR()); // weights must be a row-vector

    auto vfn = [vr, &w, &u](unsigned int k, unsigned int i, unsigned int j) {
        double vkij = vote(vr, w(0, k), u(k, i), u(k, j));
        return vkij;
    };

    auto c = coalitions(vfn, numAct, numOpt); // c(i,j) = strength of coaltion for i against j
    KMatrix p = vProb(vpm, c);  // p(i,j) = prob Ai defeats Aj
    return p;
}



// Given square matrix of Prob[i>j] returns a column vector for Prob[i]
KMatrix Model::probCE(PCEModel pcm, const KMatrix & pv) {
    const double pTol = 1E-6;
    unsigned int numOpt = pv.numR();
    assert(numOpt == pv.numC()); // must be square
    auto test = [&pv, pTol](unsigned int i, unsigned int j) {
        assert(0 <= pv(i, j));
        assert(fabs(pv(i, j) + pv(j, i) - 1.0) < pTol);
        return;
    };
    KMatrix::mapV(test, numOpt, numOpt); // catch gross errors

    auto p = KMatrix ();
    switch (pcm) {
    case PCEModel::MarkovPCM:
        p = markovPCE(pv);
        break;
    case PCEModel::ConditionalPCM:
        p = condPCE(pv);
        break;
    default:
        throw KException("Model::probCE unrecognized PCEModel");
        break;
    }
    assert(fabs(sum(p) - 1.0) < pTol);
    return p;
}


// Given square matrix of Prob[i>j] returns a column vector for Prob[i].
// Uses Markov process, not 1-step conditional probability
KMatrix Model::markovPCE(const KMatrix & pv) {
    const double pTol = 1E-6;
    unsigned int numOpt = pv.numR();
    auto p = KMatrix(numOpt, 1, 1.0) / numOpt;  // all 1/n
    auto q = p;
    unsigned int iMax = 1000;  // 10-30 is typical
    unsigned int iter = 0;
    double change = 1.0;
    while (pTol < change) {
        change = 0;
        for (unsigned int i = 0; i < numOpt; i++) {
            double pi = 0.0;
            for (unsigned int j = 0; j < numOpt; j++) {
                pi = pi + pv(i, j)*(p(i, 0) + p(j, 0));
            }
            assert(0 <= pi); // double-check
            q(i, 0) = pi / numOpt;
            double c = fabs(q(i, 0) - p(i, 0));
            change = (c > change) ? c : change;
        }
        p = q;
        iter++;
        assert(fabs(sum(p) - 1.0) < pTol); // double-check
    }
    assert(iter < iMax); // no way to recover
    return p;
}


// Given square matrix of Prob[i>j] returns a column vector for Prob[i].
// Uses 1-step conditional probabilities, not Markov process
KMatrix Model::condPCE(const KMatrix & pv) {
    unsigned int numOpt = pv.numR();
    auto p = KMatrix(numOpt, 1);
    for (unsigned int i = 0; i < numOpt; i++) {
        double pi = 1.0;
        for (unsigned int j = 0; j < numOpt; j++) {
            pi = pi * pv(i, j);
        }
        // double-check
        assert(0 <= pi);
        assert(pi <= 1);
        p(i, 0) = pi; // probability that i beats all alternatives
    }
    double probOne = sum(p); // probability that one option, any option, beats all alternatives
    p = (p / probOne); // conditional probability that i is that one.
    return p;
}

// This assumes scalar capabilities of actors (w), so that the voting strength
// is a direct function of difference in utilities.Therefore, we can use
// Model::vProb(VotingRule vr, const KMatrix & w, const KMatrix & u)
KMatrix Model::scalarPCE(unsigned int numAct, unsigned int numOpt, const KMatrix & w, const KMatrix & u,
                         VotingRule vr, VPModel vpm, ReportingLevel rl) {

    auto pv = Model::vProb(vr, vpm, w, u);
    auto p = Model::probCE(PCEModel::ConditionalPCM, pv);

    if (ReportingLevel::Low < rl) {
        printf("Num actors: %i \n", numAct);
        printf("Num options: %i \n", numOpt);


        if ((numAct <= 20) && (numOpt <= 20)) {
            cout << "Actor strengths: " << endl;
            w.mPrintf(" %6.2f ");
            cout << endl << flush;
            cout << "Voting rule: " << vr << endl;
            // printf("         aka %s \n", KBase::vrName(vr).c_str());
            cout << flush;
            cout << "Utility to actors of options: " << endl;
            u.mPrintf(" %+8.3f ");
            cout << endl;

            auto vfn = [vr, &w, &u](unsigned int k, unsigned int i, unsigned int j) {
                double vkij = vote(vr, w(0, k), u(k, i), u(k, j));
                return vkij;
            };

            auto c = coalitions(vfn, numAct, numOpt); // c(i,j) = strength of coaltion for i against j
            KMatrix p2 = vProb(vpm, c);  // p(i,j) = prob Ai defeats Aj

            cout << "Coalition strengths of (i:j): " << endl;
            c.mPrintf(" %8.3f ");
            cout << endl;

            assert(norm(pv - p2) < 1E-8); // better be close

            cout << "Probability Opt_i > Opt_j" << endl;
            pv.mPrintf(" %.4f ");
            cout << "Probability Opt_i" << endl;
            p.mPrintf(" %.4f ");
        }
        cout << "Found stable PCE distribution" << endl << flush;
    }
    return p;
}


// -------------------------------------------------
Actor::Actor(string n, string d) {
    name = n;
    desc = d;
}


Actor::~Actor() {
    // empty
}

// this uses the evenly-weighted "Sum of Utilities" model
// Note that the pi and pj numbers already include the contribution of k to the hypothetical little
// i:j conflict with just i,j,k involved
// TODO: implement and offer the "Expected Value of State-Utility" model
// returns the vote v_k(i:j), and the utilities of ik>j and i>jk.
tuple<double, double, double>
Actor::thirdPartyVoteSU(double wk, VotingRule vr, ThirdPartyCommit comm,
                        double pik, double pjk, double uki, double ukj, double ukk) {
    const double pTol = 1E-8;
    assert(0 <= pik);
    assert(0 <= pjk);
    assert(fabs(pik + pjk - 1.0) < pTol);
    double p_ik_j = pik; // estimated probability ik > j
    double p_jk_i = pjk; // estimated probability jk > i
    double p_j_ik = pjk; // estimated probability j  > ik
    double p_i_jk = pik; // estimated probability i  > jk
    double u_ik_def_j = 0;
    double u_j_def_ik = 0;
    double u_i_def_jk = 0;
    double u_jk_def_i = 0;
    // strictly speaking, we should add in all the utilities of unchanged positions, then divide by numAct,
    // thus computing the expected utility for a flat probability distribution,
    // so that the utility of each alternative state is between 0 and 1.
    // However,
    // A: subtraction will make all utilities of unchanged positions cancel here when vk is determined, and
    // B: dividing in P[1>2] = C12 / (C12 + C21) will cancel the factors of 1/4.
    //
    // Therefore, we can drop those terms as long we make sure all comparisons use the sum
    // of three position-utilities, and we obviously do so.
    switch (comm) {
    case ThirdPartyCommit::FullCommit:
        u_ik_def_j = uki + uki + uki;
        u_j_def_ik = ukj + ukj + ukj;
        u_i_def_jk = uki + uki + uki;
        u_jk_def_i = ukj + ukj + ukj;
        break;
    case ThirdPartyCommit::SemiCommit:
        u_ik_def_j = uki + uki + ukk;
        u_j_def_ik = ukj + ukj + ukj;
        u_i_def_jk = uki + uki + uki;
        u_jk_def_i = ukj + ukj + ukk;
        break;
    case ThirdPartyCommit::NoCommit:
        u_ik_def_j = uki + uki + ukk;
        u_j_def_ik = ukj + ukj + ukk;
        u_i_def_jk = uki + uki + ukk;
        u_jk_def_i = ukj + ukj + ukk;
        break;
    }
    double u_ik_j = (p_ik_j * u_ik_def_j) + (p_j_ik * u_j_def_ik);
    double u_i_jk = (p_i_jk * u_i_def_jk) + (p_jk_i * u_jk_def_i);
    double vk = Model::vote(vr, wk, u_ik_j, u_i_jk);
    return tuple<double, double, double>(vk, u_ik_def_j, u_i_def_jk);
}


// Third-party actor n determines his contribution on influence by analyzing the hypothetical
// "little conflict" of just three actors, i.e.  (i:j) with n weighing in on whichever side it favors.
double Actor::vProbLittle(VotingRule vr, double wn, double uni, double unj, double contrib_i_ij, double contrib_j_ij) {

    // the primary actors are assigned at least the minisucle
    // minimum influence contribution, to avoid 0/0 errors.
    if (contrib_i_ij <= 0) {
        assert(contrib_i_ij > 0);
    }
    if (contrib_j_ij <= 0) {
        assert(contrib_j_ij > 0);
    }

    // order-zero estimate depends only on the voting between their two positions
    double contrib_n_ij = Model::vote(vr, wn, uni, unj);

    // add that to whichever side they preferred
    double cni = (contrib_n_ij > 0) ? contrib_i_ij + contrib_n_ij : contrib_i_ij;
    double cnj = (contrib_n_ij < 0) ? contrib_j_ij - contrib_n_ij : contrib_j_ij;

    // assert(0 < cni);
    // assert(0 < cnj);


    double pin = cni / (cni + cnj);
    //double pjn = cnj / (cni + cnj); // just FYI
    return pin;
}

} // end of namespace

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
