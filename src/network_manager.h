/**
 * @file network_manager.h
 * @brief Gerenciador de rede com failover automatico WiFi <-> Ethernet
 *
 * Gateway LoRa JVTECH v4.1
 *
 * Este modulo gerencia as interfaces de rede disponíveis (WiFi e Ethernet)
 * e implementa failover automatico quando a interface primaria falha.
 */

#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include "network_interface.h"
#include "wifi_adapter.h"
#include "ethernet_adapter.h"
#include <ArduinoJson.h>

// Forward declaration for UDPForwarder
class UDPForwarder;

// Configuracao padrao
#define NET_FAILOVER_TIMEOUT_DEFAULT   30000   // 30 segundos
#define NET_RECONNECT_INTERVAL_DEFAULT 10000   // 10 segundos
#define NET_STATUS_CHECK_INTERVAL      1000    // 1 segundo
#define NET_STABILITY_PERIOD_DEFAULT   60000   // 60 segundos

// Failover callback type
// Called when failover occurs: callback(fromInterface, toInterface)
typedef void (*FailoverCallback)(const char* fromInterface, const char* toInterface);

/**
 * @enum PrimaryInterface
 * @brief Interface primaria configurada
 */
enum class PrimaryInterface {
    WIFI,
    ETHERNET
};

/**
 * @struct NetworkManagerConfig
 * @brief Configuracao do gerenciador de rede
 */
struct NetworkManagerConfig {
    bool wifiEnabled;
    bool ethernetEnabled;
    PrimaryInterface primary;
    bool failoverEnabled;
    uint32_t failoverTimeout;    // Tempo para failover (ms)
    uint32_t reconnectInterval;  // Intervalo de tentativa de reconexao (ms)
    bool healthCheckEnabled;     // Usar health check baseado em ACK do ChirpStack
    uint32_t stabilityPeriod;    // Periodo de estabilidade antes de voltar para primaria (ms)
};

/**
 * @struct NetworkManagerStats
 * @brief Estatisticas do gerenciador de rede
 */
struct NetworkManagerStats {
    uint32_t wifiConnections;
    uint32_t wifiDisconnections;
    uint32_t ethernetConnections;
    uint32_t ethernetDisconnections;
    uint32_t failoverCount;
    uint32_t totalUptimeWifi;      // ms conectado via WiFi
    uint32_t totalUptimeEthernet;  // ms conectado via Ethernet
    uint32_t lastFailoverTime;
};

/**
 * @class NetworkManager
 * @brief Gerenciador de interfaces de rede com failover
 */
class NetworkManager {
public:
    /**
     * @brief Construtor
     * @param bridge Referencia para ATmega Bridge (usado pelo EthernetAdapter)
     */
    NetworkManager(ATmegaBridge& bridge);
    ~NetworkManager();

    // ================== Inicializacao ==================

    /**
     * @brief Inicializar gerenciador
     * @return true se pelo menos uma interface inicializou
     */
    bool begin();

    /**
     * @brief Atualizar estado (chamar no loop principal)
     */
    void update();

    // ================== Configuracao ==================

    /**
     * @brief Carregar configuracao do JSON
     * @param doc Documento JSON
     */
    void loadConfig(const JsonDocument& doc);

    /**
     * @brief Salvar configuracao
     * @return true se sucesso
     */
    bool saveConfig();

    /**
     * @brief Obter configuracao atual
     * @return Referencia para configuracao
     */
    NetworkManagerConfig& getConfig() { return _config; }

    /**
     * @brief Definir interface primaria
     * @param primary Interface primaria
     */
    void setPrimary(PrimaryInterface primary);

    /**
     * @brief Ativar/desativar failover
     * @param enabled Estado do failover
     */
    void setFailoverEnabled(bool enabled);

    /**
     * @brief Definir timeout de failover
     * @param timeoutMs Timeout em ms
     */
    void setFailoverTimeout(uint32_t timeoutMs);

    /**
     * @brief Definir referencia ao UDPForwarder para health checks
     * @param forwarder Ponteiro para UDPForwarder
     */
    void setUDPForwarder(UDPForwarder* forwarder) { _udpForwarder = forwarder; }

    /**
     * @brief Registrar callback para eventos de failover
     * @param callback Funcao a ser chamada quando failover ocorrer
     *
     * O callback recebe dois parametros:
     * - fromInterface: nome da interface de origem (ex: "WiFi", "Ethernet")
     * - toInterface: nome da interface de destino
     *
     * Usado para atualizar displays com notificacao de failover.
     */
    void setFailoverCallback(FailoverCallback callback) { _failoverCallback = callback; }

