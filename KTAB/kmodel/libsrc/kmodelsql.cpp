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

#include "kmodel.h"


namespace KBase
{

using std::cout;
using std::endl;
using std::flush;
using std::get;
using std::tuple;

// JAH 20160728 added KTable class constructor
KTable::KTable(unsigned int ID, const string &name, const string &SQL, unsigned int grpID)
{
  tabID = ID;
  tabName = name;
  tabSQL = SQL;
  tabGrpID = grpID;
}

KTable::~KTable() {};

// --------------------------------------------

void Model::demoSQLite()
{
  cout << endl << "Starting basic demo of SQLite in Model class" << endl;

  auto callBack = [](void *data, int numCol, char **stringFields, char **colNames)
  {
    for (int i = 0; i < numCol; i++)
    {
      printf("%s = %s\n", colNames[i], stringFields[i] ? stringFields[i] : "NULL");
    }
    printf("\n");
    return ((int)0);
  };

  sqlite3 *db = nullptr;
  char* zErrMsg = nullptr;
  int  rc;
  string sql;

  auto sOpen = [&db](unsigned int n)
  {
    int rc = sqlite3_open("test.db", &db);
    if (rc != SQLITE_OK)
    {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      exit(0);
    }
    else
    {
      fprintf(stderr, "Opened database successfully (%i)\n", n);
    }
    return;
  };

  auto sExec = [callBack, &db, &zErrMsg](string sql, string msg)
  {
    int rc = sqlite3_exec(db, sql.c_str(), callBack, nullptr, &zErrMsg); // nullptr is the 'data' argument
    if (rc != SQLITE_OK)
    {
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
    }
    else
    {
      fprintf(stdout, msg.c_str());
    }
    return rc;
  };


  // Open database
  cout << endl << flush;
  sOpen(1);
  
  // try some pragmas. these should speed up operations on larger tables,
  // at the expense of making the table vulnerable to corruption if the
  // application abruptly terminates mid-operation.
  sql = "PRAGMA synchronous = off;";
  rc = sExec(sql, "Set synchronous to off \n");
  
  sql = "PRAGMA journal_mode = off;";
  rc = sExec(sql, "Set journal_mode to off \n");
  
  sql = "PRAGMA locking_mode = exclusive;";
  rc = sExec(sql, "Set locking_mode to exclusive \n");

  // Create SQL statement
  sql = "create table if not exists PETS("  \
        "ID INT PRIMARY KEY     NOT NULL," \
        "NAME           text    NOT NULL," \
        "AGE            int     NOT NULL," \
        "BREED         char(50)," \
        "COLOR         char(50) );";

  // Execute SQL statement
  rc = sExec(sql, "Created table successfully \n");
  sqlite3_close(db);


  cout << endl << flush;
  sOpen(2);
  

  // This SQL statement has one deliberate error.
  sql = "INSERT INTO PETS (ID, NAME, AGE, BREED, COLOR) "   \
        "VALUES (1, 'Alice', 6, 'Greyhound', 'Grey' ); "    \
        "INSERT INTO PETS (ID, NAME, AGE, BREED, COLOR) "   \
        "VALUES (2, 'Bob', 4, 'Newfoundland', 'Black' ); "  \
        "INSERT INTO PETS (ID, NAME, AGE, BREED, COLOR) "   \
        "VALUES (3, 'Carol', 7, 'Chihuahua', 'Tan' );"      \
        "INSERT INTO PETS (ID, NAME, AGE, SPECIES, COLOR) " \
        "VALUES (4, 'David', 5, 'Alsation', 'Mixed' );";
  \
  "INSERT INTO PETS (ID, NAME, AGE, SPECIES, COLOR) " \
  "VALUES (5, 'Ellie', 8, 'Rhodesian', 'Red' );";

  cout << "NB: This should get one planned SQL error at ID=4" << endl << flush;
  rc = sExec(sql, "Records inserted successfully \n");
  sqlite3_close(db);


  cout << endl << flush;
  sOpen(3);
  sql = "SELECT * from PETS where AGE>5;";
  rc = sExec(sql, "Records selected successfully\n");
  cout << "NB: ID=5 was never inserted due to planned SQL error at ID=4" << endl;
  sqlite3_close(db);


  cout << endl << flush;
  sOpen(4);
  sql = "DROP TABLE PETS;";
  rc = sExec(sql, "Dropped table successfully \n");
  sqlite3_close(db);

  return;
}

bool testMultiThreadSQLite (bool tryReset, KBase::ReportingLevel rl) { 
  int mutexP = sqlite3_threadsafe() ;
  bool parP = (0 != mutexP);
  if (ReportingLevel::Silent < rl) {
    if (parP) {
      cout << "This SQLite3 library WAS compiled to be threadsafe."<<endl << flush;
    } else {
      cout << "This SQLite3 library was NOT compiled to be threadsafe."<<endl << flush;
    }
  }
  if (tryReset && (!parP)) {
    int configRslt = sqlite3_config(SQLITE_CONFIG_SERIALIZED);
    if (configRslt == SQLITE_OK) {
      parP = true;
    }
    else  {
      parP = false;
    }
    if (ReportingLevel::Low < rl){
      cout << "  Note that multi-threading might have been disabled via sqlite3_config."<<endl<<flush;
      cout << "  Tried to reconfigure to SQLITE_CONFIG_SERIALIZED ... " << flush;
      if (configRslt == SQLITE_OK) {
        cout << "SUCCESS"<< endl;
      }
      else  {
        cout << "FAILED"<< endl;
      }
    }
  }
  if (ReportingLevel::Silent < rl) {
    if (parP)  {
      cout << "Possible to continue multi-threaded"<< endl << flush;
    }
    else {
      cout << "Necessary to continue single-threaded"<< endl << flush;
    }
  }
  return parP;
}

// note that the function to write to table #k must be kept
// synchronized with the result of createSQL(k) !
// JAH 20160728 modified to return a KTable object instead of the SQL string
KTable * Model::createSQL(unsigned int n)
{

  string sql = "";
  string name = "";
  unsigned int grpID = 0;

  assert(n < Model::NumTables);
  switch (n)
  {
  case 0:
    // position-utility table
    // the estimated utility to each actor of each other's position
    sql = "create table if not exists PosUtil ("  \
          "ScenarioId TEXT(32) NOT NULL DEFAULT 'None', "\
          "Turn_t	INTEGER NOT NULL DEFAULT 0, "\
          "Est_h	INTEGER NOT NULL DEFAULT 0, "\
          "Act_i	INTEGER NOT NULL DEFAULT 0, "\
          "Pos_j	INTEGER NOT NULL DEFAULT 0, "\
          "Util	REAL NOT NULL DEFAULT 0.0"\
          ");";
    name = "PosUtil";
    grpID = 4;
    break;

  case 1: // pos-vote table
    // estimated vote of each actor between each pair of positions
    sql = "create table if not exists PosVote ("  \
          "ScenarioId TEXT(32) NOT NULL DEFAULT 'None', "\
          "Turn_t	INTEGER NOT NULL DEFAULT 0, "\
          "Est_h	INTEGER NOT NULL DEFAULT 0, "\
          "Voter_k	INTEGER NOT NULL DEFAULT 0, "\
          "Pos_i	INTEGER NOT NULL DEFAULT 0, "\
          "Pos_j	INTEGER NOT NULL DEFAULT 0, "\
          "Vote	REAL NOT NULL DEFAULT 0.0"\
          ");";
    name = "PosVote";
    grpID = 1;
    break;

  case 2: // pos-prob table. Note that there may be duplicates, unless we limit it to unique positions
    sql = "create table if not exists PosProb ("  \
          "ScenarioId TEXT(32) NOT NULL DEFAULT 'None', "\
          "Turn_t	INTEGER NOT NULL DEFAULT 0, "\
          "Est_h	INTEGER NOT NULL DEFAULT 0, "\
          "Pos_i	INTEGER NOT NULL DEFAULT 0, "\
          "Prob	REAL NOT NULL DEFAULT 0.0"\
          ");";
    name = "PosProb";
    grpID = 1;
    break;

  case 3: // pos-equiv table. E(i)= lowest j s.t. Pos(i) ~ Pos(j). if j < i, it is not unique.
    sql = "create table if not exists PosEquiv ("  \
          "ScenarioId TEXT(32) NOT NULL DEFAULT 'None', "\
          "Turn_t	INTEGER NOT NULL DEFAULT 0, "\
          "Pos_i	INTEGER NOT NULL DEFAULT 0, "\
          "Eqv_j	INTEGER NOT NULL DEFAULT 0 "\
          ");";
    name = "PosEquiv";
    grpID = 1;
    break;

  case 4:
    // h's estimate of the utility to k of i challenging j, preceded by intermediate values
    // U{^h}{_k} ( SQ ), current status quo positions
    // U{^h}{_k} ( i >  j ), i defeats j
    // U{^h}{_k} ( i :  j ), contest between i and j
    // U{^h}{_k} ( i => j ), i challenges j
    // Utilities are evaluated so that UtilSQ, UtilVict, UtilChlg, UtilContest,
    // UtilTPVict, UtilTPLoss are comparable, i.e. the differences are meaningful
    sql = "create table if not exists UtilChlg ("  \
          "ScenarioId TEXT(32) NOT NULL DEFAULT 'none', "\
          "Turn_t	INTEGER NOT NULL DEFAULT 0, "\
          "Est_h	INTEGER NOT NULL DEFAULT 0, "\
          "Aff_k	INTEGER NOT NULL DEFAULT 0, "\
          "Init_i	INTEGER NOT NULL DEFAULT 0, "\
          "Rcvr_j	INTEGER NOT NULL DEFAULT 0, "\
          "Util_SQ	   	REAL    NOT NULL DEFAULT 0, "\
          "Util_Vict	REAL    NOT NULL DEFAULT 0, "\
          "Util_Cntst	REAL    NOT NULL DEFAULT 0, "\
          "Util_Chlg	REAL    NOT NULL DEFAULT 0  "\
          ");";
    name = "UtilChlg";
    grpID = 2;
    break;

  case 5:
    // h's estimate that i will defeat j, including all third party contributions
    // P{^h}( i > j )
    sql = "create table if not exists ProbVict ("  \
          "ScenarioId TEXT(32) NOT NULL DEFAULT 'None', "\
          "Turn_t	INTEGER NOT NULL DEFAULT 0, "\
          "Est_h	INTEGER NOT NULL DEFAULT 0, "\
          "Init_i	INTEGER NOT NULL DEFAULT 0, "\
          "Rcvr_j	INTEGER NOT NULL DEFAULT 0, "\
          "Prob	REAL NOT NULL DEFAULT 0"\
          ");";
    name = "PosVict";
    grpID = 2;
    break;

  case 6:
    // h's estimates in a little 3-actor contest with no contribution from others:
    // P{^h}( ik > j )      probability ik defeats j,
    // U{^h}{_k} (ik > j)   utility to k of winning with i, over j
    // U{^h}{_k} (i > jk)   utility to k of losing to i, with j
    sql = "create table if not exists TP_Prob_Vict_Loss ("  \
          "ScenarioId TEXT(32) NOT NULL DEFAULT 'None', "\
          "Turn_t	INTEGER NOT NULL DEFAULT 0, "\
          "Est_h	INTEGER NOT NULL DEFAULT 0, "\
          "Init_i	INTEGER NOT NULL DEFAULT 0, "\
          "ThrdP_k	INTEGER NOT NULL DEFAULT 0, "\
          "Rcvr_j	INTEGER NOT NULL DEFAULT 0, "\
          "Prob	REAL    NOT NULL DEFAULT 0, "\
          "Util_V	REAL    NOT NULL DEFAULT 0, "\
          "Util_L	REAL    NOT NULL DEFAULT 0  "\
          ");";
    name = "TP_Prob_Vict_Loss";
    grpID = 2;
    break;

  case 7:   // short-name and long-description of actors
  {
    char *sqlBuff = newChars(500);
    sprintf(sqlBuff, "create table if not exists ActorDescription ("  \
                     "ScenarioId TEXT(32) NOT NULL DEFAULT 'None', "\
                     "Act_i	INTEGER NOT NULL DEFAULT 0, "\
                     "Name	TEXT(%u) NOT NULL DEFAULT 'NoName', "\
                     "Desc	TEXT(%u) NOT NULL DEFAULT 'NoName' "\
                     ");", maxActNameLen, maxActDescLen);
    sql = std::string(sqlBuff);
    delete sqlBuff;
    sqlBuff = nullptr;
    name = "ActorDescription";
    grpID = 0;
  }
    break;

  case 8: // Bargain table
    sql = "create table if not exists Bargn ("  \
          "ScenarioId TEXT(32) NOT NULL DEFAULT 'None', "\
          "Turn_t	INTEGER NOT NULL DEFAULT 0, "\
          "BargnId INTEGER NOT NULL DEFAULT 0, "\
          "Init_Act_i INTEGER NOT NULL DEFAULT 0, "\
          "Recd_Act_j INTEGER NOT NULL DEFAULT 0, "\
          "Value REAL NOT NULL DEFAULT 0.0, "\
          "Init_Prob REAL NULL DEFAULT 0, "\
          "Init_Seld	BOOLEAN NULL ,"\
          "Recd_Prob REAL NULL DEFAULT 0, "\
          "Recd_Seld	BOOLEAN NULL"\
          ");";
    name = "Bargn";
    grpID = 3;
    break;
  case 9:  //
    // BargnCoords table
    sql = "create table if not exists BargnCoords ("  \
          "ScenarioId TEXT(32) NOT NULL DEFAULT 'None', "\
          "Turn_t	INTEGER NOT NULL DEFAULT 0, "\
          "BargnId INTEGER NOT NULL DEFAULT 0, "\
          "Dim_k INTEGER NOT NULL DEFAULT 0, "\
          "Init_Coord	REAL NULL DEFAULT 0.0,"\
          "Recd_Coord	REAL NOT NULL DEFAULT 0.0"\
          ");";
    name = "BargnCoords";
    grpID = 3;
    break;

  case 10:  // BargnUtil table creation
    sql = "create table if not exists BargnUtil ("  \
          "ScenarioId TEXT(32) NOT NULL DEFAULT 'None', "\
          "Turn_t	INTEGER NOT NULL DEFAULT 0, "\
          "BargnId    INTEGER NOT NULL	DEFAULT 0, "\
          "Act_i 	INTEGER NOT NULL DEFAULT 0, "\
          "Util	REAL NOT NULL DEFAULT 0.0"\
          ");";
    name = "BargnUtil";
    grpID = 3;
    break;

  case 11:
    // During bargain resolution, this table records the actual vote
    // of each actor between each pair of competing bargains
    sql = "create table if not exists BargnVote ("  \
          "ScenarioId TEXT(32) NOT NULL DEFAULT 'None', "\
          "Turn_t	INTEGER NOT NULL DEFAULT 0, "\
          "BargnId_i  INTEGER NOT NULL DEFAULT 0, "\
          "BargnId_j INTEGER NOT NULL DEFAULT 0, "\
          "Act_k 	INTEGER NOT NULL DEFAULT 0, "\
          "Vote	REAL NOT NULL DEFAULT 0.0"\
          ");";
    name = "BargnVote";
    grpID = 3;
    break;

  case 12:  //ScenarioDesc creation
    // JAH 20160711 added the RNGSeed field
    sql = "create table if not exists ScenarioDesc ("  \
          "Scenario TEXT(512) NOT NULL DEFAULT 'NoName', "\
          "Desc TEXT(512) NOT NULL DEFAULT 'No Description', "\
          "ScenarioId TEXT(32) NOT NULL UNIQUE DEFAULT 'None'," \
          "RNGSeed TEXT(20) NOT NULL DEFAULT '0'," \
          "VictoryProbModel INTEGER NULL DEFAULT NULL," \
          "ProbCondorcetElection INTEGER NULL DEFAULT NULL," \
          "StateTransition INTEGER NULL DEFAULT NULL," \
          "VotingRule INTEGER NULL DEFAULT NULL," \
          "BigRAdjust INTEGER NULL DEFAULT NULL," \
          "BigRRange INTEGER NULL DEFAULT NULL," \
          "ThirdPartyCommit INTEGER NULL DEFAULT NULL," \
          "InterVecBrgn INTEGER NULL DEFAULT NULL," \
          "BargnModel INTEGER NULL DEFAULT NULL" \
          ");";
    name = "ScenarioDesc";
    grpID = 0;
    break;
  default:
    throw(KException("Model::createTableSQL unrecognized table number"));
  }

  assert(grpID < NumSQLLogGrps);
  auto tab = new KTable(n,name,sql,grpID);
  return tab;
}


void Model::sqlAUtil(unsigned int t)
{
  assert(nullptr != smpDB);
  assert(t < history.size());
  State* st = history[t];
  assert(nullptr != st);
  assert(numAct == st->aUtil.size());

  // I don't like passing 'this' into lambda-functions,
  // so I copy the pointer into a local variable I can pass in
  // without exposing everything in 'this'. If this were a big,
  // mission-critical RDBMS, rather than a 1-off record of this run,
  // doing so might be disasterous in case the system crashed before
  // things were cleaned up.
  sqlite3 * db = smpDB;
  smpDB = nullptr;

  char* zErrMsg = nullptr;
  auto sqlBuff = newChars(200);
  sprintf(sqlBuff,
          "INSERT INTO PosUtil (ScenarioId, Turn_t, Est_h, Act_i, Pos_j, Util) VALUES ('%s', ?1, ?2, ?3, ?4, ?5)",
          scenId.c_str());

  assert(nullptr != db);
  const char* insStr = sqlBuff;
  sqlite3_stmt *insStmt;
  sqlite3_prepare_v2(db, insStr, strlen(insStr), &insStmt, NULL);
  // Prepared statements cache the execution plan for a query after the query optimizer has
  // found the best plan, so there is no big gain with simple insertions.
  // What makes a huge difference is bundling a few hundred into one atomic "transaction".
  // For this case, runtime droped from 62-65 seconds to 0.5-0.6 (vs. 0.30-0.33 with no SQL at all).
  assert(nullptr != insStmt); // make sure it is ready

  sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);
  for (unsigned int h = 0; h < numAct; h++)   // estimator is h
  {
    KMatrix uij = st->aUtil[h]; // utility to actor i of the position held by actor j
    for (unsigned int i = 0; i < numAct; i++)
    {
      for (unsigned int j = 0; j < numAct; j++)
      {
        int rslt = 0;
        rslt = sqlite3_bind_int(insStmt, 1, t);
        assert(SQLITE_OK == rslt);
        rslt = sqlite3_bind_int(insStmt, 2, h);
        assert(SQLITE_OK == rslt);
        rslt = sqlite3_bind_int(insStmt, 3, i);
        assert(SQLITE_OK == rslt);
        rslt = sqlite3_bind_int(insStmt, 4, j);
        assert(SQLITE_OK == rslt);
        rslt = sqlite3_bind_double(insStmt, 5, uij(i, j));
        assert(SQLITE_OK == rslt);
        rslt = sqlite3_step(insStmt);
        assert(SQLITE_DONE == rslt);
        sqlite3_clear_bindings(insStmt);
        assert(SQLITE_DONE == rslt);
        rslt = sqlite3_reset(insStmt);
        assert(SQLITE_OK == rslt);
      }
    }
  }
  sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &zErrMsg);
  sqlite3_finalize(insStmt); // finalize statement to avoid resource leaks
  //printf("Stored SQL for turn %u of all estimators, actors, and positions \n", t);

  delete sqlBuff;
  sqlBuff = nullptr;

  smpDB = db; // give it the new pointer

  return;
}
// populates record for table PosEquiv for each step of
// module run
void Model::sqlPosEquiv(unsigned int t)
{
  // Check database intact
  assert(nullptr != smpDB);
  assert(t < history.size());
  State* st = history[t];
  assert(nullptr != st);
  sqlite3 * db = smpDB;
  smpDB = nullptr;
  // In case of error
  char* zErrMsg = nullptr;
  auto sqlBuff = newChars(200);
  sprintf(sqlBuff,
          "INSERT INTO PosEquiv (ScenarioId, Turn_t, Pos_i, Eqv_j) VALUES ('%s', ?1, ?2, ?3)",
          scenId.c_str());

  assert(nullptr != db);
  const char* insStr = sqlBuff;
  sqlite3_stmt *insStmt;
  sqlite3_prepare_v2(db, insStr, strlen(insStr), &insStmt, NULL);
  assert(nullptr != insStmt); // make sure it is ready

  // Start inserting record
  sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);
  for (unsigned int i = 0; i < numAct; i++)
  {
    // calculate the equivalance
    int je = numAct + 1;
    for (unsigned int j = 0; j < numAct && je > numAct; j++)
    {
      if (st->equivNdx(i, j))
      {
        je = j;
      }
    }
    int rslt = 0;
    rslt = sqlite3_bind_int(insStmt, 1, t);
    assert(SQLITE_OK == rslt);
    rslt = sqlite3_bind_int(insStmt, 2, i);
    assert(SQLITE_OK == rslt);
    rslt = sqlite3_bind_int(insStmt, 3, je);
    assert(SQLITE_OK == rslt);
    rslt = sqlite3_step(insStmt);
    assert(SQLITE_DONE == rslt);
    sqlite3_clear_bindings(insStmt);
    assert(SQLITE_DONE == rslt);
    rslt = sqlite3_reset(insStmt);
    assert(SQLITE_OK == rslt);
  }
  // end databse transaction
  sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &zErrMsg);
  sqlite3_finalize(insStmt); // finalize statement to avoid resource leaks
  //printf("Stored SQL for turn %u of all estimators, actors, and positions \n", t);
  delete sqlBuff;
  sqlBuff = nullptr;
  smpDB = db;
  return;
}

