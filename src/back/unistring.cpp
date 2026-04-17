#include "unistring.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>

using std::string;

using utf8::Unistring;

Unistring::Unistring() : offsets_dirty(true) { update_offsets(); };

Unistring::Unistring(const string s) : value(s), offsets_dirty(true) {
  update_offsets();
};

Unistring::Unistring(const char *s) : value(s), offsets_dirty(true) {
  update_offsets();
};

string Unistring::to_string() const { return value; }

Unistring &Unistring::operator=(const Unistring &right) {
  if (this == &right) {
    return *this;
  }
  value = right.value;
  update_offsets();
  return *this;
}

Unistring &Unistring::operator=(const string &right) {
  value = right;
  offsets_dirty = true;
  update_offsets();
  return *this;
}

Unistring &Unistring::operator=(const char *right) {
  value = right;
  offsets_dirty = true;
  update_offsets();
  return *this;
}

Unistring Unistring::operator+(const Unistring &right) {
  Unistring result(*this);
  result.value += right.value;
  result.offsets_dirty = true;
  result.update_offsets();
  return result;
}

Unistring Unistring::operator+(const string &right) {
  Unistring uniright(right);
  return (*this) + uniright;
}

/**
 * @brief Оператор сложения Unistring с Unistring.
 * @param right Правый операнд.
 * @return Ссылка на текущий объект.
 */
Unistring &Unistring::operator+=(const Unistring &right) {
  value += right.value;
  char_offsets.insert(char_offsets.end(), right.char_offsets.begin(),
                      right.char_offsets.end());
  return *this;
}

/**
 * @brief Оператор сложения Unistring со string.
 * @param right Правый операнд.
 * @return Ссылка на текущий объект.
 */
Unistring &Unistring::operator+=(const string &right) {
  Unistring uniright(right);
  value += uniright.value;
  char_offsets.insert(char_offsets.end(), uniright.char_offsets.begin(),
                      uniright.char_offsets.end());
  return *this;
}

void Unistring::update_offsets() const {
  if (!offsets_dirty)
    return;

  char_offsets.clear();
  size_t len = value.length();
  size_t i = 0;
  uint8_t bytes;
  while (i < len) {
    char_offsets.push_back(i);
    bytes = bytes_to_encode_symbol(value[i]);
    if (bytes == 0)
      bytes = 1;
    i += bytes;
  }
}

Unistring Unistring::operator[](size_t index) const {
  if (index >= char_offsets.size()) {
    return Unistring();
  }

  update_offsets();

  size_t offset = char_offsets[index];
  uint8_t bytes = bytes_to_encode_symbol(value[offset]);
  size_t start_byte = char_offsets[index];
  // Определить длину следующего символа, чтобы знать, сколько байт копировать
  // Можно взять разницу между следующим смещением и текущим,
  // либо вычислить длину текущего символа
  size_t end_byte;
  if (index + 1 < char_offsets.size()) {
    end_byte = char_offsets[index + 1];
  } else {
    end_byte = value.size();
  }

  std::string symbol_str = value.substr(start_byte, end_byte - start_byte);
  return Unistring(symbol_str);
}

Unistring Unistring::operator[](int index) const {
  if (index < 0 or index >= char_offsets.size()) {
    return Unistring();
  }

  update_offsets();

  size_t offset = char_offsets[index];
  uint8_t bytes = bytes_to_encode_symbol(value[offset]);
  size_t start_byte = char_offsets[index];
  // Определить длину следующего символа, чтобы знать, сколько байт копировать
  // Можно взять разницу между следующим смещением и текущим,
  // либо вычислить длину текущего символа
  size_t end_byte;
  if (index + 1 < char_offsets.size()) {
    end_byte = char_offsets[index + 1];
  } else {
    end_byte = value.size();
  }

  std::string symbol_str = value.substr(start_byte, end_byte - start_byte);
  return Unistring(symbol_str);
}

uint8_t utf8::bytes_to_encode_symbol(const string &symbol) {
  const unsigned char ch = static_cast<const unsigned char>(symbol[0]);

  if ((ch & 0b10000000) == 0) { // 0xxxxxxxx
    return 1;
  } else if ((ch & 0b11100000) == 0b11000000) { // 110xxxxx
    return 2;
  } else if ((ch & 0b11110000) == 0b11100000) { // 1110xxxx
    return 3;
  } else if ((ch & 0b11111000) == 0b11110000) { // 11110xxx
    return 4;
  } else {
    return 0;
  }
}

uint8_t utf8::bytes_to_encode_symbol(const unsigned char symbol) {
  if ((symbol & 0b10000000) == 0) { // 0xxxxxxxx
    return 1;
  } else if ((symbol & 0b11100000) == 0b11000000) { // 110xxxxx
    return 2;
  } else if ((symbol & 0b11110000) == 0b11100000) { // 1110xxxx
    return 3;
  } else if ((symbol & 0b11111000) == 0b11110000) { // 11110xxx
    return 4;
  } else {
    return 0;
  }
}

size_t Unistring::length() const { return char_offsets.size(); }

vector<size_t> Unistring::compute_prefix_function() const {
  size_t m = this->length();
  if (m == 0)
    return {};

  vector<size_t> pi(m, 0);
  // длина текущего префикс-суффикса
  size_t k = 0;

  // со второго символа (индекс 1)
  for (size_t q = 1; q < m; ++q) {
    // Пока не совпадает и k > 0, откатываем k
    while (k > 0 && (*this)[k] != (*this)[q]) {
      k = pi[k - 1];
    }

    // если символы совпали, увеличиваем k
    if ((*this)[k] == (*this)[q]) {
      ++k;
    }

    pi[q] = k;
  }
  return pi;
}

