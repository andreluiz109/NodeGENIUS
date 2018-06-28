#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Prototipo das funcoes
void IntCallback();
void setup_wifi();
void reconnect();
void callback(char *topic, byte *payload, unsigned int length);

// Definição de estados
#define ESPERA 0
#define INICIO_JOGO 1
#define GERAR_SEQUENCIA 2
#define ESPERA_USUARIO 3
#define COMPARA_SEQUENCIA 4
#define MODO_MQTT 5
#define MODO_OFFLINE 6

// Definição de variáveis globais e constantes
#define MAX_ROADADAS 5
volatile unsigned int estado_atual = 0;
unsigned int rodada_atual = 0;
unsigned int contador = 0;
unsigned int seq_maquina[MAX_ROADADAS];
unsigned int seq_jogador[MAX_ROADADAS];
int sorteio = -1;
unsigned int modo = MODO_OFFLINE;

// Definicoes para usar MQTT
const char *mqtt_server = "test.mosquitto.org"; // Servidor Broker MQTT
WiFiClient espClient;                           // Instancia uma conexao para wifi
PubSubClient client(espClient);                 // Instancia clente MQTT
char msg[50];// define o tamanho da msg a ser enviada
long lastMsg = 0; // contador de tempo
int value = 0;// incremento para publicar msg

// Definição de IO
#define LED_UM 16    //D0
#define LED_DOIS 5   //D1
#define LED_TRES 4   //D2
#define LED_QUATRO 0 //D3
//------------------------------------
#define BOTAO_UM 14     //D5
#define BOTAO_DOIS 12   //D6
#define BOTAO_TRES 13   //D7
#define BOTAO_QUATRO 15 //D8
#define BOTAO_START 2   //D4

void setup()
{
    //-------------Saindas---------------------
    Serial.begin(115200);
    randomSeed(analogRead(A0));
    pinMode(LED_UM, OUTPUT);
    pinMode(LED_DOIS, OUTPUT);
    pinMode(LED_TRES, OUTPUT);
    pinMode(LED_QUATRO, OUTPUT);
    //--------------Entradas--------------------
    pinMode(BOTAO_UM, INPUT);
    pinMode(BOTAO_DOIS, INPUT);
    pinMode(BOTAO_TRES, INPUT);
    pinMode(BOTAO_QUATRO, INPUT);
    pinMode(BOTAO_START, INPUT);
    // interrupcao para comecar outra roda so funciona no modo offline 
    attachInterrupt(digitalPinToInterrupt(BOTAO_START), IntCallback, FALLING);
    // configuracoes iniciais para o MQTT
    client.setServer(mqtt_server, 1883);// conexao com o broker
    client.setCallback(callback);// chama a funcao de desvio quando recebe uma msg de mqtt
    // Boot no modo online
    while (digitalRead(BOTAO_UM))// se o botao um estiver precionado no momento do boot entra em modo online
    {
        modo = MODO_MQTT;
    }
}

