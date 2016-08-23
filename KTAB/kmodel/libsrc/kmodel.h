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
// Define the abstract top-level classes of KTAB models.
// -------------------------------------------------
// We try to handle matrices and vectors in such a way
// as to minimize transpositions.
// The KMatrix u_ij is the utility to actor i of the position held by actor j.
// If p_j is a column-vector of the probabilities of each position being
// the actual outcome of this state, then  e = u*p is the column-vector
// of expected value to each actor, e_i.
// With that same u matrix, suppose w_j is the row-vector of
// the effective weights of each actor in this state, then
// z = w * u is the row-vector of weighted sums of utilities.
// Hence, we always treat u_ij, p_i, e_i, w_j, and z_j as
// matrix, c-vec, c-vec, r-vec, and r-vec respectively.
//
// Notice that we declare operators at the very end, outside the namespace.
// -------------------------------------------------
#ifndef KTAB_MODEL_H
#define KTAB_MODEL_H

#include <sqlite3.h>

#include "kutils.h"
#include "kmatrix.h"
#include "prng.h"

namespace KBase {
using std::ostream;
using std::shared_ptr;
using std::string;
using std::tuple;
using std::vector;
using KBase::KMatrix;
using KBase::ReportingLevel;

class KMatrix;
class PRNG;
class State;
class Actor;
class KTable;

// How much influence to exert (vote) given a difference in [0,1] utility.
// NOTE WELL: ASymProsp is asymmetric in the sense that v(i:k) + v(k:i) != 0.
// The response to positive delta-Util is only 2/3 of the response to negative,
// in line with "prospect theory". It is there largely to ensure that we are
// ready to handle more realistic asymmetric rules, and not always assume symmetry.
enum class VotingRule {
    Binary, PropBin, Proportional, PropCbc, Cubic, ASymProsp
};
const unsigned int NumVotingRule = 6;
string vrName(const VotingRule& vr);
ostream& operator<< (ostream& os, const VotingRule& vr);

// Will state transitions be in deterministic or stochastic mode
enum class StateTransMode {
    DeterminsticSTM, StochasticSTM
};
string stmName(const StateTransMode& stm);
ostream& operator<< (ostream& os, const StateTransMode& stm);

// At this point, the LISP keyword 'defmacro' should leap to mind.
// Will this PCE be Markov or Conditional or ...
enum class PCEModel {
    MarkovPCM, ConditionalPCM
};
string pcmName(const PCEModel& pcm);
ostream& operator<< (ostream& os, const PCEModel& pcm);



// whether you consider the probability of a coalition winning to go up linearly
// quadratically, quartically, or even discontinuously with strength ratios
enum class VPModel {
    // Linear law says 2:1 advantage gives pv = 2/3, and 3:1 gives 3/4
    //            and that 11:10 advantage gives 52.4%
    // Square law says 2:1 advantage gives pv = 4/5, and 3:1 gives 9/10
    //            and that 11:10 advantage gives 54.8%
    // Quartic law says 2:1 advantage gives pv = 16/17, and 3:1 gives 81/82
    //            and that 11:10 advantage gives 59.4%
    // Binary law says that any percentage difference over a small threshold gives
    //            guaranteed success (or loss), with linear interpolation between
    //            to avoid weird round-off effects.

    Linear,  // first power
    Square,  // second power
    Quartic, // fourth power
    Binary
};
string vpmName(const VPModel& vpm);
ostream& operator<< (ostream& os, const VPModel& vpm);


enum class ThirdPartyCommit {
    NoCommit, SemiCommit, FullCommit
};
string tpcName(const ThirdPartyCommit& tpc);
ostream& operator<< (ostream& os, const ThirdPartyCommit& tpc);


// estimating risk attitudes from probabilities might have use outside the SMP
enum class BigRRange {
    Min, Mid, Max
};
string bigRRName(const BigRRange & rRng);
ostream& operator << (ostream& os, const BigRRange& rRng);

enum class BigRAdjust {
    NoRA, OneThirdRA, HalfRA, TwoThirdsRA, FullRA
};
string bigRAName(const BigRAdjust & rAdj);
ostream& operator << (ostream& os, const BigRAdjust& rAdj);

// -------------------------------------------------
// There is not much to say about abstract positions, even
// though the set of possible positions/outcomes is key
// to defining each particular problem.
class Position {
public:
    Position();
    virtual ~Position();

