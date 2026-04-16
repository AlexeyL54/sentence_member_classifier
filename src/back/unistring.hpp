#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

using std::string;
using std::vector;

namespace utf8 {

/**
 * @brief Класс для работы с UTF-8 строками.
 *
 * Предоставляет функционал для корректной обработки многобайтовых символов
 * UTF-8, включая получение длины в символах, доступ по индексу, поиск подстрок
 * и приведение к нижнему регистру.
 */
class Unistring {
public:
  /**
   * @brief Конструктор по умолчанию.
   *
   * Инициализирует пустую строку.
   */
  Unistring();

  /**
   * @brief Конструктор из std::string.
   * @param str Исходная строка в формате std::string (предположительно UTF-8).
   */
  Unistring(const string str);

  /**
   * @brief Конструктор из C-строки.
   * @param str Исходная C-строка (предположительно UTF-8).
   */
  Unistring(const char *str);

  /**
   * @brief Преобразует объект Unistring обратно в std::string.
   * @return Копия внутренней строки в формате std::string.
   */
  string to_string() const;

  /**
   * @brief Преобразует все символы строки в нижний регистр.
   *
   * Поддерживает базовые кириллические символы (А-Я, Ё, І, Є) и ASCII.
   * @return Новый объект Unistring со строчными буквами.
   */
  Unistring to_lower();

  /**
   * @brief Возвращает количество символов (кодовых точек) в строке.
   * @return Длина строки в символах UTF-8.
   */
  size_t length() const;

  /**
   * @brief Разбить строку на вектор подстрок по разделителю.
   * @param splitter Строка-разделитель.
   * @return Вектор подстрок.
   * @todo Реализовать метод split.
   */
  // TODO: vector<Unistring> split(const Unistring &splitter);

  /**
   * @brief Найти индекс первого вхождения подстроки.
   *
   * Использует алгоритм Кнута-Морриса-Пратта для поиска.
   * @param substr Искомая подстрока.
   * @return Индекс начала подстроки в текущей строке, если найдена; иначе
   * SIZE_MAX.
   */
  size_t find(const Unistring &substr);

  /**
   * @brief Найти индекс первого вхождения подстроки, начиная с указанного
   * индекса.
   *
   * Использует алгоритм Кнута-Морриса-Пратта для поиска.
   * @param substr Искомая подстрока.
   * @param start_index Индекс символа, с которого начинается поиск.
   * @return Индекс начала подстроки в текущей строке, если найдена; иначе
   * SIZE_MAX.
   */
  size_t find(const Unistring &substr, size_t start_index);

  /**
   * @brief Оператор присваивания другого объекта Unistring.
   * @param right Правый операнд.
   * @return Ссылка на текущий объект.
   */
  Unistring &operator=(const Unistring &right);

  /**
   * @brief Оператор присваивания из std::string.
   * @param right Правый операнд.
   * @return Ссылка на текущий объект.
   */
  Unistring &operator=(const string &right);

  /**
   * @brief Оператор присваивания из C-строки.
   * @param right Правый операнд.
   * @return Ссылка на текущий объект.
   */
  Unistring &operator=(const char *right);

  /**
   * @brief Оператор сложения Unistring с Unistring.
   * @param right Правый операнд.
   * @return Ссылка на текущий объект.
   */
  Unistring operator+(const Unistring &right);

  /**
   * @brief Оператор сложения Unistring со string.
   * @param right Правый операнд.
   * @return Ссылка на текущий объект.
   */
  Unistring operator+(const string &right);

  /**
   * @brief Оператор сложения Unistring с Unistring.
   * @param right Правый операнд.
   * @return Ссылка на текущий объект.
   */
  Unistring &operator+=(const Unistring &right);

  /**
   * @brief Оператор сложения Unistring со string.
   * @param right Правый операнд.
   * @return Ссылка на текущий объект.
   */
  Unistring &operator+=(const string &right);

  /**
   * @brief Оператор доступа к символу по индексу (size_t).
   *
   * Возвращает новый объект Unistring, содержащий один символ по указанному
   * индексу. Если индекс выходит за границы, возвращает пустую строку.
   * @param index Индекс символа.
   * @return Unistring, содержащий один символ.
   */
  Unistring operator[](size_t index) const;

