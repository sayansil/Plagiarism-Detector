#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();

    void on_actionProject_triggered();

    void on_actionDeveloper_triggered();

    void on_actionExit_triggered();

    void on_actionNew_File_triggered();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
