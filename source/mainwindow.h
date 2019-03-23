#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QLabel>
#include <QSharedPointer>
#include "widgetterminal.h"

namespace Ui {
class MainWindow;
}

typedef struct
{
    QString exe;
    QString param;
    QStringList param_list;     // same as param but split in list
    QProcess *process;
    WidgetTerminal *console;
} PROCESS;

typedef struct
{
    QString exe;
    QString param;
} PIPELINEELEMENT;


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private:
    //QProcess process;
    WidgetTerminal *console;
    QLabel *statusLabel;

    QList <PIPELINEELEMENT> pipeline;
    QList<PROCESS> processes;

    void loadPipeline(QString fname);
    void printPipeline();
    void pipelineKill();
    void processesClear();

    void print(QString str);
    int printf(const char *format, ...);
    void Run();
    void ClearProcessTabs();



    void setModeRun();
    void setModeIdle();

    void closeEvent(QCloseEvent *event);

private slots:
    void slotProcessReadyRead(int processidx);
    void slotProcessErrorOccurred(QProcess::ProcessError error);
    void slotProcessFinished(int processidx, int exitCode,QProcess::ExitStatus exitStatus);

    void toto(int i);



    void on_action_Load_triggered();
    void on_action_Start_triggered();
    void on_action_Stop_triggered();

    void on_action_Quit_triggered();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
