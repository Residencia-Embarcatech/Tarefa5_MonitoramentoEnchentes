#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "hardware/pio.h"
#include "pio_matrix.pio.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define JOYSTICK_X 26 //Eixo x do Joystick, simula o sensor de chuva
#define JOYSTICK_Y 27 //Eixo y do Joystick, simula o sensor ultrassônico para medir o nível do rio

/**
 * Definições de status possíveis de acordo com os valores mapeados
 */
#define ATTENTION 0 //Indica pouco risco de enchete, com o nível do rio um pouco acima do normal.
#define ALERT 1 //Indica existência do risco de enchete
#define DANGER 2 //Indica risco de Inundação
#define SAFE 3 //Sem risco de enchete ou inundação

/**
 * Definições para uso do I2C
 */
#define I2C_PORT i2c1
#define I2C_SDA 14  
#define I2C_SCL 15 
#define address 0x3C 

QueueHandle_t xQueueJoystickData; //Definição da Fila para Valores do Joystick
QueueHandle_t xQueueModeData; //Definição da Fila para Valores do Modo de Operação

//Definição de Struct para guardar os valores lido pelo Joystick
typedef struct {
    uint32_t x; //Valor lido do Eixo X 
    uint32_t y; //Valor lido do Eixo Y
    float river; //Valor normalizado para o nível do rio
    float rain; //Valor normalizado para intensidade de chuva
}Joystick_data_t;

//Definição de Struct para guardar o tipo de operação atual
typedef struct 
{
    bool alertMode; //Define o modo de operação
    char *status; //Armazena o status Atual
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
            joystick.river = river_level + (river_level * (adc_y_value - 2048) / 2047);
        }else if (adc_y_value < 1800){
            //Indica que o nível do rio desceu; calcula o valor atual (pode diminuir até 0 metros)
            joystick.river = river_level - (river_level * (2048 - adc_y_value) / 2047);
        }else {
            joystick.river = river_level;
        }
    
        //Calcula a intensidade da chuva com base nos valores do eixo X
        joystick.rain = (intense_rain * adc_x_value) / 4095.0;

        //Envia os dados para a fila
        xQueueSend(xQueueJoystickData, &joystick, 0);
        //Gera um delay de 1s
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief Task que calcula o nível de perigo com base nos dados lidos
 * 
 * Mapeia e define o status atual e altera o modo de operação de acordo com o valor mapeado
 */
void vMapStatus()
{
    Joystick_data_t joystick;
    OperationMode_data_t mode;
    float river_level = 5.0;

    while (true){
        if(xQueueReceive(xQueueJoystickData, &joystick, portMAX_DELAY) == pdTRUE)
        {
            mode.status = (char *)malloc(10 * sizeof(char));

            if (joystick.river >= 9.0 || (joystick.river >= 7.0 && joystick.rain > 50.0))
            {
                strcpy(mode.status, "PERIGO");
            }else if ((joystick.river >= 7.0 && joystick.rain > 50.0) || (joystick.river > river_level && joystick.rain > 50.0)){
                strcpy(mode.status, "ALERTA");
            }else if ((joystick.river > river_level && joystick.rain <= 50.0) || (joystick.river <= river_level && joystick.rain > 70.0)){
                strcpy(mode.status, "ATENCAO");
            }else {
                strcpy(mode.status, "SEGURO");
            }

            //Adiciona novamente os dados calculados na fila
            xQueueSend(xQueueJoystickData, &joystick, 0);

            //Verifica se o Modo de Alerta deve ser ativado
            if (joystick.river >= 7.0 || joystick.rain > 80.0) {
                mode.alertMode = true;
            }else {
                mode.alertMode = false;
            }
            
            //Salva na pilha 3 vezes para que possa ser acessado corretamente por cada task de periféricos
            for (int i = 0; i < 3; i++) xQueueSend(xQueueModeData, &mode, 0);
            
            vTaskDelay(pdMS_TO_TICKS(1500)); //Espera 1.5s
        }//End: queueReceive
    }
}

/**
 * @brief Task que exibe os alertas visuais (na matriz de LEDS e no LED RGB)
 */

/**
 * @brief Task que exibe os resultados de leitura no display SSD1306
 */
void vRealTimeInfo()
{
    ssd1306_t ssd;

    /**
     * Primeiro, realiza as configurações de I2C e Display SSD1306
     */
    i2c_init(I2C_PORT, 400*1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    //Configuração do display
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, address, I2C_PORT); 
    ssd1306_config(&ssd); 
    ssd1306_send_data(&ssd); 
    //Garante que o display inicialize com todos os pixels apagados    
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    OperationMode_data_t mode;
    Joystick_data_t joystick;
    bool cor = true;
    char *status;

    while (true)
    {
        if (xQueueReceive(xQueueModeData, &mode, portMAX_DELAY) == pdTRUE 
            && xQueueReceive(xQueueJoystickData, &joystick, portMAX_DELAY) == pdTRUE)
        {
            if (!mode.alertMode)
            {
                char level_river[20], rain_in[20];
                sprintf(level_river, "%.2f", joystick.river);
                sprintf(rain_in, "%.2f", joystick.rain);

                ssd1306_fill(&ssd, !cor);                          // Limpa o display
                ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);      // Desenha um retângulo
                ssd1306_line(&ssd, 3, 37, 123, 37, cor);           // Desenha uma linha
                ssd1306_draw_string(&ssd, "status: ", 20, 6);      // Linha 1: label  
                ssd1306_draw_string(&ssd, mode.status, 68, 6);     // Linha 1: Status atual 
                ssd1306_draw_string(&ssd, "N Rio: ", 20, 16);      // Linha 2: label
                ssd1306_draw_string(&ssd, level_river, 60, 16);    // Linha 2: nivel rio atual
                ssd1306_draw_string(&ssd, "Chuva", 13, 41);        // Desenha uma string
                ssd1306_draw_string(&ssd, rain_in, 60, 52);        // Desenha uma string
                ssd1306_send_data(&ssd);                           // Atualiza o display
            }else {
                ssd1306_fill(&ssd, !cor);                          // Limpa o display
                ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);      // Desenha um retângulo
                ssd1306_draw_string(&ssd, "RISCO ALTO", 10, 32);   // Desenha uma string
                ssd1306_send_data(&ssd);                           // Atualiza o display

                printf("R: %.2f\nC: %.2f\n", joystick.river, joystick.rain);
            }

            vTaskDelay(pdMS_TO_TICKS(500)); //Atualiza a cada 0.5s
        }//End: queueReceive
    }
}

int main()
{
    stdio_init_all();

    //Cria a fila para armazenar os valores do Joystick
    xQueueJoystickData = xQueueCreate(5, sizeof(Joystick_data_t));
    //Cria a fila para armazenar qual o tipo de operação atual
    xQueueModeData = xQueueCreate(5, sizeof(OperationMode_data_t));

    xTaskCreate(vReadJoystickValuesTask, "Read Joystick Task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vMapStatus, "Define Status Task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vRealTimeInfo, "Display Task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    vTaskStartScheduler();
    panic_unsupported();
}
