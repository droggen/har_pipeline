/*
https://stackoverflow.com/questions/34245235/qt-how-to-redirect-qprocess-stdout-to-textedit
*/
/*
    - Load pipeline
        Read a batch/shell file
        Display all in the pipeline window
        Any line corresponding to an executable is executed
        Any other line is skipped (comment)


    - When process terminate change color of title
        red - error
        blue - ok

    - When process gets data change title boldness
        - Bold when new data
        - Normal if switch to tab, or if tab that got data is the visible one


    - Change reception of data to lamda sending the id of the process


    V - Disable load, exec function when running
    V - Reenable load, exec after running

    - Optional: set tab header to bold
    - TODO: pass parameters to qprocess

    v TODO: When quitting stop all processes
    v TODO: icon
    v TODO: window title
    v TODO: delete widgets created when adding pipeline
    TODO: deploy
    TODO: github




*/


#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QTabBar>
#include "mainwindow.h"
#include "ui_mainwindow.h"


#include "helper.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Always create the console, but show only in DEVELMODE
    console = new WidgetTerminal(this);
    console->setTitle("Console");
    ui->verticalLayout->addWidget(console);


#ifdef DEVELMODE
    print("Starting up\n");
#else
    // Hide console
    console->setVisible(false);
    ui->label_console->setVisible(false);
#endif

    // Remove all tabs in uitwProcessTabs
    ClearProcessTabs();

    //
    statusBar()->addWidget(statusLabel = new QLabel);

    //statusBar()
    statusLabel->setText("Hello");

    //  Go to idle mode
    setModeIdle();


}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::print(QString str)
{
#ifdef DEVELMODE
    console->addString(str);
#endif
}

/*
 * printf equivalent writing to the debug console
*/
int MainWindow::printf(const char *format, ...)
{
    va_list arg;
    int done;
    char buffer[16384];

    va_start(arg, format);
    done = vsnprintf(buffer,16384, format, arg);
    va_end(arg);

    print(buffer);

    return done;
}



/*
 * Set UI in RUN mode: don't allow load, don't allow run
*/
void MainWindow::setModeRun()
{
    // Deactivate run/load button
    ui->pbStart->setEnabled(false);
    ui->pbLoad->setEnabled(false);
    // Deactivate run/load menu entry
    ui->action_Start->setEnabled(false);
    ui->action_Load->setEnabled(false);
    // Activate stop button
    ui->pbStop->setEnabled(true);
    // Activate stop menu entry
    ui->action_Stop->setEnabled(true);
    // Status bar to run
    statusLabel->setText("Running");
}
/*
 * Set UI in IDLE mode: allow load, allow run
*/
void MainWindow::setModeIdle()
{
    // Activate start/load button
    ui->pbStart->setEnabled(true);
    ui->pbLoad->setEnabled(true);
    // Activate start/load menu
    ui->action_Load->setEnabled(true);
    ui->action_Start->setEnabled(true);
    // Deactivate stop button
    ui->pbStop->setEnabled(false);
    // Deactivate stop menu
    ui->action_Stop->setEnabled(false);
    // Status bar to idle
    statusLabel->setText("Idle");
}



/*
    Called when process processidx has received data
*/
void MainWindow::slotProcessReadyRead(int processidx)
{
    //printf("slotProcessReadyRead3: process %d\n",processidx);

    // Test isOpen may be superfluous
    if(processes[processidx].process->isOpen())
    {
        QByteArray s = processes[processidx].process->readAll();
        //print(QString("Read %1 from %2").arg(s.size()).arg(processidx));

        processes[processidx].console->addBytes(s);
    }

}
void MainWindow::slotProcessErrorOccurred(QProcess::ProcessError error)
{
    printf("Error occured: %d\n",error);
}

void MainWindow::toto(int i)
{
    printf("toto: i=%d\n",i);
}

