#pragma once

#include <glib.h>
#include <pthread.h>

// Контейнер для хранения топ-N элементов
typedef struct TopNContainer TopNContainer;

// Инициализация контейнера для хранения топ-N элементов
TopNContainer* topn_cont_init(int capacity);

// Освобождение контейнера для хранения топ-N элементов
void topn_cont_free(TopNContainer* cont);

// Добавляет элемент в контейнер для хранения топ-N элементов
void topn_cont_add(TopNContainer* cont, gchar* key, gint value);

// Вывод топ-N элементов контейнера
void topn_cont_print(TopNContainer* cont, size_t count);

// Объединение двух контейнеров топ-N для хранения топ-N элементов
void topn_cont_merge(TopNContainer* dst, TopNContainer* src);

// Функция для тестирования контейнера для хранения топ-N элементов
void topn_test();
