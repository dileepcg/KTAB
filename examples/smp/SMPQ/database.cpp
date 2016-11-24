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

#include "database.h"

Database::Database()
{
    // Database Initiaization
    QStringList driverList;
    driverList = QSqlDatabase::drivers();

    if (!driverList.contains("QSQLITE", Qt::CaseInsensitive))
        emit Message("Database Error", "No QSQLITE support! Check all needed dll-files!");

}

Database::~Database()
{

}

void Database::openDB(QString dbPath, bool run)
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);

    if(!db.open())
    {
        emit Message("Database Error", db.lastError().text());
    }
    else
    {
        // Scenarios list in db
        getScenarioList(run);

        getActorsDescriptionDB();


        // to update numActors in db
        getNumActors();

        // number of states/turns in db
        getNumStates();


        readVectorPositionTable(0,scenarioM,0);//turn
    }
}

void Database::openDBEdit(QString dbPath)
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);

    if(!db.open())
    {
        emit Message("Database Error", db.lastError().text());
    }
    else
    {
        readVectorPositionTableEdit(scenarioM);//turn

        // Scenarios list in db
        //  getScenarioList();

        // readVectorPositionTableEdit(scenario_m);//turn
    }
}

void Database::getScenarioData(int turn, QString scenario,int dim)
{
    scenarioM=scenario;
    readVectorPositionTable(turn,scenarioM,dim);//turn
}

void Database::getScenarioDataEdit(QString scenario)
{
    scenarioM=scenario;
}

void Database::getStateCount()
{
    getNumStates();
}

void Database::getDimensionCount()
{
    dimensionsList = new QStringList;
    QSqlQuery qry;
    QString query= QString("select Dim_k, Desc from DimensionDescription where ScenarioId = '%1'").arg(scenarioM);
    qry.exec(query);

    while(qry.next())
    {
        numDimension = qry.value(0).toInt();
        dimensionsList->append(qry.value(1).toString());
    }

    emit dimensionsCount(numDimension,dimensionsList);
}

void Database::getActorsDescriptionDB()
{
    actorNameList.clear();
    actorDescList.clear();

    QSqlQuery qry;
    QString query= QString("select Name,DESC from ActorDescription  where ScenarioId = '%1' ").arg(scenarioM);

    qry.exec(query);

    while(qry.next())
    {
        actorNameList.append(qry.value(0).toString());
        actorDescList.append(qry.value(1).toString());
    }

    emit actorsNameDesc(actorNameList , actorDescList);
}

void Database::getInfluenceDB(int turn)
{
    actorInfluence.clear();

    //qDebug()<<scenario_m << "turn" << turn ;
    QSqlQuery qry;
    QString query= QString(" select SpatialCapability.Cap from SpatialCapability,ActorDescription where "
                           " ActorDescription.Act_i = SpatialCapability.Act_i "
                           " and SpatialCapability.ScenarioId='%1' "
                           " and ActorDescription.ScenarioId='%1'"
                           " and SpatialCapability.Turn_t='%2' "
                           ).arg(scenarioM).arg(turn);

    qry.exec(query);

    while(qry.next())
    {
        actorInfluence.append(qry.value(0).toString());
        //qDebug()<<actorInfluence << "Influence" <<turn;
    }

    emit actorsInflu(actorInfluence);
}

void Database::getPositionDB(int dim, int turn)
{
    actorPosition.clear();
    //qDebug()<<scenario_m;

    QSqlQuery qry;
    QString query= QString(" select VectorPosition.Coord from VectorPosition,ActorDescription where"
                           " ActorDescription.Act_i = VectorPosition.Act_i"
                           " and VectorPosition.ScenarioId='%2' "
                           " and ActorDescription.ScenarioId='%2'"
                           " and VectorPosition.Turn_t='%3'"
                           " and VectorPosition.dim_k='%1'")
            .arg(dim).arg(scenarioM).arg(turn);

    qry.exec(query);

    while(qry.next())
    {
        actorPosition.append(qry.value(0).toString());
    }
    emit actorsPostn(actorPosition,dim);
}