void Model::sqlBargainEntries(unsigned int t, int bargainId, int initiator, int receiver, double val)
{
  // initiate the database
  sqlite3 * db = smpDB;

  // Error message in case
  char* zErrMsg = nullptr;
  auto sqlBuff = newChars(200);
  // prepare the sql statement to insert
  sprintf(sqlBuff,
          "INSERT INTO Bargn (ScenarioId, Turn_t, BargnID, Init_Act_i, Recd_Act_j,Value) VALUES ('%s',?1, ?2, ?3, ?4,?5)",
          scenId.c_str());

  assert(nullptr != db);
  const char* insStr = sqlBuff;
  sqlite3_stmt *insStmt;
  sqlite3_prepare_v2(db, insStr, strlen(insStr), &insStmt, NULL);
  assert(nullptr != insStmt); // make sure it is ready

  // start for the transaction
  sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);

  int rslt = 0;
  // Turn_t
  rslt = sqlite3_bind_int(insStmt, 1, t);
  assert(SQLITE_OK == rslt);
  //BargnID
  rslt = sqlite3_bind_int(insStmt, 2, bargainId);
  assert(SQLITE_OK == rslt);
  //Init_Act_i
  rslt = sqlite3_bind_int(insStmt, 3, initiator);
  assert(SQLITE_OK == rslt);
  //Recd_Act_j
  rslt = sqlite3_bind_int(insStmt, 4, receiver);
  assert(SQLITE_OK == rslt);
  //Value
  rslt = sqlite3_bind_double(insStmt, 5, val);
  assert(SQLITE_OK == rslt);
  rslt = sqlite3_step(insStmt);
  assert(SQLITE_DONE == rslt);
  sqlite3_clear_bindings(insStmt);
  assert(SQLITE_DONE == rslt);
  rslt = sqlite3_reset(insStmt);
  assert(SQLITE_OK == rslt);

  sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &zErrMsg);
  //printf("Stored SQL for turn %u of all estimators, actors, and positions \n", t);
  sqlite3_finalize(insStmt); // finalize statement to avoid resource leaks
  delete sqlBuff;
  sqlBuff = nullptr;

  smpDB = db;
}



