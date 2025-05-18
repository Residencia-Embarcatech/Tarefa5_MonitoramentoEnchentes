# Sistema de Monitoramento de Enchentes em Rios com FreeRTOS

## Descrição do Projeto

Este projeto implementa um sistema embarcado para monitoramento do nível de rios, com objetivo de controlar e alertar sobre possíveis enchentes ou inundações. O sistema utiliza um joystick para simular:

- **Sensor ultrassônico (eixo Y)** → nível do rio  
- **Sensor de chuva (eixo X)** → intensidade da chuva  

Com base nessas medições, o firmware classifica o risco em quatro níveis — **SEGURO**, **ATENÇÃO**, **ALERTA** e **PERIGO** — e adota dois modos de operação:

1. **Modo Normal**  
   Exibe informações em tempo real no display SSD1306.

2. **Modo Alerta**  
   Exibe mensagem de risco no SSD1306, pisca LEDs (matriz 5×5 e LED RGB) e soa o buzzer, garantindo um alerta acessível (visual e sonoro).

A organização multitarefa é feita com FreeRTOS, usando filas para compartilhar variáveis entre as tasks.

---

## Funcionalidades

- Leitura do nível do rio (sensor ultrassônico simulado)  
- Leitura da intensidade da chuva (sensor de chuva simulado)  
- Exibição de status e alertas no display OLED SSD1306 via I2C  
- Alertas visuais em matriz de LEDs 5×5 e LED RGB  
- Alertas sonoros com buzzer  
- Classificação de risco em **SEGURO**, **ATENÇÃO**, **ALERTA** e **PERIGO**

---

## Hardware Utilizado

| Componente                  | Descrição                                     |
| --------------------------- | --------------------------------------------- |
| **Microcontrolador**        | Raspberry Pi Pico W                           |
| **Joystick**                | Simula sensores (X → chuva; Y → nível de rio) |
| **Display OLED SSD1306**    | Informações dos sensores em tempo real        |
| **Matriz de LEDs 5×5**      | Alerta visual                                 |
| **LED RGB**                 | Alerta visual                                 |
| **Buzzer**                  | Alerta sonoro                                 |

---

## Mapeamento de Pinos (GPIO)

| Função                      | GPIO |
| --------------------------- | ---- |
| Joystick X (chuva)          | 26   |
| Joystick Y (nível do rio)   | 27   |
| Matriz de LEDs 5×5          | 7    |
| I2C SDA (SSD1306)           | 14   |
| I2C SCL (SSD1306)           | 15   |
| LED RGB (vermelho)          | 13   |
| Buzzer                      | 10   |

---

## Compilação e Upload

1. **Pré-requisitos**:
   - Ter o ambiente de desenvolvimento para o Raspberry Pi Pico configurado (compilador, SDK, etc.).
   - CMake instalado.

2. **Compilação**:
   - Clone o repositório ou baixe os arquivos do projeto.
   - Navegue até a pasta do projeto e crie uma pasta de build:
     ```bash
     mkdir build
     cd build
     ```
   - Execute o CMake para configurar o projeto:
     ```bash
     cmake ..
     ```
   - Compile o projeto:
     ```bash
     make
     ```

3. **Upload para a placa**:
   - Conecte o Raspberry Pi Pico ao computador.
   - Copie o arquivo `.uf2` gerado para a placa.