    friend ostream& operator<< (ostream& os, const Position& p) {
        p.print(os);
        return os;
    }

protected:
    virtual void print(ostream& os) const = 0;

private:
};

ostream& operator<< (ostream& os, const Position& p);

bool testMultiThreadSQLite (bool tryReset, KBase::ReportingLevel rl);
// -------------------------------------------------
// Basic vector position: just a column-vector of numbers.
// They could be interpretted in many ways, as policies
// are often described by a vector of numbers (e.g. tax/subsidy rates).
class VctrPstn : public Position, public KMatrix {
public:
    VctrPstn();
    VctrPstn(unsigned int nr, unsigned int nc);
    explicit VctrPstn(const KMatrix & m); // copy constructor
    virtual ~VctrPstn();
protected:
    virtual void print(ostream& os) const;
private:
};

// -------------------------------------------------
// this is a matching of N items to M categories.
// Note that this is intended to be independent of MtchState, MtchActor, etc.
// Each category is a bucket into which 0, 1, or more items can be put.
// Each item goes in exactly one category. A Matching states, for each of N items,
// which of M categories it goes into. Thus, there are M^N possible matchings.
// Examples are items = pieces of candy, categories = which actor gets them,
// or items = projects to fund, categories = {High, Medium, Low} priority, actors = interest groups
// or items = cabinet seats, categories = parties, actors = interest groups
// or ....
class MtchPstn : public Position {
public:
    MtchPstn();
    virtual ~MtchPstn();
    virtual vector<MtchPstn> neighbors(unsigned int nVar) const;
    // assumes no interaction between items (permutation requires interaction)

    unsigned int numItm = 0;
    unsigned int numCat = 0;
    VUI match = {}; // must be of length numItm

protected:
    virtual void print(ostream& os) const;

private:
};


// bundle up methods relevant to GA over MtchPstn
class MtchGene : public MtchPstn {
public:
    MtchGene();
    ~MtchGene();

    void randomize(PRNG* rng);
    MtchGene * mutate(PRNG * rng) const;
    tuple<MtchGene*, MtchGene*> cross(const MtchGene * g2, PRNG * rng) const;
    //void show() const;
    bool equiv(const MtchGene * g2) const;

    void setState(vector<Actor*> as, vector<MtchPstn*> ps);

protected:
    virtual void print(ostream& os) const;
    void copySelf(MtchGene*) const;
    // links to the State are necessary to evaluate the net support, EU, etc.
    vector<Actor*> actrs = {};
    vector<MtchPstn*> pstns = {};
};


// -------------------------------------------------
class Model {
public:

    sqlite3 *smpDB = nullptr; // keep this protected, to ease later multi-threading
    // JAH 20160711 added seed 20160730 JAH added sql flags
    explicit Model(PRNG * r, string d, uint64_t s, vector<bool> f);
    virtual ~Model();

    static const unsigned int minNumActor = 3;
    static const unsigned int maxNumActor = 250; //quite generous, as we expect 10-30.

    static const unsigned int maxScenNameLen = 512; // might be auto-generated in sensitivy analysis
    static const unsigned int maxActNameLen = 25; // quite generous, as we expect 1-5
    static const unsigned int maxActDescLen = 256;

    static const unsigned int sqlBuffSize = 250; // big enough buffer to build all desired SQLite statements


    // parameters which every Model instance must set
    VPModel vpm = VPModel::Linear; // arbitrary default
    PCEModel pcem = PCEModel::ConditionalPCM; // arbitrary default

    // In the abstract, you run a model by stepping it until it is time to stop.
    // In detail, each step is likely to record copious information to
    // a database for analysis, and the stopping criterion is likely to
    // lambda-bind a lot of parameters to determine whether anything
    // significant is likely to happen if the run were to continue.
    void run();

    static double nProd(double x, double y);

    // simple voting based on the difference in utility.
    static double vote(VotingRule vr, double wi, double uij, double uik);

    // The voting function can index into only occupied states, or a mix of occupied and hypothetical states,
    // or into a set of potential two- or three-actor bargains, etc. etc.
    // The vfn can estimate from any actor's perspective.
    // The actors can do simple or complex voting.

    // calculate strength of coalitions for general actors and options.
    static KMatrix coalitions(function<double(unsigned int ak, unsigned int pi, unsigned int pj)> vfn,
                              unsigned int numAct, unsigned int numOpt);

    // calculate pv[i>j] from coalitions
    static KMatrix vProb(VPModel vpm, const KMatrix & c);