/*
 * Called everytime a process terminates, successfully or unsuccessfully.
 * Check if all processes terminated: if yes, go back to idle.
 *
*/
void MainWindow::slotProcessFinished(int i,int rv,QProcess::ExitStatus es)
{
    printf("slotProcessFinished: i=%d rv=%d exitstatus=%d\n",i,rv,es);

    // Set the tab color according to return value
    if(rv)
        ui->uitwProcessTabs->tabBar()->setTabTextColor(i,Qt::red);
    else
        ui->uitwProcessTabs->tabBar()->setTabTextColor(i,Qt::blue);


    // Print status of each process
    /*for(int i=0;i<processes.size();i++)
    {
        //printf("Process %d at end: %d\n",i,(int)processes[i].process->atEnd());
        printf("Process %d state: ",i);
        switch(processes[i].process->state())
        {
            case QProcess::NotRunning:
                printf("NotRunning\n");
                break;
            case QProcess::Starting:
                printf("Starting\n");
                break;
            case QProcess::Running:
                printf("Running\n");
                break;
            default:
                printf("Other\n");
        }
    }*/

    // Check if all process finished
    bool allfinished=true;
    int numrunning=0;
    for(int i=0;i<processes.size();i++)
    {
        if(processes[i].process->state()!=QProcess::NotRunning)
        {
            allfinished=false;
            numrunning++;
        }

    }
    // If all finished go back to idle
    if(allfinished)
    {
        setModeIdle();
        // Switch to run tab
        ui->tabWidget->setCurrentIndex(0);
    }
    ui->statusBar->showMessage(QString("Running processes: %1").arg(numrunning),500);
}

/*
 * Remove all tabs in the status tab.
 */
void MainWindow::ClearProcessTabs()
{
    ui->uitwProcessTabs->clear();
}


/*
 * Runs the pipeline described in processes (QList<PROCESS>)
*/
void MainWindow::Run()
{
    // Clear all process tabs
    ClearProcessTabs();


    // Iterate all the processes to launch
    for(int i=0;i<processes.size();i++)
    {
        printf("Starting process %s\n",processes[i].exe.toStdString().c_str());

        // Create the console
        processes[i].console = new WidgetTerminal;

        // Set the header above the terminal field with the command line parameters
        processes[i].console->setTitle(processes[i].param);

        // Add the console to the tabwidget
        ui->uitwProcessTabs->addTab(processes[i].console,QFileInfo(processes[i].exe).baseName());

        // Create the process and hook it up
        processes[i].process = new QProcess;

        // Connect readyRead - use lambda to pass process number.
        connect(processes[i].process,&QProcess::readyRead,this,[=](){ slotProcessReadyRead(i); } );

        // Connect finished - use lambda to pass process number.
        connect(processes[i].process, (void(QProcess::*)(int _rv,QProcess::ExitStatus _es))&QProcess::finished, this, [=](int _rv,QProcess::ExitStatus _es){ slotProcessFinished(i,_rv,_es); } );

        // Connect error
        connect(processes[i].process,&QProcess::errorOccurred,this,&MainWindow::slotProcessErrorOccurred);
        //connect(&process, &QProcess::finished, this, &MainWindow::processFinished);

        // Combine stdout and stderr
        processes[i].process->setProcessChannelMode(QProcess::MergedChannels);

        // Start
        //processes[i].process->start(processes[i].exe,QProcess::Unbuffered | QProcess::ReadWrite);
        processes[i].process->start(processes[i].exe,processes[i].param_list,QProcess::Unbuffered | QProcess::ReadWrite);
        //QProcess::Unbuffered


    }
}





void MainWindow::printPipeline()
{
    PIPELINEELEMENT pe;
    print("Current pipeline:");
    foreach(pe, pipeline)
    {
        printf("Exe: '%s' param: '%s'\n",pe.exe.toStdString().c_str(),pe.param.toStdString().c_str());
    }
}












void MainWindow::on_action_Load_triggered()
{
    // Load a file and display it in the pipeline window
    QString fileName = QFileDialog::getOpenFileName(this,tr("Open pileline"), "", tr("Pipeline Files (*.bat *.sh *.txt *.pip);;All files (*.*)"));

    // Check if file exists, open, load, display and switch tab to the pipeline view
    if(!fileName.isNull())
    {
          loadPipeline(fileName);
          ui->tabWidget->setCurrentIndex(0);
    }
}

