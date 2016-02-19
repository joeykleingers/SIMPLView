/* ============================================================================
* Copyright (c) 2009-2015 BlueQuartz Software, LLC
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* Redistributions of source code must retain the above copyright notice, this
* list of conditions and the following disclaimer.
*
* Redistributions in binary form must reproduce the above copyright notice, this
* list of conditions and the following disclaimer in the documentation and/or
* other materials provided with the distribution.
*
* Neither the name of BlueQuartz Software, the US Air Force, nor the names of its
* contributors may be used to endorse or promote products derived from this software
* without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* The code contained herein was partially funded by the followig contracts:
*    United States Air Force Prime Contract FA8650-07-D-5800
*    United States Air Force Prime Contract FA8650-10-D-5210
*    United States Prime Contract Navy N00173-07-C-2068
*
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


#ifndef _pmdirgenerator_h_
#define _pmdirgenerator_h_

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtWidgets/QTreeWidgetItem>

/*
 *
 */
class PMDirGenerator : public QObject
{
    Q_OBJECT

  public:

    PMDirGenerator(QString outputDir, QString pathTemplate,
                   QString dirName,
                   QString codeTemplateResourcePath,
                   QTreeWidgetItem* wi,
                   QObject* parent = 0);


    virtual ~PMDirGenerator();

    void setNameChangeable(bool v);
    bool isNameChangeable();

    void setDoesGenerateOutput(bool v);
    bool doesGenerateOutput();

    void setOutputDir(QString v);
    QString getOutputDir();

    QString getPathTemplate();
    QString getDirName();
    QString getCodeTemplateResourcePath();

    QTreeWidgetItem* getTreeWidgetItem();

    QString getPluginName();
    QString getFilterName();
    void setFilterName(const QString &filterName);
    void setDisplaySuffix(QString v);
    QString getDisplaySuffix();
    void setPluginName(QString pluginName);

    QString cleanName(QString name);

    virtual QString generateFileContents(QString replaceStr = "");

  protected:
    QString                                                   m_FilterName;

  protected slots:
    virtual void pluginNameChanged (const QString& plugname);
    virtual void outputDirChanged (const QString& outputDir);
    virtual void generateOutput();

  signals:
    void outputError(const QString& message);
    void filterSourceError(const QString& message);


  private:
    QString                                                   m_OutputDir;
    QString                                                   m_PathTemplate;
    QString                                                   m_DirName;
    QString                                                   m_CodeTemplateResourcePath;
    bool                                                      m_NameChangeable;
    bool                                                      m_DoesGenerateOutput;
    QTreeWidgetItem*                                          m_TreeWidgetItem;
    QString                                                   m_PluginName;
    QString                                                   m_DisplaySuffix;

};

#endif /* PMDIRTWI_H_ */