void Database::getSalienceDB(int dim, int turn)
{
    actorSalience.clear();
    //qDebug()<<scenario_m;

    QSqlQuery qry;
    QString query= QString(" select SpatialSalience.Sal from SpatialSalience,ActorDescription where"
                           " ActorDescription.Act_i = SpatialSalience.Act_i"
                           " and SpatialSalience.ScenarioId='%2' "
                           " and ActorDescription.ScenarioId='%2' "
                           " and SpatialSalience.Turn_t='%3'"
                           " and SpatialSalience.dim_k='%1'")
            .arg(dim).arg(scenarioM).arg(turn);

    qry.exec(query);

    while(qry.next())
    {
        actorSalience.append(qry.value(0).toString());
    }
    emit actorsSalnce(actorSalience,dim);
}

void Database::getActorsInRangeFromDB(double lowerRng, double higherRng, int dim, int turn)
{
    double lwr = lowerRng/100;
    double upr = higherRng/100;
    actorIdsList.clear();
    actorSalienceList.clear();
    actorCapabilityList.clear();

    QSqlQuery qry;
    QString query= QString(" select Act_i from VectorPosition where"
                           " Coord >= '%1'  AND Coord < '%2' AND "
                           " Dim_k='%3' AND ScenarioId='%4' "
                           "AND Turn_t='%5'")
            .arg(lwr).arg(upr).arg(dim).arg(scenarioM).arg(turn);

    qry.exec(query);

    while(qry.next())
    {
        actorIdsList.append(qry.value(0).toInt());
    }
    // qDebug()<<actorIdsList << "ACat tat"  << query;

    for(int actInd =0; actInd < actorIdsList.length() ; actInd++)
    {
        QSqlQuery qry;
        QString query= QString(" select Sal from SpatialSalience where"
                               " Act_i = '%1' AND ScenarioId='%2' AND Turn_t='%3'"
                               " AND Dim_k='%4'")
                .arg(actorIdsList.at(actInd)).arg(scenarioM).arg(turn).arg(dim);

        qry.exec(query);

        while(qry.next())
        {
            actorSalienceList.append(qry.value(0).toDouble());
        }
    }
    for(int actInd =0; actInd < actorIdsList.length() ; actInd++)
    {
        QSqlQuery qry1;
        QString query1= QString(" select Cap from SpatialCapability where"
                                " Act_i = '%1' AND ScenarioId='%2' AND Turn_t='%3'")
                .arg(actorIdsList.at(actInd)).arg(scenarioM).arg(turn);

        qry1.exec(query1);

        while(qry1.next())
        {
            actorCapabilityList.append(qry1.value(0).toDouble());
        }
    }
    emit listActorsSalienceCapability(actorIdsList,actorSalienceList,actorCapabilityList,lwr,upr);
}

void Database::getDims()
{
    dimList = new QStringList;
    QSqlQuery qry;
    QString query= QString("select Desc from DimensionDescription where ScenarioId = '%1'").arg(scenarioM);
    qry.exec(query);

    while(qry.next())
    {
        dimList->append(qry.value(0).toString());
    }
    emit dimensList(dimList);
}

void Database::getUtilChlgAndSQvalues(QList<int> VHAxisValues)
{
    if(10==VHAxisValues.length())
    {
        QSqlQuery qry;
        QString query = QString(" select H.diff as Hori_Coord, V.diff as Vert_Coord from "
                                "(select Turn_t,Est_h,Aff_k,Init_i,Rcvr_j,Util_Chlg - Util_SQ as diff from UtilChlg "
                                " where ScenarioId = '%1' and  Turn_t = '%2' and Init_i = '%3' and Rcvr_j = '%4' "
                                " and aff_k = '%5' and est_h = '%6') as V "
                                " inner join "
                                "(select Turn_t,Est_h,Aff_k,Init_i,Rcvr_j,Util_Chlg - Util_SQ as diff from UtilChlg "
                                " where ScenarioId = '%1' and Turn_t = '%2' and Init_i = '%3' and Rcvr_j ='%4' "
                                " and aff_k = '%7' and est_h = '%8') as H "
                                " on (V.Turn_t = H.Turn_t) and (V.Init_i = H.Init_i) and (V.Rcvr_j = H.Rcvr_j)")
                .arg(scenarioM).arg(VHAxisValues[0]).arg(VHAxisValues[3]).arg(VHAxisValues[4])
                .arg(VHAxisValues[2]).arg(VHAxisValues[1]).arg(VHAxisValues[7]).arg(VHAxisValues[6]);

        qry.exec(query);

        while(qry.next())
        {
            hCord=qry.value(0).toDouble();
            vCord=qry.value(1).toDouble();
        }
        emit utilChlngAndSQ(VHAxisValues[0],hCord,vCord,VHAxisValues[4]);
    }
}

