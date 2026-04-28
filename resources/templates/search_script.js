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
  var items = document.querySelectorAll('.item');
  var found = 0;
  var searchLower = searchText.toLowerCase();

  for (var i = 0; i < items.length; i++) {
    var item = items[i];
    var text = item.getAttribute('data-text');
    var textLower = text.toLowerCase();
    var type = item.getAttribute('data-type');
    var show = false;

    if (searchType && type !== searchType) {
      show = false;
    } else if (searchText && textLower.indexOf(searchLower) !== -1) {
      show = true;
    } else if (searchText && !show) {
      var sentences = item.querySelectorAll('.sentence');
      for (var j = 0; j < sentences.length; j++) {
        var sentText = sentences[j].innerText;
        if (sentText.toLowerCase().indexOf(searchLower) !== -1) {
          show = true;
          break;
        }
      }
    } else if (!searchText && !searchType) {
      show = true;
    } else if (!searchText && searchType && type === searchType) {
      show = true;
    }

    if (show) {
      item.classList.remove('hidden');
      item.classList.add('highlight');
      found++;

      var textSpan = item.querySelector('.text');
      if (searchText && textLower.indexOf(searchLower) !== -1) {
        textSpan.innerHTML = highlightText(text, searchText);
      } else {
        textSpan.innerHTML = text;
      }

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
      item.classList.remove('highlight');
    }
  }

  var stats = document.getElementById('stats');
  if (searchText || searchType) {
    if (found === 0) {
      stats.innerHTML = '❌ Ничего не найдено';
    } else {
      stats.innerHTML = '✅ Найдено ' + found + ' элементов по запросу "' + searchText + '"';
    }
  } else {
    stats.innerHTML = '';
  }
}

function clearSearch() {
  document.getElementById('searchText').value = '';
  document.getElementById('searchType').value = '';
  var items = document.querySelectorAll('.item');

  for (var i = 0; i < items.length; i++) {
    var item = items[i];
    item.classList.remove('hidden', 'highlight');
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