    // assumes simple voting over those options with that utility matrix,
    // builds coalitions, and return pv[i>j]
    static KMatrix vProb(VotingRule vr, VPModel vpm, const KMatrix & w, const KMatrix & u);

    // calculate column vector P[i] from square matrix pv[i>j]
    static KMatrix probCE(PCEModel pcm, const KMatrix & pv);

    static KMatrix scalarPCE(unsigned int numAct, unsigned int numOpt, const KMatrix & w,
                             const KMatrix & u, VotingRule vr, VPModel vpm, ReportingLevel rl);

    virtual unsigned int addActor(Actor* a); // returns new number of actors, always at least 1
    int actrNdx(const Actor* a) const;

    unsigned int addState(State* s); // returns new number of states, always at least 1

    function <bool(unsigned int iter, const State* s)> stop = nullptr;
    // you have to provide this λ-fn

    // these should probably be less public and more protected
    vector<Actor*> actrs = {};
    unsigned int numAct = 0;
    PRNG * rng = nullptr;
    vector<State*> history = {};

    vector<KTable*> KTables = {}; // JAH added 20160728 this will hold info for all defined tables
    vector<bool> sqlFlags= {};    // JAH added 20160730 this will hold the logging flag for each group of tables

    // output an existing actor util table, for the given turn, to SQLite
    void sqlAUtil(unsigned int t);
    // output an existing PosEquiv table, for the given turn, to SQLite
    void sqlPosEquiv(unsigned int t);
    // output an existing actor posprob table, for the given turn, to SQLite
    void sqlPosProb(unsigned int t);
    void sqlPosVote(unsigned int t);
	void sqlBargainEntries(unsigned int t, int bargainId, int initiator, int receiver, double val);
    void sqlBargainCoords(unsigned int t, int bargnID,  const KBase::VctrPstn & initPos, const KBase::VctrPstn & rcvrPos);
	void sqlBargainUtil(unsigned int t, int Bargn_i,  KBase::KMatrix Util_mat);
    void sqlBargainVote(unsigned int t, int Bargn_i, int Bargn_j, KBase::KMatrix Util_mat, unsigned int act_i);

    void LogInfoTables(); // JAH 20160731

    static void demoSQLite();

    static KMatrix bigRfromProb(const KMatrix & p, BigRRange rr);

    // returns h's estimate of i's risk attitude, using the risk-adjustment-rule
    static double estNRA(double rh, double  ri, BigRAdjust ra) ;

    // string getScenarioName() const { return scenName; };
	string getScenarioID() const { return scenId; };

    static KTable * createSQL(unsigned int n);
protected:
    //static string createTableSQL(unsigned int tn);
    static const int NumTables = 13; //TODO: constant need to be redefined when new table is added
    static const int NumSQLLogGrps = 4; // TODO : Add one to this num when new logging group is added
    // note that the function to write to table #k must be kept
    // synchronized with the result of createSQL(k) !


    // TODO: rename this from 'smpDB' to 'scenarioDB'
    // Note that, with composite models, there many be dozens interacting.
    string scenName = "Scen"; // default is set from UTC time
	string scenId = "none";
    uint64_t rngSeed = 0; // JAH 20160711 rng seed
    // this is the basic model of victory dependent on strength-ratio
    static tuple<double, double> vProb(VPModel vpm, const double s1, const double s2);

    static KMatrix markovPCE(const KMatrix & pv);
    static KMatrix condPCE(const KMatrix & pv);


private:
};


// -------------------------------------------------
class State {
public:
    explicit State(Model* mod);
    virtual ~State();


    void randomizeUtils(double minU, double maxU, double uNoise); //testing only

    void clear();
    virtual void addPstn(Position* p);

    // use the parameters of your state to compute the relative probability of each unique position.
    // persp = -1 means use everyone's separate perspectives (i.e. get actual probabilities, not one actor's beliefs).
    // Because the voting mechanisms may differ, so v_k(i:j) could differ widely from sub-class to sub-class,
    // it is tricky to make a single function to do this.
    virtual tuple< KMatrix, VUI> pDist(int persp) const = 0;

    Model * model = nullptr;
    function <State* ()> step = nullptr; // you have to provide this λ-fn
    vector<Position*> pstns = {};

    vector<KMatrix> aUtil = {}; // aUtil[h](i,j) is h's estimate of the utility to A_i of Pos_j

    // This sets the actor/position utility matrix as estimated by H.
    // If H == -1, then set them all.
    void setAUtil(int perspH = -1, ReportingLevel rl = ReportingLevel::Silent);

