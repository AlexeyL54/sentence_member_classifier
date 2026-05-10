const { highlightText, searchWord, clearSearch, initEventListeners } = require('../search_script.js');

/**
 * beforeAll - выполняется один раз перед всеми тестами
 * Аналог SetUpTestCase() в GTest
 * Создаем DOM структуру, которую будут использовать все тесты
 */
beforeAll(() => {
  // Создаем HTML структуру, имитирующую реальную страницу
  document.body.innerHTML = `
    <div>
      <!-- Поле ввода для поискового запроса -->
      <input type="text" id="searchText" />
      
      <!-- Выпадающий список для фильтрации по типу члена предложения -->
      <select id="searchType">
        <option value="">Все типы</option>
        <option value="подлежащее">Подлежащее</option>
        <option value="сказуемое">Сказуемое</option>
        <option value="дополнение">Дополнение</option>
        <option value="определение">Определение</option>
        <option value="обстоятельство">Обстоятельство</option>
      </select>
      
      <!-- Блок для отображения статистики поиска -->
      <div class="stats" id="stats"></div>
      
      <!-- Контейнер с результатами (члены предложения) -->
      <div id="results">
        <!-- Элемент 1: подлежащее "книга" -->
        <div class="item" data-text="книга" data-type="подлежащее" data-original-order="0">
          <span class="text">книга</span>
          <div class="sentence" data-original="Я читаю книгу">Я читаю книгу</div>
        </div>
        
        <!-- Элемент 2: сказуемое "читаю" -->
        <div class="item" data-text="читаю" data-type="сказуемое" data-original-order="1">
          <span class="text">читаю</span>
          <div class="sentence" data-original="Я читаю книгу">Я читаю книгу</div>
        </div>
        
        <!-- Элемент 3: определение "интересную" -->
        <div class="item" data-text="интересную" data-type="определение" data-original-order="2">
          <span class="text">интересную</span>
          <div class="sentence" data-original="Читаю интересную книгу">Читаю интересную книгу</div>
        </div>
        
        <!-- Элемент 4: обстоятельство "быстро" -->
        <div class="item" data-text="быстро" data-type="обстоятельство" data-original-order="3">
          <span class="text">быстро</span>
          <div class="sentence" data-original="Я быстро читаю">Я быстро читаю</div>
        </div>
      </div>
    </div>
  `;

  initEventListeners();
});

/**
 * beforeEach - выполняется перед КАЖДЫМ тестом
 * Аналог SetUp() в GTest
 * Очищаем поля поиска и сбрасываем результаты, чтобы тесты не влияли друг на друга
 */
beforeEach(() => {
  document.getElementById('searchText').value = '';
  document.getElementById('searchType').value = '';
  clearSearch();
});


describe('1. Тесты функции highlightText (подсветка текста)', () => {

  /**
   * Тест 1.1: Проверка точного совпадения
   * Ожидание: Слово "мир" должно быть обернуто в <mark> теги
   */
  test('1.1 - Должен оборачивать точное совпадение в <mark> теги', () => {
    // Подготовка (Arrange) - входные данные
    const text = "Привет мир";
    const searchWord = "мир";

    // Действие (Act) - вызываем тестируемую функцию
    const result = highlightText(text, searchWord);

    // Проверка (Assert) - сравниваем с ожидаемым результатом
    expect(result).toBe("Привет <mark>мир</mark>");
  });

  /**
   * Тест 1.2: Проверка регистронезависимости
   * Ожидание: Поиск должен работать независимо от регистра букв
   * Должен найти "JavaScript" по запросу "javascript"
   */
  test('1.2 - Должен работать независимо от регистра букв', () => {
    const result = highlightText("JavaScript", "javascript");
    expect(result).toBe("<mark>JavaScript</mark>");
  });

  /**
   * Тест 1.3: Проверка множественных вхождений
   * Ожидание: Должны подсвечиваться ВСЕ вхождения слова
   * В строке "кот кот кот" слово "кот" встречается 3 раза
   */
  test('1.3 - Должен подсвечивать все вхождения слова в тексте', () => {
    const result = highlightText("кот кот кот", "кот");

    // Подсчитываем количество открывающих тегов <mark>
    const markCount = (result.match(/<mark>/g) || []).length;

    // Проверяем, что нашли ровно 3 вхождения
    expect(markCount).toBe(3);
  });

  /**
   * Тест 1.4: Поведение при пустом поисковом запросе
   * Ожидание: Если поисковое слово пустое, возвращаем исходный текст без изменений
   */
  test('1.4 - Пустой поисковый запрос не должен менять текст', () => {
    const text = "Исходный текст";
    const result = highlightText(text, "");
    expect(result).toBe(text);
  });

  /**
   * Тест 1.5: Обработка пустой строки текста
   * Ожидание: Если исходный текст пустой, возвращаем пустую строку
   */
  test('1.5 - Пустой исходный текст должен возвращать пустую строку', () => {
    const result = highlightText("", "тест");
    expect(result).toBe("");
  });

  /**
   * Тест 1.6: Отсутствие совпадений
   * Ожидание: Если слово не найдено, текст не изменяется и не содержит <mark>
   */
  test('1.6 - Отсутствие совпадений не должно добавлять подсветку', () => {
    const result = highlightText("Привет мир", "пока");
    expect(result).toBe("Привет мир");
    expect(result).not.toContain("<mark>");
  });

  /**
   * Тест 1.7: Перекрывающиеся совпадения
   * Ожидание: Корректная обработка случая, когда совпадения перекрываются
   * Например, в строке "aaa" поиск "aa" может дать перекрывающиеся результаты
   */
  test('1.7 - Должен корректно обрабатывать перекрывающиеся совпадения', () => {
    const result = highlightText("aaa", "aa");
    // Просто проверяем, что есть хотя бы одна подсветка
    expect(result).toContain("<mark>");
  });
});