void Model::sqlBargainCoords(unsigned int t, int bargnID, const KBase::VctrPstn & initPos, const KBase::VctrPstn & rcvrPos)
{
  int nDim = initPos.numR();
  assert(nDim == rcvrPos.numR());

  // initiate the database
  sqlite3 * db = smpDB;
  // Error message in case
  char* zErrMsg = nullptr;
  auto sqlBuff = newChars(200);

  // prepare the sql statement to insert
  sprintf(sqlBuff,
          "INSERT INTO BargnCoords (ScenarioId, Turn_t,BargnId, Dim_k, Init_Coord,Recd_Coord) VALUES ('%s',?1, ?2, ?3, ?4,?5)",
          scenId.c_str());

  assert(nullptr != db);
  const char* insStr = sqlBuff;
  sqlite3_stmt *insStmt;
  sqlite3_prepare_v2(db, insStr, strlen(insStr), &insStmt, NULL);
  assert(nullptr != insStmt); // make sure it is ready

  // start for the transaction
  sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);

  for (int k = 0; k < nDim; k++)
  {

    int rslt = 0;
    // Turn_t
    rslt = sqlite3_bind_int(insStmt, 1, t);
    assert(SQLITE_OK == rslt);
    //Baragainer
    rslt = sqlite3_bind_int(insStmt, 2, bargnID);
    assert(SQLITE_OK == rslt);
    //Dim_K
    rslt = sqlite3_bind_int(insStmt, 3, k);
    assert(SQLITE_OK == rslt);

    //Init_Coord
    rslt = sqlite3_bind_double(insStmt, 4, initPos(k,0));
    assert(SQLITE_OK == rslt);
    //Recd_Coord

    rslt = sqlite3_bind_double(insStmt, 5, rcvrPos(k, 0));
    assert(SQLITE_OK == rslt);
    rslt = sqlite3_step(insStmt);
    assert(SQLITE_DONE == rslt);
    sqlite3_clear_bindings(insStmt);
    assert(SQLITE_DONE == rslt);
    rslt = sqlite3_reset(insStmt);
    assert(SQLITE_OK == rslt);

  }

  sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &zErrMsg);
  sqlite3_finalize(insStmt); // finalize statement to avoid resource leaks
  delete sqlBuff;
  sqlBuff = nullptr;
  smpDB = db;
}



