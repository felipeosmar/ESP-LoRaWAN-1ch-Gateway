# ESP32 Single Channel LoRaWAN Gateway

Um gateway LoRaWAN single-channel baseado em ESP32 com rádio SX1276/SX1278, compatível com ChirpStack e outros Network Servers LoRaWAN.

## Funcionalidades

- Recepção de pacotes LoRa em canal único
- Protocolo Semtech UDP Packet Forwarder v2
- Interface web para configuração
- Atualização de firmware OTA (Over-The-Air)
- Suporte a display OLED (Heltec, TTGO)
- Sincronização de tempo via NTP
- Gerenciador de arquivos integrado

## Hardware Suportado

| Placa | Status | Observações |
|-------|--------|-------------|
| Heltec WiFi LoRa 32 V2 | ✅ Testado | Display OLED integrado |
| TTGO LoRa32 V1 | ✅ Suportado | Display OLED integrado |
| ESP32 + SX1276/SX1278 | ✅ Suportado | Conexão SPI manual |

### Pinagem Padrão (Heltec V2)

| Função | GPIO |
|--------|------|
| MISO | 19 |
| MOSI | 23 |
| SCK | 18 |
| NSS (CS) | 14 |
| RST | 33 |
| DIO0 | 32 |
| OLED SDA | 4 |
| OLED SCL | 15 |
| OLED RST | 16 |
| Vext | 21 |

## Instalação

### Pré-requisitos

