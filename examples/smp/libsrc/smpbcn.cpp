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
// Demonstrate a very basic, but highly parameterizable, Spatial Model of Politics.
//
// --------------------------------------------

#include "smp.h"


namespace SMPLib {
using std::cout;
using std::endl;
using std::flush;
using std::function;
using std::get;
using std::string;

using KBase::PRNG;
using KBase::KMatrix;
using KBase::KException;
using KBase::Actor;
using KBase::Model;
using KBase::Position;
using KBase::VctrPstn;
using KBase::BigRAdjust;
using KBase::BigRRange;
using KBase::VPModel;
using KBase::State;
using KBase::StateTransMode;
using KBase::VotingRule;
using KBase::PCEModel;
using KBase::ReportingLevel;


// --------------------------------------------
uint64_t BargainSMP::highestBargainID = 1000;

// big enough buffer to build all desired SQLite statements
const unsigned int sqlBuffSize = 250;

// --------------------------------------------
string bModName(const SMPBargnModel& bMod) {
    string rs = "Unrecognized SMPBargnModel";
    switch (bMod) {
    case SMPBargnModel::InitOnlyInterpSMPBM:
        rs = "InitOnlyInterpSMPBM";
        break;

    case SMPBargnModel::InitRcvrInterpSMPBM:
        rs = "InitRcvrInterpSMPBM";
        break;

    case SMPBargnModel::PWCompInterSMPBM:
        rs = "PWCompInterSMPBM";
        break;
    default:
        cout << "bModName: Unrecognized SMPBargnModel"<<endl<<flush;
        break;
    }
    return rs;
}

ostream& operator<< (ostream& os, const SMPBargnModel& bMod) {
    os << bModName(bMod);
    return os;
}
// --------------------------------------------

BargainSMP::BargainSMP(const SMPActor* ai, const SMPActor* ar, const VctrPstn & pi, const VctrPstn & pr) {
    assert(nullptr != ai);
    assert(nullptr != ar);
    actInit = ai;
    actRcvr = ar;
    posInit = pi;
    posRcvr = pr;
    myBargainID = BargainSMP::highestBargainID++;
}

BargainSMP::~BargainSMP() {
    actInit = nullptr;
    actRcvr = nullptr;
    posInit = VctrPstn(KMatrix(0, 0));
    posRcvr = VctrPstn(KMatrix(0, 0));
    auto mbid = myBargainID;
    myBargainID = 0;
}


uint64_t BargainSMP::getID() const {
    return myBargainID;
}


// --------------------------------------------


SMPState* SMPState::doBCN() const {
    const bool recordBargainingP = true;
    auto brgns = vector< vector < BargainSMP* > >();
    const unsigned int na = model->numAct;
    brgns.resize(na);
    for (unsigned int i = 0; i < na; i++) {
        brgns[i] = vector<BargainSMP*>();
        brgns[i].push_back(nullptr); // null bargain is SQ
    }

    const int t = myTurn();
    assert(0 <= t); // need to be in the model's history list

    auto ivb = SMPActor::InterVecBrgn::S2P2;
    //sqlite3_stmt* stmt = NULL; // never used
    // int rowtoupdate = 0; // never used
    //int rc = 0; // never used
    // For each actor, identify good targets, and propose bargains to them.
    // (This loop would be an excellent place for high-level parallelism)
    for (unsigned int i = 0; i < na; i++) {
        auto chlgI = bestChallenge(i, recordBargainingP);
        const double bestEU = get<2>(chlgI);
        if (0 < bestEU) {
            const int bestJ = get<0>(chlgI); //
            const double piiJ = get<1>(chlgI); // i's estimate of probability i defeats j
            assert(0 <= bestJ);
            const unsigned int j = bestJ; // for consistency in code below

            const int t = myTurn();
            assert(0 <= t); // need to be on model's history list
            printf("In turn %i actor %u has most advantageous target %u worth %.3f\n", t, i, j, bestEU);

            auto ai = ((const SMPActor*)(model->actrs[i]));
            auto aj = ((const SMPActor*)(model->actrs[j]));
            auto posI = ((const VctrPstn*)pstns[i]);
            auto posJ = ((const VctrPstn*)pstns[j]);

            // Look for counter-intuitive cases
            if (piiJ < 0.5) {
                cout << "turn " << t << " , ";
                cout << "i " << i << " , ";
                cout << "j " << j << " , ";
                cout << "bestEU worth " << bestEU << " , ";
                cout << "piiJ " << piiJ << endl;
                cout << endl << flush; // get it printed
                //assert(0.5 <= piiJ);
            }


            {   // make the variables local to lexical scope of this block.
                // for testing, calculate and print out a block of data showing each's perspective
                const bool recordTmpSQLP = false;  // Do not record this in SQLite
                auto pFn = [this, recordTmpSQLP](unsigned int h, unsigned int k, unsigned int i, unsigned int j) {
                    auto est = probEduChlg(h, k, i, j, recordTmpSQLP); // H's estimate of the effect on K of I->J
                    double phij = get<0>(est);
                    double edu_hk_ij = get<1>(est);
                    printf("Est by %2u of prob %.4f that [%2u>%2u], with expected gain to %2u of %+.4f \n",
                           h, phij, i, j, k, edu_hk_ij);
                };

                pFn(i, i, i, j); // I's estimate of the effect on I of I->J
                pFn(i, j, i, j); // I's estimate of the effect on J of I->J

                pFn(j, i, i, j); // J's estimate of the effect on I of I->J
                pFn(j, j, i, j); // J's estimate of the effect on I of I->J
            }

            // interpolate a bargain from I's perspective
            BargainSMP* brgnIIJ = SMPActor::interpolateBrgn(ai, aj, posI, posJ, piiJ, 1 - piiJ, ivb);
            const int nai = model->actrNdx(brgnIIJ->actInit);
            const int naj = model->actrNdx(brgnIIJ->actRcvr);
            // verify that identities match up as expected
            assert(nai == i);
            assert(naj == j);

            // interpolate a bargain from targeted J's perspective
            auto Vjij = probEduChlg(j, i, i, j, recordBargainingP);
            double pjiJ = get<1>(Vjij); // j's estimate of the probability that i defeats j
            BargainSMP* brgnJIJ = SMPActor::interpolateBrgn(ai, aj, posI, posJ, pjiJ, 1 - pjiJ, ivb);

            // calcluate weights as capability times salience
            double sci = brgnIIJ->actInit->sCap;
            double svi = sum(brgnIIJ->actInit->vSal);
            double wi = sci*svi;
            double scj = brgnJIJ->actInit->sCap;
            double svj = sum(brgnIIJ->actRcvr->vSal);
            double wj = scj*svj;

            // create a new bargain whose positions are the weighted averages
            auto bpi = VctrPstn((wi*brgnIIJ->posInit + wj*brgnJIJ->posInit) / (wi + wj));
            auto bpj = VctrPstn((wi*brgnIIJ->posRcvr + wj*brgnJIJ->posRcvr) / (wi + wj));
            BargainSMP *brgnIJ = new  BargainSMP(brgnIIJ->actInit, brgnIIJ->actRcvr, bpi, bpj);

            printf("\n");
            printf("Bargain ");
            showOneBargain(brgnIIJ);
            printf(" from %2u's perspective (brgnIIJ) \n", i);
            printf("  %2u proposes %2u adopt: ", i, i);
            KBase::trans(brgnIIJ->posInit).mPrintf(" %.3f ");


            printf("  %2u proposes %2u adopt: ", i, j);
            KBase::trans(brgnIIJ->posRcvr).mPrintf(" %.3f ");
            printf("\n");
            printf("Bargain  ");
            showOneBargain(brgnJIJ);
            printf(" from %2u's perspective (brgnJIJ) \n", j);
            printf("  %2u proposes %2u adopt: ", j, i);

            KBase::trans(brgnJIJ->posInit).mPrintf(" %.3f ");
            printf("  %2u proposes %2u adopt: ", j, j);
            KBase::trans(brgnJIJ->posRcvr).mPrintf(" %.3f ");
            //Brgn table base entries


            printf("\n");
            printf("Power-weighted compromise  ");
            showOneBargain(brgnIJ);
            printf(" bargain (brgnIJ) \n");
            printf("  compromise proposes %2u adopt: ", i);
            KBase::trans(brgnIJ->posInit).mPrintf(" %.3f ");
            printf("  compromise proposes %2u adopt: ", j);
            KBase::trans(brgnIJ->posRcvr).mPrintf(" %.3f ");
            printf("\n");


            // TODO: make one-perspective an option.
            // For now, emulate it by swapping
            //auto tIJ = brgnIJ;
            //auto tIIJ = brgnIIJ;
            //brgnIJ = tIIJ;
            //brgnIIJ = tIJ;

            // TODO: what does this data represent?
            //Brgn table base entries
            // JAH 20160802 control logging added
            if (model->sqlFlags[3])
            {
                model->sqlBargainEntries(t, brgnJIJ->getID(), i, i, i, bestEU);
                model->sqlBargainEntries(t, brgnIIJ->getID(), i, i, j, bestEU);
             }

            cout << "Using "<<bMod<<" to form proposed bargains" << endl;
            switch (bMod) {
            case SMPBargnModel::InitOnlyInterpSMPBM:
                // record the only one used into SQLite JAH 20160802 use the flag
                if(model->sqlFlags[3])
                {
                    model->sqlBargainCoords(t, brgnIIJ->getID(), brgnIIJ->posInit, brgnIIJ->posRcvr);
                }
                // record this one onto BOTH the initiator and receiver queues
                brgns[i].push_back(brgnIIJ); // initiator's copy, delete only it later
                brgns[j].push_back(brgnIIJ); // receiver's copy, just null it out later
                // clean up unused
                delete brgnIJ;
                brgnIJ = nullptr;
                delete brgnJIJ;
                brgnJIJ = nullptr;
                break;


            case SMPBargnModel::InitRcvrInterpSMPBM:
                // record the pair used into SQLite JAH 20160802 use the flag
                if(model->sqlFlags[3])
                {
                    model->sqlBargainCoords(t, brgnIIJ->getID(), brgnIIJ->posInit, brgnIIJ->posRcvr);
                    model->sqlBargainCoords(t, brgnJIJ->getID(), brgnJIJ->posInit, brgnJIJ->posRcvr);
                }
                // record these both onto BOTH the initiator and receiver queues
                brgns[i].push_back(brgnIIJ); // initiator's copy, delete only it later
                brgns[i].push_back(brgnJIJ); // initiator's copy, delete only it later
                brgns[j].push_back(brgnIIJ); // receiver's copy, just null it out later
                brgns[j].push_back(brgnJIJ); // receiver's copy, just null it out later
                // clean up unused
                delete brgnIJ;
                brgnIJ = nullptr;
                break;


            case SMPBargnModel::PWCompInterSMPBM:
                // record the only one used into SQLite JAH 20160802 use the flag
                if(model->sqlFlags[3])
                {
                    model->sqlBargainCoords(t, brgnIJ->getID(), brgnIJ->posInit, brgnIJ->posRcvr);
                }
                // record this one onto BOTH the initiator and receiver queues
                brgns[i].push_back(brgnIJ); // initiator's copy, delete only it later
                brgns[j].push_back(brgnIJ); // receiver's copy, just null it out later
                // clean up unused
                delete brgnIIJ;
                brgnIIJ = nullptr;
                delete brgnJIJ;
                brgnJIJ = nullptr;
                break;

            default:
                cout << "SMPState::doBCN unrecognized SMPBargnModel" << endl << flush;
                assert(false);
            }

        }
        else {
            printf("Actor %u has no advantageous targets \n", i);
        }
    }

    // PRNG* rng = model->rng; // was never used

    cout << endl << "Bargains to be resolved" << endl << flush;
    showBargains(brgns);

    auto w = actrCaps();
    cout << "w:" << endl;
    w.mPrintf(" %6.2f ");

    // of course, you can change these parameters.
    // ideally, they should be read from the scenario object.
    auto vr = VotingRule::Proportional;
    auto vpm = VPModel::Linear;
    auto stm = StateTransMode::DeterminsticSTM;

    auto ndxMaxProb = [](const KMatrix & cv) {
        const double pTol = 1E-8;
        assert(fabs(KBase::sum(cv) - 1.0) < pTol);
        assert(0 < cv.numR());
        assert(1 == cv.numC());
        auto ndxIJ = ndxMaxAbs(cv);
        unsigned int iMax = get<0>(ndxIJ);
        return iMax;
    };

    // what is the utility to actor nai of the state resulting after
    // the nbj-th bargain of the k-th actor is implemented?
    auto brgnUtil = [this, brgns](unsigned int nk, unsigned int nai, unsigned int nbj) {
        const unsigned int na = model->numAct;
        BargainSMP * b = brgns[nk][nbj];
        double uAvrg = 0.0;

        if (nullptr == b) { // SQ bargain
            uAvrg = 0.0;
            for (unsigned int n = 0; n < na; n++) {
                // nai's estimate of the utility to nai of position n, i.e. the true value
                uAvrg = uAvrg + aUtil[nai](nai, n);
            }
        }

        if (nullptr != b) { // all positions unchanged, except Init and Rcvr
            uAvrg = 0.0;
            auto ndxInit = model->actrNdx(b->actInit);
            assert((0 <= ndxInit) && (ndxInit < na)); // must find it
            double uPosInit = ((SMPActor*)(model->actrs[nai]))->posUtil(&(b->posInit), this);
            uAvrg = uAvrg + uPosInit;

            auto ndxRcvr = model->actrNdx(b->actRcvr);
            assert((0 <= ndxRcvr) && (ndxRcvr < na)); // must find it
            double uPosRcvr = ((SMPActor*)(model->actrs[nai]))->posUtil(&(b->posRcvr), this);
            uAvrg = uAvrg + uPosRcvr;

            for (unsigned int n = 0; n < na; n++) {
                if ((ndxInit != n) && (ndxRcvr != n)) {
                    // again, nai's estimate of the utility to nai of position n, i.e. the true value
                    uAvrg = uAvrg + aUtil[nai](nai, n);
                }
            }
        }

        uAvrg = uAvrg / na;

        assert(0.0 < uAvrg); // none negative, at least own is positive
        assert(uAvrg <= 1.0); // can not all be over 1.0
        return uAvrg;
    };
    // end of λ-fn


    // The key is to build the usual matrix of U_ai (Brgn_m) for all bargains in brgns[k],
    // making sure to divide the sum of the utilities of positions by 1/N
    // so 0 <= Util(state after Brgn_m) <= 1, then do the standard scalarPCE for bargains involving k.

    SMPState* s2 = new SMPState(model);

    // (This loop would be a good place for high-level parallelism)
    for (unsigned int k = 0; k < na; k++) {
        unsigned int nb = brgns[k].size();
        auto buk = [brgnUtil, k](unsigned int nai, unsigned int nbj) {
            return brgnUtil(k, nai, nbj);
        };
        auto u_im = KMatrix::map(buk, na, nb);

        cout << "u_im: " << endl;
        u_im.mPrintf(" %.5f ");

        cout << "Doing probCE for the " << nb << " bargains of actor " << k << " ... " << flush;
        auto p = Model::scalarPCE(na, nb, w, u_im, vr, vpm, ReportingLevel::Medium);
        assert(nb == p.numR());
        assert(1 == p.numC());
        cout << "done" << endl << flush;

        int maxArrcount = p.numR();
        //int rslt = 0; // never used
        unsigned int mMax = nb; // indexing actors by i, bargains by m
        switch (stm) {
        case StateTransMode::DeterminsticSTM:
            mMax = ndxMaxProb(p);
            break;
        case StateTransMode::StochasticSTM:
            mMax = model->rng->probSel(p);
            break;
        default:
            throw KException("SMPState::doBCN - unrecognized StateTransMode");
            break;
        }
        // 0 <= mMax assured for uint
        assert(mMax < nb);
        cout << "Chosen bargain (" << stm << "): " << mMax << "/" << nb << endl;
        // update for bern table for remaining fields
        model->sqlUpdateBargainTable(t, p(0, 0), mMax, p(maxArrcount - 1, 0), mMax, k);


        //populate the Bargain Vote & Util tables
        // JAH added sql flag logging control
        if (model->sqlFlags[3])
        {
            // model->sqlBargainVote(t, k, k, w);
            model->sqlBargainUtil(t, k, u_im);
        }

        // TODO: create a fresh position for k, from the selected bargain mMax.
        VctrPstn * pk = nullptr;
        auto bkm = brgns[k][mMax];
        if (nullptr == bkm) {
            auto oldPK = (VctrPstn *)pstns[k];
            pk = new VctrPstn(*oldPK);
        }
        else {
            const unsigned int ndxInit = model->actrNdx(bkm->actInit);
            const unsigned int ndxRcvr = model->actrNdx(bkm->actRcvr);
            if (ndxInit == k) {
                pk = new VctrPstn(bkm->posInit);
            }
            else if (ndxRcvr == k) {
                pk = new VctrPstn(bkm->posRcvr);
            }
            else {
                cout << "SMPState::doBCN: unrecognized actor in bargain" << endl;
                assert(false);
            }
        }
        assert(nullptr != pk);

        assert(k == s2->pstns.size());
        s2->pstns.push_back(pk);

        cout << endl << flush;
    }

    // Some bargains are nullptr, and there are two copies of every non-nullptr randomly
    // arranged. If we delete them as we find them, then the second occurance will be corrupted,
    // so the code crashes when it tries to access the memory to see if it matches something
    // already deleted. Hence, we scan them all, building a list of unique bargains,
    // then delete those in order.
    auto uBrgns = vector<BargainSMP*>();

    for (unsigned int i = 0; i < brgns.size(); i++) {
        auto ai = ((const SMPActor*)(model->actrs[i]));
        for (unsigned int j = 0; j < brgns[i].size(); j++) {
            BargainSMP* bij = brgns[i][j];
            if (nullptr != bij) {
                if (ai == bij->actInit) {
                    uBrgns.push_back(bij); // this is the initiator's copy, so save it for deletion
                }
            }
            brgns[i][j] = nullptr; // either way, null it out.
        }
    }

    for (auto b : uBrgns) {
        //int aI = model->actrNdx(b->actInit);
        //int aR = model->actrNdx(b->actRcvr);
        //printf("Delete bargain [%2i:%2i] \n", aI, aR);
        delete b;
    }

    // TODO: this really should do all the assessment: ueIndices, rnProb, all U^h_{ij}, raProb
    s2->setUENdx();


    if (0 == accomodate.numC()) { // nothing to copy
        s2->setAccomodate(1.0); // set to identity matrix
    }
    else {
        s2->accomodate = accomodate;
    }

    if (0 == ideals.size()) { // nothing to copy
        s2->idealsFromPstns(); // set s2's current ideals to s2's current positions
    }
    else {
        s2->ideals = ideals; // copy s1's old ideals
    }
    s2->newIdeals(); // adjust s2 ideals toward new ones
    double ipDist = s2->posIdealDist(ReportingLevel::Medium);
    printf("rms (pstn, ideal) = %.5f \n", ipDist);
    cout << flush;
    return s2;
}



// h's estimate of the victory probability and expected delta in utility for k from i challenging j,
// compared to status quo.
// Note that the  aUtil vector of KMatrix must be set before starting this.
// TODO: offer a choice the different ways of estimating value-of-a-state: even sum or expected value.
// TODO: we may need to separate euConflict from this at some point
tuple<double, double> SMPState::probEduChlg(unsigned int h, unsigned int k, unsigned int i, unsigned int j, bool sqlP) const {

    // you could make other choices for these two sub-models
    auto vr = VotingRule::Proportional;
    auto tpc = KBase::ThirdPartyCommit::SemiCommit;

    double uii = aUtil[h](i, i);
    double uij = aUtil[h](i, j);
    double uji = aUtil[h](j, i);
    double ujj = aUtil[h](j, j);

    // h's estimate of utility to k of status-quo positions of i and j
    const double euSQ = aUtil[h](k, i) + aUtil[h](k, j);
    assert((0.0 <= euSQ) && (euSQ <= 2.0));

    // h's estimate of utility to k of i defeating j, so j adopts i's position
    const double uhkij = aUtil[h](k, i) + aUtil[h](k, i);
    assert((0.0 <= uhkij) && (uhkij <= 2.0));

    // h's estimate of utility to k of j defeating i, so i adopts j's position
    const double uhkji = aUtil[h](k, j) + aUtil[h](k, j);
    assert((0.0 <= uhkji) && (uhkji <= 2.0));

    auto ai = ((const SMPActor*)(model->actrs[i]));
    double si = KBase::sum(ai->vSal);
    double ci = ai->sCap;
    auto aj = ((const SMPActor*)(model->actrs[j]));
    double sj = KBase::sum(aj->vSal);
    assert((0 < sj) && (sj <= 1));
    double cj = aj->sCap;
    const double minCltn = 1E-10;

    // get h's estimate of the principal actors' contribution to their own contest

    // h's estimate of i's unilateral influence contribution to (i:j).
    // When ideals perfectly track positions, this must be positive
    double contrib_i_ij = Model::vote(vr, si*ci, uii, uij);
    if (KBase::iMatP(accomodate)) {
        assert(0 <= contrib_i_ij);
    }
    // If not, you could have the ordering (Idl_i, Pos_j, Pos_i)
    // so that i would prefer j's position over his own.


    // h's estimate of j's unilateral influence contribution to (i:j).
    // When ideals perfectly track positions, this must be negative
    double contrib_j_ij = Model::vote(vr, sj*cj, uji, ujj);
    if (KBase::iMatP(accomodate)) {
        assert(contrib_j_ij <= 0);
    }
    // Similarly, you could have an ordering like (Idl_j, Pos_i, Pos_j)
    // so that j would prefer i's position over his own.

    double chij = minCltn; // strength of complete coalition supporting i over j (initially empty)
    double chji = minCltn; // strength of complete coalition supporting j over i (initially empty)

    // add i's contribution to the appropriate coalition
    if (contrib_i_ij > 0.0) {
        chij = chij + contrib_i_ij;
    }
    assert(0.0 < chij);

    if (contrib_i_ij < 0.0) {
        chji = chji - contrib_i_ij;
    }
    assert(0.0 < chji);

    // add j's contribution to the appropriate coalition
    if (contrib_j_ij > 0.0) {
        chij = chij + contrib_j_ij;
    }
    assert(0.0 < chij);

    if (contrib_j_ij < 0.0) {
        chji = chji - contrib_j_ij;
    }
    assert(0.0 < chji);

    // cache those sums
    contrib_i_ij = chij;
    contrib_j_ij = chji;


    const unsigned int na = model->numAct;

    // we assess the overall coalition strengths by adding up the contribution of
    // individual actors (including i and j, above). We assess the contribution of third
    // parties (n) by looking at little coalitions in the hypothetical (in:j) or (i:nj) contests.
    auto tpvArray = KMatrix(na, 3);
    for (unsigned int n = 0; n < na; n++) {
        if ((n != i) && (n != j)) { // already got their influence-contributions
            auto an = ((const SMPActor*)(model->actrs[n]));

            double cn = an->sCap;
            double sn = KBase::sum(an->vSal);
            double uni = aUtil[h](n, i);
            double unj = aUtil[h](n, j);
            double unn = aUtil[h](n, n);

            // notice that each third party starts afresh,
            // considering only contributions of principals and itself
            double pin = Actor::vProbLittle(vr, sn*cn, uni, unj, contrib_i_ij, contrib_j_ij);

            assert(0.0 <= pin);
            assert(pin <= 1.0);
            double pjn = 1.0 - pin;
            auto vt_uv_ul = Actor::thirdPartyVoteSU(sn*cn, vr, tpc, pin, pjn, uni, unj, unn);
            const double vnij = get<0>(vt_uv_ul);
            chij = (vnij > 0) ? (chij + vnij) : chij;
            assert(0 < chij);
            chji = (vnij < 0) ? (chji - vnij) : chji;
            assert(0 < chji);

            const double utpv = get<1>(vt_uv_ul);
            const double utpl = get<2>(vt_uv_ul);
            // record for SQLite
            tpvArray(n, 0) = pin;
            tpvArray(n, 1) = utpv;
            tpvArray(n, 2) = utpl;
        }
    }

    const double phij = chij / (chij + chji); // ProbVict, for i
    const double phji = chji / (chij + chji);

    const double euVict = uhkij;  // UtilVict
    const double euCntst = phij*uhkij + phji*uhkji; // UtilContest,
    const double euChlg = (1 - sj)*euVict + sj*euCntst; // UtilChlg
    const double duChlg = euChlg - euSQ; //  delta-util of challenge versus status-quo
    auto rslt = tuple<double, double>(phij, duChlg);

    // JAH 20160802 switched to use the model sql flags vector to control logging
    // I keep sqlP and short-circuit & it because sometimes probEduChlg is called to
    // do some temporary calcs which should not be store - this is controlled with sqlP
    if (sqlP && model->sqlFlags[2]) {
        // now that the computation is finished, record everything into SQLite
        //
        // record tpvArray into SQLite turn, est (h), init (i), third party (n), receiver (j), and tpvArray[n]
        // printf ("SMPState::probEduChlg(%2i, %2i, %2i, %i2) = %+6.4f - %+6.4f = %+6.4f\n", h, k, i, j, euCh, euSQ, euChlg);

        unsigned int t = myTurn();

        sqlite3 * db = model->smpDB;
        char* zErrMsg = nullptr; // Error message in case

        auto sqlBuff = newChars(sqlBuffSize);
        // prepare the sql statement to insert. as it does not depend on tpk, keep it outside the loop.
        sprintf(sqlBuff,
                "INSERT INTO TP_Prob_Vict_Loss (ScenarioId, Turn_t, Est_h,Init_i,ThrdP_k,Rcvr_j,Prob,Util_V,Util_L) VALUES ('%s', ?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8)",
                model->getScenarioID().c_str());

        // The whole point of a prepared statement is to reuse it.
        // Therefore, we prepare it before the loop, and reuse it inside the loop:
        // just moving it outside loop cut dummyData_3Dim.csv run time from 30 to 10 seconds
        // (with Electric Fence).
        assert(nullptr != db);
        sqlite3_stmt *insStmt;
        sqlite3_prepare_v2(db, sqlBuff, strlen(sqlBuff), &insStmt, NULL);
        assert(nullptr != insStmt); //make sure it is ready

        sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);

        for (int tpk = 0; tpk < na; tpk++) {  // third party voter, tpk
            auto an = ((const SMPActor*)(model->actrs[tpk]));
            int rslt = 0;

            // bind the indices
            rslt = sqlite3_bind_int(insStmt, 1, t);
            assert(SQLITE_OK == rslt);
            rslt = sqlite3_bind_int(insStmt, 2, h);
            assert(SQLITE_OK == rslt);
            rslt = sqlite3_bind_int(insStmt, 3, i);
            assert(SQLITE_OK == rslt);
            rslt = sqlite3_bind_int(insStmt, 4, tpk);
            assert(SQLITE_OK == rslt);
            rslt = sqlite3_bind_int(insStmt, 5, j);
            assert(SQLITE_OK == rslt);

            // bind the data
            rslt = sqlite3_bind_double(insStmt, 6, tpvArray(tpk, 0));
            assert(SQLITE_OK == rslt);
            rslt = sqlite3_bind_double(insStmt, 7, tpvArray(tpk, 1));
            assert(SQLITE_OK == rslt);
            rslt = sqlite3_bind_double(insStmt, 8, tpvArray(tpk, 2));
            assert(SQLITE_OK == rslt);

            // actually record it
            rslt = sqlite3_step(insStmt);
            assert(SQLITE_DONE == rslt);
            sqlite3_clear_bindings(insStmt);
            assert(SQLITE_DONE == rslt);
            rslt = sqlite3_reset(insStmt);
            assert(SQLITE_OK == rslt);
        }

        // formatting note: %d means an integer, base 10
        // we will use base 10 by default, and these happen to be unsigned integers, so %i is appropriate

        memset(sqlBuff, '\0', sqlBuffSize);
        sprintf(sqlBuff,
                "INSERT INTO ProbVict (ScenarioId, Turn_t, Est_h,Init_i,Rcvr_j,Prob) VALUES ('%s',%u,%u,%u,%u,%f)",
                model->getScenarioID().c_str(), t, h, i, j, phij);
        sqlite3_exec(db, sqlBuff, NULL, NULL, &zErrMsg);

        // the following four statements could be combined into one table
        memset(sqlBuff, '\0', sqlBuffSize);
        sprintf(sqlBuff,
                "INSERT INTO UtilChlg (ScenarioId, Turn_t, Est_h,Aff_k,Init_i,Rcvr_j,Util_SQ,Util_Vict,Util_Cntst,Util_Chlg) VALUES ('%s',%u,%u,%u,%u,%u,%f,%f,%f,%f)",
                model->getScenarioID().c_str(), t, h, k, i, j, euSQ, euVict, euCntst, euChlg);
        sqlite3_exec(db, sqlBuff, NULL, NULL, &zErrMsg);

        sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &zErrMsg);
        sqlite3_finalize(insStmt); // finalize statement to avoid resource leaks
        printf("Stored SQL for turn %u of all estimators, actors, and positions \n", t);
        cout << flush; // so this prints before subsequent assertion failures

        delete sqlBuff;
        sqlBuff = nullptr;
    }
    return rslt;
}


tuple<int, double, double> SMPState::bestChallenge(unsigned int i, bool sqlP) const {
    int bestJ = -1;
    double pIJ = 0;
    double bestEU = -1.00;

    // for SMP, positive expected gains on the first turn are typically in the 0.5 to 0.01 range
    // I take a fraction of the minimum.
    const double minSigEDU = 1e-5; // TODO: 1/20 of the minimum, or 0.0005

    for (unsigned int j = 0; j < model->numAct; j++) {
        if (j != i) {
            auto peij = probEduChlg(i, i, i, j, sqlP);
            const double pij = get<0>(peij); // i's estimate of the victory-Prob for i challenging j
            const double edu = get<1>(peij); // i's estimate of the change in utility to i of i challenging j, compared to SQ
            if ((minSigEDU < edu) && (bestEU < edu)) {
                bestJ = j;
                pIJ = pij;
                bestEU = edu;
            }
        }
    }
    if (0 <= bestJ) {
        assert(minSigEDU < bestEU);
    }
    auto rslt = tuple<int, double, double>(bestJ, pIJ, bestEU);
    return rslt;
}




}; // end of namespace

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