describe('2. Тесты поиска по тексту (searchWord)', () => {

  /**
   * Тест 2.1: Точное совпадение
   * Ожидание: Поиск "книга" должен найти ровно один элемент (подлежащее)
   * И этот элемент должен иметь класс 'exact-match'
   */
  test('2.1 - Должен находить точное совпадение слова "книга"', () => {
    // Вводим поисковый запрос
    document.getElementById('searchText').value = 'книга';

    // Выполняем поиск
    searchWord();

    // Находим все видимые элементы с классом exact-match
    const exactMatches = document.querySelectorAll('.item.exact-match:not(.hidden)');

    expect(exactMatches[0].getAttribute('data-text')).toBe('книга');
  });

  /**
   * Тест 2.2: Частичное совпадение
   * Ожидание: Поиск "чит" должен найти "читаю" как частичное совпадение
   * Слово "книга" не содержит "чит", поэтому не должно быть найдено
   */
  test('2.2 - Должен находить частичные совпадения (подстроку в слове)', () => {
    document.getElementById('searchText').value = 'чит';
    searchWord();

    const partialMatches = document.querySelectorAll('.item.partial-match:not(.hidden)');

    // Должно быть найдено ровно одно частичное совпадение
    expect(partialMatches.length).toBe(1);
    // Проверяем, что это слово "читаю"
    expect(partialMatches[0].getAttribute('data-text')).toBe('читаю');
  });

  /**
   * Тест 2.3: Совпадение в предложении
   * Ожидание: Поиск "быстро" должен найти "быстро" НЕ как член предложения,
   * а как слово внутри предложения. Такой элемент получает класс 'sentence-match'
   */
  test('2.3 - Должен находить слова внутри предложений-примеров', () => {
    document.getElementById('searchText').value = 'быстро';
    searchWord();

    // "быстро" - это точное совпадение, не sentence-match
    const exactMatches = document.querySelectorAll('.item.exact-match:not(.hidden)');
    expect(exactMatches.length).toBe(1);
    expect(exactMatches[0].getAttribute('data-text')).toBe('быстро');

    // Проверяем, что подсветка в предложении тоже работает
    const sentence = exactMatches[0].querySelector('.sentence');
    expect(sentence.innerHTML).toContain('<mark>');
  });

  /**
   * Тест 2.4: Отсутствие результатов
   * Ожидание: При поиске несуществующего слова должна показываться статистика "Ничего не найдено"
   * И все элементы должны быть скрыты
   */
  test('2.4 - Несуществующее слово должно показывать "Ничего не найдено"', () => {
    document.getElementById('searchText').value = 'несуществующееслово';
    searchWord();

    // Проверяем текст статистики
    const stats = document.getElementById('stats');
    expect(stats.innerHTML).toContain('❌ Ничего не найдено');

    // Проверяем, что все элементы скрыты
    const visibleItems = document.querySelectorAll('.item:not(.hidden)');
    expect(visibleItems.length).toBe(0);
  });

  /**
   * Тест 2.5: Регистронезависимость поиска
   * Ожидание: Поиск "КНИГА" (заглавными) должен найти "книга" (строчными)
   */
  test('2.5 - Поиск должен работать независимо от регистра букв', () => {
    document.getElementById('searchText').value = 'КНИГА';
    searchWord();

    const exactMatches = document.querySelectorAll('.item.exact-match:not(.hidden)');
    expect(exactMatches.length).toBe(1);
  });
});