void loop()
{
    switch (modo)// confere o modo a depender do boot e encaminha para o modo Offline ou Online
    {
    case MODO_OFFLINE:

        switch (estado_atual)
        {
        case ESPERA:
            digitalWrite(LED_UM, HIGH);
            digitalWrite(LED_QUATRO, HIGH);
            delay(20);
            digitalWrite(LED_UM, LOW);
            digitalWrite(LED_QUATRO, LOW);
            delay(200);
            digitalWrite(LED_DOIS, HIGH);
            digitalWrite(LED_TRES, HIGH);
            delay(20);
            digitalWrite(LED_DOIS, LOW);
            digitalWrite(LED_TRES, LOW);
            delay(200);
            break;
        case INICIO_JOGO:
            rodada_atual = 0;
            for (int i = 0; i <= 4; i++)
            {
                seq_maquina[i] = 0;
                seq_jogador[i] = 0;
            }
            estado_atual = GERAR_SEQUENCIA;
            delay(500);
            break;
        case GERAR_SEQUENCIA:
            sorteio = random(5);
            if (sorteio == 0)
            {
                seq_maquina[rodada_atual] = LED_UM;
            }
            else if (sorteio == 1)
            {
                seq_maquina[rodada_atual] = LED_DOIS;
            }
            else if (sorteio == 3)
            {
                seq_maquina[rodada_atual] = LED_TRES;
            }
            else if (sorteio == 4)
            {
                seq_maquina[rodada_atual] = LED_QUATRO;
            }
            for (int i = 0; i <= rodada_atual; i++)
            {
                digitalWrite(seq_maquina[i], HIGH);
                delay(500);
                digitalWrite(seq_maquina[i], LOW);
                delay(500);
            }
            estado_atual = ESPERA_USUARIO;
            break;
        case ESPERA_USUARIO:
            if (contador <= rodada_atual)
            {
                int flag_botao = 0;
                while (digitalRead(BOTAO_UM))
                {
                    digitalWrite(LED_UM, HIGH);
                    delay(20);
                    seq_jogador[rodada_atual] = LED_UM;
                    flag_botao = 1;
                }
                while (digitalRead(BOTAO_DOIS))
                {
                    digitalWrite(LED_DOIS, HIGH);
                    delay(20);
                    seq_jogador[rodada_atual] = LED_DOIS;
                    flag_botao = 1;
                }
                while (digitalRead(BOTAO_TRES))
                {
                    digitalWrite(LED_TRES, HIGH);
                    delay(20);
                    seq_jogador[rodada_atual] = LED_TRES;
                    flag_botao = 1;
                }
                while (digitalRead(BOTAO_QUATRO))
                {
                    digitalWrite(LED_QUATRO, HIGH);
                    delay(20);
                    seq_jogador[rodada_atual] = LED_QUATRO;
                    flag_botao = 1;
                }
                if (flag_botao)
                {
                    contador++;
                    digitalWrite(LED_UM, LOW);
                    digitalWrite(LED_DOIS, LOW);
                    digitalWrite(LED_TRES, LOW);
                    digitalWrite(LED_QUATRO, LOW);
                    delay(50);
                }
            }
            if (contador > rodada_atual)
            {
                contador = 0;
                delay(1000);
                estado_atual = COMPARA_SEQUENCIA;
            }
            break;
        case COMPARA_SEQUENCIA:
            int errou = 0;
            for (int i = 0; i <= rodada_atual; i++)
            {
                if (seq_jogador[i] != seq_maquina[i])
                {
                    // Informa que errou, perdeu
                    digitalWrite(LED_DOIS, HIGH);
                    delay(100);
                    digitalWrite(LED_DOIS, LOW);
                    delay(100);
                    digitalWrite(LED_DOIS, HIGH);
                    delay(100);
                    digitalWrite(LED_DOIS, LOW);
                    delay(100);
                    digitalWrite(LED_DOIS, HIGH);
                    delay(100);
                    digitalWrite(LED_DOIS, LOW);
                    delay(100);
                    digitalWrite(LED_DOIS, HIGH);
                    delay(100);
                    digitalWrite(LED_DOIS, LOW);
                    delay(100);

                    errou = 1;
                }
            }
            if (errou)
            {
                estado_atual = ESPERA;
            }
            else
            {
                rodada_atual++;
                if (rodada_atual >= MAX_ROADADAS)
                {
                    // Informa que usuário ganhou o jogo
                    digitalWrite(LED_UM, HIGH);
                    delay(100);
                    digitalWrite(LED_UM, LOW);
                    delay(100);
                    digitalWrite(LED_UM, HIGH);
                    delay(100);
                    digitalWrite(LED_UM, LOW);
                    delay(100);
                    digitalWrite(LED_UM, HIGH);
                    delay(100);
                    digitalWrite(LED_UM, LOW);
                    delay(100);
                    digitalWrite(LED_UM, HIGH);
                    delay(100);
                    digitalWrite(LED_UM, LOW);
                    delay(100);

                    estado_atual = ESPERA;
                }
                else
                {
                    estado_atual = GERAR_SEQUENCIA;
                }
            }
            break;
        }
        break;
        //////////////////////////////      INTERNET     //////////////////////////////////////////////
    case MODO_MQTT:
        setup_wifi();
        // fica publicando uma msg via mqtt a cada 2 segundos no nodo genius/envia com "hello world #numero da msg "
        while (true)
        {
            if (!client.connected())
            {
                reconnect();
            }
            client.loop();
            long now = millis();
            if (now - lastMsg > 2000)
            {
                lastMsg = now;
                ++value;
                snprintf(msg, 75, "hello world #%ld", value);
                Serial.print("Publish message: ");
                Serial.println(msg);
                client.publish("genius/envia", msg);
            }
        }

        break;
    }
}
// função chamada quando a uma interrupcao no botão start, mas so e valida no modo offline 
void IntCallback()
{
    estado_atual = INICIO_JOGO;
}

void setup_wifi()
{
    delay(10);
    WiFi.begin("Luiz Andrade", "homenet01");
    while (WiFi.status() != WL_CONNECTED)
    { // espera a conexao
        digitalWrite(LED_QUATRO, HIGH);
        delay(100);
        digitalWrite(LED_QUATRO, LOW);
        delay(100);
    }
    digitalWrite(LED_QUATRO, LOW);
}

void reconnect()
{
    while (!client.connected())// fazer se o cliente estiver desconectado
    {
        if (client.connect("GENIUS_Andre"))// se conectar então publica e assina node MQTT
        {
            client.publish("genius/envia", "hello world");
            client.subscribe("genius/recebe");
        }
        else// se nao aguarda 5 seguntos e tenta novamente a conexao
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

// funcao que e chamada caso tenha alguma msg recebida via mqtt
void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message recebida [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println();
    if ((char)payload[0] == '1')
    {
        digitalWrite(LED_UM, HIGH);// liga led quando recebe 1 via MQTT
    }
    else
    {
        digitalWrite(LED_UM, LOW); // desliga led quando recebe 0 via MQTT
    }
}