void Database::getVectorPosition(int actor, int dim, int turn, QString scenario)
{
    QSqlQuery qry;
    QString query;

    int i =0;
    QVector<double> x(numStates+1), y(numStates+1);

    query= QString("select * from VectorPosition where Act_i='%1' and Dim_k='%2' and Turn_t<='%3' and  ScenarioId = '%4' ")
            .arg(actor).arg(dim).arg(turn).arg(scenario);

    qry.exec(query);
    while(qry.next())
    {
        x[i]=qry.value(1).toDouble();
        y[i]=qry.value(4).toDouble()*100;// y scales from 0 to 100
        ++i;
    }

    QSqlQuery qry2;
    QString actorName;

    QString query2 = QString("select * from ActorDescription where Act_i='%1' and ScenarioId='%2'")
            .arg(actor).arg(scenario);
    qry2.exec(query2);
    while(qry2.next())
    {
        actorName = qry2.value(2).toString();
    }

    emit vectorPosition(x,y,actorName,turn);
}

void Database::readVectorPositionTable(int turn, QString scenario, int dim)
{
    //change in scenario should update STATES ACTORS
    getNumStates();
    if(numStates<turn)
        turn=numStates;

    getNumActors();

    sqlmodel = new QStandardItemModel(this);
    QSqlQuery qry;
    QString query;

    query= QString("select * from VectorPosition where Turn_t='%1' and Dim_k='%2' and ScenarioId='%3'")
            .arg(turn).arg(dim).arg(scenario);

    qry.exec(query);

    int rowindex =0;
    while(qry.next())
    {
        QString value = qry.value(0).toString();
        QString value1 = qry.value(1).toString();

        QStandardItem *item = new QStandardItem(value.trimmed());
        QStandardItem *item1 = new QStandardItem(value1.trimmed());

        sqlmodel->setItem(rowindex,0,item);
        sqlmodel->setItem(rowindex,1,item1);

        ++rowindex;
    }
    // load parsed data to model accordingly
    emit dbModel(sqlmodel);

    //To plot graph

    for(int actors=0; actors <= numActors; ++actors)
        getVectorPosition(actors,dim,turn,scenario);//actors, dimension, turn
}

void Database::readVectorPositionTableEdit(QString scenario)
{
    //choose the first scenario as default
    sqlmodelEdit = new QSqlTableModel(this);
    sqlmodelEdit->setTable("VectorPosition");
    sqlmodelEdit->setFilter(QString("Turn_t=0 and ScenarioId='%1'").arg(scenario));
    sqlmodelEdit->select();

    emit dbModelEdit(sqlmodelEdit);
}

void Database::getNumActors()
{
    QSqlQuery qry;
    QString query= QString("select Act_i from ActorDescription where ScenarioId='%1'" )
            .arg(scenarioM);

    qry.exec(query);

    while(qry.next())
    {
        numActors = qry.value(0).toInt();
    }

    emit actorCount(numActors);
}

void Database::getNumStates()
{
    QSqlQuery qry;
    QString query= QString("select DISTINCT Turn_t from VectorPosition where ScenarioId='%1'" )
            .arg(scenarioM);

    qry.exec(query);

    while(qry.next())
    {
        numStates = qry.value(0).toInt();
    }

    emit statesCount(numStates);
}

void Database::getScenarioList(bool run)
{
    scenarioList = new QStringList;
    scenarioIdList = new QStringList;
    scenarioDescList = new QStringList;
    QSqlQuery qry;
    QString query= QString("select Scenario,ScenarioId,DESC from ScenarioDesc ");

    qry.exec(query);

    while(qry.next())
    {
        scenarioList->append(qry.value(0).toString());
        scenarioIdList->append(qry.value(1).toString());
        scenarioDescList->append(qry.value(2).toString());
    }

    if(scenarioList->length()>0)
        if(run)
        {
            scenarioM =  scenarioIdList->at(scenarioIdList->length()-1);
            emit scenarios(scenarioList,scenarioIdList,scenarioDescList,scenarioIdList->length()-1);
        }
        else
        {
            scenarioM =  scenarioIdList->at(0);
            emit scenarios(scenarioList,scenarioIdList,scenarioDescList,0);
        }
    else
        Message("Database","there are no Scenario's");

}
// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