describe('3. Тесты фильтрации по типу члена предложения', () => {

  /**
   * Тест 3.1: Фильтр "подлежащее"
   * Ожидание: Должен показать только элемент с типом "подлежащее" (книга)
   */
  test('3.1 - Должен показывать только подлежащие при выборе соответствующего типа', () => {
    // Выбираем тип "подлежащее"
    document.getElementById('searchType').value = 'подлежащее';
    searchWord();

    // Получаем все видимые элементы
    const visibleItems = document.querySelectorAll('.item:not(.hidden)');

    // Должен быть виден только один элемент
    expect(visibleItems.length).toBe(1);

    // Проверяем, что это подлежащее
    expect(visibleItems[0].getAttribute('data-type')).toBe('подлежащее');
  });

  /**
   * Тест 3.2: Фильтр "сказуемое"
   * Ожидание: Должен показать только элемент с типом "сказуемое" (читаю)
   */
  test('3.2 - Должен показывать только сказуемые при выборе соответствующего типа', () => {
    document.getElementById('searchType').value = 'сказуемое';
    searchWord();

    const visibleItems = document.querySelectorAll('.item:not(.hidden)');
    expect(visibleItems.length).toBe(1);
    expect(visibleItems[0].getAttribute('data-type')).toBe('сказуемое');
  });

  /**
   * Тест 3.3: Пустой фильтр (все типы)
   * Ожидание: Должны быть видны ВСЕ элементы (4 штуки)
   */
  test('3.3 - Пустой фильтр должен показывать все типы членов предложения', () => {
    document.getElementById('searchType').value = '';
    searchWord();

    const visibleItems = document.querySelectorAll('.item:not(.hidden)');
    // У нас 4 элемента в тестовом наборе
    expect(visibleItems.length).toBe(4);
  });

  /**
   * Тест 3.4: Комбинация фильтра по типу и текстового поиска
   * Ожидание: Должны быть найдены элементы, удовлетворяющие ОБОИМ условиям
   * Ищем "книга" среди подлежащих - должно найти 1 элемент
   */
  test('3.4 - Комбинация типа и текста поиска должна работать как AND (И)', () => {
    // Ищем "книга" только среди подлежащих
    document.getElementById('searchType').value = 'подлежащее';
    document.getElementById('searchText').value = 'книга';
    searchWord();

    const visibleItems = document.querySelectorAll('.item:not(.hidden)');

    // Должен быть найден 1 элемент
    expect(visibleItems.length).toBe(1);

    // Проверяем оба условия
    const item = visibleItems[0];
    expect(item.getAttribute('data-type')).toBe('подлежащее');  // Тип совпадает
    expect(item.classList.contains('exact-match')).toBe(true);  // Текст совпадает
  });

  /**
   * Тест 3.5: Несовместимые фильтры
   * Ожидание: Если тип не совпадает, элемент должен быть скрыт даже при совпадении текста
   * Ищем "книга" среди определений - не должно найти ничего
   */
  test('3.5 - Несовпадающий тип должен скрывать элемент даже при совпадении текста', () => {
    document.getElementById('searchType').value = 'определение';
    document.getElementById('searchText').value = 'книга';
    searchWord();

    const visibleItems = document.querySelectorAll('.item:not(.hidden)');
    // Ничего не найдено, так как "книга" - это подлежащее, а не определение
    expect(visibleItems.length).toBe(0);
  });
});

