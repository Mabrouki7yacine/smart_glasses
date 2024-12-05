#include <stdio.h>
#include <string.h>
#include "get_txt.h"
#include "wifi_connect.h"
#include "ssd1306.h"
#include "bitmaps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define TAG "smart_glass"
#define DEBOUNCE_TIME 300     
#define INPUT_PIN1 5  // First interrupt pin
#define INPUT_PIN2 18 // Second interrupt pin

int menu = 0, last_menu = -1;
int menu_changed = 0;
char data[512];
char last_data[512];
char last_time[15];
char name[15];
int i = 0;

#define SDA 14
#define SCL 27

typedef enum {home_page, face_rec, text_trans, AI_helper} menues;

QueueHandle_t interruptQueue;

static void IRAM_ATTR gpio_interrupt_handler(void *args)
{   int pinNumber = (int)args;
    xQueueSendFromISR(interruptQueue, &pinNumber, NULL);
}

void LED_Control_Task(void *params){
    int pinNumber;
    double last_time = 0;
    while (true){
        if (xQueueReceive(interruptQueue, &pinNumber, portMAX_DELAY)){
            if (pinNumber == INPUT_PIN1) {
                if (esp_timer_get_time()/1000 - last_time >= DEBOUNCE_TIME){
					menu = (menu + 1)%4;
					printf("menu : %d\n",menu);
					menu_changed = 1;
                    last_time = esp_timer_get_time()/1000;
                }
            } else if (pinNumber == INPUT_PIN2){
                if (esp_timer_get_time()/1000 - last_time >= DEBOUNCE_TIME){
					menu = (menu - 1)%4;
					if (menu < 0) {
						menu = 3;
					}
					printf("menu : %d\n",menu);
					menu_changed = 1;
                    last_time = esp_timer_get_time()/1000;
                }	
	    }
        }
    }
}

