#ifndef SEARCHPAGE_HPP
#define SEARCHPAGE_HPP

#include <QObject>

#include <QComboBox>
#include <QFrame>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QString>
#include <QStandardItemModel>
#include <QVector>
#include <QWidget>
#include <QVBoxLayout>

// TODO: предложения и номера сделать массивами
struct SearchResultItem
{
    QString word;
    // Тип члена предложения (подлежащее/сказуемое/...)
    QString member;
    // Номер предложения в исходном тексте
    int sentenceNo = 0;
    // Текст предложения (для отладки/контекста)
    QString sentenceText;
    // Сколько раз слово встретилось в тексте (для сортировки)
    int count = 0;
};

class SearchResultsList : public QWidget
{
public:
    explicit SearchResultsList(QWidget* parent = nullptr);

    void setItems(const QVector<SearchResultItem>& items);

private:
    void clearLayout();

    QWidget* container_ = nullptr;
    QVBoxLayout* layout_ = nullptr;
};

class SearchPage : public QWidget
{
    Q_OBJECT
public:
    // Создаёт страницу поиска и принимает исходный набор данных.
    explicit SearchPage(const QVector<SearchResultItem>& items, QWidget* parent = nullptr);

signals:
    // Нажатие на кнопку "Назад".
    void backRequested();

protected:
    // Нужен для обработки кликов в выпадающем списке с мультивыбором.
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    // Фильтрация + сортировка данных и обновление списка карточек.
    void filterAndRender(const QString& text);
    // Считывает выбранные пункты из чекбоксов "члены предложения".
    void updateSelectedMembers();
    // Обновляет текст в поле combo по выбранным пунктам.
    void updateMemberComboSummary();
    // Применяет текущие параметры поиска/фильтра/сортировки.
    void applyCurrentFilters();

private slots:
    void onSearchTextChanged(const QString& text);
    void onMemberFilterChanged();
    void onSortModeChanged(int index);

private:
    QVector<SearchResultItem> allItems_;
    SearchResultsList* resultsList_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;

    QComboBox* memberFilterCombo_ = nullptr;
    QComboBox* sortCombo_ = nullptr;
    QStandardItemModel* memberModel_ = nullptr;
    QVector<QString> selectedMembers_;
    int sortModeIndex_ = 0;
};

#endif // SEARCHPAGE_HPP