- [PlatformIO](https://platformio.org/) (CLI ou extensão VSCode)
- Python 3.x
- Cabo USB para conexão com ESP32

### Compilação e Upload

```bash
# Compilar o firmware
pio run -e jvtechgateway

# Upload do filesystem (interface web)
pio run -t uploadfs -e jvtechgateway

# Upload do firmware
pio run -t upload -e jvtechgateway

# Monitorar serial
pio device monitor -b 115200
```

## Configuração

### Primeira Inicialização

1. Na primeira inicialização, o gateway cria um ponto de acesso WiFi:
   - **SSID**: `LoRaGateway`
   - **Senha**: `lorawan123`

2. Conecte-se ao WiFi e acesse `http://192.168.4.1`

3. Configure sua rede WiFi na aba **WiFi**

4. Após reiniciar, acesse o gateway pelo IP obtido na sua rede

### Interface Web

A interface web está disponível em `http://<IP_DO_GATEWAY>/` e possui as seguintes abas:

| Aba | Descrição |
|-----|-----------|
| **Dashboard** | Status geral, estatísticas e log de atividades |
| **LoRa** | Configuração do rádio LoRa (frequência, SF, BW) |
| **Server** | Configuração do Network Server (ChirpStack) |
| **WiFi** | Configuração de rede WiFi |
| **NTP** | Configuração de sincronização de tempo |
| **Files** | Gerenciador de arquivos do filesystem |
| **System** | Informações do sistema e atualização OTA |

### Arquivo de Configuração

O arquivo `/config.json` armazena todas as configurações:

```json
{
  "wifi": {
    "ssid": "SuaRedeWiFi",
    "password": "SuaSenha",
    "ap_mode": false
  },
  "lora": {
    "enabled": true,
    "frequency": 916800000,
    "spreading_factor": 7,
    "bandwidth": 125.0,
    "coding_rate": 5,
    "tx_power": 14,
    "sync_word": 52
  },
  "server": {
    "enabled": true,
    "host": "10.1.82.18",
    "port_up": 1700,
    "port_down": 1700,
    "description": "ESP32 1ch Gateway",
    "region": "AU915",
    "latitude": -26.26381,
    "longitude": -48.85947,
    "altitude": 19
  },
  "ntp": {
    "enabled": true,
    "server1": "pool.ntp.org",
    "server2": "time.google.com",
    "timezone_offset": -10800,
    "daylight_offset": 0,
    "sync_interval": 3600000
  }
}
```

## Regiões LoRaWAN

### Região Recomendada para o Brasil: AU915

A ANATEL regulamenta as frequências ISM no Brasil na faixa de **915-928 MHz**.

| Região | Faixa de Frequência | Status no Brasil |
|--------|---------------------|------------------|
| **AU915** | 915-928 MHz | ✅ **Recomendada** - Totalmente dentro da faixa permitida |
| US915 | 902-928 MHz | ⚠️ Parcial - Frequências 902-915 MHz podem ter restrições |
| EU868 | 863-870 MHz | ❌ Não permitida no Brasil |

### Canais Recomendados (AU915 Subband 2)

| Canal | Frequência | Uso |
|-------|------------|-----|
| 8 | 916.8 MHz | Uplink |
| 9 | 917.0 MHz | Uplink |
| 10 | 917.2 MHz | Uplink |
| 11 | 917.4 MHz | Uplink |
| 12 | 917.6 MHz | Uplink |
| 13 | 917.8 MHz | Uplink |
| 14 | 918.0 MHz | Uplink |
| 15 | 918.2 MHz | Uplink |
| 65 | 917.5 MHz | Downlink (500kHz BW) |

## Sync Word (Palavra de Sincronização)

O **Sync Word** é um valor usado pelo rádio LoRa para identificar e filtrar pacotes na camada física.

```
Pacote LoRa:
┌──────────┬───────────┬─────────┬─────────┐
│ Preâmbulo│ Sync Word │ Header  │ Payload │
└──────────┴───────────┴─────────┴─────────┘
```

### Valores Padrão

| Sync Word | Valor | Uso |
|-----------|-------|-----|
| **0x34** | 52 decimal | LoRaWAN Público (TTN, ChirpStack, etc.) |
| **0x12** | 18 decimal | LoRa Privado (ponto-a-ponto) |

> **Importante**: Para compatibilidade com redes LoRaWAN, mantenha o sync word em **0x34**.

Dispositivos com sync words diferentes **não se comunicam**, mesmo estando na mesma frequência.

## Versão do LoRaWAN MAC

Este gateway utiliza o **Semtech UDP Packet Forwarder Protocol v2** e **não implementa** o LoRaWAN MAC stack.

```
[Sensor LoRa] ──(rádio)──► [Gateway] ──(UDP)──► [Network Server]
                              │
                   Apenas repassa pacotes
                   Não processa MAC layer
```

O LoRaWAN MAC é implementado em:
- **End Device (sensor)** - lado do dispositivo
- **Network Server (ChirpStack)** - lado do servidor

Portanto, qualquer versão de LoRaWAN MAC (1.0.x, 1.1.x) pode funcionar, desde que o sensor e o Network Server suportem.

## Limitações do Gateway Single-Channel

Por ser um gateway de canal único, existem limitações importantes:

| Funcionalidade | Status | Observação |
|----------------|--------|------------|
| **Class A** | ⚠️ Parcial | Uplinks funcionam, downlinks limitados |
| **Class B** | ❌ Não funciona | Requer sincronização de beacon |
| **Class C** | ⚠️ Muito limitado | Requer múltiplos canais |
| **OTAA** | ⚠️ Problemático | Join Accept pode ser perdido |
| **ABP** | ✅ Recomendado | Mais confiável em single-channel |
| **ADR** | ❌ Não funciona | Network server não vê todos os canais |
| **Frequency Hopping** | ❌ Não suporta | Escuta apenas 1 canal |

### Recomendações para Melhor Funcionamento

1. **Use ABP** (Activation By Personalization) em vez de OTAA
2. **Desabilite ADR** nos dispositivos finais
3. **Configure SF fixo** nos dispositivos (mesmo SF do gateway)
4. **Use o mesmo canal** que o gateway está configurado
5. Este gateway é ideal para **testes, desenvolvimento e ambientes controlados**

## Integração com ChirpStack

### Configuração do Gateway no ChirpStack

1. Acesse a interface web do ChirpStack
2. Vá em **Gateways** → **Add gateway**
3. Configure:
   - **Gateway ID**: EUI do gateway (mostrado no dashboard)
   - **Name**: Nome descritivo
   - **Region**: AU915 (para Brasil)

### Configuração do ChirpStack Gateway Bridge

O ChirpStack Gateway Bridge recebe os pacotes UDP do gateway. Certifique-se de que:

1. O serviço está rodando na porta **1700** (UDP)
2. A região está configurada corretamente

```toml
# /etc/chirpstack-gateway-bridge/chirpstack-gateway-bridge.toml
[backend.semtech_udp]
udp_bind = "0.0.0.0:1700"
```

## API REST

O gateway disponibiliza uma API REST para integração:

| Endpoint | Método | Descrição |
|----------|--------|-----------|
| `/api/status` | GET | Status geral do gateway |
| `/api/stats` | GET | Estatísticas detalhadas |
| `/api/stats/reset` | POST | Resetar estatísticas |
| `/api/lora/config` | GET/POST | Configuração LoRa |
| `/api/server/config` | GET/POST | Configuração do servidor |
| `/api/wifi/config` | GET/POST | Configuração WiFi |
| `/api/wifi/scan` | GET | Scan de redes WiFi |
| `/api/ntp/config` | GET/POST | Configuração NTP |
| `/api/ntp/sync` | POST | Forçar sincronização NTP |
| `/api/files/list` | GET | Listar arquivos |
| `/api/files/upload` | POST | Upload de arquivo |
| `/api/files/download` | GET | Download de arquivo |
| `/api/files/delete` | POST | Deletar arquivo |
| `/api/ota` | POST | Upload de firmware |
| `/api/restart` | POST | Reiniciar dispositivo |

### Exemplo de Uso

```bash
# Obter status
curl http://192.168.1.100/api/status

# Obter configuração do servidor
curl http://192.168.1.100/api/server/config

# Forçar sincronização NTP
curl -X POST http://192.168.1.100/api/ntp/sync
```

## Estrutura do Projeto

```
ESP-LoRaWAN-1ch-Gateway/
├── src/
│   ├── main.cpp           # Ponto de entrada principal
│   ├── config.h           # Definições e constantes
│   ├── lora_gateway.h/cpp # Gerenciamento do rádio LoRa
│   ├── udp_forwarder.h/cpp# Protocolo Semtech UDP
│   ├── web_server.h/cpp   # Servidor web e API REST
│   ├── oled_manager.h/cpp # Gerenciamento do display OLED
│   └── ntp_manager.h/cpp  # Sincronização NTP
├── data/
│   ├── config.json        # Arquivo de configuração
│   └── web/
│       ├── index.html     # Interface web
│       ├── style.css      # Estilos CSS
│       └── app.js         # JavaScript da interface
├── platformio.ini         # Configuração do PlatformIO
└── README.md              # Esta documentação
```

## Troubleshooting

### Gateway não aparece no ChirpStack ("Never seen")

1. **Verifique a conectividade**:
   ```bash
   ping <IP_DO_CHIRPSTACK>
   nc -vz <IP_DO_CHIRPSTACK> 1700
   ```

2. **Verifique os logs do ChirpStack Gateway Bridge**:
   ```bash
   journalctl -u chirpstack-gateway-bridge -f
   ```

3. **Verifique se o NTP está sincronizado** (timestamp incorreto causa erros)

4. **Verifique a região configurada** no gateway e no ChirpStack

### Rádio LoRa não inicializa

1. Verifique as conexões SPI
2. Confira a pinagem no `config.h` ou `config.json`
3. Verifique se o Vext está habilitado (placas Heltec)

### Pacotes não são recebidos

1. Verifique se a **frequência** do gateway corresponde à do sensor
2. Verifique se o **Spreading Factor** é compatível
3. Verifique se o **Sync Word** está correto (0x34 para LoRaWAN)
4. Verifique a **região** configurada

## Licença

Este projeto é disponibilizado sob a licença MIT.

## Contribuições

Contribuições são bem-vindas! Sinta-se à vontade para abrir issues e pull requests.

## Referências

- [LoRa Alliance](https://lora-alliance.org/)
- [ChirpStack Documentation](https://www.chirpstack.io/docs/)
- [Semtech UDP Packet Forwarder Protocol](https://github.com/Lora-net/packet_forwarder)
- [RadioLib Library](https://github.com/jgromes/RadioLib)
- [ANATEL - Regulamentação ISM](https://www.gov.br/anatel/)