void app_main(void) {
	// Declared on the stack to avoid conflict with another var named time also
    char time[15] = {0};
	gpio_set_direction(19, GPIO_MODE_OUTPUT);
	gpio_set_level(19, 1);
    // init gpio 1 interrupt
	gpio_set_direction(INPUT_PIN1, GPIO_MODE_INPUT);
    gpio_pulldown_en(INPUT_PIN1);
    gpio_pullup_dis(INPUT_PIN1);
    gpio_set_intr_type(INPUT_PIN1, GPIO_INTR_POSEDGE);
    // init gpio 2 interrupt
	gpio_set_direction(INPUT_PIN2, GPIO_MODE_INPUT);
    gpio_pulldown_en(INPUT_PIN2);
    gpio_pullup_dis(INPUT_PIN2);
    gpio_set_intr_type(INPUT_PIN2, GPIO_INTR_POSEDGE);
	// init the queue
    interruptQueue = xQueueCreate(10, sizeof(int));
    xTaskCreate(LED_Control_Task, "LED_Control_Task", 2048, NULL, 1, NULL);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(INPUT_PIN1, gpio_interrupt_handler, (void *)INPUT_PIN1);
    gpio_isr_handler_add(INPUT_PIN2, gpio_interrupt_handler, (void *)INPUT_PIN2);
	// init wifi connection
    wifi_connection();
    vTaskDelay(250 / portTICK_PERIOD_MS);
    printf("WIFI was initiated ...........\n\n");
	//init OLED
    SSD1306_t dev;
    i2c_master_init(&dev, SDA, SCL, 0);
    dev._flip = true;
    ESP_LOGW(TAG, "Flip upside down");
    ESP_LOGI(TAG, "Panel is 128x64");
    ssd1306_init(&dev, 128, 64);
    ssd1306_contrast(&dev, 0x11);
    ssd1306_clear_screen(&dev, false);
	// display boot screen
    ssd1306_bitmaps(&dev, 32, 8, boot, 64, 56, false);
    ssd1306_display_text(&dev, 1, "  smart glasses", 15, false);
    vTaskDelay(750 / portTICK_PERIOD_MS);
	//here's our main loop
    while (1) {
	menu_changed = 0;
    	switch (menu) {
    	    case home_page:
                get_request("http://192.168.43.238:5000/home");
    	        if(last_menu != home_page){
    	            ssd1306_clear_screen(&dev, false);
    	            vTaskDelay(250 / portTICK_PERIOD_MS);
		            ssd1306_bitmaps(&dev, 0, 0, image, 128, 64, false);
    	        }
    	        memset(time, 0, sizeof(time));  // Clear the time buffer before use
    	        strcpy(time, "     ");
    	        strcat(time,data);
                if(strcmp(time,last_time) != 0){
		            ssd1306_display_text(&dev, 4, time, 12, false);
                }
    	        memset(time, 0, sizeof(time));
    	        memset(data, 0, sizeof(data));
				for (int j = 0 ; j <50 ;j++){
					if (menu_changed) break;
					vTaskDelay(10 / portTICK_PERIOD_MS);
				}    	        
				last_menu = home_page;	
                strcpy(last_time,time);
    	    break;

    	    case face_rec:
                get_request("http://192.168.43.238:5000/face_rec");
    	        if(last_menu != face_rec){
    	            ssd1306_clear_screen(&dev, false);
    	            ssd1306_display_text(&dev, 3, "    face rec", 13, false);
    	            vTaskDelay(500 / portTICK_PERIOD_MS);	
    	            ssd1306_clear_screen(&dev, false);
    	        }
    	        memset(name, 0, sizeof(name));  // Clear the name buffer before use
    	        for (i = 0; i < (8 - strlen(data)/2); i++) {
    	            strcat(name, " ");
    	        }
				if (strcmp(data ,last_data) != 0){
    	            ssd1306_clear_screen(&dev, false);
				}
    	        strcat(name, data);
    	        ssd1306_bitmaps(&dev, 56, 4, person, 16, 16, false);
    	        ssd1306_display_text(&dev, 4, name, strlen(name), false);
    	        memset(name, 0, sizeof(name));
    	        memset(last_data, 0, sizeof(last_data));
				strcpy(last_data,data);
    	        memset(data, 0, sizeof(data));
				for (int j = 0 ; j < 100 ;j++){
					if (menu_changed) break;
					vTaskDelay(10 / portTICK_PERIOD_MS);
				} 
    	        last_menu = face_rec;
    	    break;

			case text_trans:
                get_request("http://192.168.43.238:5000/translation");
    		    textwrapped *dis_trans;
                dis_trans = wrp(data);	
                if(last_menu != text_trans){
    		        ssd1306_clear_screen(&dev, false);
    		        ssd1306_display_text(&dev, 3, "txt translation", 15, false);
    		        vTaskDelay(500 / portTICK_PERIOD_MS);
    		        dis_trans = wrp(data);	
    		        ssd1306_clear_screen(&dev, false);
    		    }
    		    // display text
				if (strcmp(data ,last_data) != 0){
    	            ssd1306_clear_screen(&dev, false);
				}

    		    for (i = 0;i < dis_trans->size ; i++){
    		        ssd1306_display_text(&dev, i%8, dis_trans->textw[i], strlen(dis_trans->textw[i]), false);
    		        if(i%8 == 7){
						for (int j = 0 ; j <75 ;j++){
							if (menu_changed) break;
							vTaskDelay(10 * (i%8) / portTICK_PERIOD_MS);
							}
						ssd1306_clear_screen(&dev, false);
    		        }
    		    }
				if(i%8 != 7){
					for (int j = 0 ; j <75 ;j++){
						if (menu_changed) break;
						vTaskDelay(10 * (i%8) / portTICK_PERIOD_MS);
					}
    		    }
    		    ssd1306_clear_screen(&dev, false);
    		    free(dis_trans);
    	        memset(last_data, 0, sizeof(last_data));
				strcpy(last_data,data);
				memset(data, 0, sizeof(data));
    		    last_menu = text_trans;	
    	    break;

    		case AI_helper:
                get_request("http://192.168.43.238:5000/AI_assistant");
                textwrapped *dis_txt;
    		    dis_txt = wrp(data);
    		    if(last_menu != AI_helper){
    		        ssd1306_clear_screen(&dev, false);
    		        ssd1306_display_text(&dev, 3, "  AI Assistant", 14, false);
    		        vTaskDelay(500 / portTICK_PERIOD_MS);
    		        dis_txt = wrp(data);
    		        ssd1306_clear_screen(&dev, false);	
    		    }
    		    for (i = 0;i < dis_txt->size ; i++){
    		        ssd1306_display_text(&dev, i%8, dis_txt->textw[i], strlen(dis_txt->textw[i]), false);
    		        if(i%8 == 7){
						for (int j = 0 ; j <75 ;j++){
							if (menu_changed) break;
							vTaskDelay(10 * (i%8) / portTICK_PERIOD_MS);
							}
						ssd1306_clear_screen(&dev, false);
    		        }
    		    }
				if (menu_changed) break;
				if(i%8 != 7){
					for (int j = 0 ; j <75 ;j++){
						if (menu_changed) break;
						vTaskDelay(10 * (i%8) / portTICK_PERIOD_MS);
					}
    		    }
    		    ssd1306_clear_screen(&dev, false);
    		    free(dis_txt);
				memset(data, 0, sizeof(data));
    		    last_menu = AI_helper;
    		break;

            default:
                break;
        }
    }
}
