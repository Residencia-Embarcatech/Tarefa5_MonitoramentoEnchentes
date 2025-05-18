#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "pio_matrix.pio.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define JOYSTICK_X 26 //Eixo x do Joystick, simula o sensor de chuva
#define JOYSTICK_Y 27 //Eixo y do Joystick, simula o sensor ultrassônico para medir o nível do rio

/**
 * Definições para uso do I2C
 */
#define I2C_PORT i2c1
#define I2C_SDA 14  
#define I2C_SCL 15 
#define address 0x3C 

#define MATRIX 7 //Pino GPIO da matriz de LEDS
#define RED_LED 13 //Pino GPIO do Led Vermelho
#define BUZZER 10// Pino GPIO do Buzzer 

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
        //Gera um delay de 0.5s
        vTaskDelay(pdMS_TO_TICKS(500));
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
            
            vTaskDelay(pdMS_TO_TICKS(500)); //Espera 0.5s
        }//End: queueReceive
    }
}

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

                // Limpa o display (preenche com cor inversa)
                ssd1306_fill(&ssd, !cor);                           
                // Moldura externa
                ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);        
                // Linha abaixo do título "STATUS"
                ssd1306_line(&ssd, 3, 14, 122, 14, cor);             
                // Linha abaixo da palavra "PERIGO"
                ssd1306_line(&ssd, 3, 30, 122, 30, cor);             
                // Linha horizontal separando as duas linhas da "tabela"
                ssd1306_line(&ssd, 3, 45, 122, 45, cor);             
                // Linha vertical da tabela, separando letra e número
                ssd1306_line(&ssd, 25, 30, 25, 60, cor);             
                // Texto no topo (status)
                ssd1306_draw_string(&ssd, "status", 45, 5);          
                // Palavra que indica o status atual
                ssd1306_draw_string(&ssd, mode.status, 35, 18);      
                // Primeira linha da tabela: R e Valor do nível
                ssd1306_draw_string(&ssd, "R", 10, 34);              
                ssd1306_draw_string(&ssd, level_river, 30, 34);      
                // Segunda linha da tabela: C e valor da intensidade de chuva
                ssd1306_draw_string(&ssd, "C", 10, 49);              
                ssd1306_draw_string(&ssd, rain_in, 30, 49);          
                // Atualiza o display
                ssd1306_send_data(&ssd);
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

/**
 * @brief Função auxiliar para task vAlertModeTask()
 */
uint32_t matrix_rgb(double r, double g, double b)
{
    unsigned char R = (unsigned char)(r * 255);
    unsigned char G = (unsigned char)(g * 255);
    unsigned char B = (unsigned char)(b * 255);

    return (G << 24) | (R << 16) | (B << 8);
}

/**
 * @brief Task que exibe os alertas sonoros e visuais quando identifica o modo de alerta 
 */
void vAlertModeTask()
{
    /**
     * Inicialização da PIO para utilizar a matriz de LEDS
     */
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &pio_matrix_program);
    uint sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, MATRIX);

    /**
     * Inicializa e Configura o PWM para uso do buzzer 
     */
    gpio_set_function(BUZZER, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER);
    pwm_set_clkdiv(slice_num, 62.5f);
    pwm_set_wrap(slice_num, 1000);
    pwm_set_gpio_level(BUZZER, 500);
    pwm_set_enabled(slice_num, false);
    
    /**
     * Inicializa o LED RGB vermelho  
     */
    gpio_init(RED_LED);
    gpio_set_dir(RED_LED, GPIO_OUT);

    OperationMode_data_t mode;
    uint32_t led_value;

    //Array com Símbolo a ser desenhado na matriz
    const int frame[25] = {
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,0,0,0,
        0,0,1,0,0
    };
    
    while (true)
    {
        if (xQueueReceive(xQueueModeData, &mode, portMAX_DELAY) == pdTRUE)
        {
            if (mode.alertMode)
            {
                //Exibe o símbolo ! para indicar alerta visual
                for (int i = 0; i < 25; i++)
                {
                    if (frame[24-i] == 1) led_value = matrix_rgb(1.0,0.0,0.0);
                    else led_value = matrix_rgb(0.0,0.0,0.0);

                    pio_sm_put_blocking(pio, sm, led_value);
                }

                //Liga o LED RGB Vermelho
                gpio_put(RED_LED, true);

                //Ativa o Buzzer
                pwm_set_enabled(slice_num, true);
            }else {
                //Mantém a matriz de Leds apagada caso não esteja no modo de alerta
                for (int i = 0; i < 25; i++) {
                    led_value = matrix_rgb(0.0,0.0,0.0);
                    pio_sm_put_blocking(pio, sm, led_value);
                }

                //Desliga o LED RGB Vermelho
                gpio_put(RED_LED, false);
                //Desativa o Buzzer
                pwm_set_enabled(slice_num, false);
                gpio_put(BUZZER, false); //Garante que o buzzer está em nível baixo
            }

            vTaskDelay(pdTICKS_TO_MS(500)); //Atualiza a cada 0.5s
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
    xTaskCreate(vAlertModeTask, "AlertMode Task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    vTaskStartScheduler();
    panic_unsupported();
}
