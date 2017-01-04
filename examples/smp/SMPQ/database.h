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
// -------------------------------------------------


#ifndef DATABASE_H
#define DATABASE_H

#include <QtCore>
#include <QtSql>
#include <QSqlQuery>
#include <QMessageBox>
#include <QSqlError>
#include <QStandardItemModel>

class Database : public QObject
{
    Q_OBJECT
public:
    Database();
    ~Database();

public slots :
    void openDB(QString dbPath,bool run=false);
    void openDBEdit(QString dbPath);
    void getScenarioData(int turn, QString scenario, int dim);
    void getScenarioDataEdit(QString scenario);
    void getStateCount();
    void getDimensionCount();

    //DB to CSV
    void getActorsDescriptionDB();
    void getInfluenceDB(int turn =0);
    void getPositionDB(int dim,int turn =0);
    void getSalienceDB(int dim,int turn =0);
    void getAffinityDB();

    //BarCharts
    void getActorsInRangeFromDB(double lowerRng, double higherRng, int dim, int turn);
    void getDims();

    //QuadMap
    void getUtilChlgAndSQvalues(QList<int> VHAxisValues);

signals:
    void Message(QString , QString );
    void vectorPosition(QVector<double> x, QVector<double> y, QString actor,int turn);
    void dbModel(QStandardItemModel  *);
    void dbModelEdit(QSqlTableModel *);
    void actorCount(int value);
    void statesCount(int value);
    void dimensionsCount(int value,QStringList *dimensions);
    void dimensList(QStringList *dimensions);
    void scenarios(QStringList *scenariosList,QStringList *scenariosIdList, QStringList *scenarioDesc, int indx);

    //DB to CSV
    void actorsNameDesc(QList <QString>, QList <QString>);
    void actorsInflu(QList <QString>);
    void actorsPostn(QList<QString>,int);
    void actorsSalnce(QList<QString>,int);
    void actorsAffinity(QList<QString>,QList<int>,QList<int>);

    //BarCharts
    void listActorsSalienceCapability(QList<int>,QList<double>,QList<double>,double r1,double r2);

    //QuadMap
    void utilChlngAndSQ(int turn, double hor, double ver , int actorID);

private:
    QSqlDatabase db;
    //    QSqlTableModel * sqlmodel;
    QSqlTableModel * sqlmodelEdit;
    QStandardItemModel * sqlmodel;

    int numActors =0;
    int numStates =0;
    int numDimension =0;
    QStringList * dimensionsList;
    QStringList * dimList;
    QStringList * scenarioList;
    QStringList * scenarioIdList;
    QStringList * scenarioDescList;
    QString scenarioM;

    //DB to CSV
    QList <QString> actorNameList;
    QList <QString> actorDescList;
    QList <QString> actorInfluence;
    QList <QString> actorPosition;
    QList <QString> actorSalience;
    QList <QString> actorAffinity;
    QList <int>     actorI;
    QList <int>     actorJ;

    //BarChart
    QList <int> actorIdsList;
    QList <double> actorSalienceList;
    QList <double> actorCapabilityList;

    //quadMap
    QList <double> utilChlgV;
    QList <double> utilSQV;
    QList <double> utilChlgH;
    QList <double> utilSQH;

    double hCord;
    double vCord;

    void getVectorPosition(int actor, int dim, int turn, QString scenario);

    //Default read Turn_t=0
    void readVectorPositionTable(int state, QString scenario, int dim);

    // number of actors
    void getNumActors();
    // number of states/turns in db
    void getNumStates();
    // Scenarios list in db
    void getScenarioList(bool run);

    void readVectorPositionTableEdit(QString scenario);
};

#endif // DATABASE_H



// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
