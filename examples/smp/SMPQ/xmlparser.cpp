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

#include "xmlparser.h"
#include <QDebug>

Xmlparser::Xmlparser()
{

}

void Xmlparser::openXmlFile(QString xmlFilePath)
{
    QFile* xmlFile = new QFile(xmlFilePath);

    if(xmlFile->open(QIODevice::ReadOnly | QIODevice::Text))
    {
        xmlFileName=xmlFilePath;
        emit openXMLStatus(true);
    }
    else
        emit openXMLStatus(false);
}

void Xmlparser::readXmlFile()
{
    xmlModel = new QStandardItemModel;

    dimensionNames.clear();
    modelParametes.clear();
    actorsList.clear();
    idealAdjustmentList.clear();

    QFile* xmlFile = new QFile(xmlFileName);

    xmlFile->open(QIODevice::ReadOnly | QIODevice::Text);

    xmlReader.setDevice(xmlFile);

    //Reading from the file
    while(!xmlReader.atEnd() && !xmlReader.hasError())
    {
        QXmlStreamReader::TokenType token =xmlReader.readNext();
        if(token == QXmlStreamReader::StartDocument)
            continue;

        if (xmlReader.isStartElement())
        {
            QString tokenName = xmlReader.name().toString();
            if (tokenName == "name")
            {
                name = xmlReader.readElementText();
//                qDebug() << name;
            }
            else if(tokenName == "desc" )
            {
                desc = xmlReader.readElementText();
//                qDebug() << desc;
            }
            else if(tokenName == "prngSeed" )
            {
                prngSeed = xmlReader.readElementText();
//                qDebug() << prngSeed;
            }
            else if(tokenName == "ModelParameters" )
            {
                processModelParameters();
            }
            else if(tokenName == "Dimensions" )
            {
                processDimensions();
            }
            else if(tokenName == "Actor" )
            {
                ProcessActors();

            }
            else if(tokenName == "iaPair" )
            {
                ProcessIdealAdjustments();
            }
            else
            {
            }
        }
        else
        {
        }
    }
    if (xmlReader.hasError())
    {
        qDebug()<<"Error here !!" << xmlReader.errorString();
    }
    else
    {
        organizeXmlData();
    }
    xmlFile->close();
}

void Xmlparser::saveToXmlFile(QStringList parameters, QStandardItemModel *smpData, QStandardItemModel *affMatrix)
{
    QString filename = QFileDialog::getSaveFileName(0,tr("Save Xml"), ".",tr("Xml files (*.xml)"));

    if(!filename.endsWith(".xml"))
        filename.append(".xml");

    QFile file(filename);
    file.open(QIODevice::WriteOnly);
    QXmlStreamWriter xmlWriter;
    xmlWriter.setAutoFormatting(true);
    xmlWriter.setDevice(&file);

    /* Writes a document start with the XML version number. */
    xmlWriter.writeStartDocument();

    xmlWriter.writeStartElement("Scenario");
    xmlWriter.writeAttribute("xmlns:xsi","http://www.w3.org/2001/XMLSchema-instance");
    xmlWriter.writeAttribute("xsi:noNamespaceSchemaLocation","smpSchema.xsd");

    tagElements(&xmlWriter,"name",parameters.at(0));
    tagElements(&xmlWriter,"desc",parameters.at(1));
    tagElements(&xmlWriter,"prngSeed",parameters.at(2));

    //ModuleParameters
    xmlWriter.writeStartElement("ModelParameters");
    tagElements(&xmlWriter,"VictoryProbModel",parameters.at(3));
    tagElements(&xmlWriter,"PCEModel",parameters.at(4));
    tagElements(&xmlWriter,"StateTransitions",parameters.at(5));
    tagElements(&xmlWriter,"VotingRule",parameters.at(6));
    tagElements(&xmlWriter,"BigRAdjust",parameters.at(7));
    tagElements(&xmlWriter,"BigRRange",parameters.at(8));
    tagElements(&xmlWriter,"ThirdPartyCommit",parameters.at(9));
    tagElements(&xmlWriter,"InterVecBrgn",parameters.at(10));
    tagElements(&xmlWriter,"BargnModel",parameters.at(11));
    //End Module Parameters
    xmlWriter.writeEndElement();

    int dims = parameters.length() - 12;
//    qDebug()<<dims;

    //Dimensions
    xmlWriter.writeStartElement("Dimensions");
    if(dims>=1)
        tagElements(&xmlWriter,"dName",parameters.at(12));
    if(dims>=2)
        tagElements(&xmlWriter,"dName",parameters.at(13));
    if(dims>=3)
        tagElements(&xmlWriter,"dName",parameters.at(14));
    //end dimensions
    xmlWriter.writeEndElement();

    //Actors
    xmlWriter.writeStartElement("Actors");

    for(int i=0; i< smpData->rowCount();++i)
    {
        xmlWriter.writeStartElement("Actor");
        tagElements(&xmlWriter,"name",smpData->item(i,0)->text());
        tagElements(&xmlWriter,"description",smpData->item(i,1)->text());
        tagElements(&xmlWriter,"capability",smpData->item(i,2)->text());

        xmlWriter.writeStartElement("Position");
        if(dims>=1)
            tagElements(&xmlWriter,"dCoord",smpData->item(i,3)->text());
        if(dims>=2)
            tagElements(&xmlWriter,"dCoord",smpData->item(i,5)->text());
        if(dims>=3)
            tagElements(&xmlWriter,"dCoord",smpData->item(i,7)->text());
        xmlWriter.writeEndElement();//end position

        xmlWriter.writeStartElement("Salience");
        if(dims>=1)
            tagElements(&xmlWriter,"dSal",smpData->item(i,4)->text());
        if(dims>=2)
            tagElements(&xmlWriter,"dSal",smpData->item(i,6)->text());
        if(dims>=3)
            tagElements(&xmlWriter,"dSal",smpData->item(i,8)->text());
        xmlWriter.writeEndElement();//end Salience

        xmlWriter.writeEndElement();//end actor
    }
    xmlWriter.writeEndElement();//end Actors

    //Ideal Adjustment
    xmlWriter.writeStartElement("IdealAdjustment");

    for(int i=0; i< affMatrix->rowCount();++i)
    {
        for(int j=0; j<affMatrix->columnCount();++j)
        {
            xmlWriter.writeStartElement("iaPair");
            tagElements(&xmlWriter,"adjustingIdeal",affMatrix->headerData(i,Qt::Horizontal).toString());
            tagElements(&xmlWriter,"referencePos",affMatrix->headerData(j,Qt::Vertical).toString());
            tagElements(&xmlWriter,"adjust",affMatrix->item(i,j)->text());
            xmlWriter.writeEndElement();//end iaPair
        }
    }
    xmlWriter.writeEndElement();//end IdealAdjustment

    /*end scenario */
    xmlWriter.writeEndDocument();

    file.close();
    emit newXmlFilePath(filename);
}