void Model::sqlBargainUtil(unsigned int t, vector<uint64_t> bargnIds,  KBase::KMatrix Util_mat)
{
  // initiate the database
  sqlite3 * db = smpDB;

  // Error message in case
  char* zErrMsg = nullptr;
  auto sqlBuff = newChars(200);

  int Util_mat_row = Util_mat.numR();
  int Util_mat_col = Util_mat.numC();


  // prepare the sql statement to insert
  sprintf(sqlBuff,
          "INSERT INTO BargnUtil  (ScenarioId, Turn_t,BargnId, Act_i, Util) VALUES ('%s',?1, ?2, ?3, ?4)",
          scenId.c_str());

  assert(nullptr != db);
  const char* insStr = sqlBuff;
  sqlite3_stmt *insStmt;
  sqlite3_prepare_v2(db, insStr, strlen(insStr), &insStmt, NULL);
  assert(nullptr != insStmt); // make sure it is ready
	// start for the transaction
	sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);
	uint64_t Bargn_i = 0;
    for (unsigned int i = 0; i < Util_mat_row; i++)
    {
		for (unsigned int j = 0; j < Util_mat_col; j++)
		{
			
			int rslt = 0;
			// Turn_t
			rslt = sqlite3_bind_int(insStmt, 1, t);
			assert(SQLITE_OK == rslt);
			//Bargn_i
			Bargn_i = bargnIds[j];
			rslt = sqlite3_bind_int(insStmt, 2, Bargn_i);
			assert(SQLITE_OK == rslt);
			//Act_i
			rslt = sqlite3_bind_int(insStmt, 3, i);
			assert(SQLITE_OK == rslt);
			//Util
			rslt = sqlite3_bind_double(insStmt, 4, Util_mat(i, j));
			assert(SQLITE_OK == rslt);
			// finish  
			assert(SQLITE_OK == rslt);
			rslt = sqlite3_step(insStmt);
			assert(SQLITE_DONE == rslt);
			sqlite3_clear_bindings(insStmt);
			assert(SQLITE_DONE == rslt);
			rslt = sqlite3_reset(insStmt);
			assert(SQLITE_OK == rslt);
		}
	}

  sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &zErrMsg);
  sqlite3_finalize(insStmt); // finalize statement to avoid resource leaks

  delete sqlBuff;
  sqlBuff = nullptr;

  smpDB = db;
}