    void setUENdx();
    virtual bool equivNdx(unsigned int i, unsigned int j) const = 0;
    double posProb(unsigned int i, const VUI & unq, const KMatrix & pdt) const;

    // return the turn-number of this state.
    // 0 == initial state, and error if not in the model's history
    unsigned int myTurn() const;

protected:
    VUI uIndices = {}; // which positions occupied postions are unique, generated by KBase::ueIndices
    VUI eIndices = {}; // to which unique position each occupied postions matches, generated by KBase::ueIndices

    KMatrix uProb = KMatrix(); // probability of each Unique state

    virtual void setAllAUtil(ReportingLevel rl) = 0;

    virtual void setOneAUtil(unsigned int perspH, ReportingLevel rl); // TODO: make this non-dummy

    /*
     // exactly like Actor::vProbLittle, but with optional logging to SQLite
     double vProbLittle(VotingRule vr, double wn, double uni, double unj,
                                  double contrib_i_ij, double contrib_j_ij,
                                  bool logP) const;
    */



private:
};


// In the abstract, we can not say much about actors.
// Different utility models might assume different parameters
// in the utility function - and the associated actor
// would have to have their own value for those parameters.
// For example, their capability may change over time,
// so an actor like that would have to have a history of capabilities
// into  which the State could look.
// An actor in multi-dimensional issues might have different
// capabilities on different issues.
// They do not have fixed positions, but change over time.
// And so on.
class Actor {
public:
    Actor(string n, string d);
    virtual ~Actor();

    // returns the vote v_k(i:j), and the utilities of ik>j and i>jk.
    static tuple<double, double, double>
    thirdPartyVoteSU(double wk, VotingRule vr, ThirdPartyCommit comm,
                     double pik, double pjk, double uki, double ukj, double ukk);


    // Third-party actor n determines his contribution on influence by analyzing the hypothetical
    // "little conflict" of just three actors, i.e.  (i:j) with n weighing in on whichever side it favors.
    static double vProbLittle(VotingRule vr, double wn, double uni, double unj, double contrib_i_ij, double contrib_j_ij);

    string name = "GA"; // a short name, usually 2-5 characters
    string desc = "Generic Actor"; // short description, like a line or two.


    // the most common kinds of votes for actors are the following:

    // Vote between positions occupied by two different actors in the same
    // state, just looking up stored information.
    // Note well: it cannot be assumed that the vote between two
    // options can be determined simply by looking at the difference
    // in stored utilities.
    virtual double vote(unsigned int est,unsigned int p1, unsigned int p2, const State* st) const = 0;

    // Bear in mind that some Domain Specific Utility Models, like
    // full-scale computable general equilibrium (CGE) models, may
    // be very expensive to evaluate. Evaluating one policy position yields
    // a predicted state to which all the actors react with their differing utility fns.
    // Thus, we can get a whole column of the U_ij matrix from one CGE evaluation;
    // it would be very slow to re-evaluate the same model (Pj) once for each actor (Ai).
    // Of course, if every actor has a completely different model to predict the
    // states that will result from a policy, then you have no choice but to
    // run each model separately for each actor - so it's best to keep them simple.
    // It may turn out that in policy debates, actors actually use very simple
    // heuristics that are *informed* by complex off-line considerations, so you'd
    // be back to using very simple models.
    // Most actors will have functions somewhat like the following, but
    // the number and type of inputs will vary. Use lambda-fns to
    // construct the 'vfn' for Models::coalition, binding whatever extra
    // parameters your actor needs (e.g. a State* is necessary for strategic voting).
    // double posUtil(const Position * ap1) const = 0;
    // double vote(const Position * ap1, const Position * ap2) const = 0;


protected:
};

// JAH 20160728 KTAB logging table object
// this holds the SQL code needed to generate the table, the name of the table
// (to make it easy to find a table in a vector of KTables), the id number used
// for createSQL, as well as a group ID number, which is used to toggle logging
// for logical groups of tables
class KTable {
public:
    KTable(unsigned int ID, const string &name, const string &SQL, unsigned int grpID);
    virtual ~KTable();

    unsigned int tabID;
    string tabName;
    string tabSQL;
    unsigned int tabGrpID;
// be sure you use protection on your privates!
protected:

private:
};

}; // end of namespace


bool operator==(const KBase::MtchPstn& mp1, const KBase::MtchPstn& mp2);


// -------------------------------------------------
#endif
// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
