#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>

#include "InputPage.hpp"
#include "ResultPage.hpp"

class MainWindow : public QWidget {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();
};

#endif // MAINWINDOW_H
