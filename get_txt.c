#include "get_txt.h"

extern char data[512];
//extern int menu ;
//int menu ;
char readings[512];

char* my_socket = "http://192.168.1.1";

void extract_values(const char *json_string) {
    //const char *pos0 = strstr(json_string, "\"menu\":");
    const char *pos1 = strstr(json_string, "\"data\":");

    // Extract the values using sscanf
    //sscanf(pos0 , "\"menu\":%d", &menu);
    sscanf(pos1, "\"data\":\"%[^\"]\"", data);
    //strcpy(data, temp_data);
    }

esp_err_t client_event_get_handler(esp_http_client_event_handle_t evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:
        // Ensure we don't write more than the buffer size
        if (evt->data_len < sizeof(readings)) {
            snprintf(readings, sizeof(readings), "%.*s", evt->data_len, (char *)evt->data);
            extract_values(readings);
        } else {
            ESP_LOGE("HTTP_CLIENT", "Data too long for buffer");
        }
        break;

    default:
        break;
    }
    return ESP_OK;
}

void get_request(const char* url){
    esp_http_client_config_t config_get = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .cert_pem = NULL,
        .event_handler = client_event_get_handler};
        
    esp_http_client_handle_t client = esp_http_client_init(&config_get);
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        ESP_LOGI("HTTP_CLIENT", "GET request successful");
    } else {
        ESP_LOGE("HTTP_CLIENT", "GET request failed: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);
}


textwrapped* wrp(char* original) {
    textwrapped* my_output = (textwrapped*)malloc(sizeof(textwrapped));
    if (my_output == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    char **words = (char **)malloc(256 * sizeof(char *));
    if (words == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        free(my_output);
        return NULL;
    }

    char word[50] = "";
    int index = 0;
    int word_length = 0;
    size_t original_len = strlen(original);

    for (size_t i = 0; i <= original_len; i++) {
        if (original[i] == ' ' || original[i] == '\0') {
            words[index] = (char *)malloc((word_length + 1) * sizeof(char));
            //check allocation
            if (words[index] == NULL) {
                fprintf(stderr, "Memory allocation failed\n");
                for (int j = 0; j < index; j++) {
                    free(words[j]);
                }
                free(words);
                free(my_output);
                return NULL;
            }
            //store word to words array
            strcpy(words[index], word);
            memset(word, 0, sizeof(word));
            word_length = 0;
            // to keep track of number of words
            index++;
        } else {
            //add elements to word until ' ' 
            word[word_length] = original[i];
            word_length++;
        }
    }

    char txt[16] = "";
    int m = 0;
    // store words into sentences such that it's less than 14
    for (int i = 0; i < index; i++) {
        if (strlen(txt) + strlen(words[i]) > 15) { // +1 for the space
            strcpy(my_output->textw[m], txt);
            m++;
            memset(txt, 0, sizeof(txt));
        }
        strcat(txt, words[i]);
        strcat(txt, " ");
    }
    // add last sentence
    if (strlen(txt) > 0) {
        strcpy(my_output->textw[m], txt);
        m++;
    }
    // free allocated mem
    for (int i = 0; i < index; i++) {
        free(words[i]);
    }
    free(words);

    my_output->size = m;
    return my_output;
}
