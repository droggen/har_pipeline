#include <QFileInfo>
#include <QProcess>
#include <QDir>
#include "helper.h"

/*
 * Check if fileName is in the current directory or anywhere in the path
*/
bool isFileInPath(QString fileName)
{
    // Add extension under windows
    fileName = addFileExtensionWindows(fileName);

    // Check current directory
    if (QFileInfo::exists(fileName) && QFileInfo(fileName).isFile())
    {
        //printf("Exists in current dir\n");
        return true;
    }

    // Check path
    QProcessEnvironment pe=QProcessEnvironment::systemEnvironment();
    if(pe.contains("PATH"))
    {

        //printf("there is path\n");
        QString s = pe.value("PATH");
        //printf("%s\n",s.toStdString().c_str());

        QStringList s2 = s.split(";");
        foreach(QString s3,s2)
        {
            //printf("\t%s\n",s3.toStdString().c_str());

            QString full = QDir(s3).filePath(fileName);
            //printf("full: %s\n",full.toStdString().c_str());

            if (QFileInfo::exists(full) && QFileInfo(full).isFile())
            {
                //printf("Exists in %s\n",full.toStdString().c_str());
                return true;
            }
        }

    }
    return false;
}

QString addFileExtensionWindows(QString fileName)
{
#if defined(Q_OS_WIN)
    // Under windows check if the file ends with .exe; if not, add this.
    if(!fileName.endsWith(".exe",Qt::CaseInsensitive))
        fileName=fileName+".exe";
#endif
    return fileName;
}
