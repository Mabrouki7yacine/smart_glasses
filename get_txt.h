#ifndef GET_TEXT_H
#define GET_TEXT_H

#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_client.h"

typedef struct {
    char textw[40][16];
    int size;
} textwrapped;

// Function declarations
textwrapped* wrp(char* original);
void extract_values(const char *json_string);
esp_err_t client_event_get_handler(esp_http_client_event_handle_t evt);
void get_request();


#endif