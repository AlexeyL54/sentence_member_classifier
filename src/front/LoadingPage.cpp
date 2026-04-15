#include "LoadingPage.hpp"
#include "qnamespace.h"

LoadingPage::LoadingPage(QWidget *parent) : QWidget(parent) { setupUI(); }

void LoadingPage::setupUI() {
  mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(40, 40, 40, 40);
  mainLayout->setSpacing(20);
  // mainLayout->addStretch(1);

  // Заголовок
  titleLabel = new QLabel("Обработка текста", this);
  titleLabel->setAlignment(Qt::AlignCenter);
  QFont titleFont = titleLabel->font();
  titleFont.setPointSize(18);
  titleFont.setBold(true);
  titleLabel->setFont(titleFont);
  mainLayout->addWidget(titleLabel);

  // Статус
  statusLabel = new QLabel("Подготовка к анализу...", this);
  statusLabel->setAlignment(Qt::AlignCenter);
  mainLayout->addWidget(statusLabel);

  // Прогресс-бар
  progressBar = new QProgressBar(this);
  progressBar->setRange(0, 100);
  progressBar->setValue(0);
  progressBar->setTextVisible(true);
  progressBar->setFormat("%p%");
  mainLayout->addWidget(progressBar);

  // Добавляем растягиватели для центрирования контента
  mainLayout->setAlignment(Qt::AlignCenter);
  // mainLayout->addStretch(1);
}

void LoadingPage::setTotal(int total) {
  if (total > 0) {
    progressBar->setRange(0, total);
  } else {
    progressBar->setRange(0, 100);
  }
}

void LoadingPage::setProgress(int current) {
  progressBar->setValue(current);

  if (progressBar->maximum() > 0) {
    int percent = (current * 100) / progressBar->maximum();
    statusLabel->setText(QString("Обработано предложений: %1 из %2")
                             .arg(current)
                             .arg(progressBar->maximum()));
    // .arg(percent));
  } else {
    statusLabel->setText(QString("Обработано предложений: %1").arg(current));
  }
}

void LoadingPage::reset() {
  progressBar->setValue(0);
  statusLabel->setText("Подготовка к анализу...");
}
