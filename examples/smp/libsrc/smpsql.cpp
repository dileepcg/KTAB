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
// Demonstrate a very basic SQLite database for the Spatial Model of Politics.
//
// --------------------------------------------

#include "smp.h"
#include "sqlite3.h"


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
using KBase::State;
using KBase::VotingRule;
using KBase::ReportingLevel;


// --------------------------------------------

void bindExecuteBargnTableUpdate(sqlite3_stmt *updateStmt, size_t turn, int bargnID,
                                 int initActor, double initProb, bool isInitSelected,
                                 int recvActor, double recvProb, bool isRecvSelected);

// --------------------------------------------
// JAH 20160728 modified to return a KTable object instead of the SQL string
KTable * SMPModel::createSQL(unsigned int n)  {
  string sql = "";
  string name = "";
  unsigned int grpID = 0;
  // check total number of table exceeds
  assert(n < Model::NumTables + NumTables);
  if (n < Model::NumTables) {
    return Model::createSQL(n);
  }
  else
  {
    switch (n-Model::NumTables) {
    case  0: // coordinates of each actor's position
      sql = "create table if not exists VectorPosition ("  \
            "ScenarioId TEXT(32) NOT NULL DEFAULT 'None', "\
            "Turn_t		INTEGER NOT NULL DEFAULT 0, "\
            "Act_i		INTEGER NOT NULL DEFAULT 0, "\
            "Dim_k		INTEGER NOT NULL DEFAULT 0, "\
            "Coord		REAL NOT NULL DEFAULT 0"\
            ");";
      name = "VectorPosition";
      grpID = 4;// JAH 20161010 put in group 4 all by itself
      break;

    case 1: // salience to each actor of each dimension
      sql = "create table if not exists SpatialSalience ("  \
            "ScenarioId TEXT(32) NOT NULL DEFAULT 'None', "\
            "Turn_t		INTEGER NOT NULL DEFAULT 0, "\
            "Act_i		INTEGER NOT NULL DEFAULT 0, "\
            "Dim_k		INTEGER NOT NULL DEFAULT 0, "\
            "Sal		REAL NOT NULL DEFAULT 0.0"\
            ");";
      name = "SpatialSalience";
      grpID = 0;
      break;

    case 2: // scalar capability of each actor
      sql = "create table if not exists SpatialCapability ("  \
            "ScenarioId TEXT(32) NOT NULL DEFAULT 'None', "\
            "Turn_t		INTEGER NOT NULL DEFAULT 0, "\
            "Act_i		INTEGER NOT NULL DEFAULT 0, "\
            "Cap		REAL NOT NULL DEFAULT 0.0"\
            ");";
      name = "SpatialCapability";
      grpID = 0;
      break;


    case 3: // names of dimensions, so the GUI can use it JAH 20160727
    {
      char *sqlBuff = newChars(500);
      sprintf(sqlBuff, "create table if not exists DimensionDescription ("  \
                       "ScenarioId TEXT(32) NOT NULL DEFAULT 'None', "\
                       "Dim_k	INTEGER NOT NULL DEFAULT 0, "\
                       "Desc	TEXT(%u) NOT NULL DEFAULT 'NoDesc' "\
                       ");", maxDimDescLen);
      sql = std::string(sqlBuff);
      delete sqlBuff;
      sqlBuff = nullptr;

      name = "DimensionDescription";
      grpID = 0;
    }
      break;
    default:
      throw(KException("SMPModel::createSQL unrecognized table number"));
    }
  }

  assert(grpID<(Model::NumSQLLogGrps+NumSQLLogGrps));
  auto tab = new KTable(n,name,sql,grpID);
  return tab;
}


