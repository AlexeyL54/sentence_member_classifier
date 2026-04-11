#ifndef SEARCHPAGE_HPP
#define SEARCHPAGE_HPP

#include <vector>

#include <QObject>

#include <QComboBox>
#include <QFrame>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QStandardItemModel>
#include <QString>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>

#include <QtGlobal>

#include "../back/statistics.hpp"

class SearchResultsList : public QWidget {
public:
  explicit SearchResultsList(QWidget *parent = nullptr);

  void setItems(const std::vector<SearchItem> &items);

private:
  void clearLayout();

  QWidget *container_ = nullptr;
  QVBoxLayout *layout_ = nullptr;
};

class SearchPage : public QWidget {
  Q_OBJECT
public:
  explicit SearchPage(QWidget *parent = nullptr);

  void setSearchItems(std::vector<SearchItem> &items);

signals:
  void backRequested();

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  void filterAndRender(const QString &text);
  void updateSelectedMembers();
  void updateMemberComboSummary();
  void applyCurrentFilters();

private slots:
  void onSearchTextChanged(const QString &text);
  void onMemberFilterChanged();
  void onSortModeChanged(int index);

private:
  std::vector<SearchItem> allItems_;
  SearchResultsList *resultsList_ = nullptr;
  QLineEdit *searchEdit_ = nullptr;

  QComboBox *memberFilterCombo_ = nullptr;
  QComboBox *sortCombo_ = nullptr;
  QStandardItemModel *memberModel_ = nullptr;
  QVector<QString> selectedMembers_;
  qint64 memberBlockShowPopupUntilMs_ = 0;
  bool memberSkipBlockOnNextHide_ = false;
  bool pendingMemberFilterApply_ = false;
  static constexpr int kMemberPopupBlockMs = 40;
  int sortModeIndex_ = 0;
};

#endif // SEARCHPAGE_HPP