void Xmlparser::saveNewDataToXmlFile(QStringList parameters, QTableWidget *smpDataWidget,
                                     QTableWidget *affintyDataWidget)
{
    QString filename =parameters.at(0);
    if(!filename.endsWith(".xml"))
        filename.append(".xml");

    QFile file(filename);
    file.open(QIODevice::WriteOnly);

    QXmlStreamWriter xmlWriter;
    xmlWriter.setAutoFormatting(true);
    xmlWriter.setDevice(&file);

    /* Writes a document start with the XML version number. */
    xmlWriter.writeStartDocument();

    xmlWriter.writeStartElement("Scenario");
    xmlWriter.writeAttribute("xmlns:xsi","http://www.w3.org/2001/XMLSchema-instance");
    xmlWriter.writeAttribute("xsi:noNamespaceSchemaLocation","smpSchema.xsd");

    tagElements(&xmlWriter,"name",parameters.at(1));
    tagElements(&xmlWriter,"desc",parameters.at(2));
    tagElements(&xmlWriter,"prngSeed",parameters.at(3));

    //ModuleParameters
    xmlWriter.writeStartElement("ModelParameters");
    tagElements(&xmlWriter,"VictoryProbModel",parameters.at(4));
    tagElements(&xmlWriter,"PCEModel",parameters.at(5));
    tagElements(&xmlWriter,"StateTransitions",parameters.at(6));
    tagElements(&xmlWriter,"VotingRule",parameters.at(7));
    tagElements(&xmlWriter,"BigRAdjust",parameters.at(8));
    tagElements(&xmlWriter,"BigRRange",parameters.at(9));
    tagElements(&xmlWriter,"ThirdPartyCommit",parameters.at(10));
    tagElements(&xmlWriter,"InterVecBrgn",parameters.at(11));
    tagElements(&xmlWriter,"BargnModel",parameters.at(12));
    //End Module Parameters
    xmlWriter.writeEndElement();

    int dims = parameters.length() - 13;
//    qDebug()<<dims;

    //Dimensions
    xmlWriter.writeStartElement("Dimensions");
    if(dims>=1)
        tagElements(&xmlWriter,"dName",parameters.at(13));
    if(dims>=2)
        tagElements(&xmlWriter,"dName",parameters.at(14));
    if(dims>=3)
        tagElements(&xmlWriter,"dName",parameters.at(15));
    //end dimensions
    xmlWriter.writeEndElement();


    //Actors
    xmlWriter.writeStartElement("Actors");

    for(int i=0; i< smpDataWidget->rowCount();++i)
    {
        xmlWriter.writeStartElement("Actor");
        tagElements(&xmlWriter,"name",smpDataWidget->item(i,0)->text());
        tagElements(&xmlWriter,"description",smpDataWidget->item(i,1)->text());
        tagElements(&xmlWriter,"capability",smpDataWidget->item(i,2)->text());

        xmlWriter.writeStartElement("Position");
        if(dims>=1)
            tagElements(&xmlWriter,"dCoord",smpDataWidget->item(i,3)->text());
        if(dims>=2)
            tagElements(&xmlWriter,"dCoord",smpDataWidget->item(i,5)->text());
        if(dims>=3)
            tagElements(&xmlWriter,"dCoord",smpDataWidget->item(i,7)->text());
        xmlWriter.writeEndElement();//end position

        xmlWriter.writeStartElement("Salience");
        if(dims>=1)
            tagElements(&xmlWriter,"dSal",smpDataWidget->item(i,4)->text());
        if(dims>=2)
            tagElements(&xmlWriter,"dSal",smpDataWidget->item(i,6)->text());
        if(dims>=3)
            tagElements(&xmlWriter,"dSal",smpDataWidget->item(i,8)->text());
        xmlWriter.writeEndElement();//end Salience

        xmlWriter.writeEndElement();//end actor
    }
    xmlWriter.writeEndElement();//end Actors

    //Ideal Adjustment
    xmlWriter.writeStartElement("IdealAdjustment");

    for(int i=0; i< affintyDataWidget->rowCount();++i)
    {
        for(int j=0; j<affintyDataWidget->columnCount();++j)
        {
            xmlWriter.writeStartElement("iaPair");
            tagElements(&xmlWriter,"adjustingIdeal",affintyDataWidget->horizontalHeaderItem(i)->text());
            tagElements(&xmlWriter,"referencePos",affintyDataWidget->verticalHeaderItem(j)->text());
            tagElements(&xmlWriter,"adjust",affintyDataWidget->item(i,j)->text());
            xmlWriter.writeEndElement();//end iaPair
        }
    }
    xmlWriter.writeEndElement();//end IdealAdjustment

    /*end scenario */
    xmlWriter.writeEndDocument();
    file.close();
    qDebug()<<xmlWriter.hasError();


}