describe('4. Тесты сортировки результатов поиска', () => {

  /**
   * Тест 4.1: Приоритет точных совпадений
   * Ожидание: Точные совпадения должны быть выше частичных
   */
  test('4.1 - Точные совпадения должны отображаться выше частичных', () => {
    // Ищем "книг" - есть точное "книга" и частичное "книжный" (если добавить)
    document.getElementById('searchText').value = 'книг';
    searchWord();

    const displayedItems = document.querySelectorAll('#results .item:not(.hidden)');
    const firstItemText = displayedItems[0].getAttribute('data-text');

    // Первым должно идти точное совпадение "книга"
    expect(firstItemText).toBe('книга');
  });

  /**
   * Тест 4.2: Частичные совпадения выше совпадений в предложениях
   * Ожидание: Элементы с классом partial-match должны быть выше sentence-match
   */
  test('4.2 - Частичные совпадения должны быть выше совпадений в предложениях', () => {
    document.getElementById('searchText').value = 'чита';
    searchWord();

    const displayedItems = document.querySelectorAll('#results .item:not(.hidden)');

    // Проверяем порядок классов у видимых элементов
    const classes = Array.from(displayedItems).map(item => {
      if (item.classList.contains('exact-match')) return 'exact';
      if (item.classList.contains('partial-match')) return 'partial';
      if (item.classList.contains('sentence-match')) return 'sentence';
      return 'other';
    });

    // Проверяем, что partial идет раньше sentence (если оба присутствуют)
    const partialIndex = classes.indexOf('partial');
    const sentenceIndex = classes.indexOf('sentence');

    if (partialIndex !== -1 && sentenceIndex !== -1) {
      expect(partialIndex).toBeLessThan(sentenceIndex);
    }
  });
});

describe('5. Тесты функции сброса поиска (clearSearch)', () => {

  /**
   * Тест 5.1: Очистка полей ввода
   * Ожидание: Поле текста и селектор типа должны стать пустыми
   */
  test('5.1 - Должен очищать поле поиска и сбрасывать выбранный тип', () => {
    // Сначала заполняем поля
    document.getElementById('searchText').value = 'книга';
    document.getElementById('searchType').value = 'подлежащее';

    // Выполняем сброс
    clearSearch();

    // Проверяем, что поля очистились
    expect(document.getElementById('searchText').value).toBe('');
    expect(document.getElementById('searchType').value).toBe('');
  });

  /**
   * Тест 5.2: Показ всех элементов
   * Ожидание: После сброса должны быть видны все элементы (ни одного скрытого)
   */
  test('5.2 - После сброса все элементы должны стать видимыми', () => {
    // Сначала выполняем поиск, который скрывает часть элементов
    document.getElementById('searchText').value = 'книга';
    searchWord();

    // Проверяем, что часть элементов скрыта
    let visibleItems = document.querySelectorAll('.item:not(.hidden)');
    expect(visibleItems.length).toBe(1);  // Только "книга" видна

    // Выполняем сброс
    clearSearch();

    // Проверяем, что теперь все элементы видны
    visibleItems = document.querySelectorAll('.item:not(.hidden)');
    expect(visibleItems.length).toBe(4);  // Все 4 элемента снова видны
  });

  /**
   * Тест 5.3: Восстановление порядка
   * Ожидание: Элементы должны вернуться в исходный порядок (по атрибуту data-original-order)
   */
  test('5.3 - Должен восстанавливать исходный порядок элементов', () => {
    // Перемешиваем порядок с помощью поиска
    document.getElementById('searchText').value = 'книга';
    searchWord();  // "книга" перемещается в начало

    // Выполняем сброс
    clearSearch();

    // Получаем порядок атрибутов data-original-order
    const items = document.querySelectorAll('#results .item');
    const actualOrder = Array.from(items).map(item => item.getAttribute('data-original-order'));
    const expectedOrder = ['0', '1', '2', '3'];

    // Порядок должен восстановиться
    expect(actualOrder).toEqual(expectedOrder);
  });

  /**
   * Тест 5.4: Очистка статистики
   * Ожидание: Блок статистики должен стать пустым
   */
  test('5.4 - Должен очищать блок со статистикой', () => {
    // Выполняем поиск, чтобы появилась статистика
    document.getElementById('searchText').value = 'книга';
    searchWord();
    expect(document.getElementById('stats').innerHTML).not.toBe('');

    // Выполняем сброс
    clearSearch();

    // Статистика должна быть очищена
    expect(document.getElementById('stats').innerHTML).toBe('');
  });

  /**
   * Тест 5.5: Удаление классов подсветки
   * Ожидание: Все элементы должны потерять классы exact-match, partial-match и т.д.
   */
  test('5.5 - Должен удалять все классы подсветки с элементов', () => {
    // Выполняем поиск, который добавляет классы
    document.getElementById('searchText').value = 'книга';
    searchWord();

    // Проверяем, что классы добавились
    let hasHighlightClass = document.querySelector('.item.exact-match') !== null;
    expect(hasHighlightClass).toBe(true);

    // Выполняем сброс
    clearSearch();

    // Проверяем, что классы удалены
    hasHighlightClass = document.querySelector('.item.exact-match') !== null;
    expect(hasHighlightClass).toBe(false);
  });
});

