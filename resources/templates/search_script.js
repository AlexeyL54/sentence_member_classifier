/**
 * @fileoverview Модуль поиска членов предложения
 * @description Обеспечивает функциональность поиска и фильтрации элементов по тексту и типу
 */

/**
 * Экранирует специальные символы для использования в регулярном выражении
 * @param {string} str - Исходная строка
 * @returns {string} Строка с экранированными спецсимволами
 */
function escapeRegex(str) {
  return str.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
}

/**
 * Подсвечивает вхождения поискового слова в тексте
 * @param {string} text - Исходный текст
 * @param {string} searchWord - Слово для поиска
 * @returns {string} Текст с обёрнутыми в <mark> теги совпадениями
 */
function highlightText(text, searchWord) {
  if (!searchWord || !text) return text || '';

  const escaped = escapeRegex(searchWord);
  const regex = new RegExp(`(${escaped})`, 'gi');

  return text.replace(regex, '<mark>$1</mark>');
}

/**
 * Класс для управления состоянием поиска
 * @typedef {Object} SearchState
 * @property {string} text - Поисковый текст
 * @property {string} type - Тип члена предложения
 */

/**
 * Получает текущее состояние поиска из формы
 * @returns {SearchState} Объект с текстом поиска и типом
 */
function getSearchState() {
  const searchText = document.getElementById('searchText')?.value.trim() || '';
  const searchType = document.getElementById('searchType')?.value || '';

  return { text: searchText, type: searchType };
}

/**
 * Классифицирует элемент по типу совпадения
 * @param {HTMLElement} item - DOM элемент
 * @param {string} searchText - Поисковый текст (нижний регистр)
 * @param {string} originalText - Оригинальный текст элемента (нижний регистр)
 * @returns {string} Тип совпадения: 'exact' | 'partial' | 'sentence' | 'typeOnly' | 'none'
 */
function classifyMatch(item, searchText, originalText) {
  if (!searchText) return 'typeOnly';

  // Точное совпадение (полное соответствие слова)
  // Для однобуквенных слов это также будет работать корректно
  if (originalText === searchText) return 'exact';

  // Проверка на точное совпадение по границам слова
  const wordBoundaryRegex = new RegExp(`\\b${escapeRegex(searchText)}\\b`, 'i');
  if (wordBoundaryRegex.test(originalText)) {
    // Если слово полностью совпадает с искомым (по границам)
    if (originalText.toLowerCase() === searchText.toLowerCase()) {
      return 'exact';
    }
    return 'partial';
  }

  // Частичное совпадение (внутри слова)
  if (originalText.includes(searchText)) return 'partial';

  // Совпадение в предложениях
  const sentences = item.querySelectorAll('.sentence');
  for (const sentence of sentences) {
    if (sentence.textContent.toLowerCase().includes(searchText)) {
      return 'sentence';
    }
  }

  return 'none';
}

/**
 * Применяет CSS классы к элементу в зависимости от типа совпадения
 * @param {HTMLElement} item - DOM элемент
 * @param {string} matchType - Тип совпадения
 */
function applyMatchClasses(item, matchType) {
  // Удаляем все классы состояния
  item.classList.remove('exact-match', 'partial-match', 'sentence-match', 'type-only');

  switch (matchType) {
    case 'exact':
      item.classList.add('exact-match');
      break;
    case 'partial':
      item.classList.add('partial-match');
      break;
    case 'sentence':
      item.classList.add('sentence-match');
      break;
    case 'typeOnly':
      item.classList.add('type-only');
      break;
  }
}

/**
 * Обновляет подсветку в заголовке элемента
 * @param {HTMLElement} item - DOM элемент
 * @param {string} searchText - Поисковый текст
 * @param {string} originalText - Оригинальный текст
 * @param {string} matchType - Тип совпадения
 */
function updateHeaderHighlight(item, searchText, originalText, matchType) {
  const textSpan = item.querySelector('.text');
  const shouldHighlight = matchType === 'exact' || matchType === 'partial';

  textSpan.innerHTML = shouldHighlight && searchText
    ? highlightText(originalText, searchText)
    : originalText;
}

