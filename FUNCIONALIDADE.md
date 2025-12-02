
  ┌─────────────────────────────────────────────────────────────────────────────────────────┐
  │                        GATEWAY LoRa JVTECH v4.1 - DIAGRAMA DE BLOCOS                    │
  │                                    (João Vitor Alvares)                                 │
  └─────────────────────────────────────────────────────────────────────────────────────────┘

                                      ┌─────────────────┐
                                      │   ALIMENTAÇÃO   │
                                      │    EXTERNA      │
                                      │   (7-24V DC)    │
                                      └────────┬────────┘
                                               │
                                               ▼
  ┌─────────────────────────────────────────────────────────────────────────────────────────┐
  │                              FONTE DE ALIMENTAÇÃO                                        │
  │  ┌──────────────┐         ┌──────────────┐         ┌──────────────┐                     │
  │  │  Conector DC │────────▶│   LM2596S    │────────▶│   SPX3819    │                     │
  │  │  (7-24V)     │         │  Buck Conv.  │         │  Regulador   │                     │
  │  └──────────────┘         │  24V → 5V    │         │  5V → 3.3V   │                     │
  │                           └──────┬───────┘         └──────┬───────┘                     │
  │                                  │ 5V                     │ 3.3V                        │
  └──────────────────────────────────┼─────────────────────────┼────────────────────────────┘
                                     │                         │
           ┌─────────────────────────┴─────────────────────────┴──────────────────┐
           │                                                                       │
           ▼                                                                       ▼
  ┌─────────────────────────────────────────────────────────────────────────────────────────┐
  │                           MICROCONTROLADORES (CÉREBROS)                                 │
  │                                                                                          │
  │   ┌─────────────────────────────────┐      ┌─────────────────────────────────┐          │
  │   │      ESP32-WROOM-32D            │      │       ATmega328P                │          │
  │   │      (Processador Principal)    │      │       (Processador Auxiliar)    │          │
  │   │                                 │      │                                 │          │
  │   │  • WiFi integrado               │◀────▶│  • Controle de periféricos      │          │
  │   │  • Bluetooth                    │ SPI  │  • Comunicação RS-485           │          │
  │   │  • Controle geral do sistema    │      │  • Backup de comunicação        │          │
  │   │  • Comunicação LoRa             │      │                                 │          │
  │   └───────────┬─────────────────────┘      └─────────────────────────────────┘          │
  │               │                                                                          │
  └───────────────┼──────────────────────────────────────────────────────────────────────────┘
                  │
      ┌───────────┴───────────┬─────────────────┬─────────────────┬─────────────────┐
      │                       │                 │                 │                 │
      ▼                       ▼                 ▼                 ▼                 ▼
  ┌────────────┐      ┌────────────┐     ┌────────────┐    ┌────────────┐    ┌────────────┐
  │   LoRa     │      │  Ethernet  │     │    GPS     │    │  Cartão SD │    │   USB      │
  │   RFM95W   │      │   W5500    │     │  L80-M39   │    │  TF-01A    │    │  CP2102    │
  │            │      │            │     │            │    │            │    │            │
  │ • 915 MHz  │      │ • RJ-45    │     │ • Posição  │    │ • Logs     │    │ • Program. │
  │ • Longo    │      │ • 10/100   │     │ • Hora     │    │ • Config   │    │ • Debug    │
  │   alcance  │      │   Mbps     │     │   precisa  │    │ • Dados    │    │ • Serial   │
  │ • IoT      │      │ • Internet │     │            │    │            │    │            │
  └─────┬──────┘      └─────┬──────┘     └────────────┘    └────────────┘    └─────┬──────┘
        │                   │                                                       │
        ▼                   ▼                                                       ▼
  ┌────────────┐      ┌────────────┐                                          ┌────────────┐
  │  Antena    │      │ Conector   │                                          │ Conector   │
  │  U.FL      │      │   RJ-45    │                                          │   USB      │
  │  (LoRa)    │      │ (Rede)     │                                          │ (PC)       │
  └────────────┘      └────────────┘                                          └────────────┘

  ┌─────────────────────────────────────────────────────────────────────────────────────────┐
  │                              PERIFÉRICOS E INTERFACE                                     │
  │                                                                                          │
  │   ┌─────────────┐   ┌─────────────┐   ┌─────────────┐   ┌─────────────┐                 │
  │   │   Display   │   │    RTC      │   │   Buzzer    │   │   Botão     │                 │
  │   │   OLED      │   │  DS1307Z    │   │             │   │             │                 │
  │   │             │   │             │   │             │   │             │                 │
  │   │ • Status    │   │ • Relógio   │   │ • Alarmes   │   │ • Reset     │                 │
  │   │ • Info      │   │ • Bateria   │   │ • Avisos    │   │ • Config    │                 │
  │   │ • I2C       │   │ • Backup    │   │ • Sons      │   │             │                 │
  │   └─────────────┘   └─────────────┘   └─────────────┘   └─────────────┘                 │
  │                                                                                          │
  │   ┌─────────────┐   ┌─────────────┐                                                     │
  │   │  LED Debug  │   │  LED Debug  │   ← Indicadores de status                           │
  │   │     01      │   │     02      │                                                     │
  │   └─────────────┘   └─────────────┘                                                     │
  └─────────────────────────────────────────────────────────────────────────────────────────┘


  ═══════════════════════════════════════════════════════════════════════════════════════════
                                   CONEXÕES PRINCIPAIS
  ═══════════════════════════════════════════════════════════════════════════════════════════

    BARRAMENTO I2C (2 fios):
    ├── Display OLED
    ├── RTC DS1307Z
    └── (Expansível para outros sensores)

    BARRAMENTO SPI #1 (ESP32):
    ├── Módulo LoRa RFM95W
    └── Cartão SD

    BARRAMENTO SPI #2 (ATmega328P):
    └── Chip Ethernet W5500

    SERIAL/UART:
    ├── GPS L80-M39 ←→ ESP32
    ├── ESP32 ←→ ATmega328P (comunicação entre MCUs)
    └── USB CP2102 ←→ ESP32/ATmega (programação)


  ═══════════════════════════════════════════════════════════════════════════════════════════
                                RESUMO DAS FUNCIONALIDADES
  ═══════════════════════════════════════════════════════════════════════════════════════════

    📡 COMUNICAÇÃO:
       • LoRa 915MHz - Recebe dados de sensores remotos (longo alcance)
       • WiFi/Bluetooth - Via ESP32 (configuração e dados locais)
       • Ethernet - Conexão cabeada com internet
       • USB - Programação e debug

    📍 LOCALIZAÇÃO:
       • GPS L80-M39 - Posição geográfica e hora UTC precisa

    💾 ARMAZENAMENTO:
       • Cartão SD - Logs, configurações e dados

    ⏰ TEMPO:
       • RTC DS1307Z com bateria backup - Mantém hora mesmo sem energia

    🖥️ INTERFACE:
       • Display OLED - Mostra status e informações
       • LEDs - Indicadores visuais de debug
       • Buzzer - Alertas sonoros
       • Botão - Interação do usuário

  ═══════════════════════════════════════════════════════════════════════════════════════════

  Explicação Simplificada

  Este é um Gateway LoRa - um dispositivo que funciona como "ponte" entre:
  - Sensores remotos (que enviam dados via rádio LoRa)
  - Internet (via WiFi ou cabo Ethernet)

  Fluxo de Dados:

  1. 📻 Sensores enviam dados via LoRa (rádio de longo alcance)
  2. 🧠 O ESP32 recebe e processa os dados
  3. 🌐 Envia para a internet via WiFi ou Ethernet
  4. 💾 Salva backup no cartão SD
  5. 🖥️ Mostra status no display OLED

  Alimentação:

  - Entrada: 7-24V DC (fonte externa)
  - Internamente converte para 5V e 3.3V para os componentes