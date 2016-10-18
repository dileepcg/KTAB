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


#include "vimcp.h"
#include "demoMatrixEMod.h"
#include "demoPMatData.h"


namespace eModKEM {
using std::cout;
using std::endl;
using std::flush;
using std::string;
using std::vector;

using KBase::KMatrix;
using KBase::Model;
using KBase::ReportingLevel;
using KBase::VotingRule;
using KBase::EModel;
using KBase::EState;
using KBase::EActor;
using KBase::EPosition;


const auto weightMat = KMatrix::arrayInit(weightArray, 1, numActKEM);
const auto utilMat = KMatrix::arrayInit(utilArray, numActKEM, numPolKEM);


// --------------------------------------------
PMatrixModel::PMatrixModel(string d, uint64_t s, vector<bool> vb) : EModel< unsigned int >(d,s,vb) {
    // nothing yet
}


PMatrixModel::~PMatrixModel() {
    // nothing yet
}


// check the matrix and set as utilities,
// if valid for [numAct,numOpt] utilities in [0,1] range.
void PMatrixModel::setPMatrix(const KMatrix pm0) {
    const unsigned int nr = pm0.numR();
    const unsigned int nc = pm0.numC();
    if (0 < numAct) {
        assert (nr == numAct);
    }
    else {
        numAct = nr;
    }

    if (0 < numOptions()) {
        assert (nc == numOptions());
    }
    else {
        theta.resize(nc); // was size zero
        for (unsigned int i=0; i<nc; i++) {
            theta[i] = i;
        }
    }

    assert (minNumActor <= numAct);
    assert (numAct <= maxNumActor);

    assert (minNumOptions <= numOptions());

    for (auto u : pm0) {
        assert (0.0 <= u);
        assert (u <= 1.0);
    }

    // if all OK, set it
    polUtilMat = pm0;

    return;
}


// check the row-matrix and set as weight,
// if valid for non-negative [numAct,1] weights
void PMatrixModel::setWeights(const KMatrix w0) {
    const unsigned int nr = w0.numR();
    const unsigned int nc = w0.numC();

    assert (1 == nr);

    if (0 < numAct) {
        assert (nc == numAct);
    }
    else {
        numAct = nc;
    }

    for (auto w : w0) {
        assert (0.0 <= w);
    }
    assert (minNumActor <= numAct);
    assert (numAct <= maxNumActor);

    // if it is OK, set it
    wghtVect = w0;
    return;
}


void PMatrixModel::setActors(vector<string> names, vector<string> descriptions) {
    const unsigned int na = numAct;
    numAct = 0;

    assert (0 < na);
    assert (na == names.size());
    assert (na == descriptions.size());
    assert (na == wghtVect.numC());
    assert (na == polUtilMat.numR());

    for (unsigned int i=0; i<na; i++) {
        auto ai = new EActor<unsigned int>(this, names[i], descriptions[i]);
        ai->vr = VotingRule::Proportional;
        ai->sCap = wghtVect(0,i);
        addActor(ai);
    }
    assert (na == numAct);
    return;
}

tuple<double, KMatrix, KMatrix> PMatrixModel::minProbError(
    const KMatrix& wMat, const KMatrix& uMat, double errWeight) {
    using std::get;
    using KBase::VHCSearch;

    assert (0 < errWeight);

    assert (1 == wMat.numR());
    const unsigned int nAct = wMat.numC();
    assert (nAct == uMat.numR());
    const unsigned int nOpt = uMat.numC();

    KMatrix wAdj1 = KMatrix(1, nAct, 1.0);
    wAdj1(0,7) = 1E-8;  // CO2 quant min group
    wAdj1(0,8) = 1E-8;  // CO2 price max group

    KMatrix pSel1 = KMatrix(nOpt, 1, 0.0);
    pSel1(0,0)=1.0;
    const double thresh1 = 0.75;

    KMatrix wAdj2 = KMatrix(1, nAct, 1.0);
    KMatrix pSel2 = KMatrix(nOpt, 1, 0.0);
    pSel2( 8,0)=1.0;
    pSel2( 9,0)=1.0;
    pSel2(10,0)=1.0;
    pSel2(11,0)=1.0;
    const double thresh2 = thresh1;

    assert (KBase::sameShape(wMat, wAdj1));
    assert (KBase::sameShape(wMat, wAdj2));


    auto vhc = new VHCSearch();
    auto eRL = ReportingLevel::Silent;
    auto rRL = ReportingLevel::Low;

    vhc->eval = [eRL, wMat,  uMat,
                 wAdj1,   pSel1,   thresh1,
                 wAdj2,   pSel2,   thresh2,
                 errWeight]
    (const KMatrix & p) {
        auto c12 =  probCost (p,
                                       wMat,  uMat,
                                       wAdj1,   pSel1,   thresh1,
                                       wAdj2,   pSel2,   thresh2,
                                       errWeight, eRL);

        const double eCost =get<0>(c12);
        // to minimize error cost, maximize 100-eCost.
        return 100.0 - eCost;
    };

    vhc->nghbrs = VHCSearch::vn1; // vn2 takes 10 times as long, w/o improvement
    auto p0 = KMatrix(nAct, 1); // all zeros
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
    
    auto c12 = probCost (pBest,
                             wMat,  uMat,
                             wAdj1,   pSel1,   thresh1,
                             wAdj2,   pSel2,   thresh2,
                             errWeight, ReportingLevel::High);


    cout << endl;
    return c12;
}

tuple<double, KMatrix, KMatrix> PMatrixModel::probCost (const KMatrix& pnt,
                               const KMatrix& wMat, const KMatrix& uMat,
                               const KMatrix& wAdj1, const KMatrix& pSel1, double thresh1,
                               const KMatrix& wAdj2, const KMatrix& pSel2, double thresh2,
                               double errWeight, ReportingLevel rl) {
    assert (1 == wMat.numR());
    const unsigned int nAct = wMat.numC();
    const unsigned int nOpt = uMat.numC();

    assert (1 == pnt.numC());
    assert (nAct == pnt.numR());


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
        double s = f + (1.0/f)-2.0;
        return (s*s); // zero if f=1, positive otherwise
    },
    fVec);

    auto pCost = sum(cVec);

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
    auto w1 = KMatrix (1, nAct);
    for (unsigned int i=0; i<nAct; i++) {
        w1(0,i)= wMat(0,i)*fVec(i,0)*wAdj1(0,i);
        //printf("w1[%2i] = %7.2f \n", i, w1(0,i));
        assert (0.0 < w1(0,i));
    }
    auto pDist1 = Model::scalarPCE(nAct, nOpt, w1, uMat, vr, vpm, pcem, rl);
    auto err1 = sPlus(thresh1 - KBase::dot(pDist1, pSel1));
    err1 = err1 * err1;

    // --------------------------------------------
    // cost of falling below threshold in case #1
    auto w2 = KMatrix(1, nAct);
    for (unsigned int i=0; i<nAct; i++) {
        w2(0,i)=wMat(0,i)*fVec(i,0)*wAdj2(0,i);
        //printf("w2[%2i] = %7.2f \n", i, w2(0,i));
        assert(0.0 < w2(0,i));
    }
    auto pDist2 = Model::scalarPCE(nAct, nOpt, w2, uMat, vr, vpm, pcem, rl);
    auto err2 = sPlus(thresh2 - KBase::dot(pDist2, pSel2));
    err2 = err2 * err2;

    const double totalCost = (pCost/errWeight) + ((err1 + err2)*errWeight);

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

        printf("Total cost= %.4f = %f/%.2f + (%f + %f)*%.2f",
               totalCost, pCost, errWeight, err1, err2, errWeight);
        cout << endl << flush;
    }

    auto rslt = tuple<double, KMatrix, KMatrix>(totalCost, w1, w2);
    return rslt;
}

