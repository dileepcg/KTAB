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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "csv.h"
#include "database.h"
#include "../qcustomplot/qcustomplot.h"

#include <QtSql>
#include <QSqlQuery>
#include <QMessageBox>
#include <QSqlError>
#include <QRegExp>

#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <string>

#include <QtWidgets>
#ifndef QT_NO_PRINTDIALOG
//#include <QtPrintSupport>
#endif

#include <QDebug>
#include <QMessageBox>
#include <QMenu>

class QAction;
class QListWidget;
class QMenu;
class QTextEdit;

namespace Ui {
class MainWindow;
}

static const int N = 2;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    //explicit MainWindow(QWidget *parent = 0);
    MainWindow();
    ~MainWindow();

private slots:
    void about();
    void dockWindowChanged();
    //CSV
    void setCSVItemModel(QStandardItemModel * model, QStringList scenarioName);
    void csvGetFilePAth(bool bl);
    //Database
    void setDBItemModel(QStandardItemModel *model);
    void setDBItemModelEdit();
    void displayMessage(QString cls, QString message);
    void dbGetFilePAth(bool bl, QString dbPath =0,bool run = false );
    void dbEditGetFilePAth(bool bl);
    void updateStateCountSliderRange(int states);
    void updateScenarioListComboBox(QStringList * scenarios, QStringList *scenarioIds, QStringList *scenarioDesc, int indx);
    void updateDimensionCount(int dim, QStringList *dimList);
    void updateActorCount(int actNum);

    //Central-  Controls Frame
    void sliderStateValueToQryDB(int value);
    void scenarioComboBoxValue(int scenarioBox);
    void createNewCSV(bool bl);
    void cellSelected(QStandardItem *in);
    void insertNewRowCSV();
    void insertNewColumnCSV();
    void donePushButtonClicked(bool bl);

    //    bool eventFilter(QObject*, QEvent*);
    void displayMenuTableWidget(QPoint pos);
    void displayMenuTableView(QPoint pos);

    //DB to CSV
    void actorsNameDesc(QList<QString> actorName, QList<QString> actorDescription);
    void actorsInfluence(QList<QString> ActorInfluence);
    void actorsPosition(QList<QString> actorPosition, int dim);
    void actorsSalience(QList<QString> actorSalience, int dim);

signals:
    //CSV
    void csvFilePath(QString path);

    //Database
    void dbFilePath(QString path,bool runScenario = false);
    void dbEditFilePath(QString path);

    void getScenarioRunValues(int state, QString scenarioBox,int dim);
    void getScenarioRunValuesEdit(QString scenarioBox);
    void getStateCountfromDB();
    void getDimensionCountfromDB();
    void getDimforBar();

    //save DB to csv
    void getActorsDesc();
    void getInfluence(int turn);
    void getPosition(int dim,int turn);
    void getSalience(int dim,int turn);

private:
    //MainWindow
    void createActions();
    void createStatusBar();
    void createLinePlotsDockWindows();
    void createBarPlotsDockWindows();
    void createQuadMapDockWindows();
    void createModuleParametersDockWindow();
    void saveTableViewToCSV();
    void saveTableWidgetToCSV();
    int validateControlButtons(QString viewName);

    void updateDBViewColumns();
    QString checkForHeaderString(QString header);

    // Central Main Frame
    QFrame *central;
    QFrame *tableControlsFrame;
    QGridLayout *gLayout;
    QStackedWidget * stackWidget;

    QComboBox * scenarioComboBox;
    QPushButton * actorsPushButton;
    QLineEdit * actorsLineEdit;
    QPushButton * dimensionsPushButton;
    QLineEdit * dimensionsLineEdit;
    QPushButton * donePushButton;
    QLineEdit * scenarioDescriptionLineEdit;
    QSlider * turnSlider;

    //Model parameters
    QFrame * frames[N];
    QPushButton * runButton;

    QRadioButton * rparam1;
    QRadioButton * rparam2;
    QRadioButton * rparam3;

    QCheckBox * cparam1;
    QCheckBox * cparam2;
    QCheckBox * cparam3;

    //csv window
    QTableView * csvTableView;
    CSV *csvObj;

    QTableWidget * csvTableWidget;

    QMenu *viewMenu;

    QDockWidget * moduleParametersDock;
    QDockWidget * lineGraphDock;
    QDockWidget * barGraphDock;
    QDockWidget * quadMapDock;

    QGridLayout * VLayout;
    QFrame * moduleFrame;

    //Database Obj
    Database * dbObj ;
    QSqlDatabase db;
    int dimensions;
    QString dbPath;

    //Graph 1 widget
    QFrame *lineGraphMainFrame;
    QGridLayout * lineGraphGridLayout;
    QCustomPlot * lineCustomGraph;

    void initializeLineGraphPlot();
    void plotGraph();
    void initializeCentralViewFrame();

    QString scenarioBox;
    QStringList mScenarioIds;
    QStringList mScenarioDesc;
    QStringList mScenarioName;
    //graph - customplot

    int numAct;
    int prevTurn;
    bool firstVal;
    //    to  edit headers

    QLineEdit* headerEditor;
    int editorIndex;

    QString  tableType; // CSV, Database, NewCSV
    QStandardItemModel *modeltoCSV;
    QStandardItemModel *modeltoDB;

    QString inputCSV;
    void createSeperateColumn(QTableWidgetItem *hdr);

    //DB to CSV
    int dimension;
    QList <QString> actorsName;
    QList <QString> actorsDescription;
    QList <QString> actorsInfl;
    QList <QString> actorsPos[3];
    QList <QString> actorsSal[3];

    QStringList dimensionList;

