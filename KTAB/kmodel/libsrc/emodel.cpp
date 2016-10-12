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
EActor<PT>::EActor(EModel<PT>* m, string n, string d) : Actor(n,d) {
    eMod = m;
}


template <class PT>
EActor<PT>::~EActor() {
    // nothing yet
}


template <class PT>
double EActor<PT>::vote(unsigned int est, unsigned int p1, unsigned int p2, const State* st) const {
    /// vote between the current positions to actors at positions p1 and p2 of this state
    auto ep1 = ((const EPosition<PT>*)(st->pstns[p1]));
    auto ep2 = ((const EPosition<PT>*)(st->pstns[p2]));
    
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
  os << "Generic EPosition";
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

template <class PT>
void EState<PT>::setAllAUtil(ReportingLevel rl) {
    // nothing yet
    return;
}

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
    cout << "EState<PT>::show()  not yet implemented" << endl << flush;
    return;
}

template <class PT>
EState<PT>* EState<PT>::stepSUSN() {
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



template <class PT>
EState<PT>* EState<PT>::doSUSN(ReportingLevel rl) const {

    // do something
    const unsigned int numA = eMod->numAct;
    assert(numA == eMod->actrs.size());

    const unsigned int numU = uIndices.size();
    assert((0 < numU) && (numU <= numA));
    assert(numA == eIndices.size());

    const auto u = aUtil[0]; // all have same beliefs in this demo

    auto vpm = eMod->vpm; // get the 'victory probability model'
    const unsigned int numP = pstns.size();


    auto euMat = [rl, numA, numP, vpm, this](const KMatrix & uMat) {
        return expUtilMat(rl, numA, numP, vpm, uMat);
    };
    auto euState = euMat(u);
    cout << "Actor expected utilities in actual state: ";
    KBase::trans(euState).mPrintf("%6.4f, ");
    cout << endl << flush;


    if (ReportingLevel::Low < rl) {
        printf("--------------------------------------- \n");
        printf("Assessing utility of actual state to all actors \n");
        for (unsigned int h = 0; h < numA; h++)
        {
            cout << "not available" << endl;
        }
        cout << endl << flush;
        printf("Out of %u positions, %u were unique: ", numA, numU);
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
    const auto eu0 = euMat(uUnique); // 'u' with duplicates, 'uUnique' without duplicates

     
    EState<PT>* s2 = new EState<PT>(eMod);
    // do some more

    assert(nullptr != s2);
    return s2;
}


// TODO: use this instead of the near-duplicate code in expUtilMat
/// Calculate the probability distribution over states from this perspective
template<class PT>
tuple <KMatrix, VUI> EState<PT>::pDist(int persp) const {
    const unsigned int numA = eMod->numAct;
    const unsigned int numP = numA; // for this demo, the number of positions is exactly the number of actors

    // get unique indices and their probability
    assert(0 < uIndices.size()); // should have been set with setUENdx();
    //auto uNdx2 = uniqueNdx(); // get the indices to unique positions

    const unsigned int numU = uIndices.size();
    assert(numU <= numP); // might have dropped some duplicates

    cout << "Number of aUtils: " << aUtil.size() << endl << flush;

    const auto u = aUtil[0]; // all have same beliefs in this demo

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

    // the following uses exactly the values in the given euMat,
    // which may or may not be square
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



template <class PT>
bool EState<PT>::equivNdx(unsigned int i, unsigned int j) const {
    bool rslt = false;


    // do nothing
    assert (false);


    return rslt;
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

    auto vkij = [this, uMat](unsigned int k, unsigned int i, unsigned int j) {  // vote_k(i:j)
        auto ak = (const EActor<PT>*)(eMod->actrs[k]);
        auto v_kij = Model::vote(ak->vr, ak->sCap, uMat(k, i), uMat(k, j));
        return v_kij;
    };
    
    // the following uses exactly the values in the given euMat,
    // which is usually NOT square
    const auto c = Model::coalitions(vkij, uMat.numR(), uMat.numC());
    const auto ppv = Model::probCE2(eMod->pcem, eMod->vpm, c);
    const auto p = get<0>(ppv); // column
    const auto pv = get<1>(ppv); // square

    if (KBase::testProbCE) {
      cout << "Testing EState::expUtilMat ... " << flush;
      const auto pv0 = Model::vProb(eMod->vpm, c); // square
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

    if (ReportingLevel::Low < rl)
    {
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
// end of expUtilMat





} // end of namespace

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