/**
 * Сохраняет оригинальный текст предложения и обновляет подсветку
 * @param {HTMLElement} sentence - DOM элемент предложения
 * @param {string} searchText - Поисковый текст (нижний регистр)
 * @returns {boolean} Было ли обновление
 */
function updateSentenceHighlight(sentence, searchText) {
  let original = sentence.getAttribute('data-original');

  if (!original) {
    original = sentence.innerHTML;
    sentence.setAttribute('data-original', original);
  }

  if (searchText && original.toLowerCase().includes(searchText)) {
    sentence.innerHTML = highlightText(original, searchText);
    return true;
  }

  sentence.innerHTML = original;
  return false;
}

/**
 * Обновляет подсветку во всех предложениях элемента
 * @param {HTMLElement} item - DOM элемент
 * @param {string} searchText - Поисковый текст (нижний регистр)
 */
function updateSentencesHighlight(item, searchText) {
  const sentences = item.querySelectorAll('.sentence');

  for (const sentence of sentences) {
    updateSentenceHighlight(sentence, searchText);
  }
}

/**
 * Проверяет, соответствует ли элемент выбранному типу
 * @param {HTMLElement} item - DOM элемент
 * @param {string} searchType - Тип для фильтрации
 * @returns {boolean}
 */
function matchesType(item, searchType) {
  if (!searchType) return true;
  const itemType = item.getAttribute('data-type');
  return itemType === searchType;
}

/**
 * Обновляет видимость элемента на основе результатов поиска
 * @param {HTMLElement} item - DOM элемент
 * @param {boolean} isVisible - Флаг видимости
 */
function setItemVisibility(item, isVisible) {
  if (isVisible) {
    item.classList.remove('hidden');
    item.classList.add('highlight');
  } else {
    item.classList.add('hidden');
    item.classList.remove('highlight');
  }
}

/**
 * Собирает статистику по найденным элементам
 * @param {Array<{matchType: string}>} itemsData - Массив данных о совпадениях
 * @returns {Object} Статистика поиска
 */
function collectStatistics(itemsData) {
  const stats = {
    exact: 0,
    partial: 0,
    sentence: 0,
    typeOnly: 0
  };

  for (const data of itemsData) {
    if (stats[data.matchType] !== undefined) {
      stats[data.matchType]++;
    }
  }

  return stats;
}

/**
 * Формирует текст статистики для отображения
 * @param {Object} stats - Статистика поиска
 * @param {string} searchText - Поисковый текст
 * @param {string} searchType - Тип фильтрации
 * @returns {string} HTML строка со статистикой
 */
function formatStatistics(stats, searchText, searchType) {
  const hasSearch = searchText || searchType;
  if (!hasSearch) return '';

  const totalFound = stats.exact + stats.partial + stats.sentence;

  if (totalFound === 0 && stats.typeOnly === 0) {
    return '❌ Ничего не найдено';
  }

  const parts = [];
  if (stats.exact > 0) parts.push(`${stats.exact} точных совпадений`);
  if (stats.partial > 0) parts.push(`${stats.partial} частичных совпадений`);
  if (stats.sentence > 0) parts.push(`${stats.sentence} совпадений в предложениях`);
  if (stats.typeOnly > 0 && !searchText) parts.push(`${stats.typeOnly} по типу`);

  return `✅ Найдено: ${parts.join(', ')}`;
}

/**
 * Сортирует элементы результат поиска
 * @param {HTMLElement} container - Контейнер результатов
 * @param {HTMLElement[]} items - Массив элементов в нужном порядке
 */
function reorderResults(container, items) {
  // Сохраняем порядок, перемещая элементы в контейнере
  for (const item of items) {
    container.appendChild(item);
  }
}

/**
 * Получает приоритет сортировки для типа совпадения
 * @param {string} matchType - Тип совпадения
 * @returns {number} Приоритет (меньше = выше)
 */