private slots:
    void titleDoubleClick(QMouseEvent *event, QCPPlotTitle *barGraphTitle);
    //    void axisLabelDoubleClick(QCPAxis* axis, QCPAxis::SelectablePart part);
    //    void legendDoubleClick(QCPLegend* legend, QCPAbstractLegendItem* item);
    //    void selectionChanged();
    void mousePress();
    void mouseWheel();
    void addGraphOnModule1(const QVector<double> &x, const QVector<double> &y, QString Actor, int turn);
    //    void removeSelectedGraph();
    void removeAllGraphs();
    void contextMenuRequest(QPoint pos);
    void moveLegend();
    void graphClicked(QCPAbstractPlottable *plottable);
    void updateBarDimension(QStringList* dims);

    //Bar Charts
private :
    void initializeBarGraphDock();
    void initializeBarGraphPlot();
    void populateBarGraphActorsList();
    void populateBarGraphDimensions(int dim);
    void populateBarGraphStateRange(int states);
    void generateColors();
    void getActorsInRange(int dim);
    void setStackedBars();
    void deleteBars();

    QCPBars *createBar(int actorId);

    QFrame * barGraphControlsFrame;
    QFrame * barGraphMainFrame;
    QGridLayout * barGraphGridLayout;
    QGridLayout * barGridLayout;
    QScrollArea * barGraphActorsScrollArea;
    QCustomPlot * barCustomGraph;

    QCheckBox * barGraphSelectAllCheckBox;
    QRadioButton * barGraphRadioButton;
    QComboBox * barGraphDimensionComboBox;
    QLineEdit * barGraphGroupRangeLineEdit;

    QSlider * barGraphTurnSlider;
    QList <QCheckBox *> barGraphActorsCheckBoxList;
    QList <bool> barGraphCheckedActorsIdList;
    QPushButton * barGraphBinWidthButton;


    QList<QCPBars *> bars[100];
    QCPBars * prevBar;
    QCPBars * prevBarU;
    QList<QColor> colorsList;

    QList <QCheckBox * > barActorCBList;
    QList <int> actorsIdsClr;

    double yAxisLen;

    int in;
    int barsCount;

    double currentStackHeight[100];
    double binWidth;

    QCPPlotTitle * barGraphTitle;

signals :
    void getActorIdsInRange(double lowerRange, double upperRange, int dimension, int turn);

private slots:
    void barGraphSelectAllActorsCheckBoxClicked(bool Click);
    void barGraphActorsCheckboxClicked(bool click);
    void barGraphDimensionChanged(int value);
    void barGraphTurnSliderChanged(int value);
    void barGraphBinWidthButtonClicked(bool bl);
    void barGraphActorsSalienceCapability(QList<int> aId, QList<double> sal, QList<double>cap, double r1, double r2);
    void xAxisRangeChanged( const QCPRange &newRange, const QCPRange &oldRange );
    void yAxisRangeChanged( const QCPRange &newRange, const QCPRange &oldRange );

    //line Graph
