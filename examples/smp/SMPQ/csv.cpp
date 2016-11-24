
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

#include "csv.h"

CSV::CSV()
{
    model = new QStandardItemModel;
}
void CSV::readCSVFile(QString path)
{
    QRegExp space("^\\s*$");
    //getting the csv file path
    QStringList scenarioName;
    if(!path.isEmpty())
    {
        // model->clear();

        QFile file(path);
        if (file.open(QIODevice::ReadOnly) && file.size() > 0)
        {
            int rowindex = 0;                     // file row counter
            QTextStream in(&file);                 // read to text stream

            while (!in.atEnd())
            {
                // read one line from textstream(separated by "\n")
                QString fileLine = in.readLine();

                // parse the read line into separate pieces(tokens) with "," as the delimiter
                QStringList lineToken = fileLine.split(",", QString::SkipEmptyParts);
                QString lastField = lineToken.at(lineToken.length()-1);
                if(lastField.contains(space))
                    lineToken.removeLast();


                if(rowindex >= 2 )
                {
                    // load parsed data to model accordingly
                    for (int j = 0; j < lineToken.size(); j++)
                    {
                        QString value = lineToken.at(j);
                        QStandardItem *item = new QStandardItem(value.trimmed());
                        model->setItem(rowindex-2, j, item);
                    }
                }
                else if(rowindex == 1)
                {
                    model->setHorizontalHeaderLabels(lineToken);
                }
                else if(rowindex == 0)
                {
                    scenarioName.append(lineToken.at(0));
                    scenarioName.append(lineToken.at(1));
                    lineToken.clear();
                }
                rowindex++;
            }

            file.close();
        }
        else
        {
            emit sendMessage("CSV","Empty File !");
            return;
        }
    }
    emit csvModel(model,scenarioName);
}
// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
