#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define JOYSTICK_X 26 //Eixo x do Joystick, simula o sensor de chuva
#define JOYSTICK_Y 27 //Eixo y do Joystick, simula o sensor ultrassônico para medir o nível do rio

QueueHandle_t xQueueJoystickData; //Definição da Fila para Valores do Joystick
QueueHandle_t xQueueModeData; //Definição da Fila para Valores do Modo de Operação

//Definição de Struct para guardar os valores lido pelo Joystick
typedef struct {
    uint32_t x;
    uint32_t y; 
}Joystick_data_t;

//Definição de Struct para guardar o tipo de operação atual
typedef struct 
{
    bool normalMode;
    bool alertMode;
}OperationMode_data_t;

/**
 * @brief Task usada para fazer a leitura dos sensores (eixo x e y do ADC)
 * 
 * Após a leitura, realiza a normalização para definir os valores de nível do rio
 * e volume de chuva 
 */
void vReadJoystickValuesTask()
{
    adc_init();
    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y);

    Joystick_data_t joystick;
    uint32_t adc_x_value, adc_y_value;
    float river_level = 5.0, //Valor para definir o nível normal do Rio
          intense_rain = 100.0;  //Intensidade Máxima de Chuva

    while(true){
        //Faz a leitura do eixo X
        adc_select_input(0);
        adc_x_value = adc_read();

        //Faz a leitura do eixo Y
        adc_select_input(1);
        adc_y_value = adc_read();

        if (adc_y_value > 2100){
            //Indica que o nível do rio subiu; calcula o valor atual (pode aumentar até 10.0 metros)
            joystick.y = river_level + (river_level * (adc_y_value - 2048) / 2047);
        }else if (adc_y_value < 1800){
            //Indica que o nível do rio desceu; calcula o valor atual (pode diminuir até 0 metros)
            joystick.y = river_level - (river_level * (2048 - adc_y_value) / 2047);
        }else {
            joystick.y = river_level;
        }
    
        //Calcula a intensidade da chuva com base nos valores do eixo X
        joystick.x = (intense_rain * adc_x_value) / 4095.0;

        //Envia os dados para a fila
        xQueueSend(xQueueJoystickData, &joystick, 0);
        //Gera um delay de 1s
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief Task usada para fazer o calculo do nível do rio e volume de chuva
 */
void vCalculateLevelsTask()
{
    Joystick_data_t joystick;
    while(true)
    {
        
    }
}

/**
 * @brief Task que exibe os resultados de leitura no display SSD1306
 */

 /**
  * @brief Task que exibe os alertas na matriz de LEDS e no LedRGB quando estiver em Modo Alerta
  */

int main()
{
    stdio_init_all();

    //Cria a fila para armazenar os valores do Joystick
    xQueueJoystickData = xQueueCreate(5, sizeof(Joystick_data_t));

    xTaskCreate(vReadJoystickValuesTask, "Read Joystick Task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    while (true) {

    }
}
