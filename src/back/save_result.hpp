/**
 * @file save_result.hpp
 * @brief Заголовочный файл для сохранения результатов анализа в HTML формате
 *
 * Этот модуль предоставляет функциональность для сохранения результатов
 * синтаксического анализа текста в виде HTML-страниц с интерактивным поиском
 * и визуализацией статистики.
 */

#pragma once

#include "statistics.hpp"
#include <string>
#include <vector>

using std::string;
using std::vector;

/**
 * @brief Сохраняет результаты анализа в HTML файлы
 *
 * Создает два HTML файла в указанной директории:
 * - list.html: страница с результатами поиска членов предложения
 * - statistics.html: страница со статистикой и графиками
 *
 * @param path Путь к директории для сохранения файлов
 * @param items Вектор элементов поиска (членов предложения)
 * @param stats Глобальная статистика анализа текста
 */
void saveAnalysis(const std::string path, std::vector<SearchItem> &items,
                  GlobalStats &stats);

/**
 * @brief Загружает содержимое файла в строку
 *
 * @param filename Имя файла для загрузки
 * @return std::string Содержимое файла или пустая строка при ошибке
 */
std::string loadTemplateFile(const std::string &filename);