// JAH 20160731 added this function in replacement to the separate
// populate* functions that separately logged information tables
// this only covers the general "Model" info tables; currently only Actors and Scenarios
void Model::LogInfoTables()
{
  int rslt = 0;
  // check the database availability
  sqlite3 * db = smpDB;
  assert(nullptr != smpDB);
  char* zErrMsg = nullptr;

  // assert tests for all tables here at the start
  assert(numAct == actrs.size());

  // for efficiency sake, we'll do all tables in a single transaction
  // form the insert cmmands
  auto sqlBuffA = newChars(sqlBuffSize);
  sprintf(sqlBuffA,"INSERT INTO ActorDescription (ScenarioId,Act_i,Name,Desc) VALUES ('%s', ?1, ?2, ?3)",
          scenId.c_str());
  auto sqlBuffS = newChars(sqlBuffSize);
  sprintf(sqlBuffS,"INSERT INTO ScenarioDesc (Scenario,Desc,ScenarioId,RNGSeed,"
      "VictoryProbModel,ProbCondorcetElection,StateTransition) VALUES ('%s',?1,?2,?3,?4,?5,?6)",
          scenName.c_str());
  // prepare the prepared statement statements
  sqlite3_stmt *insStmtA;
  sqlite3_prepare_v2(smpDB, sqlBuffA, strlen(sqlBuffA), &insStmtA, NULL);
  assert(nullptr != insStmtA);
  sqlite3_stmt *insStmtS;
  sqlite3_prepare_v2(smpDB, sqlBuffS, strlen(sqlBuffS), &insStmtS, NULL);
  assert(nullptr != insStmtS);

  sqlite3_exec(smpDB, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);

  // Actor Description Table
  // For each actor fill the required information
  for (unsigned int i = 0; i < actrs.size(); i++) {
    Actor * act = actrs.at(i);
    // bind the data
    rslt = sqlite3_bind_int(insStmtA, 1, i);
    assert(SQLITE_OK == rslt);
    rslt = sqlite3_bind_text(insStmtA, 2, act->name.c_str(), -1, SQLITE_TRANSIENT);
    assert(SQLITE_OK == rslt);
    rslt = sqlite3_bind_text(insStmtA, 3, act->desc.c_str(), -1, SQLITE_TRANSIENT);
    assert(SQLITE_OK == rslt);
    // record
    rslt = sqlite3_step(insStmtA);
    assert(SQLITE_DONE == rslt);
    sqlite3_clear_bindings(insStmtA);
    assert(SQLITE_DONE == rslt);
    rslt = sqlite3_reset(insStmtA);
    assert(SQLITE_OK == rslt);
  }

  // Scenario Description
  // Turn_t
  rslt = sqlite3_bind_text(insStmtS, 1, scenDesc.c_str(), -1, SQLITE_TRANSIENT);
  assert(SQLITE_OK == rslt);
  // Scen Id
  rslt = sqlite3_bind_text(insStmtS, 2, scenId.c_str(), -1, SQLITE_TRANSIENT);
  assert(SQLITE_OK == rslt);
  // rng seed JAH 20160711
  // have to convert to text and store it that way, since sqlite3 doesn't really understand unsigned ints
  char *seedBuff = newChars(50);
  sprintf(seedBuff,"%20llu",rngSeed);
  const char* strSeed = seedBuff;
  rslt = sqlite3_bind_text(insStmtS, 3, strSeed, -1, SQLITE_TRANSIENT);
  assert(SQLITE_OK == rslt);
  rslt = sqlite3_bind_int(insStmtS, 4, static_cast<int>(vpm));
  assert(SQLITE_OK == rslt);
  rslt = sqlite3_bind_int(insStmtS, 5, static_cast<int>(pcem));
  assert(SQLITE_OK == rslt);
  rslt = sqlite3_bind_int(insStmtS, 6, static_cast<int>(stm));
  assert(SQLITE_OK == rslt);
  // record
  assert(SQLITE_OK == rslt);
  rslt = sqlite3_step(insStmtS);
  assert(SQLITE_DONE == rslt);
  sqlite3_clear_bindings(insStmtS);
  assert(SQLITE_DONE == rslt);
  rslt = sqlite3_reset(insStmtS);
  assert(SQLITE_OK == rslt);

  // finish
  sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &zErrMsg);
  // finalize statement to avoid resource leaks
  sqlite3_finalize(insStmtA);
  sqlite3_finalize(insStmtS);
  delete sqlBuffA;
  sqlBuffA = nullptr;
  delete sqlBuffS;
  sqlBuffS = nullptr;
  return;
}

