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

#ifndef SMP_LIB_H
#define SMP_LIB_H

#include <iostream>
#include <string>

#include "csv_parser.hpp"
#include "sqlite3.h"
#include "kutils.h"
#include "prng.h"
#include "kmatrix.h"
#include "gaopt.h"
#include "kmodel.h"

namespace SMPLib {
  // namespace to which KBase has no access
  using std::function;
  using std::ostream;
  using std::shared_ptr;
  using std::string;
  using std::tuple;
  using std::vector;
  using KBase::newChars;
  using KBase::KMatrix;
  using KBase::PRNG;
  using KBase::Actor;
  using KBase::Position;
  using KBase::State;
  using KBase::Model;
  using KBase::VotingRule;
  using KBase::ReportingLevel;
  using KBase::VctrPstn;
  using KBase::VUI;
  using KBase::BigRAdjust;
  using KBase::BigRRange;
  using KBase::KTable; // JAH 20160728

  class SMPActor;
  class SMPState;
  class SMPModel;

  const string appVersion = "0.1.1";

  // -------------------------------------------------
  // Plain-Old-Data
  struct BargainSMP {
  public:
    BargainSMP(const SMPActor* ai, const SMPActor* ar, const VctrPstn & pi, const VctrPstn & pr);
    ~BargainSMP();


    const SMPActor* actInit = nullptr;
    const SMPActor* actRcvr = nullptr;
    VctrPstn posInit = VctrPstn();
    VctrPstn posRcvr = VctrPstn();
    uint64_t getID() const;
  protected:
    static uint64_t highestBargainID;
    uint64_t myBargainID = 0;
  };

  enum class SMPBargnModel {
    InitOnlyInterpSMPBM,   // Init interpolates, I&R get same one
    InitRcvrInterpSMPBM,   // Init interpolates, so does Rcvr, each both from other
    PWCompInterSMPBM       // Power-weighted compromise of I-interp and R-interp, I&R both get same one
  };
  string bModName(const SMPBargnModel& bMod);
  ostream& operator<< (ostream& os, const SMPBargnModel& bMod);

  // -------------------------------------------------
  // Trivial, SMP-like actor with fixed attributes
  // the old smp.cpp file, SpatialState::developTwoPosBargain, for a discussion of
  // the interpolative bargaining rule used there. Note that the demoutil
  // results indicate that weighting by (P^2 * S^2) better approximates the NBS
  // does weighting by (P * S)
  class SMPActor : public Actor {

  public:

    enum class InterVecBrgn {
      S1P1, S2P2, S2PMax
    };
    // See the documentation in kutils/doc: eS2P2, dS2P2, and eS1P1 were the best, in that order.
    // Both estimators involving PMax were the least accurate.

    SMPActor(string n, string d);
    ~SMPActor();

    // interfaces to be provided
    double vote(unsigned int est, unsigned int i, unsigned int j, const State*st) const;
    virtual double vote(const Position * ap1, const Position * ap2, const SMPState* as1) const;
    double posUtil(const Position * ap1, const SMPState* as1) const;


    void randomize(PRNG* rng, unsigned int numD);

    // These actors have a vector position in [0,1]^m
    // They have differing saliences for different dimensions,
    // which are used to determine weighted Euclidean
    // distance from their current ideal position
    // (which is their current position in the State).
    // The distance is curved with risk attitude to get utility.
    // Their vote is determined by applying their scalar capacity
    // to differences in utility
    double sCap = 0;
    //double bigR;
    KMatrix vSal = KMatrix();
    VotingRule vr = VotingRule::Proportional; // reasonable default

    // the attributes used in this method are not generally part of
    // other actors, and not all positions can be represented as a list of doubles.
    static BargainSMP* interpolateBrgn(const SMPActor* ai, const SMPActor* aj,
                                       const VctrPstn* posI, const VctrPstn * posJ,
                                       double prbI, double prbJ, InterVecBrgn ivb);


  protected:
    static void interpBrgnSnPm(unsigned int n, unsigned int m,
                               double tik, double sik, double prbI,
                               double tjk, double sjk, double prbJ,
                               double & bik, double & bjk);
    static void interpBrgnS2PMax(double tik, double sik, double prbI,
                                 double tjk, double sjk, double prbJ,
                                 double & bik, double & bjk);


  };

  class SMPState : public State {

  public:
    explicit SMPState(Model * m);
    virtual ~SMPState();

    // Calculate the weighted-Euclidean distance matrix, compared to the given positions.
    //
    // If none are provided, it will compare ideals to the postions in this state:
    // vDiff(i,j) = distance from i's ideal to j's position, using i's salience-weights.
    //
    // If vPos provided, it will compare those to the positions in this state:
    // vDiff(i,j) = distance from vPos[i] to j's position, using i's salience-weights.
    virtual void setVDiff(const vector<VctrPstn> & vPos = {});

    // returns h's estimate of i's risk attitude, using the risk-adjustment-rule
    double estNRA(unsigned int h, unsigned int i, BigRAdjust ra) const;

    // returns row-vector of actor's capabilities
    KMatrix actrCaps() const;


    SMPState* stepBCN();

    // The key steps of BCN are to identify a target (and perhaps other target-relevant info)
    // and then to develop a Bargain (possibly nullptr if no bargain is mutually preferable
    // to conflict)
    // you have to provide λ-fn for both of the following
    function<shared_ptr<void>(const Actor* ai, const State* s)> bestTarget = nullptr;
    function<shared_ptr<void>(const Actor* aInit, const Actor* aRcvr, shared_ptr<void> btData, const State* s)> bargain = nullptr;