function getSortPriority(matchType) {
  const priorities = {
    'exact': 0,
    'partial': 1,
    'sentence': 2,
    'typeOnly': 3,
    'none': 4
  };
  return priorities[matchType] ?? 4;
}

/**
 * Сортирует элементы по релевантности
 * @param {HTMLElement[]} visibleItems - Массив видимых элементов
 * @param {Array<{matchType: string}>} itemsData - Данные о совпадениях
 * @returns {HTMLElement[]} Отсортированный массив
 */
function sortByRelevance(visibleItems, itemsData) {
  return [...visibleItems].sort((a, b) => {
    const dataA = itemsData.find(d => d.item === a);
    const dataB = itemsData.find(d => d.item === b);

    const priorityA = getSortPriority(dataA?.matchType);
    const priorityB = getSortPriority(dataB?.matchType);

    return priorityA - priorityB;
  });
}

/**
 * Обновляет отображение статистики поиска
 * @param {Object} stats - Статистика поиска
 * @param {string} searchText - Поисковый текст
 * @param {string} searchType - Тип фильтрации
 */
function updateStatsDisplay(stats, searchText, searchType) {
  const statsElement = document.getElementById('stats');
  if (statsElement) {
    statsElement.innerHTML = formatStatistics(stats, searchText, searchType);
  }
}

/**
 * Обрабатывает один элемент: проверяет тип, классифицирует, применяет стили и подсветку
 * @param {HTMLElement} item - Обрабатываемый элемент
 * @param {string} searchText - Поисковый текст (нижний регистр)
 * @param {string} searchType - Тип для фильтрации
 * @returns {{item: HTMLElement, matchType: string, isVisible: boolean}} Результат обработки
 */
function processSingleItem(item, searchText, searchType) {
  const originalText = item.getAttribute('data-text')?.toLowerCase() || '';
  const typeMatches = matchesType(item, searchType);

  if (!typeMatches) {
    setItemVisibility(item, false);
    return { item, matchType: 'none', isVisible: false };
  }

  const matchType = classifyMatch(item, searchText, originalText);
  const isVisible = matchType !== 'none';

  setItemVisibility(item, isVisible);
  applyMatchClasses(item, matchType);
  updateHeaderHighlight(item, searchText, originalText, matchType);
  updateSentencesHighlight(item, searchText);

  return { item, matchType, isVisible };
}

/**
 * Собирает данные и обрабатывает все элементы
 * @param {HTMLElement[]} items - Все элементы поиска
 * @param {string} searchText - Поисковый текст (нижний регистр)
 * @param {string} searchType - Тип для фильтрации
 * @returns {{itemsData: Array, visibleItems: HTMLElement[]}} Результаты обработки
 */
function processAllItems(items, searchText, searchType) {
  const itemsData = [];
  const visibleItems = [];

  for (const item of items) {
    const result = processSingleItem(item, searchText, searchType);
    itemsData.push({ item: result.item, matchType: result.matchType });

    if (result.isVisible) {
      visibleItems.push(result.item);
    }
  }

  return { itemsData, visibleItems };
}

/**
 * Формирует финальный порядок элементов после сортировки
 * @param {HTMLElement[]} visibleItems - Видимые элементы
 * @param {Array} itemsData - Данные о совпадениях
 * @param {HTMLElement[]} allItems - Все элементы
 * @returns {HTMLElement[]} Элементы в правильном порядке
 */
function buildFinalOrder(visibleItems, itemsData, allItems) {
  const sortedVisible = sortByRelevance(visibleItems, itemsData);
  const hiddenItems = allItems.filter(item => item.classList.contains('hidden'));
  return [...sortedVisible, ...hiddenItems];
}

/**
 * Основная функция поиска
 */
