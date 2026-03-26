#ifndef INPUTPAGE_HPP
#define INPUTPAGE_HPP

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

class InputPage : public QWidget {
  Q_OBJECT

public:
  explicit InputPage(QWidget *parent = nullptr);

signals:
  // Сигнал для перехода на страницу результатов
  void analyzeRequested();

private:
  // Методы построения интерфейса (вызываются из конструктора)
  void setupMainLayout(); // Главный вертикальный layout окна
  void setupContentRow(); // Горизонтальная строка для контента (колонки)
  void
  setupContentColumn();  // Колонка контента с отступами и вертикальным layout
  void setupIntroText(); // Поясняющий текст вверху страницы
  void
  setupInputChoice(); // Радиокнопки «С клавиатуры» / «С файла» и стек страниц
  void
  setupKeyboardPage(); // Страница: поле ввода текста + кнопка «Анализировать»
  void
  setupFilePage(); // Страница: путь к файлу + «Выбрать файл» + «Анализировать»
  void setupConnections(); // Сигналы: выбор файла, переключение страниц по
                           // радиокнопкам

  // Layout-ы и контейнеры
  QVBoxLayout *mainLayout = nullptr;
  QHBoxLayout *contentRow = nullptr;
  QWidget *contentColumn = nullptr;
  QVBoxLayout *contentLayout = nullptr;

  // Ввод: пояснение и выбор способа
  QLabel *introText = nullptr;
  QButtonGroup *inputChoiceGroup = nullptr;
  QRadioButton *radioKeyboard = nullptr;
  QRadioButton *radioFile = nullptr;

  // Стек страниц (клавиатура / файл)
  QStackedWidget *stack = nullptr;
  QWidget *pageKeyboard = nullptr;
  QWidget *pageFile = nullptr;

  // Страница «С клавиатуры»
  QPlainTextEdit *textInput = nullptr;
  QPushButton *btnAnalyzeKeyboard = nullptr;

  // Страница «С файла»
  QLineEdit *filePathEdit = nullptr;
  QPushButton *btnSelectFile = nullptr;
  QPushButton *btnAnalyzeFile = nullptr;
};

#endif // INPUTPAGE_HPP