    virtual void addPstn(Position* p);

    // use the parameters of your state to compute the relative probability of each actor's position
    virtual tuple< KMatrix, VUI> pDist(int persp) const;
    void showBargains(const vector < vector < BargainSMP* > > & brgns) const;
    void showOneBargain(const BargainSMP* b) const;

    virtual bool equivNdx(unsigned int i, unsigned int j) const;
    
    void setNRA(); // TODO: this just sets risk neutral, for now
    // return actor's normalized risk attitude (if set)
    double aNRA(unsigned int i) const;

    SMPBargnModel bMod = SMPBargnModel::InitOnlyInterpSMPBM;
    // PWCompInterSMPBM, InitOnlyInterpSMPBM or InitRcvrInterpSMPBM;

    
    void setAccomodate(double adjRate = 1.0);
    // set ideal-accomodation matrix to scaled identity matrix, for current number of actors
    
    void setAccomodate(const KMatrix & aMat);
    // set ideal-accomodation matrix to given matrix

    // initialize the actors' ideals from the given list of VctrPstn.
    // If the list is omitted or empty, it uses their current positions
    void idealsFromPstns(const vector<VctrPstn> &  ps = {});
    
  protected:

    // this sets the values in all the AUtil matrices
    virtual void setAllAUtil(ReportingLevel rl);
    
    virtual void setOneAUtil(unsigned int perspH, ReportingLevel rl);
    
    KMatrix vDiff = KMatrix(); // vDiff(i,j) = difference between idl[i] and pos[j], using actor i's saliences as weights
    KMatrix rnProb = KMatrix(); // probability of each Unique state, when actors are treated as risk-neutral

    // risk-aware probabilities are uProb
    
    KMatrix nra = KMatrix();


    SMPState* doBCN() const;
    

    // returns estimated probability k wins (given likely coaltiions), and expected delta-util of that challenge.
    // If desired, record in SQLite.
    tuple<double, double> probEduChlg(unsigned int h, unsigned int k, unsigned int i, unsigned int j, bool sqlP) const;

    // return best j, p[i>j], edu[i->j]
    tuple<int, double, double> bestChallenge(unsigned int i, bool sqlP) const;
    
    // the actor's ideal, against which they judge others' positions
    vector<VctrPstn> ideals = {};
    
    // The matrix of rates at which they adjust their ideals toward positions
    KMatrix accomodate = KMatrix();
    
    // rest the new ideal points, based on other's positions and one's old ideal point
    void newIdeals();

    // return RMS distance between ideals and positions
    double posIdealDist(ReportingLevel rl = ReportingLevel::Silent) const;
    
    // for now,set it to a scaled identity matrix.
    void setupAccomodateMatrix(double adjRate);

    void bindExecute(sqlite3_stmt *updateStmt, size_t turn, int bargnID,
                     int initActor, double initProb, bool isInitSelected,
                     int recvActor, double recvProb, bool isRecvSelected) const;
  };

  class SMPModel : public Model {
  public:
    explicit SMPModel(PRNG * rng, string desc = "", uint64_t s=0, vector<bool> f={}); // JAH 20160711 added rng seed
    virtual ~SMPModel();

    static string dbPath; //to store db file name from SMPQ GUI, default is testsmp.db
    static void setDBPath(std::string dbName); // to initialize DB Name to dbPath variable

    static const unsigned int maxDimDescLen = 256; // JAH 20160727 added

    static double bsUtil(double sd, double R);
    static double bvDiff(const KMatrix & vd, const  KMatrix & vs);
    static double bvUtil(const KMatrix & vd, const  KMatrix & vs, double R);

    static SMPModel * readCSV(string fName, PRNG * rng, uint64_t s, vector<bool> f); // JAH 20160711 added rng seed

    static  SMPModel * initModel(vector<string> aName, vector<string> aDesc, vector<string> dName,
                                 const KMatrix & cap, const KMatrix & pos, const KMatrix & sal,
                                 const KMatrix & accM, // 20160720 BPW added accomodation matrix
                                 PRNG * rng, uint64_t s, vector<bool> f); // JAH 20160711 added rng seed 20160730 JAH added sql flags

    // print history of each actor in CSV (might want to generalize to arbitrary VctrPstn)
    void showVPHistory() const;

    void LogInfoTables(); // JAH 20160731

    // output the two files needed to draw Sankey diagrams
    void sankeyOutput(string inputCSV) const;

    // number of spatial dimensions in this SMP
    void addDim(string dn);
    unsigned int numDim = 0;
    vector<string> dimName = {};
    double posTol = 1E-3; // on a scale of 0 to 100, this is a difference of just 0.1

    static double stateDist(const SMPState* s1, const SMPState* s2);

    static KTable * createSQL(unsigned int n) ;

    // this does not set AUtil, just output it to SQLite
    //virtual void sqlAUtil(unsigned int t);

  protected:
    //sqlite3 *smpDB = nullptr; // keep this protected, to ease multi-threading
    //string scenName = "Scen";
    static const int NumTables = 4; // TODO : Add one to this num when new table is added

    static const int NumSQLLogGrps = 0; // TODO : Add one to this num when new logging group is added
  protected:
    // note that the function to write to table #k must be kept
    // synchronized with the result of createTableSQL(k) !
    void sqlTest();

    // compute several useful items implied by the risk attitudes, saliences, and the matrix of differences
    static void setUtilProb(const KMatrix& vR, const KMatrix& vS, const KMatrix& vD, KBase::VotingRule vr);

  private:

  };



};// end of namespace

// --------------------------------------------
#endif
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