/*
 * Load a pipeline and shows it in the pipeline window.
 * Processes the lines to understand if comment, executable, or error (executable not found)
 * If executable, splits the exe from the parameters into a QList of PIPELINEELEMENT
*/
void MainWindow::loadPipeline(QString fname)
{
    // Load pipeline
    QFile file(fname);
    if(!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::information(this, "Error", file.errorString());
        return;
    }

    // Clear the text edit showing the pipeline
    ui->textEdit->clear();

    printf("file to open: %s\n",fname.toStdString().c_str());

    QString path = QFileInfo(fname).path();

    printf("path: %s\n",path.toStdString().c_str());
    printf("Current path: %s\n",QDir::currentPath().toStdString().c_str());

    if(!QDir::setCurrent(path))
        printf("Can't change path\n");

    printf("Current path: %s\n",QDir::currentPath().toStdString().c_str());



    QTextStream in(&file);

    pipeline.clear();

    while(!in.atEnd())
    {
        QString line = in.readLine();
        print(line);

        // Trim line
        line = line.trimmed();

        // Check if line is a comment or empty
        if(line.startsWith("#") || line.startsWith("rem",Qt::CaseInsensitive) || line.isEmpty())
        {
            // Comment
            ui->textEdit->setTextColor(QColor(0,128,0));
        }
        else
        {
            // Not a comment nor empty - valid file or error
            // Split executable from parameters - find first space
            // (TODO: fix if path has space)
            int sp = line.indexOf(" ");
            PIPELINEELEMENT pe;
            if(sp != -1)
            {
                // Found a space - split at first space
                pe.exe = line.left(sp);
                pe.param = line;
                pe.param.remove(0,sp+1);      // +1 to remove the space as well
            }
            else
            {
                pe.exe = line;
                pe.param = "";
            }

            printf("exe before: '%s'\n",pe.exe.toStdString().c_str());
            printf("param: '%s'\n",pe.param.toStdString().c_str());


            // Add .exe on windows
            pe.exe = addFileExtensionWindows(pe.exe);
            printf("exe after: '%s'\n",pe.exe.toStdString().c_str());

            // Check if file is existing and color accordingly
            //if (QFileInfo::exists(pe.exe) && QFileInfo(pe.exe).isFile())
            if(isFileInPath(pe.exe))
            {
                ui->textEdit->setTextColor(QColor(0,0,0));
                // Valid: add to pipeline
                pipeline.append(pe);
            }
            else
            {
                ui->textEdit->setTextColor(QColor(255,0,0));
            }

            //
            //printf("Checking if %s exists\n",pe.exe.toStdString().c_str());
            //bool e = isFileInPath(pe.exe);
            //printf("e: %d\n",(int)e);
        }
        ui->textEdit->append(line);
    }

    file.close();

    // Print pipeline
    printPipeline();

}


/*
 *  Clear the WidgetTerminal and QProcess in processes
*/
void MainWindow::processesClear()
{
    for(int i=0;i<processes.size();i++)
    {
        delete processes[i].console;
        delete processes[i].process;
    }
    processes.clear();
}

/*
 * Launches the pipeline loaded in pipeline.
 * First converts the PIPELINEELEMENT structure to a PROCESS struct and runs.
*/
void MainWindow::on_action_Start_triggered()
{



    // Set mode to "run" - don't allow run, don't allow load
    setModeRun();

    // Run the pipeline.
    PROCESS p;
    PIPELINEELEMENT pe;

    processesClear();
    foreach(pe,pipeline)
    {
        p.exe = pe.exe;
        p.param = pe.param;

        // Split the param as QStringList - if no parameters split returns one element so handle this special case
        if(!p.param.isEmpty())
            p.param_list = p.param.split(" ");
        else
            p.param_list.clear();

        //printf("pbstart exe: '%s'\n",p.exe.toStdString().c_str());
        //printf("pbstart param '%s' - num: %d\n",p.param.toStdString().c_str(),p.param_list.size());

        processes.append(p);
    }

    Run();

    // Switch to run tab
    ui->tabWidget->setCurrentIndex(1);
}


void MainWindow::pipelineKill()
{
    // Iterate all the processes to kill
    for(int i=0;i<processes.size();i++)
    {
        processes[i].process->kill();
    }
}



void MainWindow::on_action_Stop_triggered()
{
    pipelineKill();
}



void MainWindow::closeEvent(QCloseEvent *event)
{
    // Kill all items in the pipeline
    pipelineKill();

    event->accept();
}

void MainWindow::on_action_Quit_triggered()
{
    close();
}
