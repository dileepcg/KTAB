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

#include "sqlitedemo.h"
#include "kmodel.h"

using std::cout;
using std::endl;
using std::flush;
using std::string;
using std::vector;

// ------------------------------------------------------------------------------------

namespace MDemo
{

SQLDB::SQLDB(char* filename)
{
  database = NULL;
  dbOpen = open(filename);
}

SQLDB::~SQLDB()
{
  if (dbOpen && (nullptr != database))
  {
    close();
  }
  database = nullptr;
}

bool SQLDB::open(char* filename)
{
  if (sqlite3_open(filename, &database) == SQLITE_OK)
  {
    return true;
  }
  else
  {
    return false;
  }
}

tuple<unsigned int, vector<vector<string>>> SQLDB::query(const char* query)
{
  assert(nullptr != database);
  assert(dbOpen);
  sqlite3_stmt *statement = nullptr;
  unsigned int rowC = 0;
  vector<vector<string> > results = {};
  if (sqlite3_prepare_v2(database, query, -1, &statement, 0) == SQLITE_OK)
  {
    int cols = sqlite3_column_count(statement);
    int resC = 0;
    while (true)
    {
      resC = sqlite3_step(statement);
      if (resC == SQLITE_ROW)
      {
        vector<string> values;
        for (int col = 0; col < cols; col++)
        {
          values.push_back((char*)sqlite3_column_text(statement, col));
        }
        results.push_back(values);
        rowC++;
      }
      else
      {
        break; // end the loop
      }
    }
    //printf("Retrieved %i rows \n", rowC);
    sqlite3_finalize(statement);
  }
  string error = sqlite3_errmsg(database);
  if (error != "not an error")
  {
    cout << query << " " << error << endl;
  }
  return tuple<unsigned int, vector<vector<string>>>(rowC, results);
}

void SQLDB::close()
{
  if (dbOpen)
  {
    sqlite3_close(database);
    dbOpen = false;
  }
  return;
}

void demoDBObject()
{
  cout << endl << endl << "Demo SQLDB"<<endl;
  using std::get;

  const bool parP = KBase::testMultiThreadSQLite(false, KBase::ReportingLevel::Medium);

  cout << "Initializing sqlite3 ..."<<endl << flush;
  sqlite3_initialize();
  cout << "Creating database ..."<<endl << flush;
  auto db = new SQLDB("myDB.db");
  cout << "Inserting six records ..."<<endl << flush;
  db->query("CREATE TABLE tbl (a INTEGER, b INTEGER, c INTEGER);");
  db->query("INSERT INTO tbl VALUES(22, 50, 90);");
  db->query("INSERT INTO tbl VALUES(30, 29, 28);");
  db->query("INSERT INTO tbl VALUES(31, 17, 27);");
  db->query("INSERT INTO tbl VALUES(26, 88, 82);");
  db->query("INSERT INTO tbl VALUES(45, 61, 66);");
  db->query("INSERT INTO tbl VALUES(48, 38, 46);");

  cout << "Selecting three records ..." << endl << flush;
  auto rslt = db->query("SELECT * FROM tbl WHERE c>50;");
  unsigned int rowC = get<0>(rslt);
  printf("Retrieved %u rows \n", rowC);
  vector<vector<string> > rStr = get<1>(rslt);
  for (vector<vector<string> >::iterator it = rStr.begin(); it < rStr.end(); ++it)
  {
    vector<string> row = *it;
    cout << "Values: (a=" << row.at(0) << ", b=" << row.at(1) << ", c=" << row.at(2) << ")" << endl;
  }

  cout << "Deleting database ..."<<endl << flush;
  delete db;
  db = nullptr;

  cout << "Shutting down SQlite3."<< endl << flush;
  sqlite3_shutdown();
  return;
}


} // end of namespace

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