void Model::sqlBargainVote(unsigned int t, vector< tuple<uint64_t, uint64_t>> barginidspair_i_j, vector<double> Vote_mat,unsigned int act_k)
{
	// initiate the database
	sqlite3 * db = smpDB;

	// Error message in case
	char* zErrMsg = nullptr;
	auto sqlBuff = newChars(200);

	int Util_mat_row = Vote_mat.size();
//	int Util_mat_col = Vote_mat[0].size();

	// prepare the sql statement to insert
	sprintf(sqlBuff,
		"INSERT INTO BargnVote  (ScenarioId, Turn_t,BargnId_i,  BargnId_j, Act_k, Vote) VALUES ('%s', ?1, ?2, ?3,?4,?5)",scenId.c_str());
	
    assert(nullptr != db);
	const char* insStr = sqlBuff;
	sqlite3_stmt *insStmt;
	sqlite3_prepare_v2(db, insStr, strlen(insStr), &insStmt, NULL);
    assert(nullptr != insStmt); //make sure it is ready

	// start for the transaction
	sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);

    for (unsigned int i = 0; i <Util_mat_row ; i++)
    {
		tuple<uint64_t, uint64_t> tijids = barginidspair_i_j[i];
		uint64_t Bargn_i = std::get<0>(tijids);
		uint64_t Bargn_j = std::get<1>(tijids);
		
//for (unsigned int j = 0; j <  Util_mat_col; j++)
//		{
			int rslt = 0;
			// Turn_t
			rslt = sqlite3_bind_int(insStmt, 1, t);
			assert(SQLITE_OK == rslt);
			//Bargn_i
			rslt = sqlite3_bind_int(insStmt, 2, Bargn_i);
			assert(SQLITE_OK == rslt);
			//Bargn_j
			rslt = sqlite3_bind_int(insStmt, 3, Bargn_j);
			assert(SQLITE_OK == rslt);
			//Act_i
			rslt = sqlite3_bind_int(insStmt, 4, act_k);
			assert(SQLITE_OK == rslt);
			//Util
			rslt = sqlite3_bind_double(insStmt, 5, Vote_mat[i]);
			assert(SQLITE_OK == rslt);
			// finish  
			assert(SQLITE_OK == rslt);
			rslt = sqlite3_step(insStmt);
			assert(SQLITE_DONE == rslt);
			sqlite3_clear_bindings(insStmt);
			assert(SQLITE_DONE == rslt);
			rslt = sqlite3_reset(insStmt);
			assert(SQLITE_OK == rslt);
		//}
	}
	sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &zErrMsg);
	sqlite3_finalize(insStmt); // finalize statement to avoid resource leaks

	delete sqlBuff;
	sqlBuff = nullptr;

	smpDB = db;
}