void SMPModel::sqlTest() {
  // just a test to get linkages correct

  auto callBack = [](void *NotUsed, int argc, char **argv, char **azColName) {
    for (int i = 0; i < argc; i++) {
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return ((int)0);
  };

  smpDB = nullptr;
  sqlite3 * db; // I don't like passing 'this' into lambda-functions
  char* zErrMsg = nullptr;
  string sql;

  auto sOpen = [&db](unsigned int n, string dbPath) {
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
      fprintf(stdout, "Can't open database: %s\n", sqlite3_errmsg(db));
      exit(0);
    }
    else {
      fprintf(stdout, "Successfully opened database #%i\n", n);
    }
    cout << endl << flush;
    return;
  };

  auto sExec = [&db, callBack, &zErrMsg](string sql, string msg) {
    int rc = sqlite3_exec(db, sql.c_str(), callBack, nullptr, &zErrMsg);
    if (rc != SQLITE_OK) {
      fprintf(stdout, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
    }
    else {
      fprintf(stdout, msg.c_str());
    }
    return rc;
  };

  // Open database
  cout << endl << flush;
  sOpen(1,dbPath); //passing DB FileName

  // As we are not dealing with a long-term, mission-critical database,
  // we can shut off some of the journaling stuff intended to protect
  // the DB in case the system crashes in mid-operation.
  // Eliminating these checks can significantly speed operations.
  sqlite3_exec(db, "PRAGMA journal_mode = MEMORY", NULL, NULL, &zErrMsg);
  sqlite3_exec(db, "PRAGMA locking_mode = EXCLUSIVE", NULL, NULL, &zErrMsg);
  sqlite3_exec(db, "PRAGMA synchronous = OFF", NULL, NULL, &zErrMsg);
  
  // not a performance issue, but necessary for the data layout
  sqlite3_exec(db, "PRAGMA foreign_keys = ON", NULL, NULL, &zErrMsg);

  // Create & execute SQL statements
  // JAH 20160728 rewritten to complete the vector of KTables before creating the table
  for (unsigned int i = 0; i < SMPModel::NumTables +Model::NumTables; i++) {
    // get the table and add to the vector
    auto thistable = SMPModel::createSQL(i);
    assert(nullptr != thistable);
    KTables.push_back(thistable);
    // create the table
    auto buff = newChars(100);
    sExec(thistable->tabSQL, buff); // ignore return-code
    // talk a little now
    sprintf(buff, "Created SMPModel table %s(%u) successfully \n", thistable->tabName.c_str(),i);
    delete buff;
    buff = nullptr;
    cout << flush;
  }
  cout << endl << flush;

  smpDB = db;
  return;
}

// JAH 20160731 added this function in replacement to the separate
// populate* functions that separately logged information tables
// this calls the kmodel version for Actors and Scenarios and then handles
// salience, capability, and dimensions tables
void SMPModel::LogInfoTables()
{
  // first call the KModel version to do the actors and scenarios tables
  Model::LogInfoTables();

  // now do mine
  int rslt = 0;
  // check the database availability
  assert(nullptr != smpDB);
  char* zErrMsg = nullptr;

  // assert tests for all tables here at the start
  assert(numDim == dimName.size());

  // for efficiency sake, we'll do all tables in a single transaction
  // form insert commands
  auto sqlBuffD = newChars(sqlBuffSize);
  sprintf(sqlBuffD,"INSERT INTO DimensionDescription (ScenarioId,Dim_k,Desc) VALUES ('%s', ?1, ?2)",
          scenId.c_str());
  auto sqlBuffC = newChars(sqlBuffSize);
  sprintf(sqlBuffC,"INSERT INTO SpatialCapability (ScenarioId, Turn_t, Act_i, Cap) VALUES ('%s', ?1, ?2, ?3)",
          scenId.c_str());
  auto sqlBuffS = newChars(sqlBuffSize);
  sprintf(sqlBuffS,"INSERT INTO SpatialSalience (ScenarioId, Turn_t, Act_i, Dim_k,Sal) VALUES ('%s', ?1, ?2, ?3, ?4)",
          scenId.c_str());
  // prepare the prepared statement statements
  sqlite3_stmt *insStmtD;
  sqlite3_prepare_v2(smpDB, sqlBuffD, strlen(sqlBuffD), &insStmtD, NULL);
  assert(nullptr != insStmtD);
  sqlite3_stmt *insStmtC;
  sqlite3_prepare_v2(smpDB, sqlBuffC, strlen(sqlBuffC), &insStmtC, NULL);
  assert(nullptr != insStmtC);
  sqlite3_stmt *insStmtS;
  sqlite3_prepare_v2(smpDB, sqlBuffS, strlen(sqlBuffS), &insStmtS, NULL);
  assert(nullptr != insStmtS);

  sqlite3_exec(smpDB, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);
  // Dimension Description Table
  for (unsigned int k = 0; k < dimName.size(); k++)
  {
    // bind the data
    rslt = sqlite3_bind_int(insStmtD, 1, k);
    assert(SQLITE_OK == rslt);
    rslt = sqlite3_bind_text(insStmtD, 2, dimName[k].c_str(), -1, SQLITE_TRANSIENT);
    assert(SQLITE_OK == rslt);
    // record
    rslt = sqlite3_step(insStmtD);
    assert(SQLITE_DONE == rslt);
    sqlite3_clear_bindings(insStmtD);
    assert(SQLITE_DONE == rslt);
    rslt = sqlite3_reset(insStmtD);
    assert(SQLITE_OK == rslt);
  }

  // Spatial Capability
  // for each turn extract the information
  for (unsigned int t = 0; t < history.size(); t++) {
    auto st = history[t];
    // get each actors capability value for each turn
    auto cp = (const SMPState*)history[t];
    auto caps = cp->actrCaps();
    for (unsigned int i = 0; i < numAct; i++) {
      // bind data
      rslt = sqlite3_bind_int(insStmtC, 1, t);
      assert(SQLITE_OK == rslt);
      rslt = sqlite3_bind_int(insStmtC, 2, i);
      assert(SQLITE_OK == rslt);
      rslt = sqlite3_bind_double(insStmtC, 3, caps(0, i));
      assert(SQLITE_OK == rslt);
      // record
      rslt = sqlite3_step(insStmtC);
      assert(SQLITE_DONE == rslt);
      sqlite3_clear_bindings(insStmtC);
      assert(SQLITE_DONE == rslt);
      rslt = sqlite3_reset(insStmtC);
      assert(SQLITE_OK == rslt);
    }
  }

  // Spatial Salience
  // for each turn extract the information
  for (unsigned int t = 0; t < history.size(); t++) {
    // Get the individual turn
    auto st = history[t];
    // get the SMPState for turn
    auto cp = (const SMPState*)history[t];
    // Extract information for each actor and dimension
    for (unsigned int i = 0; i < numAct; i++) {
      for (unsigned int k = 0; k < numDim; k++) {
        // Populate the actor
        auto ai = ((const SMPActor*)actrs[i]);
        // Get the Salience Value for each actor
        //                double sal = ai->vSal(k, 0);
        //bind the data
        rslt = sqlite3_bind_int(insStmtS, 1, t);
        assert(SQLITE_OK == rslt);
        rslt = sqlite3_bind_int(insStmtS, 2, i);
        assert(SQLITE_OK == rslt);
        rslt = sqlite3_bind_int(insStmtS, 3, k);
        assert(SQLITE_OK == rslt);
        rslt = sqlite3_bind_double(insStmtS, 4, ai->vSal(k, 0)); // was sal
        assert(SQLITE_OK == rslt);
        // record
        rslt = sqlite3_step(insStmtS);
        assert(SQLITE_DONE == rslt);
        sqlite3_clear_bindings(insStmtS);
        assert(SQLITE_DONE == rslt);
        rslt = sqlite3_reset(insStmtS);
        assert(SQLITE_OK == rslt);
      }
    }
  }

  // finish
  sqlite3_exec(smpDB, "END TRANSACTION", NULL, NULL, &zErrMsg);
  // finalize statement to avoid resource leaks
  sqlite3_finalize(insStmtD);
  sqlite3_finalize(insStmtC);
  sqlite3_finalize(insStmtS);
  delete sqlBuffD;
  sqlBuffD = nullptr;
  delete sqlBuffC;
  sqlBuffC = nullptr;
  delete sqlBuffS;
  sqlBuffS = nullptr;

  return;
}


// --------------------------------------------


void bindExecuteBargnTableUpdate(sqlite3_stmt *updateStmt, size_t turn, int bargnID,
                                 int initActor, double initProb, bool isInitSelected,
                                 int recvActor, double recvProb, bool isRecvSelected)  {

  int rslt = 0;

  rslt = sqlite3_bind_double(updateStmt, 1, initProb);
  assert(SQLITE_OK == rslt);

  //Init_Seld
  rslt = isInitSelected ? sqlite3_bind_int(updateStmt, 2, 1) : sqlite3_bind_int(updateStmt, 2, 0);
  assert(SQLITE_OK == rslt);

  // For SQ cases, there would be no receiver
  if (initActor != recvActor) {
    rslt = sqlite3_bind_double(updateStmt, 3, recvProb);
    assert(SQLITE_OK == rslt);

    //Recd_Seld
    rslt = isRecvSelected ? sqlite3_bind_int(updateStmt, 4, 1) : sqlite3_bind_int(updateStmt, 4, 0);
    assert(SQLITE_OK == rslt);
  }

  rslt = sqlite3_bind_int(updateStmt, 5, turn);
  assert(SQLITE_OK == rslt);

  rslt = sqlite3_bind_int(updateStmt, 6, bargnID);
  assert(SQLITE_OK == rslt);

  rslt = sqlite3_bind_int(updateStmt, 7, initActor);
  assert(SQLITE_OK == rslt);

  rslt = sqlite3_bind_int(updateStmt, 8, recvActor);
  assert(SQLITE_OK == rslt);

  rslt = sqlite3_step(updateStmt);
  assert(SQLITE_DONE == rslt);
  sqlite3_clear_bindings(updateStmt);
  assert(SQLITE_DONE == rslt);
  rslt = sqlite3_reset(updateStmt);
  assert(SQLITE_OK == rslt);

  return;
}



void SMPState::updateBargnTable(const vector<vector<BargainSMP*>> & brgns,
                                map<unsigned int, KBase::KMatrix>  actorBargains,
                                map<unsigned int, unsigned int>   actorMaxBrgNdx) const {

  const unsigned int t = myTurn();

  sqlite3 *db = model->smpDB;
  auto sqlBuff = newChars(300);

  // JAH 20160822 fixed the oversight of not conditioning on the scenario
  sprintf(sqlBuff, "UPDATE Bargn SET Init_Prob = ?1, Init_Seld = ?2, Recd_Prob = ?3, Recd_Seld = ?4 \
          WHERE (\"%s\" = ScenarioId) and (?5 = Turn_t) and (?6 = BargnId) and (?7 = Init_Act_i) and (?8 = Recd_Act_j)",
                  model->getScenarioID().c_str());

      const char* updateStr = sqlBuff;
  sqlite3_stmt *updateStmt = nullptr;

  // prepare the sql statement to update the db
  sqlite3_prepare_v2(model->smpDB, updateStr, strlen(updateStr), &updateStmt, NULL);

  assert(nullptr != updateStmt); // make sure it is ready

  // Error message in case
  char* zErrMsg = nullptr;

  // start for the transaction
  sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);

  // Update the bargain table for the bargain values for init actor and recd actor
  // along with the info whether a bargain got selected or not in the respective actor's queue
  for (unsigned int i = 0; i < brgns.size(); i++) {
    auto ai = ((const SMPActor*)(model->actrs[i]));
    double initProb = -0.1;
    int initSelected = -1;
    auto bargains_i = brgns[i];
    int initBgnNdx = 0;
    auto initActr = -1;
    auto rcvrActr = -1;
    //uint64_t bgID = 0; // tag uninitialized value
    int countDown = 2; // Stop iterating if cases for i:i and i:j processed
    for (auto bg : bargains_i) {
      assert(nullptr != bg);
      if (bg->actInit == bg->actRcvr) { // For SQ case
        initActr = model->actrNdx(bg->actInit);
        rcvrActr = initActr;
        initProb = (actorBargains[initActr])(initBgnNdx, 0);
        initSelected = initBgnNdx == actorMaxBrgNdx[initActr] ? 1 : 0;
        /*cout << __LINE__ << " " << "SQ" << " " << bg->getID() \
          << " " << initActr << ":" << rcvrActr << " " \
          << initProb << " " << initSelected << endl;*/

        bindExecuteBargnTableUpdate(updateStmt, t, bg->getID(),
                                    initActr, initProb, initSelected,
                                    rcvrActr, -1.0, 0);

        if (0 == --countDown) {
          break;
        }
      }
      else {
        if (ai == bg->actInit) { // this bargain is initiated by current actor
          initActr = model->actrNdx(bg->actInit);
          initProb = (actorBargains[initActr])(initBgnNdx, 0);
          initSelected = initBgnNdx == actorMaxBrgNdx[initActr] ? 1 : 0;
          rcvrActr = model->actrNdx(bg->actRcvr);
          //bgID = bg->getID();

          // Get the bargains of receiver actor
          auto brgnRcvr = brgns[rcvrActr];
          int rcvrBgNdx = 0;
          double rcvrProb = -1.0;
          int rcvrSelected = -1;
          for (auto bgRcv : brgnRcvr) {
            assert(nullptr != bgRcv);
            if (ai == bgRcv->actInit) {
              rcvrProb = (actorBargains[rcvrActr])(rcvrBgNdx, 0);

              // Check if it is the selected bargain for receiver actor
              rcvrSelected = actorMaxBrgNdx[rcvrActr] == rcvrBgNdx ? 1 : 0;

              /*std::cout.precision(4);
                cout << std::fixed;
                cout << "Line " << __LINE__ << " " << bgID << " " \
                << initActr << ":" << rcvrActr \
                << " init_prob: " << initProb \
                << " init_selected: " << initSelected \
                << " rcvr_prob: " << rcvrProb \
                << " rcvr_selected: " << rcvrSelected << endl << endl;*/

              --countDown;
              bindExecuteBargnTableUpdate(updateStmt, t, bg->getID(),
                                          initActr, initProb, initSelected,
                                          rcvrActr, rcvrProb, rcvrSelected);
              break;
            }
            ++rcvrBgNdx;
          }

          if (0 == countDown) {
            break;
          }
        }
      }

      ++initBgnNdx;
    }
  }

  sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &zErrMsg);
  sqlite3_finalize(updateStmt); // finalize statement to avoid resource leaks

  delete sqlBuff;
  sqlBuff = nullptr;
  model->smpDB = db;
  return;
}



};
// end of namespace

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