void Xmlparser::tagElements(QXmlStreamWriter *xmlWriter, QString tagName, QString tagValue)
{
    xmlWriter->writeStartElement(tagName);
    xmlWriter->writeCharacters(tagValue);
    xmlWriter->writeEndElement();
}

void Xmlparser::processModelParameters()
{
    while (xmlReader.readNext() && !xmlReader.isEndElement())
    {
        QString tokenName = xmlReader.name().toString();
        if (tokenName == "VictoryProbModel")
        {
            // qDebug() << "VictoryProbModel:" << tokenName  << "text:" << xmlReader.readElementText();
            modelParametes.append(xmlReader.readElementText());
        }
        else if(tokenName == "PCEModel" )
        {
            //   qDebug() << "PCEModel:" << tokenName  << "text:" << xmlReader.readElementText();
            modelParametes.append(xmlReader.readElementText());
        }
        else if(tokenName == "StateTransitions" )
        {
            // qDebug() << "StateTransitions:" << tokenName  << "text:" << xmlReader.readElementText();
            modelParametes.append(xmlReader.readElementText());
        }
        else if(tokenName == "VotingRule" )
        {
            //qDebug() << "VotingRule:" << tokenName  << "text:" << xmlReader.readElementText();
            modelParametes.append(xmlReader.readElementText());
        }
        else if(tokenName == "BigRAdjust" )
        {
            //qDebug() << "BigRAdjust:" << tokenName  << "text:" << xmlReader.readElementText();
            modelParametes.append(xmlReader.readElementText());
        }
        else if(tokenName == "BigRRange" )
        {
            //qDebug() << "BigRRange:" << tokenName  << "text:" << xmlReader.readElementText();
            modelParametes.append(xmlReader.readElementText());
        }
        else if(tokenName == "ThirdPartyCommit" )
        {
            //qDebug() << "ThirdPartyCommit:" << tokenName  << "text:" << xmlReader.readElementText();
            modelParametes.append(xmlReader.readElementText());
        }
        else if(tokenName == "InterVecBrgn" )
        {
            //qDebug() << "InterVecBrgn:" << tokenName  << "text:" << xmlReader.readElementText();
            modelParametes.append(xmlReader.readElementText());
        }
        else if(tokenName == "BargnModel" )
        {
            //qDebug() << "BargnModel:" << tokenName  << "text:" << xmlReader.readElementText();
            modelParametes.append(xmlReader.readElementText());
        }

    }
}