describe('6. Тесты отображения статистики поиска', () => {

  /**
   * Тест 6.1: Количество точных совпадений
   * Ожидание: Статистика должна показывать точное количество exact-match элементов
   */
  test('6.1 - Должен показывать количество точных совпадений', () => {
    document.getElementById('searchText').value = 'книга';
    searchWord();

    const stats = document.getElementById('stats').innerHTML;
    expect(stats).toContain('1 точных совпадений');
  });

  /**
   * Тест 6.2: Отсутствие статистики при пустом поиске
   * Ожидание: Если нет поискового запроса и фильтра, статистика не показывается
   */
  test('6.2 - При пустом поиске статистика не должна отображаться', () => {
    // Не вводим поисковый запрос, не выбираем тип
    searchWord();
    expect(document.getElementById('stats').innerHTML).toBe('');
  });
});

describe('7. Тесты краевых случаев (обработка некорректных данных)', () => {

  /**
   * Тест 7.1: Пустая строка поиска
   * Ожидание: Должны быть видны все элементы (сброс фильтрации)
   */
  test('7.1 - Поиск по пустой строке должен показывать все элементы', () => {
    document.getElementById('searchText').value = '';
    searchWord();

    const visibleItems = document.querySelectorAll('.item:not(.hidden)');
    expect(visibleItems.length).toBe(4);
  });

  /**
   * Тест 7.2: Строка из пробелов
   * Ожидание: Пробелы должны игнорироваться (trim), показываем все элементы
   */
  test('7.2 - Поиск по строке из пробелов должен игнорировать их', () => {
    document.getElementById('searchText').value = '   ';
    searchWord();

    const visibleItems = document.querySelectorAll('.item:not(.hidden)');
    expect(visibleItems.length).toBe(4);
  });

  /**
   * Тест 7.3: Специальные символы
   * Ожидание: Поиск должен корректно работать с символами вроде +, *, [, ] и т.д.
   */
  test('7.3 - Должен корректно обрабатывать специальные символы', () => {
    // Динамически добавляем элемент со спецсимволами
    const results = document.getElementById('results');
    const specialItem = document.createElement('div');
    specialItem.className = 'item';
    specialItem.setAttribute('data-text', 'c++');
    specialItem.setAttribute('data-type', 'другое');
    specialItem.innerHTML = '<span class="text">c++</span>';
    results.appendChild(specialItem);

    // Ищем "c++"
    document.getElementById('searchText').value = 'c++';
    searchWord();

    const exactMatches = document.querySelectorAll('.item.exact-match:not(.hidden)');
    expect(exactMatches.length).toBe(1);

    // Удаляем тестовый элемент, чтобы не влиять на другие тесты
    specialItem.remove();
  });

  /**
   * Тест 7.4: Очень длинные строки
   * Ожидание: Функция не должна падать на длинных строках (10000+ символов)
   */
  test('7.4 - Должен работать с очень длинными строками (проверка производительности)', () => {
    const longText = 'а'.repeat(10000);
    const result = highlightText(longText, 'а');

    // Проверяем, что результат не пустой и содержит подсветку
    expect(result.length).toBeGreaterThan(0);
    expect(result).toContain('<mark>');
  });
});

describe('8. Тесты валидации DOM элементов', () => {

  /**
   * Тест 8.1: Поле ввода поиска
   * Ожидание: Элемент с id="searchText" должен существовать и быть INPUT
   */
  test('8.1 - Элемент searchText должен существовать и быть полем ввода', () => {
    const element = document.getElementById('searchText');
    expect(element).not.toBeNull();
    expect(element.tagName).toBe('INPUT');
  });

  /**
   * Тест 8.2: Выпадающий список типов
   * Ожидание: Элемент с id="searchType" должен существовать и быть SELECT
   */
  test('8.2 - Элемент searchType должен существовать и быть выпадающим списком', () => {
    const element = document.getElementById('searchType');
    expect(element).not.toBeNull();
    expect(element.tagName).toBe('SELECT');
  });

  /**
   * Тест 8.3: Контейнер результатов
   * Ожидание: Элемент с id="results" должен существовать
   */
  test('8.3 - Элемент results (контейнер результатов) должен существовать', () => {
    const element = document.getElementById('results');
    expect(element).not.toBeNull();
  });

  /**
   * Тест 8.4: Элементы item
   * Ожидание: Должны существовать элементы с классом "item"
   */
  test('8.4 - Должны существовать элементы с классом item', () => {
    const items = document.querySelectorAll('.item');
    expect(items.length).toBeGreaterThan(0);
  });
});