// --------------------------------------------
PMatrixPos::PMatrixPos(PMatrixModel* pm, int n) : EPosition< unsigned int >(pm,n) {
    // nothing else, yet
}

PMatrixPos::~PMatrixPos() {
    // nothing else, yet
}


// --------------------------------------------
PMatrixState::PMatrixState(PMatrixModel* pm) : EState<unsigned int>(pm) {
    // nothing else, yet
}


PMatrixState::~PMatrixState()  {
    // nothing else, yet
}

EState<unsigned int>* PMatrixState::makeNewEState() const  {
    PMatrixModel* pMod = (PMatrixModel*) model;
    PMatrixState* s2 = new PMatrixState(pMod);
    return s2;
}


vector<double> PMatrixState::actorUtilVectFn( int h, int tj) const {
    // no difference in perspective for this demo
    const unsigned int na = eMod->numAct;
    const auto pMod = (const PMatrixModel*) eMod;
    const auto pMat = pMod->getPolUtilMat();
    assert (na == pMat.numR());
    assert (0 <= tj);
    assert (tj < pMat.numC());
    vector<double> rslt = {};
    rslt.resize(na);
    for (unsigned int i=0; i<na; i++) {
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
    assert (Model::minNumActor <= na);
    assert (na <= Model::maxNumActor);
    aUtil = {};
    aUtil.resize(na);
    auto uMat = KMatrix(na,na); // they will all be the same in this demo
    for (unsigned int j=0; j<na; j++) {
        unsigned int nj = posNdx(j);
        auto utilJ = actorUtilVectFn(-1, nj); // all have objective perspective
        for (unsigned int i=0; i<na; i++) {
            uMat(i,j) = utilJ[i];
        }
    }
    for (unsigned int i=0; i<na; i++) {
        aUtil[i] = uMat;
    }
    return;
}

// --------------------------------------------
void demoWFit(uint64_t s) {
    auto c12 = PMatrixModel::minProbError(weightMat, utilMat, 500.0);
    const double score = get<0>(c12);
    
    // retrieve the weight-matrices which were fitted, so we can 
    // use them to assess coalitions in two different situations.
    const KMatrix w1 = get<1>(c12);
    const KMatrix w2 = get<2>(c12);
    
    assert (0 < score);
    assert (KBase::sameShape(w1,w2));
    
    cout << "EMod with BAU-case weights"<<endl<<flush;
    runEKEM(s, false, w1, utilMat);
    
    
    cout << "EMod with change-case weights"<<endl<<flush;
    runEKEM(s, false, w2, utilMat);
    
    
    return;
}
// --------------------------------------------

void demoEKem(uint64_t s, bool cpP) {
    using KBase::lCorr;
    using KBase::mean;

    printf("Using PRNG seed: %020lu \n", s);
    auto rng = new KBase::PRNG(s);

    cout << endl;
    printf("Creating EModel<string> objects ... \n");

    string nChar = "EModel-KEM-Policy";
    auto ekm = new EModel<string>(nChar, s);
    cout << "Populating " << nChar << endl;

    // with such a small set of options, the enumerator
    // function just returns a fixed vector of names.
    ekm->enumOptions = []() {
        return pNamesKEM;
    };
    ekm->setOptions();
    cout << "Now have " << ekm->numOptions() << " enumerated options" << endl;

    const unsigned int maxIter = 50;
    ekm->stop = [maxIter](unsigned int iter, const KBase::State * s) {
        return (iter > maxIter);
    };

    // these just to force instantiation during compilation.
    // It is NOT yet ready to run.
    //auto eks = new EState<string>(ekm);
    //eks->step = [eks] { return eks->stepSUSN(); };
    //ekm->addState(eks);

    auto ekp = new EPosition<string>(ekm, 0); // needs ::print
    auto eka = new EActor<string>(ekm, "Bob", "The second cryptographer");

    cout << "Length of state history: " << ekm->history.size() << endl << flush;

    //delete eks;
    //eks = nullptr;
    delete ekp;
    ekp = nullptr;
    delete eka;
    eka = nullptr;

    // the following gets SIGSEGV
    //delete ekm;
    //ekm = nullptr;


    cout << endl << "====================================="<<endl;
    printf("Creating EKEModel objects ... \n");

    auto eKEM = new PMatrixModel("EModel-Matrix-KEM", s);

    eKEM->pcem = KBase::PCEModel::MarkovIPCM;
    auto wMat = weightMat;
    auto uMat = utilMat;

    if (false) { // randomize?
        wMat = KMatrix::uniform(rng, 1, utilMat.numR(), 1.0, 10.0);
        wMat = KMatrix::map(
        [](double x) {
            return x*x;
        },
        wMat);
        uMat = KMatrix::uniform(rng, utilMat.numR(), utilMat.numC(), 0.0, 1.0);
        uMat = KBase::rescaleRows(uMat, 0.0, 1.0);
    }
    
    runEKEM(s, cpP, wMat, uMat);
/*
    cout << "Actor weight vector: "<<endl;
    wMat.mPrintf("%6.2f ");
    cout << endl;
    
    cout << "Utility(actor, option) matrix:"<<endl;
    uMat.mPrintf("%5.3f ");
    cout << endl;

    eKEM->setWeights(wMat);
    eKEM->setPMatrix(uMat);
    eKEM->setActors(aNamesKEM, aDescKEM);

    eKEM->stop = [maxIter, eKEM](unsigned int iter, const KBase::State * s) {
        bool doneP = iter > maxIter;
        if (doneP) {
            printf("Max iteration limit of %u exceeded \n", maxIter);
        }
        auto s2 = ((const PMatrixState *)(eKEM->history[iter]));
        for (unsigned int i = 0; i < iter; i++) {
            auto s1 = ((const PMatrixState *)(eKEM->history[i]));
            if (eKEM->equivStates(s1, s2)) {
                doneP = true;
                printf("State number %u matched state number %u \n", iter, i);
            }
        }
        return doneP;
    };

    eKEM->setWeights(wMat);
    eKEM->setPMatrix(uMat);
    eKEM->setActors(aNamesKEM, aDescKEM);

    eKEM->stop = [maxIter, eKEM](unsigned int iter, const KBase::State * s) {
        bool doneP = iter > maxIter;
        if (doneP) {
            printf("Max iteration limit of %u exceeded \n", maxIter);
        }
        auto s2 = ((const PMatrixState *)(eKEM->history[iter]));
        for (unsigned int i = 0; i < iter; i++) {
            auto s1 = ((const PMatrixState *)(eKEM->history[i]));
            if (eKEM->equivStates(s1, s2)) {
                doneP = true;
                printf("State number %u matched state number %u \n", iter, i);
            }
        }
        return doneP;
    };


    const unsigned int nOpt = eKEM->numOptions();
    printf("Number of options %i \n", nOpt);
    printf("Number of actors %i \n", eKEM->numAct);


    const auto probTheta = Model::scalarPCE(eKEM->numAct, nOpt ,
                                            wMat, uMat,
                                            VotingRule::Proportional,
                                            eKEM->vpm, eKEM->pcem,
                                            ReportingLevel::Silent);

    const auto p2 = trans(probTheta);
    cout << "PCE over entire option-space:"<<endl;
    p2.mPrintf(" %5.3f ");

    auto zeta = wMat * uMat;
    cout << "Zeta over entire option-space:"<<endl;
    zeta.mPrintf(" %5.1f ");

    auto aCorr = [] (const KMatrix & x, const KMatrix &y) {
        return lCorr(x-mean(x), y-mean(y));
    };

    printf("af-corr(p2,zeta): %.3f \n",  aCorr(p2, zeta));

    auto logP = KMatrix::map([](double x) {
        return log(x);
    }, p2);
    };
    printf("af-corr(logp2,zeta): %.3f \n", aCorr(logP, zeta));

    for (unsigned int i=0; i<nOpt; i++) {
        printf("%2i  %6.4f  %+8.3f  %5.1f  \n", i, p2(0,i), logP(0,i), zeta(0,i));
    }

    double maxZ = -1.0;
    unsigned int ndxMaxZ = 0;
    for (unsigned int i=0; i<nOpt; i++) {
        if (zeta(0,i) > maxZ) {
            maxZ = zeta(0,i);
            ndxMaxZ = i;
        }
    }
    cout << "Central position is number "<<ndxMaxZ <<endl;


    auto es1 = new PMatrixState(eKEM);

    if (cpP) {
        cout << "Assigning actors to the central position"<<endl;
    }
    else {
        cout << "Assigning actors to their self-interested initial positions"<<endl;
    }
    for (unsigned int i=0; i<eKEM->numAct; i++) {
        double maxU = -1.0;
        unsigned int bestJ = 0;
        for (unsigned int j=0; j<nOpt; j++) {
            double uij = uMat(i,j);
            if (uij > maxU) {
                maxU = uij;
                bestJ = j;
            }
        }
        unsigned int ki = cpP ? ndxMaxZ : bestJ;
        auto pi = new PMatrixPos(eKEM, ki);
        es1->addPstn(pi);
    }

    es1->setUENdx();
    eKEM->addState(es1);

    cout << "--------------"<<endl;
    cout << "First state:"<<endl;
    es1->show();

    // see if the templates can be instantiated ...
    es1->step = [es1] { return es1->stepSUSN(); };

    eKEM->run();

    const unsigned int histLen = eKEM->history.size();
    PMatrixState* esA = (PMatrixState*) (eKEM->history[histLen-1]);
    printf("Last State %i \n", histLen-1);
    esA->show();
    */

    delete eKEM;
    eKEM = nullptr;

    delete rng;
    rng = nullptr;

    return;
}

void runEKEM(uint64_t s, bool cpP, const KMatrix& wMat, const KMatrix& uMat) {
    assert (0 != s);
    cout << endl << "====================================="<<endl;
    printf("Creating EKEModel objects ... \n");

    auto eKEM = new PMatrixModel("EModel-Matrix-KEM", s);

    eKEM->pcem = KBase::PCEModel::MarkovIPCM;

    cout << "Actor weight vector: "<<endl;
    wMat.mPrintf("%6.2f ");
    cout << endl;

    cout << "Utility(actor, option) matrix:"<<endl;
    uMat.mPrintf("%5.3f ");
    cout << endl;

    eKEM->setWeights(wMat);
    eKEM->setPMatrix(uMat);
    eKEM->setActors(aNamesKEM, aDescKEM);
    
    const unsigned int maxIter = 1000;

    eKEM->stop = [maxIter, eKEM](unsigned int iter, const KBase::State * s) {
        bool doneP = iter > maxIter;
        if (doneP) {
            printf("Max iteration limit of %u exceeded \n", maxIter);
        }
        auto s2 = ((const PMatrixState *)(eKEM->history[iter]));
        for (unsigned int i = 0; i < iter; i++) {
            auto s1 = ((const PMatrixState *)(eKEM->history[i]));
            if (eKEM->equivStates(s1, s2)) {
                doneP = true;
                printf("State number %u matched state number %u \n", iter, i);
            }
        }
        return doneP;
    };


    const unsigned int nOpt = eKEM->numOptions();
    printf("Number of options %i \n", nOpt);
    printf("Number of actors %i \n", eKEM->numAct);


    const auto probTheta = Model::scalarPCE(eKEM->numAct, nOpt ,
                                            wMat, uMat,
                                            VotingRule::Proportional,
                                            eKEM->vpm, eKEM->pcem,
                                            ReportingLevel::Silent);

    const auto p2 = trans(probTheta);
    cout << "PCE over entire option-space:"<<endl;
    p2.mPrintf(" %5.3f ");

    auto zeta = wMat * uMat;
    cout << "Zeta over entire option-space:"<<endl;
    zeta.mPrintf(" %5.1f ");

    auto aCorr = [] (const KMatrix & x, const KMatrix &y) {
        return lCorr(x-mean(x), y-mean(y));
    };

    printf("af-corr(p2,zeta): %.3f \n",  aCorr(p2, zeta));

    auto logP = KMatrix::map([](double x) {
        return log(x);
    }, p2);
    printf("af-corr(logp2,zeta): %.3f \n", aCorr(logP, zeta));

    for (unsigned int i=0; i<nOpt; i++) {
        printf("%2i  %6.4f  %+8.3f  %5.1f  \n", i, p2(0,i), logP(0,i), zeta(0,i));
    }

    double maxZ = -1.0;
    unsigned int ndxMaxZ = 0;
    for (unsigned int i=0; i<nOpt; i++) {
        if (zeta(0,i) > maxZ) {
            maxZ = zeta(0,i);
            ndxMaxZ = i;
        }
    }
    cout << "Central position is number "<<ndxMaxZ <<endl;


    auto es1 = new PMatrixState(eKEM);

    if (cpP) {
        cout << "Assigning actors to the central position"<<endl;
    }
    else {
        cout << "Assigning actors to their self-interested initial positions"<<endl;
    }
    for (unsigned int i=0; i<eKEM->numAct; i++) {
        double maxU = -1.0;
        unsigned int bestJ = 0;
        for (unsigned int j=0; j<nOpt; j++) {
            double uij = uMat(i,j);
            if (uij > maxU) {
                maxU = uij;
                bestJ = j;
            }
        }
        unsigned int ki = cpP ? ndxMaxZ : bestJ;
        auto pi = new PMatrixPos(eKEM, ki);
        es1->addPstn(pi);
    }

    es1->setUENdx();
    eKEM->addState(es1);

    cout << "--------------"<<endl;
    cout << "First state:"<<endl;
    es1->show();

    // see if the templates can be instantiated ...
    es1->step = [es1] { return es1->stepSUSN(); };

    eKEM->run();

    const unsigned int histLen = eKEM->history.size();
    PMatrixState* esA = (PMatrixState*) (eKEM->history[histLen-1]);
    printf("Last State %i \n", histLen-1);
    esA->show();
    return;
}

} // end of namespace eModKEM


// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
