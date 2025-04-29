#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include <curl/curl.h>

#include <json-c/json.h>

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
      printf("not enough memory (realloc returned NULL)\n");
      return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

size_t convert_to_city_url(const char* city, char* city_url) {
    size_t city_url_len = 0, len = strlen(city);

    for (size_t i = 0; i < len; ++i) {
        if (isalpha(city[i]) || city[i] == ' ') {
            if (city[i] != ' ') {
                city_url[city_url_len++] = city[i];
            } else {
                city_url[city_url_len++] = '%';
                city_url[city_url_len++] = '2';
                city_url[city_url_len++] = '0';
            }
        } else {
            return 0;
        }
    }

    city_url[city_url_len] = '\0';

    return city_url_len;
}

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        fprintf(stderr, "City not specified!\n");
        return -1;
    }

    const char* city = argv[1];

    char city_url[230];

    if (convert_to_city_url(city, city_url) == 0) {
        fprintf(stderr, "Bad input data!\n");
        return -1;
    }

    char url[256];
    snprintf(url, sizeof(url), "https://wttr.in/%s?format=j1", city_url);

    CURL *curl_handle;
    CURLcode res;

    struct MemoryStruct chunk;

    chunk.memory = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_ALL);

    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    res = curl_easy_perform(curl_handle);
    if(res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }
    else {
        struct json_tokener* tok = json_tokener_new();

        struct json_object* root = json_tokener_parse_ex(tok, chunk.memory, chunk.size);

        if (!root) {
            fprintf(stderr, "JSON parsing error: %s\n", json_tokener_error_desc(json_tokener_get_error(tok)));
        } else {
            printf("City: %s\n", city);

            json_object* current_condition = json_object_array_get_idx(
                                                 json_object_object_get(root, "current_condition"), 0);
            json_object* weather = json_object_array_get_idx(
                                                 json_object_object_get(root, "weather"), 0);


            json_object* weather_desc = json_object_array_get_idx(
                                            json_object_object_get(current_condition, "weatherDesc"), 0);
            printf("Weather description: %s\n", json_object_get_string(json_object_object_get(weather_desc, "value")));

            json_object* wind_dir = json_object_object_get(current_condition, "winddir16Point");
            printf("Wind direction: %s\n", json_object_get_string(wind_dir));

            json_object* wind_speed = json_object_object_get(current_condition, "windspeedKmph");
            printf("Wind speed: %s km/h\n", json_object_get_string(wind_speed));

            json_object* max_temp = json_object_object_get(weather, "maxtempC");
            json_object* min_temp = json_object_object_get(weather, "mintempC");
            printf("Temperature: %s..%s °C\n", json_object_get_string(min_temp), json_object_get_string(max_temp));
        }

        json_object_put(root);

        json_tokener_free(tok);
    }

    curl_easy_cleanup(curl_handle);

    free(chunk.memory);

    curl_global_cleanup();

    return 0;
}
