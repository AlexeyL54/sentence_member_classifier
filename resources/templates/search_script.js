function highlightText(text, searchWord) {
  if (!searchWord) return text;
  var searchLower = searchWord.toLowerCase();
  var textLower = text.toLowerCase();
  var result = '';
  var lastIndex = 0;
  var index = textLower.indexOf(searchLower);

  while (index !== -1) {
    result += text.substring(lastIndex, index);
    result += '<mark>' + text.substring(index, index + searchWord.length) + '</mark>';
    lastIndex = index + searchWord.length;
    index = textLower.indexOf(searchLower, lastIndex);
  }
  result += text.substring(lastIndex);
  return result;
}

function searchWord() {
  var searchText = document.getElementById('searchText').value.trim();
  var searchType = document.getElementById('searchType').value;
  var container = document.getElementById('results');
  var items = Array.from(document.querySelectorAll('.item'));
  var searchLower = searchText.toLowerCase();

  // Категории элементов:
  var exactMatches = [];      // Точное совпадение члена предложения
  var partialMatches = [];    // Частичное совпадение члена предложения
  var sentenceMatches = [];   // Совпадение только в предложениях
  var typeOnlyMatches = [];   // Только по типу (без текста поиска)
  var nonMatches = [];        // Не подходят под фильтр

  var totalFound = 0;
  var exactCount = 0;
  var partialCount = 0;
  var sentenceCount = 0;

  for (var i = 0; i < items.length; i++) {
    var item = items[i];
    var text = item.getAttribute('data-text');
    var textLower = text.toLowerCase();
    var type = item.getAttribute('data-type');

    // Проверяем соответствие типу
    var typeMatches = (!searchType || type === searchType);

    if (!typeMatches) {
      nonMatches.push(item);
      item.classList.add('hidden');
      item.classList.remove('highlight', 'exact-match', 'partial-match', 'sentence-match');
      continue;
    }

    // Определяем тип совпадения
    var isExactMatch = false;
    var isPartialMatch = false;
    var isSentenceMatch = false;

    if (searchText) {
      // Проверяем точное совпадение (полное равенство строк)
      if (textLower === searchLower) {
        isExactMatch = true;
        totalFound++;
        exactCount++;
      }
      // Проверяем частичное совпадение (слово содержится в члене предложения, но не точно)
      else if (textLower.indexOf(searchLower) !== -1) {
        isPartialMatch = true;
        totalFound++;
        partialCount++;
      }
      // Проверяем совпадение в предложениях
      else {
        var sentences = item.querySelectorAll('.sentence');
        for (var j = 0; j < sentences.length; j++) {
          var sentText = sentences[j].innerText;
          if (sentText.toLowerCase().indexOf(searchLower) !== -1) {
            isSentenceMatch = true;
            totalFound++;
            sentenceCount++;
            break;
          }
        }
      }
    }

    // Если нет поискового текста, показываем все подходящие по типу
    var showByTypeOnly = (!searchText && typeMatches);

    if (isExactMatch || isPartialMatch || isSentenceMatch || showByTypeOnly) {
      item.classList.remove('hidden');
      item.classList.add('highlight');

      // Добавляем соответствующий класс
      if (isExactMatch) {
        item.classList.add('exact-match');
        item.classList.remove('partial-match', 'sentence-match');
        exactMatches.push(item);
      } else if (isPartialMatch) {
        item.classList.add('partial-match');
        item.classList.remove('exact-match', 'sentence-match');
        partialMatches.push(item);
      } else if (isSentenceMatch) {
        item.classList.add('sentence-match');
        item.classList.remove('exact-match', 'partial-match');
        sentenceMatches.push(item);
      } else {
        item.classList.add('type-only');
        item.classList.remove('exact-match', 'partial-match', 'sentence-match');
        typeOnlyMatches.push(item);
      }

      // Обновляем подсветку в заголовке
      var textSpan = item.querySelector('.text');
      if (isExactMatch || isPartialMatch) {
        textSpan.innerHTML = highlightText(text, searchText);
      } else {
        textSpan.innerHTML = text;
      }

      // Обновляем подсветку в предложениях
      var sentences = item.querySelectorAll('.sentence');
      for (var j = 0; j < sentences.length; j++) {
        var sent = sentences[j];
        var originalText = sent.getAttribute('data-original');
        if (!originalText) {
          sent.setAttribute('data-original', sent.innerHTML);
          originalText = sent.innerHTML;
        }
        if (searchText && originalText.toLowerCase().indexOf(searchLower) !== -1) {
          sent.innerHTML = highlightText(originalText, searchText);
        } else {
          sent.innerHTML = originalText;
        }
      }
    } else {
      item.classList.add('hidden');
      item.classList.remove('highlight', 'exact-match', 'partial-match', 'sentence-match', 'type-only');
      nonMatches.push(item);
    }
  }

  // Переупорядочиваем: сначала точные совпадения, затем частичные, затем совпадения в предложениях, затем только по типу
  if (searchText || searchType) {
    // Очищаем контейнер
    while (container.firstChild) {
      container.removeChild(container.firstChild);
    }

    // Добавляем точные совпадения (наверх)
    for (var i = 0; i < exactMatches.length; i++) {
      container.appendChild(exactMatches[i]);
    }

    // Добавляем частичные совпадения
    for (var i = 0; i < partialMatches.length; i++) {
      container.appendChild(partialMatches[i]);
    }

    // Добавляем совпадения в предложениях
    for (var i = 0; i < sentenceMatches.length; i++) {
      container.appendChild(sentenceMatches[i]);
    }

    // Добавляем только по типу
    for (var i = 0; i < typeOnlyMatches.length; i++) {
      container.appendChild(typeOnlyMatches[i]);
    }

    // Добавляем скрытые (неподходящие)
    for (var i = 0; i < nonMatches.length; i++) {
      container.appendChild(nonMatches[i]);
    }
  }

  // Обновляем статистику
  var stats = document.getElementById('stats');
  if (searchText || searchType) {
    if (totalFound === 0 && exactCount === 0 && partialCount === 0 && sentenceCount === 0) {
      stats.innerHTML = '❌ Ничего не найдено';
    } else {
      var queryText = searchText ? '"' + searchText + '"' : 'выбранному типу';
      var statsParts = [];

      if (exactCount > 0) {
        statsParts.push(exactCount + ' точных совпадений');
      }
      if (partialCount > 0) {
        statsParts.push(partialCount + ' частичных совпадений');
      }
      if (sentenceCount > 0) {
        statsParts.push(sentenceCount + ' совпадений в предложениях');
      }
      if (typeOnlyMatches.length > 0 && !searchText) {
        statsParts.push(typeOnlyMatches.length + ' по типу');
      }

      stats.innerHTML = '✅ Найдено: ' + statsParts.join(', ');
    }
  } else {
    stats.innerHTML = '';
  }
}

