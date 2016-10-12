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


#include "demoMatrixEMod.h"


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
            auto pi = new unsigned int (i);
            theta[i] = pi;
        }
    }

    assert (minNumActor <= numAct);
    assert (numAct <= maxNumActor);

    assert (minNumOptions <= numOptions());
    
    for (auto u : pm0) {
      assert (0.0 <= u);
      assert (u <= 1.0);
    }

    // if it is OK, set it
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


PMatrixPos::PMatrixPos(PMatrixModel* pm, int n) : EPosition< unsigned int >(pm,n) {
    // nothing else, yet
}

PMatrixPos::~PMatrixPos() {
    // nothing else, yet
}


// --------------------------------------------

void demoEKem(uint64_t s) {
    using KBase::lCorr;
    using KBase::mean;
    printf("Using PRNG seed: %020llu \n", s);

    cout << endl;
    printf("Creating EModel<char> objects ... \n");

    string nChar = "EModel-KEM-Policy";
    auto ekm = new EModel<char>(nChar, s);
    cout << "Populating " << nChar << endl;

    // with such a small set of options, the enumerator
    // function just returns a fixed vector of names.
    ekm->enumOptions = []() {
        return thetaKEM;
    };
    ekm->setOptions();
    cout << "Now have " << ekm->numOptions() << " enumerated options" << endl;

    const unsigned int maxIter = 5;
    ekm->stop = [maxIter](unsigned int iter, const KBase::State * s) {
        return (iter > maxIter);
    };

    // these just to force instantiation during compilation.
    // It is NOT yet ready to run.
    auto eks = new EState<char>(ekm);
    eks->step = [eks] { return eks->stepSUSN(); };
    ekm->addState(eks);

    auto ekp = new EPosition<char>(ekm, 0); // needs ::print
    auto eka = new EActor<char>(ekm, "Bob", "The second cryptographer");

    cout << "Length of state history: " << ekm->history.size() << endl << flush;

    // cannot delete this, because it would try to delete some
    // constant strings, and crash.
    // printf("Deleting %s ... \n", nChar.c_str());
    // delete ekm;

    cout << endl;
    printf("Creating EKEModel objects ... \n");

    auto eKEM = new PMatrixModel("EModel-Matrix-KEM", s);
    
    eKEM->pcem = KBase::PCEModel::ConditionalPCM;
    eKEM->pcem = KBase::PCEModel::MarkovUPCM;

    eKEM->setWeights(weightMat);
    eKEM->setPMatrix(utilMat);

    const auto ep1 = new PMatrixPos(eKEM, 17);


    const auto probTheta = Model::scalarPCE(eKEM->numAct, eKEM->numOptions() , weightMat,
                                      utilMat, VotingRule::Proportional,
                                      eKEM->vpm, eKEM->pcem, ReportingLevel::Medium);
    
    const auto p2 = trans(probTheta);
    cout << "PCE over entire option-space:"<<endl;
    p2.mPrintf(" %4.2f "); 
    
    auto zeta = weightMat * utilMat;
    cout << "Zeta over entire option-space:"<<endl;
    zeta.mPrintf(" %5.1f");
    
    auto aCorr = [] (const KMatrix & x, const KMatrix &y) { return lCorr(x-mean(x), y-mean(y)); };
    
    printf("af-corr(p2,zeta): %.3f \n",  aCorr(p2, zeta));
    
    
    auto lij = [p2] (unsigned int i, unsigned int j) { return log(p2(i,j));};
    auto logP = KMatrix::map(lij, p2.numR(), p2.numC());
    printf("af-corr(logp2,zeta): %.3f \n", aCorr(logP, zeta));
    
    for (unsigned int i=0; i<eKEM->numOptions(); i++) {
      printf("%2i  %6.4f  %+8.3f  %5.1f  \n", i, p2(0,i), logP(0,i), zeta(0,i));
    }
    
    
    delete ep1;
    //ep1 = nullptr;
    delete eKEM;
    eKEM = nullptr;
    return;
}

} // end of namespace eModKEM


// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