function searchWord() {
  const { text: searchTextRaw, type: searchType } = getSearchState();
  const searchText = searchTextRaw.toLowerCase();

  const container = document.getElementById('results');
  if (!container) return;

  const items = Array.from(document.querySelectorAll('.item'));

  // Обработка всех элементов
  const { itemsData, visibleItems } = processAllItems(items, searchText, searchType);

  // Сортировка и применение порядка
  const finalOrder = buildFinalOrder(visibleItems, itemsData, items);
  reorderResults(container, finalOrder);

  // Обновление статистики
  const stats = collectStatistics(itemsData);
  updateStatsDisplay(stats, searchTextRaw, searchType);
}

/**
 * Восстанавливает оригинальный порядок элементов
 * @param {HTMLElement} container - Контейнер результатов
 * @param {HTMLElement[]} items - Элементы для сортировки
 */
function restoreOriginalOrder(container, items) {
  const sorted = [...items].sort((a, b) => {
    const orderA = parseInt(a.getAttribute('data-original-order') || '0', 10);
    const orderB = parseInt(b.getAttribute('data-original-order') || '0', 10);
    return orderA - orderB;
  });

  reorderResults(container, sorted);
}

/**
 * Сбрасывает подсветку всех элементов
 * @param {HTMLElement[]} items - Элементы для сброса
 */
function resetHighlighting(items) {
  for (const item of items) {
    item.classList.remove('hidden', 'highlight', 'exact-match', 'partial-match', 'sentence-match', 'type-only');

    // Восстанавливаем оригинальный текст заголовка
    const textSpan = item.querySelector('.text');
    const originalText = item.getAttribute('data-text');
    if (textSpan && originalText) {
      textSpan.innerHTML = originalText;
    }

    // Восстанавливаем оригинальный текст предложений
    const sentences = item.querySelectorAll('.sentence');
    for (const sentence of sentences) {
      const original = sentence.getAttribute('data-original');
      if (original) {
        sentence.innerHTML = original;
      }
    }
  }
}

/**
 * Очищает поиск и восстанавливает исходное состояние
 */
function clearSearch() {
  const searchTextInput = document.getElementById('searchText');
  const searchTypeSelect = document.getElementById('searchType');
  const container = document.getElementById('results');
  const statsElement = document.getElementById('stats');

  // Очищаем поля ввода
  if (searchTextInput) searchTextInput.value = '';
  if (searchTypeSelect) searchTypeSelect.value = '';
  if (statsElement) statsElement.innerHTML = '';

  if (!container) return;

  const items = Array.from(document.querySelectorAll('.item'));

  // Сбрасываем подсветку
  resetHighlighting(items);

  // Восстанавливаем порядок
  restoreOriginalOrder(container, items);
}

/**
 * Обработчик нажатия клавиши Enter
 * @param {KeyboardEvent} e - Событие клавиатуры
 */
function handleKeyPress(e) {
  if (e.key === 'Enter') {
    searchWord();
  }
}

/**
 * Инициализирует обработчики событий
 */
function initEventListeners() {
  const searchTextEl = document.getElementById('searchText');
  const searchTypeEl = document.getElementById('searchType');

  if (searchTextEl) {
    searchTextEl.removeEventListener('keypress', handleKeyPress);
    searchTextEl.addEventListener('keypress', handleKeyPress);
  }

  if (searchTypeEl) {
    searchTypeEl.removeEventListener('change', searchWord);
    searchTypeEl.addEventListener('change', searchWord);
  }
}

// Инициализация в браузере
if (typeof window !== 'undefined' && typeof document !== 'undefined') {
  const init = () => initEventListeners();

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }
}

// Экспорт для тестирования
if (typeof module !== 'undefined' && module.exports) {
  module.exports = {
    escapeRegex,
    highlightText,
    getSearchState,
    classifyMatch,
    applyMatchClasses,
    updateHeaderHighlight,
    updateSentenceHighlight,
    updateSentencesHighlight,
    matchesType,
    setItemVisibility,
    collectStatistics,
    formatStatistics,
    reorderResults,
    getSortPriority,
    sortByRelevance,
    updateStatsDisplay,
    searchWord,
    restoreOriginalOrder,
    resetHighlighting,
    clearSearch,
    handleKeyPress,
    initEventListeners
  };
}
