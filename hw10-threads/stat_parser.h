#pragma once

#include "topn_container.h"

#include <pcre.h>


typedef struct {
    pthread_mutex_t mutex;
    size_t total;
} TotalSize;

TotalSize* ts_init();

void ts_free(TotalSize* total_size);

void ts_add(TotalSize* total_size, size_t size);


// Функция для парсинга и записи в структуру для хранения
void parse_combined_logline(TopNContainer* url_to_sz,
                            TopNContainer* ref_to_ct,
                            TotalSize*     total_sz,
                            const char*    logline);
