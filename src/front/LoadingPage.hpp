#ifndef LOADINGPAGE_HPP
#define LOADINGPAGE_HPP

#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QWidget>

class LoadingPage : public QWidget {
  Q_OBJECT

public:
  explicit LoadingPage(QWidget *parent = nullptr);

  /** @brief Установить общее количество элементов для обработки */
  void setTotal(int total);

  /** @brief Обновить прогресс обработки */
  void setProgress(int current);

  /** @brief Сбросить прогресс */
  void reset();

private:
  void setupUI();

  QVBoxLayout *mainLayout = nullptr;
  QLabel *titleLabel = nullptr;
  QLabel *statusLabel = nullptr;
  QProgressBar *progressBar = nullptr;
};

#endif // LOADINGPAGE_HPP
