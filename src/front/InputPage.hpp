#ifndef INPUTPAGE_HPP
#define INPUTPAGE_HPP

#include <string>

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QFrame>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QToolButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

class InputPage : public QWidget
{
    Q_OBJECT

public:
    explicit InputPage(QWidget *parent = nullptr);

signals:
    /** @brief Текст для анализа в UTF-8 (клавиатура или содержимое файла). */
    void analysisRequested(const std::string &text);

private slots:
    void onAnalyzeFromKeyboard();
    void onAnalyzeFromFile();

private:
    // Методы построения интерфейса (вызываются из конструктора)
    void setupMainLayout();    // Главный вертикальный layout окна
    void setupContentRow();    // Горизонтальная строка для контента (колонки)
    void setupContentColumn(); // Колонка контента с отступами и вертикальным layout
    void setupIntroText();     // Поясняющий текст вверху страницы
    void setupInputChoice();   // Радиокнопки «С клавиатуры» / «С файла» и стек страниц
    void setupKeyboardPage();  // Страница: поле ввода текста + кнопка «Анализировать»
    void setupFilePage();      // Страница: путь к файлу + «Выбрать файл» + «Анализировать»
    void setupConnections();   // Сигналы: выбор файла, переключение страниц по радиокнопкам

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
    QToolButton *btnClearKeyboard = nullptr;
    QPushButton *btnAnalyzeKeyboard = nullptr;

    // Страница «С файла»
    QLineEdit *filePathEdit = nullptr;
    QToolButton *btnClearFile = nullptr;
    QPushButton *btnSelectFile = nullptr;
    QPushButton *btnAnalyzeFile = nullptr;
};

#endif // INPUTPAGE_HPP
