# Sensor IoT ESP8266 de Temperatura em Tempo Real

Este é um projeto de sensor IoT que mede a temperatura em tempo real utilizando um microcontrolador ESP8266. Ele é capaz de enviar as leituras de temperatura para um tópico MQTT hospedado em um serviço de ECS (Elastic Container Service) na AWS.

## Pré-requisitos

- Hardware:
  - Microcontrolador ESP8266
  - Sensor de temperatura (por exemplo, DS18B20)
  - Conexão à Internet

- Software:
  - Plataforma de desenvolvimento Arduino IDE
  - Biblioteca PubSubClient para comunicação MQTT no ESP8266 (disponível no Gerenciador de Bibliotecas do Arduino IDE)

## Configuração

1. Clone este repositório ou baixe o código-fonte para o seu ambiente de desenvolvimento.

2. Abra o arquivo `sensor_temp.ino` na Arduino IDE.

3. Configure o código-fonte com as credenciais do seu Wi-Fi e as configurações do servidor MQTT AWS ECS:
    ```cpp
    const char* ssid = "SEU_SSID_WIFI";
    const char* password = "SUA_SENHA_WIFI";
    const char* mqtt_server = "MQTT_SERVER_ENDPOINT"; // Por exemplo: "mqtt.example.com"
    const int mqtt_port = 1883; // Porta MQTT padrão
    ```

4. Compile e carregue o código no microcontrolador ESP8266.

## Uso

O sensor começará a ler a temperatura automaticamente após a inicialização. As leituras serão publicadas no tópico MQTT especificado no servidor GCE da Google Cloud Platform.

## Arquitetura AWS ECS

- Serviço ECS: Crie um cluster e um serviço ECS para hospedar o broker MQTT. Você pode usar um serviço como o Mosquitto para isso.

- Configuração do Tópico MQTT: Configure um tópico MQTT para receber as leituras de temperatura do sensor.

- Permissões IAM: Certifique-se de que o microcontrolador ESP8266 tenha permissões adequadas para publicar no tópico MQTT. Crie uma política IAM e atribua-a ao papel ou usuário IAM usado pelo ESP8266.