// populates record for table PosProb for each step of
// module run
void Model::sqlPosProb(unsigned int t)
{
  assert(nullptr != smpDB);
  assert(t < history.size());
  State* st = history[t];
  // check module for null
  assert(nullptr != st);
  // initiate the database
  sqlite3 * db = smpDB;
  smpDB = nullptr;
  // Error message in case
  char* zErrMsg = nullptr;
  auto sqlBuff = newChars(200);
  // prepare the sql statement to insert
  sprintf(sqlBuff,
          "INSERT INTO PosProb (ScenarioId, Turn_t, Est_h,Pos_i, Prob) VALUES ('%s', ?1, ?2, ?3, ?4)",
          scenId.c_str());
  const char* insStr = sqlBuff;

  assert(nullptr != db);
  sqlite3_stmt *insStmt;
  sqlite3_prepare_v2(db, insStr, strlen(insStr), &insStmt, NULL);
  assert(nullptr != insStmt); //make sure it is ready

  // start for the transaction
  sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);
  // collect the information from each estimator,actor
  for (unsigned int h = 0; h < numAct; h++)   // estimator is h
  {
    // calculate the probablity with respect to each estimator
    auto pn = st->pDist(h);
    auto pdt = std::get<0>(pn); // note that these are unique positions
    assert( fabs(1 - sum(pdt)) < 1e-4);
    auto unq = std::get<1>(pn);
    // for each actor pupulate the probablity information
    for (unsigned int i = 0; i < numAct; i++)
    {
      int rslt = 0;
      // Extract the probabity for each actor
      double prob = st->posProb(i, unq, pdt);
      rslt = sqlite3_bind_int(insStmt, 1, t);
      assert(SQLITE_OK == rslt);
      rslt = sqlite3_bind_int(insStmt, 2, h);
      assert(SQLITE_OK == rslt);
      rslt = sqlite3_bind_int(insStmt, 3, i);
      assert(SQLITE_OK == rslt);
      rslt = sqlite3_bind_double(insStmt, 4, prob);
      assert(SQLITE_OK == rslt);
      rslt = sqlite3_step(insStmt);
      assert(SQLITE_DONE == rslt);
      sqlite3_clear_bindings(insStmt);
      assert(SQLITE_DONE == rslt);
      rslt = sqlite3_reset(insStmt);
      assert(SQLITE_OK == rslt);
    }
  }
  sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &zErrMsg);
  sqlite3_finalize(insStmt); // finalize statement to avoid resource leaks
  //printf("Stored SQL for turn %u of all estimators, actors, and positions \n", t);

  delete sqlBuff;
  sqlBuff = nullptr;

  smpDB = db; // give it the new pointer

  return;
}
// populates record for table PosProb for each step of
// module run
void Model::sqlPosVote(unsigned int t)
{
  assert(nullptr != smpDB);
  assert(t < history.size());
  State* st = history[t];


  // check module for null
  assert(nullptr != st);
  // initiate the database
  sqlite3 * db = smpDB;
  smpDB = nullptr;
  // Error message in case
  char* zErrMsg = nullptr;
  auto sqlBuff = newChars(200);
  // prepare the sql statement to insert
  sprintf(sqlBuff,
          "INSERT INTO PosVote (ScenarioId, Turn_t, Est_h, Voter_k,Pos_i, Pos_j,Vote) VALUES ('%s', ?1, ?2, ?3, ?4,?5,?6)",
          scenId.c_str());

  assert(nullptr != db);
  const char* insStr = sqlBuff;
  sqlite3_stmt *insStmt;
  sqlite3_prepare_v2(db, insStr, strlen(insStr), &insStmt, NULL);
  assert(nullptr != insStmt); //make sure it is ready

  // start for the transaction
  sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);
  auto vr = VotingRule::Proportional;
  // collect the information from each estimator

  for (unsigned int k = 0; k < numAct; k++)   // voter is k
  {
    auto rd = st->model->actrs[k];
    for (unsigned int i = 0; i < numAct; i++)
    {
      for (unsigned int j = 0; j < numAct; j++)
      {
        for (unsigned int h = 0; h < numAct; h++)   // estimator is h
        {
          if (((h == i) || (h == j)) && (i!=j))
          {
            auto vij = rd->vote(h, i, j, st);
            int rslt = 0;
            rslt = sqlite3_bind_int(insStmt, 1, t);
            assert(SQLITE_OK == rslt);
            rslt = sqlite3_bind_int(insStmt, 2, h);
            assert(SQLITE_OK == rslt);
            //voter_k
            rslt = sqlite3_bind_int(insStmt, 3, k);
            assert(SQLITE_OK == rslt);
            // position i
            rslt = sqlite3_bind_int(insStmt, 4, i);
            assert(SQLITE_OK == rslt);
            //position j
            rslt = sqlite3_bind_int(insStmt, 5, j);
            assert(SQLITE_OK == rslt);
            // vote ?
            rslt = sqlite3_bind_double(insStmt, 6, vij);
            assert(SQLITE_OK == rslt);
            rslt = sqlite3_step(insStmt);
            assert(SQLITE_DONE == rslt);
            sqlite3_clear_bindings(insStmt);
            assert(SQLITE_DONE == rslt);
            rslt = sqlite3_reset(insStmt);
            assert(SQLITE_OK == rslt);
          }
        }
      }
    }
  }
  sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &zErrMsg);
  sqlite3_finalize(insStmt); // finalize statement to avoid resource leaks
  //printf("Stored SQL for turn %u of all estimators, actors, and positions \n", t);

  delete sqlBuff;
  sqlBuff = nullptr;

  smpDB = db; // give it the new pointer

  return;
}

void Model::createTableIndices() {
    char * zErrMsg = nullptr;
    const char * indexUtil = "CREATE INDEX IF NOT EXISTS idx_util ON PosUtil(ScenarioId, Turn_t, Est_h, Act_i, Pos_j)";
    int rc = sqlite3_exec(smpDB, indexUtil, nullptr, nullptr, &zErrMsg);
    assert(rc == SQLITE_OK);

    const char *indexActor = "CREATE INDEX IF NOT EXISTS idx_actor ON ActorDescription(ScenarioId)";
    rc = sqlite3_exec(smpDB, indexActor, nullptr, nullptr, &zErrMsg);
    assert(rc == SQLITE_OK);
}

void Model::dropTableIndices(){
    char * zErrMsg = nullptr;
    const char * indexUtil = "DROP INDEX IF EXISTS idx_util";
    int rc = sqlite3_exec(smpDB, indexUtil, nullptr, nullptr, &zErrMsg);
    assert(rc == SQLITE_OK);

    const char * indexActor = "DROP INDEX IF EXISTS idx_actor";
    rc = sqlite3_exec(smpDB, indexActor, nullptr, nullptr, &zErrMsg);
    assert(rc == SQLITE_OK);
}


} // end of namespace

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