function clearSearch() {
  document.getElementById('searchText').value = '';
  document.getElementById('searchType').value = '';
  var container = document.getElementById('results');
  var items = Array.from(document.querySelectorAll('.item'));

  // Восстанавливаем исходный порядок
  items.sort(function (a, b) {
    var orderA = parseInt(a.getAttribute('data-original-order') || '0');
    var orderB = parseInt(b.getAttribute('data-original-order') || '0');
    return orderA - orderB;
  });

  // Очищаем контейнер и вставляем в правильном порядке
  while (container.firstChild) {
    container.removeChild(container.firstChild);
  }

  for (var i = 0; i < items.length; i++) {
    var item = items[i];
    container.appendChild(item);
    item.classList.remove('hidden', 'highlight', 'exact-match', 'partial-match', 'sentence-match', 'type-only');
    var textSpan = item.querySelector('.text');
    textSpan.innerHTML = item.getAttribute('data-text');
    var sentences = item.querySelectorAll('.sentence');
    for (var j = 0; j < sentences.length; j++) {
      var sent = sentences[j];
      if (sent.getAttribute('data-original')) {
        sent.innerHTML = sent.getAttribute('data-original');
      }
    }
  }
  document.getElementById('stats').innerHTML = '';
}

document.getElementById('searchText').addEventListener('keypress', function (e) {
  if (e.key === 'Enter') searchWord();
});

document.getElementById('searchType').addEventListener('change', function () {
  searchWord();
});
