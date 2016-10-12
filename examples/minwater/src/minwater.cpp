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
// Demonstrate some very basic bargaining
// over how to minimize water usage.
// -------------------------------------------------

#include "vimcp.h"
#include "minwater.h"

#include "2015-10-24-merged-cost.h"
#include "2015-10-24-merged-port.h"
#include "2015-10-24-merged-prod.h"
#include "2015-10-24-merged-scen.h"



namespace DemoWaterMin {
using std::function;
using std::vector;
using std::cout;
using std::endl;
using std::flush;
using std::get;
using std::string;
using std::tuple;

using KBase::VUI;

using KBase::KMatrix;
using KBase::PRNG;
using KBase::dot;
using KBase::joinV;
using KBase::joinH;
using KBase::trans;
using KBase::ReportingLevel;
using KBase::VHCSearch;

using KBase::Actor;
using KBase::Model;
using KBase::Position;
using KBase::State;
using KBase::VotingRule;
using KBase::PCEModel;


// -------------------------------------------------
// row 0: waterchange
// row 3: Dates
// row 5: Wheat

// So scenarios with at least 75% cut to water and at most 5% cut to dates
// are those with date_i/date_0 >= 0.95 and wheat_i/wheat_0 <= 0.25

void setLikelyScenarios(const KMatrix & scen) {
    VUI is;
    const unsigned int numS = scen.numC();
    double d0 = scen(3, 0);
    double w0 = scen(5, 0);

    //cout << "D0: " << d0 << endl;
    //cout << "W0: " << w0 << endl;

    for (unsigned int i = 0; i < numS; i++) {
        double di = scen(3, i);
        double df = di / d0;
        double wi = scen(5, i);
        double wf = wi / w0;
        //printf(" %2i:  df: %.4f  wf: %.4f \n", i, df, wf);
        //if (df >= 0.9499) { cout << "  DF is plausible" << endl; }
        //if (wf <= 0.2501) { cout << "  WF is plausible" << endl; }
        if ((df >= 0.9499) && (wf <= 0.2501)) {
            is.push_back(i);
        }
    }

    likelyScenarios = is;

    cout << "Likely scenario numbers: ";
    for (auto i : likelyScenarios) {
        cout << i << " ";
    }
    cout << endl << flush;

    return;
}


void setUInit(const KMatrix& sq) {
    const unsigned int na = sq.numR();
    const unsigned int np = sq.numC();
    const double eps = 1E-6;

    auto val = KMatrix(na, np);
    uInit = KMatrix(na, np);

    for (unsigned int i = 0; i < na; i++) {
        double minQ = sq(i, 0);
        double maxQ = sq(i, 0);
        for (unsigned int j = 0; j < np; j++) {
            double qi = sq(i, j);
            if (qi < minQ) {
                minQ = qi;
            }
            if (maxQ < qi) {
                maxQ = qi;
            }
        }
        maxQ = maxQ + (2.0*eps);
        for (unsigned int j = 0; j < np; j++) {
            double vij = (eps + sq(i, j) - minQ) / (maxQ - minQ);
            assert(0.0 <= vij);
            assert(vij <= 1.0);
            double uij = 1.0 - ((1.0 - vij)*(1.0 - vij));
            uInit(i, j) = uij;
        }
    }
    return;
}

// -------------------------------------------------
double waterMinProb(ReportingLevel rl, const KMatrix & p0) {
    // Report the RMS error in predicted probabilities
    // TODO: change this to get both initial-year and nominal-policy
    // p0 is a set of modifiers on initial weights, w

    //const unsigned int numA = 4;
    //const unsigned int numP = 4;
    assert(p0.numR() == numA);
    assert(p0.numC() == 1);

    // base weights
    //double wArray[] = { 1600, 7000, 100, 1300 }; // numA
    //const KMatrix w0 = KMatrix::arrayInit(wArray, numA, 1);
    const double w0Sum = KBase::sum(w0);

    KMatrix w = KMatrix(numA, 1);
    for (unsigned int i = 0; i < numA; i++) {
        double pi = p0(i, 0);
        w(i, 0) = exp(pi) * w0(i, 0);
    }
    double wSum = KBase::sum(w);
    w = (w0Sum / wSum)*w;

    double pRMS = KBase::norm(p0) / sqrt(p0.numC() * p0.numR()); // RMS of p0;

    auto vpm = KBase::VPModel::Linear;
    auto pcem = PCEModel::ConditionalPCM;

    // voting in base-year scenario
    auto v0fn = [w, p0](unsigned int k, unsigned int i, unsigned int j) {
        double wk = w(k, 0);
        if (0 == k) {
            wk = wk / 1.0E5;
        }
        double vkij = Model::vote(KBase::VotingRule::Proportional, wk, uInit(k, i), uInit(k, j));
        return vkij;
    };
    auto c0 = Model::coalitions(v0fn, numA, numP); // [numP, numP]
    const auto ppv0 = Model::probCE2(pcem, vpm, c0);
    const auto pr0 = get<0>(ppv0); // [numP, 1]
    const auto pv0 = get<1>(ppv0); // [numP, numP]

    if (KBase::testProbCE) {
      cout << "Testing probCE in waterMinProb-0" << endl << flush;
      auto pv00 = Model::vProb(vpm, c0); // [numP, numP]
      assert(KBase::norm(pv0 - pv00) < 1E-6);
      auto pr00 = Model::probCE(PCEModel::ConditionalPCM, pv00); // [numP, 1]
      assert(KBase::norm(pr0 - pr00) < 1E-6);
    }

    auto priorBase = pr0(0, 0);
    double err0 = trgtP0 - priorBase; // shortfall if positive
    if (err0 < 0.0) {
        err0 = 0.0;
    }

    // voting in nominal-policy scenario
    auto v1fn = [w, p0](unsigned int k, unsigned int i, unsigned int j) {
        double vkij = Model::vote(KBase::VotingRule::Proportional, w(k, 0), uInit(k, i), uInit(k, j));
        return vkij;
    };
    const auto c1 = Model::coalitions(v1fn, numA, numP); // [numP, numP]
    const auto ppv1 = Model::probCE2(pcem, vpm, c1);
    const auto pr1 = get<0>(ppv1); // [numP, 1]
    const auto pv1 = get<1>(ppv1); // [numP, numP]

    if (KBase::testProbCE) {
      cout << "Testing probCE in waterMinProb-1" << endl << flush;
      const auto pv10 = Model::vProb(vpm, c1); // [numP, numP]
      assert(KBase::norm(pv1 - pv10) < 1E-6);
      const auto pr10 = Model::probCE(PCEModel::ConditionalPCM, pv10); // [numP, 1]
      assert(KBase::norm(pr1 - pr10) < 1E-6);
    }

    double postNom = 0.0;
    for (auto i : likelyScenarios) {
        postNom = postNom + pr1(i, 0);
    }
    double err1 = trgtP1 - postNom; // shortfall if positive
    if (err1 < 0.0) {
        err1 = 0.0;
    }



    //double err = KBase::norm(pr1 - pInit) / sqrt(pr1.numC() * pr1.numR()); // RMS of difference in distributions

    double err = sqrt(((err0*err0) + (err1*err1)) / 2.0); // RMS difference of the two critical probabilities


    if (ReportingLevel::Silent < rl) {
        cout << "Actor-cap matrix" << endl;
        w.mPrintf(" %8.1f ");
        cout << endl << flush;

        if (ReportingLevel::Low < rl) {
            cout << "Raw actor-pos util matrix" << endl;
            uInit.mPrintf(" %.4f ");
            cout << endl << flush;

            cout << "Coalition strength matrix" << endl;
            c1.mPrintf(" %+9.3f ");
            cout << endl << flush;

            cout << "Probability Opt_i > Opt_j" << endl;
            pv1.mPrintf(" %.4f ");
            cout << endl;
        }

        cout << "Estimated prior probability Opt_i" << endl;
        for (unsigned int i = 0; i < pr0.numR(); i++) {
            printf("%2u , %6.4f \n", i, pr0(i, 0));
        }
        //pr0.printf(" %.4f ");
        cout << endl << flush;

        cout << "Estimated posterior probability Opt_i" << endl;
        for (unsigned int i = 0; i < pr1.numR(); i++) {
            printf("%2u , %6.4f \n", i, pr1(i, 0));
        }
        //pr1.printf(" %.4f ");
        cout << endl << flush;

        printf("Probability of base case %.3f ( %.3f )\n", priorBase, trgtP0);

        printf("Probability of nominal case(s) %.3f ( %.3f )\n", postNom, trgtP1);

        printf("RMSE of probabilities: %.4f \n", err);

        printf("RMS of weight factors: %.4f \n", pRMS);
        cout << endl << flush;
    }

    return (err + (pRMS * prmsW));
}

void minProbErr() {


    //const unsigned int numA = 4;
    auto vhc = new VHCSearch();
    auto eRL = ReportingLevel::Low;
    auto rRL = ReportingLevel::Medium;

    vhc->eval = [eRL](const KMatrix & wm) {
        const double err = waterMinProb(eRL, wm);
        return 1 - err;
    };

    vhc->nghbrs = VHCSearch::vn1;
    auto p0 = KMatrix(numA, 1); // all zeros
    cout << "Initial point: ";
    trans(p0).mPrintf(" %+.4f ");
    cout << endl;
    auto rslt = vhc->run(p0,
                         100, 10, 1E-5, // iMax, sMax, sTol
                         1.0, 0.618, 1.25, 1e-8, // step, shrink, grow, minStep
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
    return;
}


RsrcMinLP::RsrcMinLP() {
    clear();
}



RsrcMinLP::~RsrcMinLP() {
    clear();
}



void RsrcMinLP::clear() {
    numPortC = 0;
    numProd = 0;
    xInit = KMatrix();
    rCosts = KMatrix();
    bounds = KMatrix();
    portRed = KMatrix();
    portWghts = KMatrix();
    numSpplyC = 0;
    spplyWghts = KMatrix();
    dmndWghts = KMatrix();
    pNames = vector<string>();
}

RsrcMinLP* RsrcMinLP::makeRMLP(PRNG* rng, unsigned int numPd, unsigned int numPt, unsigned int numSD) {
    auto rmlp = new RsrcMinLP();
    rmlp->numProd = numPd;
    rmlp->numPortC = numPt;
    rmlp->numSpplyC = numSD;
    rmlp->xInit = KMatrix::uniform(rng, numPd, 1, 100.0, 990.0);
    rmlp->rCosts = KMatrix::uniform(rng, numPd, 1, 10.0, 90.0);

    const unsigned int nameLen = 20;
    for (unsigned int i = 0; i < numPd; i++) {
        auto nameBuff = new char[nameLen];
        sprintf(nameBuff, "Prod-%02u", i);
        rmlp->pNames.push_back(nameBuff);
    }

    // set reduction/growth fractions
    auto rgFn = [rng](unsigned int i, unsigned int j) {
        double f = 0.0;
        switch (j) {
        case 0: // reduction
            f = rng->uniform(0.05, 0.20); // at most 5% to 20% decrease
            break;
        case 1: // growth
            f = rng->uniform(0.50, 1.00); // at most 50% to 100% increase (i.e. double)
            break;
        default:
            assert(false);
            break;
        }
        return f;
    };
    rmlp->bounds = KMatrix::map(rgFn, numPd, 2);

    // set portfolio weights
    auto pwfn = [rng](unsigned int r, unsigned int c) {
        double wij = rng->uniform(0.0, 1.0);
        if (wij < 0.5) {
            wij = 0.0; // not in this portfolio
        }
        return wij;
    };
    rmlp->portWghts = KMatrix::map(pwfn, numPt, numPd);
    rmlp->portRed = KMatrix::uniform(rng, numPt, 1, 0.0, 0.10); // reductions are 0 to 10 percent

    // set supply/demand weights
    rmlp->spplyWghts = KMatrix::uniform(rng, numSD, numPd, 0.0, 1.0);
    rmlp->dmndWghts = KMatrix::uniform(rng, numSD, numPd, 0.0, 1.0);

    return rmlp;
}

void waterMin() {

    setUInit(scenQuant);

    cout << "uInit: " << endl;
    uInit.mPrintf(" %.3f ");
    cout << endl << flush;

    setLikelyScenarios(scenQuant);

    auto p = DemoWaterMin::waterMinProb(KBase::ReportingLevel::Medium, KBase::KMatrix(numA, 1));

    cout << "Error-minimizing search ..." << endl << flush;
    DemoWaterMin::minProbErr();
    return;
}

void demoRMLP(PRNG* rng) {


    auto pfn = [](string lbl, string f, KMatrix m) {
        cout << lbl << endl;
        m.mPrintf(f);
        cout << endl << flush;
        return;
    };

    const unsigned int numP = 20;
    const unsigned int numC = numP / 5;
    const unsigned int numS = numP / 5;
    const unsigned int iterLim = 100 * 1000;

    auto rmlp = RsrcMinLP::makeRMLP(rng, numP, numC, numS);
    printf("Number of products: %i \n", rmlp->numProd);
    pfn("Initial X: ", "%.2f ", rmlp->xInit);
    pfn("Resource costs: ", "%.2f ", rmlp->rCosts);
    const double rsrc0 = dot(rmlp->xInit, rmlp->rCosts);
    printf("Initial cost: %.2f \n", rsrc0);
    cout << endl << flush;

    pfn("Fractional reduction / growth bounds: ", "%8.4f ", rmlp->bounds);

    auto rBounds = KMatrix(rmlp->numProd, 1);
    auto gBounds = KMatrix(rmlp->numProd, 1);
    for (unsigned int i = 0; i < rmlp->numProd; i++) {
        double xi = rmlp->xInit(i, 0);
        assert(0.0 <= xi);
        double ri = rmlp->bounds(i, 0);
        assert(0.0 <= ri);
        assert(ri <= 1.10); // 100% reduction is OK, but not 110% (that would be negative)
        double gi = rmlp->bounds(i, 1);
        assert(-1.0 <= gi); // can force up to 100% reduction
        assert(1.0 - ri <= 1.0 + gi);
        rBounds(i, 0) = (1.0 - ri)*xi;
        gBounds(i, 0) = (1.0 + gi)*xi;
    }

    pfn("Reduction bounds: ", "%7.2f ", rBounds);
    pfn("Growth bounds: ", "%7.2f ", gBounds);

    printf("Number of portfolios: %i \n", rmlp->numPortC);
    pfn("Portfolio weights: ", "%.3f ", rmlp->portWghts);
    pfn("Portfolio reductions: ", "%.3f ", rmlp->portRed);

    auto initPV = rmlp->portWghts * rmlp->xInit;
    pfn("Initial portfolio values: ", "%8.2f", initPV);
    auto portFn = [initPV, rmlp](unsigned int i, unsigned int j) {
        double bij = initPV(i, j)*(1 - rmlp->portRed(i, j));
        return bij;
    };
    auto minPV = KMatrix::map(portFn, initPV.numR(), initPV.numC());
    pfn("Minimum portfolio values: ", "%8.2f", minPV);

    printf("Number of supply/demand ratio constraints: %i \n", rmlp->numSpplyC);

    auto initS = rmlp->spplyWghts * rmlp->xInit;
    assert(1 == initS.numC());
    assert(initS.numR() == rmlp->numSpplyC);
    auto initD = rmlp->dmndWghts * rmlp->xInit;
    assert(1 == initD.numC());
    assert(initD.numR() == rmlp->numSpplyC);
    pfn("Initial supply values: ", "%8.2f", initS);
    pfn("Initial demand values: ", "%8.2f", initD);

    auto ratFn = [initS, initD](unsigned int i, unsigned int j) {
        double rij = initS(i, j) / initD(i, j);
        return rij;
    };
    auto sdRatio = KMatrix::map(ratFn, initS.numR(), initS.numC());
    assert(1 == sdRatio.numC());
    assert(sdRatio.numR() == rmlp->numSpplyC);
    pfn("S/D ratios: ", "%8.4f", sdRatio);

    auto sdFn = [rmlp, sdRatio](unsigned int i, unsigned int j) {
        double sdij = rmlp->spplyWghts(i, j) - (sdRatio(i, 0) * rmlp->dmndWghts(i, j));
        return sdij;
    };
    auto sdMat = KMatrix::map(sdFn, rmlp->numSpplyC, rmlp->numProd);

    // assemble all the above into a problem like
    // min c*x
    // Ax >= b
    // x >= 0

    // start with portfolio constraints
    auto matA = rmlp->portWghts;
    auto matB = minPV;

    matA = joinV(matA, sdMat);
    matB = joinV(matB, KMatrix(rmlp->numSpplyC, 1)); // zero-filled

    auto ident = KBase::iMat(rmlp->numProd);

    // new value >= lower bound
    matA = joinV(matA, ident);
    matB = joinV(matB, rBounds);

    // new value <= upper bound, or
    // -(new value) >= -(upper bound)
    matA = joinV(matA, -1.0 * ident);
    matB = joinV(matB, -1.0 * gBounds);

    pfn("Full A-matrix: ", "%+.3f ", matA);
    pfn("Full b-vector: ", "%+.3f ", matB);

    const unsigned int N = rmlp->numProd;
    const unsigned int K1 = rmlp->numPortC;
    const unsigned int K2 = rmlp->numSpplyC;
    const unsigned int M = (K1 + K2) + (2 * N);
    assert(M == matA.numR());
    assert(N == matA.numC());
    assert(M == matB.numR());
    assert(1 == matB.numC());

    auto topMat = joinH(KMatrix(N, N), -1.0*trans(matA));
    auto botMat = joinH(matA, KMatrix(M, M));

    auto matM = joinV(topMat, botMat);
    auto matQ = joinV(rmlp->rCosts, -1.0 * matB);

    // reaffirm that everything is structured as expected
    assert(3 * N + (K1 + K2) == matM.numR());
    assert(3 * N + (K1 + K2) == matM.numC());
    assert(3 * N + (K1 + K2) == matQ.numR());
    assert(1 == matQ.numC());

    const double eps = 1E-6;

    // if it were scalar, sfe(1.01, 1.00) ~~ 0.01
    auto sfe = [](const KMatrix & x, const KMatrix & y) {
        return (2 * norm(x - y)) / (norm(x) + norm(y));
    };

    auto processRslt = [N, sfe, matM, matQ, eps](tuple<KMatrix, unsigned int, KMatrix> r) {
        KMatrix u = get<0>(r);
        unsigned int iter = get<1>(r);
        KMatrix res = get<2>(r);
        printf("After %u iterations  \n", iter);
        cout << "  LCP solution u:  ";
        trans(u).mPrintf(" %+.3f ");
        cout << endl << flush;
        cout << "  LCP residual r:  ";
        trans(res).mPrintf(" %+.3f ");
        cout << endl << flush;
        cout << "Dimensions: " << res.numR() << endl << flush;
        KMatrix v = matM*u + matQ;
        double tol = 100 * eps;
        double e1 = sfe(u, u);
        double e2 = sfe(v, matM*u + matQ);
        printf("SFE of u is %.3E,  SFE of v is %.3E \n", e1, e2);
        assert(e1 < tol); // inaccurate U
        assert(e2 < tol); // inaccurate V
        auto sFn = [u](unsigned int i, unsigned int j) {
            assert(0 == j);
            return u(i, 0);
        };
        auto x = KMatrix::map(sFn, N, 1);
        cout << "  LP solution x:  ";
        trans(x).mPrintf(" %+.3f ");
        cout << endl << flush;
        return x;
    };

    auto start = joinV(rmlp->xInit, KMatrix(M, 1));
    auto F = [matM, matQ](const KMatrix & x) {
        return (matM*x + matQ);
    };

    if (true) {
        try {
            cout << endl << flush;
            cout << "Solve via BSHe96" << endl;
            auto r1 = viBSHe96(matM, matQ, KBase::projPos, start, eps, iterLim);
            auto x1 = processRslt(r1);

            printf("Initial resource usage:   %10.2f \n", rsrc0);
            const double rsrc1 = dot(x1, rmlp->rCosts);
            printf("Minimized resource usage: %10.2f \n", rsrc1);
            printf("Percentage change %+.3f", (100.0*(rsrc1 - rsrc0) / rsrc0));
        }
        catch (...) {
            cout << "Caught exception" << endl << flush;
        }
    }

    if (true) {
        try {
            cout << endl << flush;
            cout << "Solve via AEG" << endl;
            auto r2 = viABG(start, F, KBase::projPos, 0.5, eps, iterLim, true);
            auto x2 = processRslt(r2);
            printf("Initial resource usage:   %10.2f \n", rsrc0);
            const double rsrc2 = dot(x2, rmlp->rCosts);
            printf("Minimized resource usage: %10.2f \n", rsrc2);
            printf("Percentage change %+.3f", (100.0*(rsrc2 - rsrc0) / rsrc0));
        }
        catch (...) {
            cout << "Caught exception" << endl << flush;
        }
    }

    if (true) { //  exceeds the iteration limit much more often than BSHe96
        try {
            cout << endl << flush;
            cout << "Solve via ABG" << endl;
            auto r2 = viABG(start, F, KBase::projPos, 0.5, eps, iterLim, false);
            auto x2 = processRslt(r2);
            printf("Initial resource usage:   %10.2f \n", rsrc0);
            const double rsrc2 = dot(x2, rmlp->rCosts);
            printf("Minimized resource usage: %10.2f \n", rsrc2);
            printf("Percentage change %+.3f", (100.0*(rsrc2 - rsrc0) / rsrc0));
        }
        catch (...) {
            cout << "Caught exception" << endl << flush;
        }
    }

    delete rmlp;
    rmlp = nullptr;
    return;
}

} // namespace

// -------------------------------------------------

int main(int ac, char **av) {
    using std::cout;
    using std::endl;
    using std::flush;
    using KBase::PRNG;
    using KBase::dSeed;

    auto sTime = KBase::displayProgramStart();

    bool waterMinP = false;
    bool rmlpP = false;
    uint64_t seed = dSeed;
    bool run = true;

    // tmp args
    //rmlpP = true;

    auto showHelp = []() {
        printf("\n");
        printf("Usage: specify one or more of these options\n");
        printf("--waterMin   minimize RMS error in probabilities\n");
        printf("--rmlp       demo resource-minimizing linear program\n");
        printf("--help       print this message\n");
        printf("--seed <n>   set a 64bit seed\n");
        printf("             0 means truly random\n");
        printf("             default: %020llu \n", dSeed);
    };

    if (ac > 1) {
        for (int i = 1; i < ac; i++) {
            if (strcmp(av[i], "--seed") == 0) {
                i++;
                seed = std::stoull(av[i]);
            }
            else if (strcmp(av[i], "--waterMin") == 0) {
                waterMinP = true;
            }
            else if (strcmp(av[i], "--rmlp") == 0) {
                rmlpP = true;
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

    PRNG * rng = new PRNG();
    seed = rng->setSeed(seed); // 0 == get a random number
    printf("Using PRNG seed:  %020llu \n", seed);
    printf("Same seed in hex:   0x%016llX \n", seed);

    if (waterMinP) {
        DemoWaterMin::waterMin();
    }
    if (rmlpP) {
        DemoWaterMin::demoRMLP(rng);
    }

    delete rng;
    rng = nullptr;
    KBase::displayProgramEnd(sTime);
    return 0;
}


// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
