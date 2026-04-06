#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QStackedWidget>
#include <QWidget>
#include <memory>
#include <vector>

#include "InputPage.hpp"
#include "ResultPage.hpp"
#include "SearchPage.hpp"

class MainWindow : public QWidget {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  // Слоты для навигации между страницами
  void onAnalyzeRequested(const std::string &text);
  void onSearchRequested();
  void onBackToResultRequested();
  void onNewAnalysisRequested();

private:
  std::unique_ptr<BertOnnxInference> inferer;
  std::vector<SentenceResult> results;
  void setupUI();
  void setupConnections();
  void createSearchItems(); // Создаёт элементы для поиска из готовых файлов

  QStackedWidget *stackedWidget = nullptr;
  InputPage *inputPage = nullptr;
  ResultPage *resultPage = nullptr;
  SearchPage *searchPage = nullptr;

  // Данные, передаваемые между страницами
  QVector<SearchItem> searchItems_;
};

#endif // MAINWINDOW_H