private :
    void initializeLineGraphDock();
    void populateLineGraphActorsList();
    void populateLineGraphDimensions(int dim);
    void populateLineGraphStateRange(int states);
    void createSpline(const QVector<double> &x, const QVector<double> &y, QString Actor, int turn);

    void clearAllLabels();
    void clearAllGraphs();
    void reconnectPlotWidgetSignals();

    QFrame * lineGraphControlsFrame;
    QScrollArea *lineGraphActorsScrollArea;
    QCheckBox *lineGraphSelectAllCheckBox;
    QRadioButton *lineGraphRadioButton ;
    QRadioButton *sankeyGraphRadioButton;
    QComboBox *lineGraphDimensionComboBox;
    QStackedWidget *graphTypeStackedWidget;
    QSlider *lineGraphTurnSlider;

    QList <QCheckBox *> lineGraphActorsCheckBoxList;
    QList <bool> lineGraphCheckedActorsIdList;

    QList <QCheckBox * > lineActorCBList;

    QCPPlotTitle * lineGraphTitle;

    int numStates;

    QList <bool> lineLabelToggleList;
    QList <QCPItemText * > lineLabelList;
    QCPItemText *textLabel ;
    int tnty;

private slots :
    void lineGraphSelectAllActorsCheckBoxClicked(bool click);
    void lineGraphDimensionChanged(int value);
    void lineGraphTurnSliderChanged(int);
    void lineGraphActorsCheckboxClicked(bool click);
    void updateLineDimension(QStringList *dims);
    void toggleLabels();
    void splineValues(const QVector<double> &x, const QVector<double> &y);

    //run smp

private :
    QString csvPath;

private slots :
    void runPushButtonClicked(bool bl);
    void smpDBPath(QString smpdbPath);
    void disableRunButton(QTableWidgetItem * itm);

    //Log
private :
    QString logFileName;
    QString defaultLogFile;

    QActionGroup * logActions;

    QAction *logDefaultAct;
    QAction *logNewAct;
    QAction *logNoneAct;

    std::streambuf *coutbuf;
    FILE *stream ;
    FILE fp_old;

    void logSMPDataOptionsAnalysis();

    //Quadmap
private :
    QFrame * quadMapMainFrame;
    QGridLayout * quadMapGridLayout;
    QCustomPlot * quadMapCustomGraph;
    QCPPlotTitle * quadMapTitle;
    QScrollArea * quadMapInitiatorsScrollArea;
    QScrollArea * quadMapReceiversScrollArea;
    QFrame * quadMapPerspectiveFrame;

    QList <QRadioButton *> quadMapInitiatorsRadioButtonList;
    QList <QCheckBox *> quadMapReceiversCheckBoxList;
    QList <bool> quadMapReceiversCBCheckedList;

    QComboBox * vComboBox;
    QComboBox * hComboBox;

    QCheckBox * selectAllReceiversCB;

    QComboBox * perspectiveComboBox;
    QSlider * quadMapTurnSlider;

    QVector <double> deltaUtilV;
    QVector <double> deltaUtilH;
    int actorIdIndexV;
    QList <int> actorIdIndexH;

    int actorsQueriedCount;

    QCPItemRect *   xRectItemPP;
    QCPItemRect *   xRectItemMP;
    QCPItemRect *   xRectItemMM;
    QCPItemRect *   xRectItemPM;

    QCheckBox * autoScale;
    QString prevScenario;

    QList <int> VHAxisValues;

    QPushButton * plotQuadMap;
    int initiatorTip;

    void initializeQuadMapDock();
    void initializeQuadMapPlot();

    void populateInitiatorsAndReceiversRadioButtonsAndCheckBoxes();
    void populatePerspectiveComboBox();
    void populateQuadMapStateRange(int states);
    void getUtilChlgHorizontalVerticalAxisData(int turn);
    void getUtilChlgHorizontalAxisData(int turn);
    void plotScatterPointsOnGraph(QVector<double> x, QVector<double> y, int actIndex);
    void plotDeltaValues();
    void removeAllScatterPoints();

private slots :
    void populateVHComboBoxPerspective(int index);
    void populateHcomboBox(QString vComboBoxText);

    void initiatorsChanged(bool bl);
    void receiversChanged(bool bl);

    void selectAllReceiversClicked(bool bl);
    void quadMapTurnSliderChanged(int turn);

    void quadMapUtilChlgandSQValues(int turn, double hor, double ver,
                                    int actorID);

    void xAxisRangeChangedQuad(QCPRange newRange, QCPRange oldRange);
    void yAxisRangeChangedQuad(QCPRange newRange, QCPRange oldRange);

    void quadMapAutoScale(bool status);
    void quadMapPlotPoints(bool status);

signals :
    void getUtilChlgAndUtilSQfromDB(QList <int > VHAxisList);

};

#endif // MAINWINDOW_H


// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