    // ================== Status ==================

    /**
     * @brief Verificar se ha conexao ativa
     * @return true se alguma interface esta conectada
     */
    bool isConnected();

    /**
     * @brief Obter interface ativa atual
     * @return Ponteiro para interface ativa ou nullptr
     */
    NetworkInterface* getActiveInterface();

    /**
     * @brief Obter tipo da interface ativa
     * @return Tipo da interface
     */
    NetworkType getActiveType();

    /**
     * @brief Obter interface WiFi
     * @return Ponteiro para WiFiAdapter
     */
    WiFiAdapter* getWiFi() { return &_wifi; }

    /**
     * @brief Obter interface Ethernet
     * @return Ponteiro para EthernetAdapter
     */
    EthernetAdapter* getEthernet() { return &_ethernet; }

    /**
     * @brief Obter estatisticas
     * @return Referencia para estatisticas
     */
    NetworkManagerStats& getStats() { return _stats; }

    /**
     * @brief Obter status em formato JSON
     * @return String JSON
     */
    String getStatusJson();

    /**
     * @brief Obter status de saude da rede em formato JSON
     * @return String JSON com: healthy, lastAckTime, failoverTimeout,
     *         failoverActive, stabilityPeriod, primaryStableFor
     *
     * Este metodo e usado pelo endpoint GET /api/network/health
     */
    String getHealthJson();

    /**
     * @brief Verificar se a conexao esta saudavel (baseado em ACKs do ChirpStack)
     * @return true se a conexao esta saudavel
     */
    bool isApplicationHealthy();

    // ================== Controle Manual ==================

    /**
     * @brief Forcar uso de interface especifica
     * @param type Tipo de interface
     * @return true se sucesso
     */
    bool forceInterface(NetworkType type);

    /**
     * @brief Voltar ao modo automatico
     */
    void setAutoMode();

    /**
     * @brief Forcar reconexao
     */
    void reconnect();

    // ================== UDP (via interface ativa) ==================

    /**
     * @brief Iniciar UDP na interface ativa
     * @param port Porta local
     * @return true se sucesso
     */
    bool udpBegin(uint16_t port);

    /**
     * @brief Parar UDP
     */
    void udpStop();

    /**
     * @brief Iniciar pacote UDP
     */
    bool udpBeginPacket(const char* host, uint16_t port);
    bool udpBeginPacket(IPAddress ip, uint16_t port);

    /**
     * @brief Escrever no pacote UDP
     */
    size_t udpWrite(const uint8_t* buffer, size_t size);

    /**
     * @brief Finalizar e enviar pacote
     */
    bool udpEndPacket();

    /**
     * @brief Verificar pacotes recebidos
     */
    int udpParsePacket();

    /**
     * @brief Ler pacote recebido
     */
    int udpRead(uint8_t* buffer, size_t maxSize);

    /**
     * @brief IP remoto do ultimo pacote
     */
    IPAddress udpRemoteIP();

    /**
     * @brief Porta remota do ultimo pacote
     */
    uint16_t udpRemotePort();

private:
    ATmegaBridge& _bridge;
    WiFiAdapter _wifi;
    EthernetAdapter _ethernet;

    NetworkManagerConfig _config;
    NetworkManagerStats _stats;

    // Referencia ao UDPForwarder para health checks
    UDPForwarder* _udpForwarder;

    // Callback para notificar failover (usado por displays)
    FailoverCallback _failoverCallback;

    // Estado atual
    NetworkInterface* _activeInterface;
    bool _manualMode;
    NetworkType _manualType;

    // Estado de conexao
    bool _wifiWasConnected;
    bool _ethernetWasConnected;

    // Timing
    uint32_t _lastStatusCheck;
    uint32_t _primaryDownSince;
    uint32_t _lastReconnectAttempt;
    bool _failoverActive;

    // Stability timer for return-to-primary
    uint32_t _primaryStableStart;

    // UDP
    uint16_t _udpPort;
    bool _udpStarted;

    // Metodos internos
    void updateInterfaces();
    void checkFailover();
    void switchToInterface(NetworkInterface* iface);
    void notifyFailover(const char* fromIface, const char* toIface);
    NetworkInterface* getPrimaryInterface();
    NetworkInterface* getSecondaryInterface();
    void updateStats();
    bool startUDP();
};

// Instancia global
extern NetworkManager* networkManager;

#endif // NETWORK_MANAGER_H