size_t Unistring::find(const Unistring &substr) {
  size_t n = this->length();
  size_t m = substr.length();

  // Пустая подстрока находится везде
  if (m == 0)
    return 0;
  // Подстрока длиннее текста
  if (n < m)
    return SIZE_MAX;

  vector<size_t> pi = substr.compute_prefix_function();
  size_t q = 0; // Количество совпавших символов

  for (size_t i = 0; i < n; ++i) {
    while (q > 0 && substr[q] != (*this)[i]) {
      q = pi[q - 1];
    }

    // Если символы совпали, увеличиваем q
    if (substr[q] == (*this)[i]) {
      ++q;
    }

    // Если все символы подстроки совпали
    if (q == m) {
      // Возвращаем индекс начала подстроки в тексте
      return i - m + 1;
    }
  }

  return SIZE_MAX;
}

size_t Unistring::find(const Unistring &substr, size_t start_index) {
  size_t n = this->length();
  size_t m = substr.length();

  // Пустая подстрока находится везде, возвращаем start_index или 0?
  // Обычно для пустой подстроки возвращают сам индекс начала поиска, если он
  // валиден.
  if (m == 0) {
    if (start_index <= n) {
      return start_index;
    }
    return SIZE_MAX;
  }

  if (start_index >= n or n - start_index < m) {
    return SIZE_MAX;
  }

  vector<size_t> pi = substr.compute_prefix_function();
  // Количество совпавших символов в подстроке
  size_t q = 0;

  for (size_t i = start_index; i < n; ++i) {
    while (q > 0 && substr[q] != (*this)[i]) {
      q = pi[q - 1];
    }

    // Если символы совпали, увеличиваем q
    if (substr[q] == (*this)[i]) {
      ++q;
    }

    // Если все символы подстроки совпали
    if (q == m) {
      // Возвращаем индекс начала подстроки в тексте
      return i - m + 1;
    }
  }

  return SIZE_MAX;
}

Unistring Unistring::to_lower() {
  std::string lower_str = value;
  size_t len = value.length();

  for (size_t i = 0; i < len;) {
    unsigned char c1 = static_cast<unsigned char>(value[i]);

    // Если это начало 2-байтового символа UTF-8 (110xxxxx)
    if ((c1 & 0xE0) == 0xC0) {

      // Защита от некорректной UTF-8
      if (i + 1 >= len)
        break;

      unsigned char c2 = static_cast<unsigned char>(value[i + 1]);

      // Специальные случаи (Ё, І, Є)
      if (c1 == 0xD0) {
        if (c2 == 0x81) { // Ё -> ё
          lower_str[i] = 0xD1;
          lower_str[i + 1] = 0x91;
        } else if (c2 == 0x86) { // І -> і
          lower_str[i] = 0xD1;
          lower_str[i + 1] = 0x96;
        } else if (c2 == 0x88) { // Є -> є
          lower_str[i] = 0xD1;
          lower_str[i + 1] = 0x94;
        }
        // А-П (D0 90-9F) -> а-п (D0 B0-BF)
        else if (c2 >= 0x90 && c2 <= 0x9F) {
          lower_str[i] = 0xD0;
          lower_str[i + 1] = c2 + 0x20;
        }
        // Р-Я (D0 A0-AF) -> р-я (D1 80-8F)
        else if (c2 >= 0xA0 && c2 <= 0xAF) {
          lower_str[i] = 0xD1;
          lower_str[i + 1] = c2 - 0x20;
        }
      }

      i += 2;
    } else {
      // TODO: Однобайтовый символ (ASCII) или другая длина UTF-8
      i++;
    }
  }

  return Unistring(lower_str);
}

vector<size_t> Unistring::get_char_offsets() const { return char_offsets; }

Unistring Unistring::substr(size_t start, size_t end) const {
  size_t len = this->length();

  if (start >= len || start > end) {
    return Unistring("");
  }
  if (end >= len) {
    end = len - 1;
  }
  if (this->offsets_dirty)
    update_offsets();

  // Получаем байтовое начало первого символа подстроки
  size_t byte_start = char_offsets[start];

  // байтовое начало символа, следующего за последним символом
  size_t byte_end;
  if (end + 1 < char_offsets.size()) {
    byte_end = char_offsets[end + 1];
  } else {
    byte_end = value.size();
  }

  std::string sub_value = value.substr(byte_start, byte_end - byte_start);
  return Unistring(sub_value);
}

bool utf8::operator==(const Unistring &s1, const Unistring &s2) {
  return s1.to_string() == s2.to_string();
}

bool utf8::operator==(const Unistring &s1, const string &s2) {
  return s1.to_string() == s2;
}

bool utf8::operator==(const Unistring &s1, const char *s2) {
  return s1.to_string() == s2;
}

bool utf8::operator!=(const Unistring &s1, const Unistring &s2) {
  return s1.to_string() != s2.to_string();
}

bool utf8::operator!=(const Unistring &s1, const string &s2) {
  return s1.to_string() != s2;
}

bool utf8::operator!=(const Unistring &s1, const char *s2) {
  return s1.to_string() != s2;
}

// TODO: поддержка многобайтовых символов
/*
 * @brief Конвертировать символ строки Unistring в int
 * @param ch символ
 * @return код типа int, -1 в случае если ch является подстрокой
 */
int utf8::unichar_to_int(const Unistring &ch) {
  if (ch.length() > 1) {
    return -1;
  }

  unsigned char first_byte = ch.to_string()[0];
  unsigned char second_byte = ch.to_string()[1];
  int code = (first_byte << 8) | second_byte;

  return code;
}