void Xmlparser::processDimensions()
{
    while (xmlReader.readNext() && !xmlReader.isEndElement())
    {
        QString tokenName = xmlReader.name().toString();
        if (tokenName == "dName")
        {
            //qDebug() << "dName:" << tokenName  << "text:" << xmlReader.readElementText();
            dimensionNames.append(xmlReader.readElementText());
        }
    }
}

void Xmlparser::ProcessActors()
{
    QStringList actors;
    while (xmlReader.readNext() && !xmlReader.isEndElement() )
    {
        QString tokenName = xmlReader.name().toString();
        if (tokenName == "name")
        {
            //            qDebug() << "name:" << tokenName  << "text:" << xmlReader.readElementText();
            actors.append(xmlReader.readElementText());
        }
        else if(tokenName == "description" )
        {
            //            qDebug() << "description:" << tokenName  << "text:" << xmlReader.readElementText();
            actors.append(xmlReader.readElementText());
        }
        else if(tokenName == "capability" )
        {
            //            qDebug() << "capability:" << tokenName  << "text:" << xmlReader.readElementText();
            actors.append(xmlReader.readElementText());
        }

        else if(tokenName == "dCoord" )
        {
            //            qDebug() << "dCoord:" << tokenName  << "text:" << xmlReader.readElementText();
            actors.append(xmlReader.readElementText());
        }

    }
    while (xmlReader.readNext() && !xmlReader.isEndElement() )
    {
        QString tokenName = xmlReader.name().toString();
        if(tokenName == "dSal" )
        {
            //            qDebug() << "dSal:" << tokenName  << "text:" << xmlReader.readElementText();
            actors.append(xmlReader.readElementText());
        }
    }
    actorsList.append(actors);
    actors.clear();
 }

void Xmlparser::ProcessIdealAdjustments()
{
    QStringList idealAdj;
    while (xmlReader.readNext() && !xmlReader.isEndElement())
    {
        QString tokenName = xmlReader.name().toString();
        if (tokenName == "adjustingIdeal")
        {
            //            qDebug() << "adjustingIdeal:" << tokenName  << "text:" << xmlReader.readElementText();
            idealAdj.append(xmlReader.readElementText());
        }
        else if(tokenName == "referencePos" )
        {
            //            qDebug() << "referencePos:" << tokenName  << "text:" << xmlReader.readElementText();
            idealAdj.append(xmlReader.readElementText());
        }
        else if(tokenName == "adjust" )
        {
            //            qDebug() << "adjust:" << tokenName  << "text:" << xmlReader.readElementText();
            idealAdj.append(xmlReader.readElementText());
        }
    }
    idealAdjustmentList.append(idealAdj);
    idealAdj.clear();
}

void Xmlparser::organizeXmlData()
{
    QStringList modelDesc;
    modelDesc.append(name);
    modelDesc.append(desc);
    modelDesc.append(prngSeed);

    //smp actors data
    for(int row = 0; row < actorsList.length() ; ++row)
    {
        for(int col = 0; col < 3 ; ++ col)
        {
            xmlModel->setItem(row,col, new QStandardItem(actorsList.at(row).at(col)));
        }
        if(actorsList.at(0).length()==5)
        {
            xmlModel->setItem(row,3, new QStandardItem(actorsList.at(row).at(3)));
            xmlModel->setItem(row,4, new QStandardItem(actorsList.at(row).at(4)));
        }
        else if(actorsList.at(0).length()==7)
        {
            xmlModel->setItem(row,3, new QStandardItem(actorsList.at(row).at(3)));
            xmlModel->setItem(row,4, new QStandardItem(actorsList.at(row).at(5)));
            xmlModel->setItem(row,5, new QStandardItem(actorsList.at(row).at(4)));
            xmlModel->setItem(row,6, new QStandardItem(actorsList.at(row).at(6)));
        }
        else if(actorsList.at(0).length()==9)
        {
            xmlModel->setItem(row,3, new QStandardItem(actorsList.at(row).at(3)));
            xmlModel->setItem(row,4, new QStandardItem(actorsList.at(row).at(6)));
            xmlModel->setItem(row,5, new QStandardItem(actorsList.at(row).at(4)));
            xmlModel->setItem(row,6, new QStandardItem(actorsList.at(row).at(7)));
            xmlModel->setItem(row,7, new QStandardItem(actorsList.at(row).at(5)));
            xmlModel->setItem(row,8, new QStandardItem(actorsList.at(row).at(8)));
        }
    }

    emit xmlParsedData(modelDesc,modelParametes,dimensionNames,xmlModel,idealAdjustmentList);
}
