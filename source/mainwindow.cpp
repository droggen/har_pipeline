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

#include <Qt>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QTabBar>
#include <QCloseEvent>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "cio.h"

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
    //ConsoleInit();            // This console conflicts with the QProcess start of processes -> do not use
#else
    // Hide console
    console->setVisible(false);
    ui->label_console->setVisible(false);
#endif

    // Remove all tabs in uitwProcessTabs
    //ClearProcessTabs();
    processesClear();

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
void MainWindow::setModeRunning()
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
void MainWindow::setModeStopping()
{
    // Deactivate start/load button
    ui->pbStart->setEnabled(false);
    ui->pbLoad->setEnabled(false);
    // Deactivate start/load menu
    ui->action_Load->setEnabled(false);
    ui->action_Start->setEnabled(false);
    // Deactivate stop button
    ui->pbStop->setEnabled(false);
    // Deactivate stop menu
    ui->action_Stop->setEnabled(false);
    // Status bar to idle
    statusLabel->setText("Stopping");
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
        printf("All processes terminated\n\n\n\n");
        setModeIdle();
        // Switch to run tab
        ui->tabWidget->setCurrentIndex(0);
    }
    ui->statusBar->showMessage(QString("Running processes: %1").arg(numrunning),500);
}



/*
 * Runs the pipeline described in processes (QList<PROCESS>)
*/
void MainWindow::Run()
{


    // Iterate all the processes to launch
    for(int i=0;i<processes.size();i++)
    {
        RunProcess(processes[i],i);
    }
}
void MainWindow::RunProcess(PROCESS &process,int i)
{
    printf("Starting process %s\n",process.exe.toStdString().c_str());
    printf("in: %s\n",process.exeandpath.toStdString().c_str());

    // Create the console
    process.console = new WidgetTerminal;

    // Activate the keyboard grab
    process.console->setCaptureKey(true);

    // Connect the signal to something
    /*connect(process.console,&WidgetTerminal::keyPressed,
            this,[=](int k){ slotProcessKeyPress(k,i); }
        );*/
    connect(process.console,&WidgetTerminal::keyPressed,
                this,[=](int k,QString t){ slotProcessKeyPress(k,t,i); }
            );

    // Set the header above the terminal field with the command line parameters
    process.console->setTitle(process.param);

    // Add the console to the tabwidget
    ui->uitwProcessTabs->addTab(process.console,QFileInfo(process.exe).baseName());

    // Create the process and hook it up
    process.process = new QProcess;

    // Connect readyRead - use lambda to pass process number.
    connect(process.process,&QProcess::readyRead,this,[=](){ slotProcessReadyRead(i); } );

    // Connect finished - use lambda to pass process number.
    connect(process.process, (void(QProcess::*)(int _rv,QProcess::ExitStatus _es))&QProcess::finished, this, [=](int _rv,QProcess::ExitStatus _es){ slotProcessFinished(i,_rv,_es); } );

    // Connect error
    connect(process.process,&QProcess::errorOccurred,this,&MainWindow::slotProcessErrorOccurred);
    //connect(&process, &QProcess::finished, this, &MainWindow::processFinished);

    // Combine stdout and stderr
    process.process->setProcessChannelMode(QProcess::MergedChannels);

    // Start: QProcess starts programs which are in the path automatically
    // On windows, the program needs not be terminated by .exe
    //printf("doing start of '%s'\n",processes[i].exe.toStdString().c_str());
    /*
     * har_relay: OK
    har_relay.exe: OK
    C:\bin\hartools-release-2020-01-25\har_relay.exe: ok
    C:\bin\hartools-release-2020-01-25\har_relay: ok
    C:/bin/hartools-release-2020-01-25/har_relay.exe
    C:/bin/hartools-release-2020-01-25/har_relay: ok
    */
    process.process->start(process.exe,process.param_list,QProcess::Unbuffered | QProcess::ReadWrite);



}





void MainWindow::printPipeline()
{
    PIPELINEELEMENT pe;
    print("Current pipeline:");
    int i=0;
    foreach(pe, pipeline)
    {
        printf("%d: '%s' in '%s' param '%s'\n",i++,pe.exe.toStdString().c_str(),pe.exeandpath.toStdString().c_str(),pe.param.toStdString().c_str());
    }
}












