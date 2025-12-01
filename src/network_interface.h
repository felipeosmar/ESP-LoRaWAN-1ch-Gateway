/**
 * @file network_interface.h
 * @brief Interface abstrata para comunicacao de rede (WiFi ou Ethernet)
 *
 * Gateway LoRa JVTECH v4.1
 *
 * Esta interface permite que o UDP Forwarder use tanto WiFi nativo
 * quanto Ethernet via ATmega Bridge de forma transparente.
 */

#ifndef NETWORK_INTERFACE_H
#define NETWORK_INTERFACE_H

#include <Arduino.h>
#include <IPAddress.h>

/**
 * @enum NetworkType
 * @brief Tipos de interface de rede disponiveis
 */
enum class NetworkType {
    NONE,
    WIFI,
    ETHERNET
};

/**
 * @enum NetworkStatus
 * @brief Status da conexao de rede
 */
enum class NetworkStatus {
    DISCONNECTED,       // Sem conexao
    CONNECTING,         // Tentando conectar
    CONNECTED,          // Conectado com IP
    LINK_DOWN,          // Cabo desconectado (apenas Ethernet)
    ERROR               // Erro de inicializacao
};

/**
 * @struct NetworkInfo
 * @brief Informacoes sobre a interface de rede
 */
struct NetworkInfo {
    NetworkType type;
    NetworkStatus status;
    IPAddress ip;
    IPAddress gateway;
    IPAddress subnet;
    IPAddress dns;
    uint8_t mac[6];
    int8_t rssi;            // Apenas para WiFi (-dBm)
    bool linkUp;            // Status do link fisico
    uint32_t connectedTime; // Tempo conectado (ms)
};

/**
 * @class NetworkInterface
 * @brief Interface abstrata para comunicacao de rede
 *
 * Esta classe define a interface comum que deve ser implementada
 * por adaptadores WiFi e Ethernet.
 */
class NetworkInterface {
public:
    virtual ~NetworkInterface() {}

    // ================== Inicializacao ==================

    /**
     * @brief Inicializar a interface de rede
     * @return true se inicializacao bem sucedida
     */
    virtual bool begin() = 0;

    /**
     * @brief Parar a interface de rede
     */
    virtual void end() = 0;

    /**
     * @brief Atualizar estado da interface (chamar no loop)
     */
    virtual void update() = 0;

    // ================== Status ==================

    /**
     * @brief Verificar se esta conectado com IP valido
     * @return true se conectado
     */
    virtual bool isConnected() = 0;

    /**
     * @brief Verificar status do link fisico
     * @return true se link ativo
     */
    virtual bool isLinkUp() = 0;

    /**
     * @brief Obter status atual
     * @return Status da conexao
     */
    virtual NetworkStatus getStatus() = 0;

    /**
     * @brief Obter tipo da interface
     * @return Tipo (WiFi ou Ethernet)
     */
    virtual NetworkType getType() = 0;

    /**
     * @brief Obter nome da interface
     * @return String com nome ("WiFi" ou "Ethernet")
     */
    virtual const char* getName() = 0;

    /**
     * @brief Obter informacoes completas
     * @param info Estrutura para receber as informacoes
     */
    virtual void getInfo(NetworkInfo& info) = 0;

    // ================== Configuracao IP ==================

    /**
     * @brief Obter endereco IP local
     * @return Endereco IP
     */
    virtual IPAddress localIP() = 0;

    /**
     * @brief Obter endereco MAC
     * @param mac Buffer de 6 bytes para o MAC
     */
    virtual void macAddress(uint8_t* mac) = 0;

    /**
     * @brief Obter endereco MAC como String
     * @return String formatada AA:BB:CC:DD:EE:FF
     */
    virtual String getMacAddress() {
        uint8_t mac[6];
        macAddress(mac);
        char str[18];
        sprintf(str, "%02X:%02X:%02X:%02X:%02X:%02X",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return String(str);
    }

    /**
     * @brief Obter endereco IP do gateway
     * @return Endereco IP do gateway
     */
    virtual IPAddress gatewayIP() = 0;

    /**
     * @brief Obter RSSI (somente WiFi)
     * @return RSSI em dBm ou 0 se nao aplicavel
     */
    virtual int8_t getRSSI() { return 0; }

    /**
     * @brief Obter SSID (somente WiFi)
     * @return Nome da rede ou vazio
     */
    virtual String getSSID() { return ""; }

    // ================== UDP ==================

    /**
     * @brief Iniciar socket UDP em uma porta local
     * @param port Porta local
     * @return true se sucesso
     */
    virtual bool udpBegin(uint16_t port) = 0;

    /**
     * @brief Fechar socket UDP
     */
    virtual void udpStop() = 0;

    /**
     * @brief Iniciar pacote UDP para envio
     * @param ip IP destino
     * @param port Porta destino
     * @return true se sucesso
     */
    virtual bool udpBeginPacket(IPAddress ip, uint16_t port) = 0;

    /**
     * @brief Iniciar pacote UDP para envio (hostname)
     * @param host Hostname destino
     * @param port Porta destino
     * @return true se sucesso
     */
    virtual bool udpBeginPacket(const char* host, uint16_t port) = 0;

    /**
     * @brief Escrever dados no pacote UDP
     * @param buffer Dados
     * @param size Tamanho
     * @return Bytes escritos
     */
    virtual size_t udpWrite(const uint8_t* buffer, size_t size) = 0;

    /**
     * @brief Finalizar e enviar pacote UDP
     * @return true se enviado com sucesso
     */
    virtual bool udpEndPacket() = 0;

    /**
     * @brief Verificar se ha pacote UDP disponivel
     * @return Tamanho do pacote ou 0
     */
    virtual int udpParsePacket() = 0;

    /**
     * @brief Ler dados do pacote UDP recebido
     * @param buffer Buffer para os dados
     * @param maxSize Tamanho maximo
     * @return Bytes lidos
     */
    virtual int udpRead(uint8_t* buffer, size_t maxSize) = 0;

    /**
     * @brief Obter IP remoto do ultimo pacote
     * @return IP remoto
     */
    virtual IPAddress udpRemoteIP() = 0;

    /**
     * @brief Obter porta remota do ultimo pacote
     * @return Porta remota
     */
    virtual uint16_t udpRemotePort() = 0;

    // ================== DNS ==================

    /**
     * @brief Resolver hostname para IP
     * @param host Hostname
     * @param result IP resolvido
     * @return true se resolvido
     */
    virtual bool hostByName(const char* host, IPAddress& result) = 0;
};

#endif // NETWORK_INTERFACE_H
