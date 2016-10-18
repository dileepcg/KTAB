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

#include "hcsearch.h"
#include "emodel.h"
#include <thread>


namespace KBase {

using std::cout;
using std::endl;
using std::flush;
using std::get;
using std::tuple;
using std::thread;

// --------------------------------------------
template <class PT>
// JAH 20160711 added rng seed
EModel<PT>::EModel(string desc, uint64_t s, vector <bool> f) : Model(desc, s, f) {
    // nothing yet
}

template <class PT>
void EModel<PT>::setOptions() {
    assert(nullptr != enumOptions);
    assert(0 == theta.size());
    theta = enumOptions();
    assert (minNumOptions <= theta.size());
    return;
}



template <class PT>
EModel<PT>::~EModel() {
    theta = {};
}


template <class PT>
unsigned int EModel<PT>::numOptions() const {
    return theta.size();
}


template <class PT>
PT EModel<PT>::nthOption(unsigned int i) const {
    assert(i < theta.size());
    return theta[i];
}

template <class PT>
bool EModel<PT>::equivStates(const EState<PT>* es1, const EState<PT>*  es2) const {
    const unsigned int np = es1->pstns.size();
    bool eqvP = (np == es2->pstns.size()); // most basic test
    eqvP = eqvP && (es1->eMod == es2->eMod); //
    for (unsigned int i=0; i<np; i++) {
        eqvP = eqvP && (es1->posNdx(i) == es2->posNdx(i));
    }
    return eqvP;
}
// --------------------------------------------

template <class PT>
EActor<PT>::EActor(EModel<PT>* m, string n, string d) : Actor(n,d) {
    eMod = m;
}


template <class PT>
EActor<PT>::~EActor() {
    // nothing yet
}


template <class PT>
double EActor<PT>::vote(unsigned int est,
                        unsigned int p1, unsigned int p2,
                        const State* st) const {
    /// vote between the current positions to actors at
    /// positions p1 and p2 of this state

    // get the two corresponding indices into Theta
    auto es = (const EState<PT>*) st;
    auto n1 = es->posNdx(p1);
    auto n2 = es->posNdx(p2);

    // TODO: lookup real utilities of Theta[n]
    assert (false);

    double u1 = 0.0; // some fn of ep1
    double u2 = 0.0; // some fn of ep2

    const double v12 = Model::vote(vr, sCap, u1, u2);
    return v12;
}


// --------------------------------------------
template <class PT>
EPosition<PT>::EPosition(EModel<PT>* m, int n) : Position() {
    assert(nullptr != m);
    eMod = m;
    assert(0 <= n);
    const unsigned int numOpt = eMod->numOptions();
    assert(n < numOpt);
    ndx = n;
}

template <class PT>
EPosition<PT>::~EPosition() {
    eMod = nullptr;
    ndx = -1;
}



template <class PT>
void EPosition<PT>::print(ostream& os) const {
    os << "[EPosition " << ndx <<"]";
    return;
}

// --------------------------------------------
template <class PT>
EState<PT>::EState( EModel<PT>* mod) : State(mod) {
    step = nullptr;
    eMod = (EModel<PT>*) model;
}

template <class PT>
EState<PT>::~EState() {
    step = nullptr;
}

/*
// this must be overriden.
// It only exists because EState::doSUSN cannot instantiate an
// abstract object.
template <class PT>
 EState<PT>* EState<PT>::makeNew() const {
    auto s2 = new EState<PT>(eMod);
    return s2;
}

// this must be overriden.
// It only exists because EState::doSUSN cannot instantiate an
// abstract object.
template <class PT>
void EState<PT>::setAllAUtil(ReportingLevel rl) {
    assert(false);
    return;
}
*/


template <class PT>
void EState<PT>::setValues() {
    const unsigned int numAct = eMod->numAct;
    const unsigned int numOpt = eMod->theta.size();
    assert(numOpt == eMod->numOptions());
    assert(0 < numOpt);
    auto actorVals = KMatrix(numAct, numOpt);
    for (unsigned int tj = 0; tj < numOpt; tj++) {
        vector<double> vp = actorVFn(tj, this);
        assert(numAct == vp.size());
        for (unsigned int i = 0; i < numAct; i++) {
            actorVals(i, tj) = vp[i];
        }
    }
    return;
}



template <class PT>
void EState<PT>::show() const {
    //cout << "EState<PT>::show()  not yet implemented" << endl << flush;
    unsigned int na = pstns.size();
    cout << "[EState";
    for (unsigned int i=0; i<na; i++) {
        cout << " "<< posNdx(i);
    }
    cout << "]"<<endl<<flush;
    return;
}


template <class PT>
bool EState<PT>::equivNdx(unsigned int i, unsigned int j) const {
    assert (i < pstns.size());
    assert (j < pstns.size());
    auto pi = (const EPosition<PT>*) (pstns[i]);
    auto pj = (const EPosition<PT>*) (pstns[j]);
    bool eqv = (pi->getIndex() == pj->getIndex());
    return eqv;
}


template <class PT>
unsigned int EState<PT>::posNdx(const unsigned int i) const {
    assert (i < pstns.size());
    auto pi = (const EPosition<PT>*)(pstns[i]);
    assert (nullptr != pi);
    int ni = pi->getIndex();
    assert (0 <= ni);
    auto ndx = (unsigned int) ni;
    return ndx;
}

template <class PT>
EState<PT>* EState<PT>::stepSUSN() {
    cout << endl << flush;
    cout << "State number " << model->history.size() - 1 << endl << flush;
    show();
    if ((0 == uIndices.size()) || (0 == eIndices.size())) {
        setUENdx();
    }
    
    cout << "Setting all utilities from objective perspective"<<endl<<flush;
    setAUtil(-1, ReportingLevel::Low);

    //cout << "Doing actual SUSN"<<endl<<flush;
    auto s2 = doSUSN(ReportingLevel::Silent);
    assert(nullptr != s2);
    s2->step = [s2]() {
        return s2->stepSUSN();
    };
    cout << flush;
    return s2;
}

template <class PT>
EState<PT>* EState<PT>::doSUSN(ReportingLevel rl) const {
    const unsigned int numA = eMod->numAct;
    const unsigned int na2 = eMod->actrs.size();
    assert(numA == na2);

    const unsigned int numU = uIndices.size();
    assert((0 < numU) && (numU <= numA));
    assert(numA == eIndices.size());

    const unsigned int aus = aUtil.size();

    assert (KBase::Model::minNumActor <= aus);
    const auto u = aUtil[0]; // all have same beliefs in this demo

    const auto vpm = eMod->vpm; // get the 'victory probability model'
    const unsigned int numP = pstns.size();

    const auto euState = expUtilMat(rl, numA, numP, vpm, u);
    cout << "Actor expected utilities in actual state: ";
    KBase::trans(euState).mPrintf("%6.4f, ");
    cout << endl << flush;


    if (ReportingLevel::Low < rl) {
        printf("--------------------------------------- \n");
        printf("Assessing utility of actual state to all actors \n");
        for (unsigned int h = 0; h < numA; h++)
        {
            cout << "  actual utilities not available" << endl;
        }
        cout << endl << flush;
        cout << "Positions in this state: ";
        show();
        cout << endl;
        printf("Out of %u positions, %u were unique, with these indices: ", numA, numU);
        cout << flush;
        for (auto i : uIndices)
        {
            printf("%2i ", i);
        }
        cout << endl;
        cout << flush;
    }


    auto uufn = [u, this](unsigned int i, unsigned int j1)  {
        return u(i, uIndices[j1]);
    };
    auto uUnique = KMatrix::map(uufn, numA, numU);

    // Get expected-utility vector, one entry for each actor, in the current state.
    const auto eu0 = expUtilMat(rl, numA, numP, vpm, uUnique); //  without duplicates

    // Note that EState::makeNewEState is a pure virtual method,
    // so what gets called here is the method from derived classes,
    // which can provide the extra structure of a derived class.
    auto s2 = makeNewEState();
    // This cannot do so:
    //auto s2 = new EState<PT>(eMod);

    assert (0 == s2->pstns.size());
    for (unsigned int h = 0; h < numA; h++) {
        s2->pstns.push_back(nullptr);
    }
    // TODO: clean up the nesting of lambda-functions: ~200 lines is too long
    // perhaps create a hypothetical state and run setOneAUtil(h,Silent) on it

    // Evaluate h's estimate of the expected utility, to h, of
    // advocating position mp. To do this, build a hypothetical utility matrix representing
    // h's estimates of the direct utilities to all other actors of h adopting this
    // Position. Do that by modifying the h-column of h's matrix.
    // Then compute the expected probability distribution, over h's hypothetical position
    // and everyone else's actual position. Finally, compute the expected utility to
    // each actor, given that distribution, and pick out the value for h's expected utility.
    // That is the expected value to h of adopting the position.
    // ehFN :: (int, EPosition<PT>) ==> double
    auto ehFN = [this, rl, u](unsigned int h, const EPosition<PT> & eph)  {
        // This correctly handles duplicated/unique options
        // We modify the given utility matrix so that the h-column
        // corresponds to the given mph, but we need to prune duplicates as well.
        // This entails some type-juggling.

        const int eNdx = eph.getIndex();
        assert (0 <= eNdx);
        auto uVec = actorUtilVectFn(h, eNdx);

        // all have same beliefs in this demo: verify
        const KMatrix uh0 = aUtil[h]; // constant
        assert(KBase::maxAbs(u - uh0) < 1E-10);
        auto uh = uh0; // to be modified

        for (unsigned int i = 0; i < eMod->numAct; i++) {
            // lookup h's prior evaluation of utils over the enumerated space
            // of utility to actor i of this hypothetical position by h
            double uih = uVec[i];
            uh(i, h) = uih;
        }


        // 'uh' now has the correct h-column. Now we need to see how many options
        // are unique in the hypothetical state, and keep only those columns.
        // This entails juggling back and forth between the all current positions
        // and the one hypothetical position (mph at h).
        // Thus, the next call to expUtilMat will consider only unique options.

        // Compare indices to see if the positions are equivalent.
        // For h, use the hypothetical 'eph', and for all others use
        // the stored pointers.
        // Cannot use State::equivNdx because the comparisons for index 'h'
        // use the hypothetical mph, not pstns[h]
        auto equivHNdx = [this, h, eph](const unsigned int i, const unsigned int j) {
            int ni = (i == h) ? eph.getIndex() : posNdx(i);
            int nj = (j == h) ? eph.getIndex() : posNdx(j);
            return (ni == nj);
        };

        auto ns = KBase::uiSeq(0, model->numAct - 1);
        const VUI uNdx = get<0>(KBase::ueIndices<unsigned int>(ns, equivHNdx));
        const unsigned int numU = uNdx.size();
        auto hypUtil = KMatrix(eMod->numAct, numU);
        // we need now to go through 'uh', copying column J the first time
        // the J-th position is determined to be equivalent to something in the unique list
        for (unsigned int i = 0; i < eMod->numAct; i++) {
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
            cout << eph << endl << flush;
            printf("Hypo-util minus base util: \n");
            (uh - uh0).mPrintf(" %+.4E ");
            cout << endl << flush;
        }

        const KMatrix eu = expUtilMat(rl,
                                      eMod->numAct, pstns.size(), eMod->vpm,
                                      hypUtil); // uh or hypUtil
        const double euh = eu(h, 0);
        assert(0.0 <= euh);
        return euh;
    };
    // end of ehFN



    // 'Returns' void in order to have the right type-signature for threading.
    //
    // Does a generic hill climb to find the best next position for actor h,
    // and stores it in s2.
    // To do that, it uses three functions for evaluation, neighbors, and show:
    // efn, nghbrPerms, and sfn.
    auto newPosFn = [this, ehFN, rl,  u, eu0, s2](const unsigned int h)  {
        if (ReportingLevel::Low < rl) {
            cout << "Started newPosFn for "<<h<<endl<<flush;
        }
        s2->pstns[h] = nullptr;
        auto ph = ((const EPosition<PT>*)(pstns[h]));


        auto ghc = new KBase::GHCSearch<EPosition<PT>>();

        ghc->eval = [ehFN, h](const EPosition<PT>  eph)  {
            return ehFN(h, eph);
        };
        const unsigned int numOpt = eMod->numOptions();

        // The 'neighbors' in general are ALL the enumerated positions,
        // so it does not even use the actor's current position
        auto nfn = [this, rl, numOpt](const EPosition<PT> & ) {
            vector<EPosition<PT>> ns = {};
            //ns.resize(numOpt); // compiler tries to fill with EPosition(), and fails.
            for (unsigned int i=0; i<numOpt; i++) {
                auto ep = EPosition<PT>(eMod, i);
                ns.push_back(ep);
            }
            if (ReportingLevel::Low < rl) {
                cout << "Found "<<ns.size()<<" neighbors"<<endl<<flush;
            }
            assert (0 < ns.size());
            return ns;
        };

        ghc->nghbrs = nfn;

        // show some representation of this position on cout
        ghc->show = [](const EPosition<PT> & ep) {
            cout << ep ;
            return;
        };

        if (ReportingLevel::Low < rl) {
            printf("---------------------------------------- \n");
            printf("Search for best next-position of actor %2i \n", h);
            cout << flush;
        }
        auto rslt = ghc->run(*ph, // start from h's current positions
                             ReportingLevel::Silent,
                             100, // iter max
                             3, 0.001); // stable-max, stable-tol
        double vBest = get<0>(rslt);
        EPosition<PT> pBest = get<1>(rslt);
        unsigned int iterN = get<2>(rslt);
        unsigned int stblN = get<3>(rslt);
        delete ghc;
        ghc = nullptr;

        if (ReportingLevel::Medium < rl) {
            printf("Iter: %u  Stable: %u \n", iterN, stblN);
            printf("Best value for %2i: %+.6f \n", h, vBest);
            cout << "Best position:    " << pBest << endl;
        }

        auto posBest = new EPosition<PT>(pBest);

        // Actually record the new position
        s2->pstns[h] = posBest;
        // no need for mutex, as s2->pstns is the only shared var,
        // and each h is different.

        double du = vBest - eu0(h, 0); // (hypothetical, future) - (actual, current)
        if (ReportingLevel::Low < rl) {
            printf("Expected EU improvement for %2i of %+.4E \n", h, du);
        }
        //printf("  vBest = %+.6f \n", vBest);
        //printf("  eu0(%i, 0) for %i = %+.6f \n", h, h, eu0(h,0));
        //cout << endl << flush;
        // Logically, du should always be non-negative, as GHC never returns a worse value than the starting point.
        // However, actors plan on the assumption that all others do not change - yet they do.
        const double eps = 0.05; //  enough to avoid problems with round-off error
        assert(-eps <= du);
        return;
    };
    // end of newPosFn


    bool parP = KBase::testMultiThreadSQLite(false, rl);

    // concurrent execution works, but mixes up the printed log.
    // So we disable it if reporting is desired.
    parP = parP && (ReportingLevel::Silent == rl);

    if (parP) {
        cout << "Will continue with multi-threaded execution"<<endl<<flush;
    }
    else {
        cout << "Will continue with single-threaded execution"<<endl<<flush;
    }

    auto ts = vector<thread>();
    // Each actor, h, finds the position which maximizes their EU in this situation.
    for (unsigned int h = 0; h < numA; h++) {
        if (parP)  { // launch all, concurrent
            //printf("Starting concurrent thread for new position %u \n", h);
            //cout << flush;
            ts.push_back(thread([newPosFn, h]() {
                newPosFn(h);
                return;
            }));
        }
        else  { // do each, sequential
            //printf("Calculating new position %u \n", h);
            //cout << flush;
            newPosFn(h);
        }
        cout << flush;
    }

    // if multi-threaded, join them all before continuing
    if (parP)  {
        for (auto& t : ts) {
            t.join();
        }
    }

    assert(nullptr != s2);
    assert(numP == s2->pstns.size());
    assert(numA == s2->model->numAct);
    for (auto p : s2->pstns)
    {
        assert(nullptr != p);
    }
    s2->setUENdx();
    return s2;
}
// end of doSUSN




template <class PT>
EState<PT>* EState<PT>::stepBCN() {
    cout << endl << flush;
    cout << "State number " << model->history.size() - 1 << endl << flush;
    if ((0 == uIndices.size()) || (0 == eIndices.size())) {
        setUENdx();
    }
    setAUtil(-1, ReportingLevel::Medium);
    show();

    auto s2 = doBCN(ReportingLevel::Low);

    s2->step = [s2]() {
        return s2->stepBCN();
    };
    cout << endl << flush;
    return s2;
}

template <class PT>
EState<PT>* EState<PT>::doBCN(ReportingLevel rl) const {
    EState<PT>* s2 = nullptr;

    // do something

    assert (s2 != nullptr);
    return s2;
}
// end of doBCN


template<class PT>
KMatrix EState<PT>::hypExpUtilMat () const {
    KMatrix eu;

    // TODO: fix or delete?
    assert(false);

    return eu;
}


// TODO: use this instead of the near-duplicate code in expUtilMat
/// Calculate the probability distribution over states from this perspective
template<class PT>
tuple <KMatrix, VUI> EState<PT>::pDist(int persp) const { 
    const unsigned int numA = eMod->numAct;
    // for this demo, the number of positions is exactly the number of actors
    const unsigned int numP = numA;

    // get unique indices and their probability
    assert(0 < uIndices.size()); // should have been set with setUENdx();
    //auto uNdx2 = uniqueNdx(); // get the indices to unique positions

    const unsigned int numU = uIndices.size();
    assert(numU <= numP); // might have dropped some duplicates

    cout << "Number of aUtils: " << aUtil.size() << endl << flush;

    const auto u = aUtil[0]; // all have same beliefs in this demo
    assert (-1 == persp);

    auto uufn = [u, this](unsigned int i, unsigned int j1) {
        return u(i, uIndices[j1]);
    };

    const auto uMat = KMatrix::map(uufn, numA, numU);
    assert(uMat.numR() == numA); // must include all actors
    assert(uMat.numC() == numU);

    // vote_k ( i : j )
    auto vkij = [this, uMat](unsigned int k, unsigned int i, unsigned int j) {
        auto ak = (EActor<PT>*)(eMod->actrs[k]);
        auto v_kij = Model::vote(ak->vr, ak->sCap, uMat(k, i), uMat(k, j));
        return v_kij;
    };

    // the following uses exactly the values in the given expected
    // utility matrix, which is usually NOT square
    const auto c = Model::coalitions(vkij, uMat.numR(), uMat.numC());
    const auto ppv = Model::probCE2(eMod->pcem, eMod->vpm, c);
    const auto p = get<0>(ppv); // column
    const auto pv = get<1>(ppv); // square

    if (KBase::testProbCE) {
        cout << "Testing EState::pDist ... " << flush;
        const auto pv0 = Model::vProb(eMod->vpm, c); // square
        assert(KBase::norm(pv - pv0) < 1E-6);
        const auto p0 = Model::probCE(eMod->pcem, pv0); // column
        assert(KBase::norm(p - p0) < 1E-6);
        cout << "ok" << endl << flush;
    }

    const auto eu = uMat*p; // column

    assert(numA == eu.numR());
    assert(1 == eu.numC());

    return tuple <KMatrix, VUI>(p, uIndices);
}


// Given the utility matrix, uMat, calculate the expected utility to each actor,
// as a column-vector. Again, this is from the perspective of whoever developed uMat.
template <class PT>
KMatrix EState<PT>::expUtilMat  (KBase::ReportingLevel rl,
                                 unsigned int numA,
                                 unsigned int numP,
                                 KBase::VPModel vpm,
                                 const KMatrix & uMat) const
{
    // BTW, be sure to lambda-bind uMat *after* it is modified.
    assert(uMat.numR() == numA); // must include all actors
    assert(uMat.numC() <= numP); // might have dropped some duplicates

    auto assertRange = [](const KMatrix& m, unsigned int i, unsigned int j) {
        // due to round-off error, we must have a tolerance factor
        const double tol = 1E-10;
        const double mij = m(i, j);
        const bool okLower = (0.0 <= mij + tol);
        const bool okUpper = (mij <= 1.0 + tol);
        if (!okLower || !okUpper) {
            printf("%f  %i  %i  \n", mij, i, j);
            cout << flush;
        }
        assert(okLower);
        assert(okUpper);
        return;
    };

    auto uRng = [assertRange, uMat](unsigned int i, unsigned int j) {
        assertRange(uMat, i, j);
        return;
    };
    KMatrix::mapV(uRng, uMat.numR(), uMat.numC());

    // vote_k(i:j)
    auto vkij = [this, uMat](unsigned int k, unsigned int i, unsigned int j) {
        auto ak = (const EActor<PT>*)(eMod->actrs[k]);
        auto v_kij = Model::vote(ak->vr, ak->sCap, uMat(k, i), uMat(k, j));
        return v_kij;
    };

    // the following uses exactly the values in the given expected
    // utility matrix, which is usually NOT square
    const auto c = Model::coalitions(vkij, uMat.numR(), uMat.numC());
    // use whatever 'vpm' was supplied
    const auto ppv = Model::probCE2(eMod->pcem, vpm, c);
    const auto p = get<0>(ppv); // column
    const auto pv = get<1>(ppv); // square

    if (KBase::testProbCE && (PCEModel::MarkovIPCM != eMod->pcem)) {
        cout << "Testing EState::expUtilMat ... " << flush;
        const auto pv0 = Model::vProb(vpm, c); // square
        assert(KBase::norm(pv - pv0) < 1E-6);
        const auto p0 = Model::probCE(eMod->pcem, pv0); // column
        assert(KBase::norm(p - p0) < 1E-6);
        cout << "ok" << endl << flush;
    }
    const auto eu = uMat*p; // column

    assert(numA == eu.numR());
    assert(1 == eu.numC());
    auto euRng = [assertRange, eu](unsigned int i, unsigned int j) {
        assertRange(eu, i, j);
        return;
    };
    KMatrix::mapV(euRng, eu.numR(), eu.numC());

    if (ReportingLevel::Low < rl) {
        printf("Util matrix is %u x %u \n", uMat.numR(), uMat.numC());
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
// end of expUtilMat





} // end of namespace

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