  /**
   * @brief Оператор доступа к символу по индексу (int).
   *
   * Возвращает новый объект Unistring, содержащий один символ по указанному
   * индексу. Поддерживает отрицательные индексы? (В текущей реализации
   * возвращает пустую строку при index < 0). Если индекс выходит за границы,
   * возвращает пустую строку.
   * @param index Индекс символа.
   * @return Unistring, содержащий один символ.
   */
  Unistring operator[](int index) const;

  vector<size_t> get_char_offsets() const;

  Unistring substr(size_t start, size_t end);

private:
  string value; // Внутреннее представление строки в виде байтовой
                // последовательности UTF-8.
  mutable vector<size_t>
      char_offsets;           // Кэш смещений байтов для каждого символа UTF-8.
  mutable bool offsets_dirty; // Флаг, указывающий, что кэш смещений устарел и
                              // требует обновления.

  /**
   * @brief Обновляет кэш смещений символов, если он помечен как грязный.
   *
   * Сканирует внутреннюю строку value и заполняет char_offsets позициями
   * начала каждого символа UTF-8.
   */
  void update_offsets() const;

  /**
   * @brief Вычисляет префикс-функцию для алгоритма Кнута-Морриса-Пратта.
   * @return Вектор значений префикс-функции для текущей строки.
   */
  vector<size_t> compute_prefix_function() const;
};

/**
 * @brief Сравнение двух объектов Unistring на равенство.
 * @param s1 Первый операнд.
 * @param s2 Второй операнд.
 * @return true, если строки идентичны; иначе false.
 */
bool operator==(const Unistring &s1, const Unistring &s2);

/**
 * @brief Сравнение Unistring и std::string на равенство.
 * @param s1 Объект Unistring.
 * @param s2 Объект std::string.
 * @return true, если строки идентичны; иначе false.
 */
bool operator==(const Unistring &s1, const string &s2);

/**
 * @brief Сравнение Unistring и C-строки на равенство.
 * @param s1 Объект Unistring.
 * @param s2 C-строка.
 * @return true, если строки идентичны; иначе false.
 */
bool operator==(const Unistring &s1, const char *s2);

/**
 * @brief Сравнение двух объектов Unistring на неравенство.
 * @param s1 Первый операнд.
 * @param s2 Второй операнд.
 * @return true, если строки различны; иначе false.
 */
bool operator!=(const Unistring &s1, const Unistring &s2);

/**
 * @brief Сравнение Unistring и std::string на неравенство.
 * @param s1 Объект Unistring.
 * @param s2 Объект std::string.
 * @return true, если строки различны; иначе false.
 */
bool operator!=(const Unistring &s1, const string &s2);

/**
 * @brief Сравнение Unistring и C-строки на неравенство.
 * @param s1 Объект Unistring.
 * @param s2 C-строка.
 * @return true, если строки различны; иначе false.
 */
bool operator!=(const Unistring &s1, const char *s2);

/**
 * @brief Определяет количество байт, необходимых для кодирования символа
 * UTF-8, начиная с переданной строки.
 * @param symbol Строка, начинающаяся с искомого символа.
 * @return Количество байт (1-4) или 0, если последовательность некорректна.
 */
uint8_t bytes_to_encode_symbol(const string &symbol);

/**
 * @brief Определяет количество байт, необходимых для кодирования символа
 * UTF-8, по первому байту.
 * @param symbol Первый байт символа UTF-8.
 * @return Количество байт (1-4) или 0, если последовательность некорректна.
 */
uint8_t bytes_to_encode_symbol(const unsigned char symbol);

/**
 * @brief Конвертирует односимвольную строку Unistring в её Unicode code point
 * (int).
 *
 * Внимание: текущая реализация поддерживает только 2-байтовые символы или
 * может работать некорректно для других случаев.
 * @param ch Объект Unistring, содержащий ровно один символ.
 * @return Код символа типа int, или -1, если длина строки больше 1.
 */
int unichar_to_int(const Unistring &ch);

} // namespace utf8