void MainWindow::on_action_Load_triggered()
{
    // Load a file and display it in the pipeline window
    QString fileName = QFileDialog::getOpenFileName(this,tr("Open pileline"), "", tr("Pipeline Files (*.bat *.sh *.txt *.pip);;All files (*.*)"));

    // Check if file exists, open, load, display and switch tab to the pipeline view
    if(!fileName.isNull())
    {
        // Clear terminal
        console->clear();
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

    printf("Pipeline: %s\n",fname.toStdString().c_str());

    QString path = QFileInfo(fname).path();

    //printf("Path of pipeline: '%s'\n",path.toStdString().c_str());
    //printf("Current path: %s\n",QDir::currentPath().toStdString().c_str());

    if(!QDir::setCurrent(path))
        printf("Error: cannot change path\n");

    //printf("Changing path to: %s\n",QDir::currentPath().toStdString().c_str());


    // in: input text stream reading from the pipeline
    QTextStream in(&file);

    pipeline.clear();

    while(!in.atEnd())
    {
        QString line = in.readLine();
        //print(line);

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
            // Not a comment nor empty: either valid file or error
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

            //printf("exe before: '%s'\n",pe.exe.toStdString().c_str());
            //printf("param: '%s'\n",pe.param.toStdString().c_str());


            // Add .exe on windows
            //pe.exe = addFileExtensionWindows(pe.exe);
            //printf("exe after: '%s'\n",pe.exe.toStdString().c_str());

            // Check if file is existing and color accordingly
            //if (QFileInfo::exists(pe.exe) && QFileInfo(pe.exe).isFile())
            QString exeandpath;
            if(isFileInPath(pe.exe,exeandpath))
            {
                pe.exeandpath=exeandpath;
                //printf("Found file '%s' in path '%s'\n",pe.exe.toStdString().c_str(),exeandpath.toStdString().c_str());
                ui->textEdit->setTextColor(QColor(0,0,0));
                // Valid: add to pipeline
                pipeline.append(pe);
            }
            else
            {
                ui->textEdit->setTextColor(QColor(255,0,0));
            }

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
    ui->uitwProcessTabs->clear();
}
/*
 * Remove all tabs in the status tab.
 */
/*void MainWindow::ClearProcessTabs()
{
    //ui->uitwProcessTabs->clear();
}*/

void MainWindow::pipelineKill()
{
    // Iterate all the processes to kill them
    for(int i=0;i<processes.size();i++)
    {
        processes[i].process->kill();
    }
    // Delete the terminal and process
    //processesClear();
}


/*
 * Launches the pipeline loaded in pipeline.
 * First converts the PIPELINEELEMENT structure to a PROCESS struct and runs.
*/
void MainWindow::on_action_Start_triggered()
{
    // Only start if there is a pipeline loaded
    if(pipeline.size()==0)
        return;

    // pipelineKill();      // Pipeline should be killed: on start there's no pipeline; and enable start only if all processes are dead.
    // Delete the terminal widgets and QProcess and remove from the tab.
    processesClear();

    // Set mode to "run" - prevents another run, load
    setModeRunning();

    // Run the pipeline.
    PROCESS p;
    PIPELINEELEMENT pe;


    foreach(pe,pipeline)
    {
        p.exe = pe.exe;
        p.exeandpath = pe.exeandpath;
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






void MainWindow::on_action_Stop_triggered()
{
    // Immediately switch the UI to "stopping" to prevent multiple calls...
    setModeStopping();
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

void MainWindow::on_action_About_triggered()
{
    // About box
    QMessageBox::about(this, "About",
       "<p><b>HAR Pipeline</b></p>\n"
       "<p>Version 2020-08-07</p>"
       "<p>(c) 2019-2020 Daniel Roggen</p>");
}

void MainWindow::on_pushButton_clicked()
{
    // Try to send a character to a terminal

    printf("Writing\n");
    processes[0].process->write("2");
}

void MainWindow::slotProcessKeyPress(int k,QString t,int processidx)
{
    //printf("Slot process keypress: process %d key %d text '%s'\n",processidx,k,t.toStdString().c_str());
    QByteArray b = t.toUtf8();
    /*const char *s = t.toStdString().c_str();
    printf("utf8 (%d): ",b.size());
    for(int i=0;i<b.size();i++)
        printf("%d ",(int)b[i]);
    printf("\n");
    printf("c_str (%d): ",strlen(s));
    for(int i=0;i<strlen(s);i++)
        printf("%d ",s[i]);
    printf("\n");*/



    // Send the key to the process
    /*if(k>=0x01000000)
    {
        //if(k==Qt::Key_Escape)
          //  processes[processidx].process->write();
        if(k==Qt::Key_Enter || k==Qt::Key_Return)
        {
            printf("writing enter\n");
            QByteArray e;
            e.append(13);
            e.append(10);
            processes[processidx].process->write(e);
        }

    }
    else
        processes[processidx].process->write(t.toUtf8());*/

    // Must send a CR/LF if enter is sent, otherwise the process reading blocks
    QByteArray ba;
    for(int i=0;i<b.size();i++)
    {
        ba.append(b[i]);
        if(((int)b[i])==13)
            ba.append(10);
    }
    processes[processidx].process->write(ba);


}



