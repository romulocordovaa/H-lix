//Bibliotecas importadas
#include <WiFi.h>
#include "ThingSpeak.h"
#include <OneWire.h>
#include <DallasTemperature.h>

//Definições das constantes
const float ADC_RES = 4096.0; // Resolução máxima do pino analógico
const float VREF = 5.0; // Voltagem de referência

const int TDS_SENSOR = 12; // pino do Sensor de tds
const int TEMP_SENSOR = 4; // pino do Sensor de temperatura
const int TURBIDEZ_SENSOR = 27; // pino do sensor de turbidez

const char* ssid = "123"; // nome da rede
const char* password = "romulo123";// senha da rede
WiFiClient client;
unsigned long myChannelNumber = 1; //número do canal
const char * myWriteAPIKey = "5OS2FHL2QZK8VK4Y"; //API Key


//Tempo de leitura 30s
unsigned long lastTime = 0;
unsigned long timerDelay = 30000;
 
//Definições das variáveis
float temperatura = 0.0; 
float tds = 0.0; 


//Instanciações das classes
OneWire oneWire(TEMP_SENSOR); // Instância do objeto oneWire
DallasTemperature sensores(&oneWire); // Instância do DallasTemperature

/**
 Função responsável por iniciar a configuração dos pinos e outros
*/
void setup() {
 //Configurando o modo de operação do pino
 pinMode(TDS_SENSOR, INPUT); //Configurando o pino do sensor TDS para modo entrada
 pinMode(TURBIDEZ_SENSOR, INPUT); //Configurando o pino do sensor PH paramodo entrada
 
 Serial.begin(115200); //inicializa comunicação serial
 sensores.begin(); //Inicializando os sensores
 
 WiFi.mode(WIFI_STA);
 ThingSpeak.begin(client); //inicializa o ThingSpeak
}

/**
 Função responsável por disparar a leitura de temperatura
*/
float lerTemperatura() {
float temp = 0.0;
sensores.requestTemperatures(); // Envia o comando para ler a temperatura
temp = sensores.getTempCByIndex(0); // Envia o comando para ler em graus celsius
delay(1000);
return temp;
}

/**
 Função responsável por disparar a leitura da tds
*/

float lerTds() {
 int SCOUNT = 30; // Contador de amostras
 int analogBuffer[SCOUNT]; // Armazena o valor analógio lido do ADC em um array
 int analogBufferTemp[SCOUNT];
 int analogBufferIndex = 0;
 int copyIndex = 0;
 float averageVoltage = 0;
 float tdsValue = 0;
 float temperature = 25;
 static unsigned long analogSampleTimepoint = millis();
 
 if (millis() - analogSampleTimepoint > 40U) { //A cada 40 millisegundos, leia o valor analógico vindo do ADC
 analogSampleTimepoint = millis();
 analogBuffer[analogBufferIndex] = analogRead(TDS_SENSOR); //Lendo o valor analógico e armazenando no buffer
 analogBufferIndex++;
 if (analogBufferIndex == SCOUNT)
 analogBufferIndex = 0;
 }
 
 static unsigned long printTimepoint = millis();
 
 if (millis() - printTimepoint > 800U) {
 printTimepoint = millis();
 for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++)
 analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
 averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / ADC_RES; // le o valor analógico mais estável pelo algoritmo de filtragem mediana e converta em valor de tensão
 float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0); //fórmula de compensação de temepratura: fFinalResult(25 ^ C) = fFinalResult(current) / (1.0 + 0.02 * (fTP - 25.0));
 float compensationVoltage = averageVoltage / compensationCoefficient; // compensação de temperatura
 tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage - 255.86 * compensationVoltage * compensationVoltage + 857.39 * compensationVoltage) * 0.5; //converter valor de tensão para valor tds
 }
 return tdsValue;
}

int getMedianNum(int bArray[], int iFilterLen)
{
 int bTab[iFilterLen];
 for (byte i = 0; i < iFilterLen; i++)
 bTab[i] = bArray[i];
 int i, j, bTemp;
 for (j = 0; j < iFilterLen - 1; j++)
 {
 for (i = 0; i < iFilterLen - j - 1; i++)
 {
 if (bTab[i] > bTab[i + 1])
 {
 bTemp = bTab[i];
 bTab[i] = bTab[i + 1];
 bTab[i + 1] = bTemp;
 }
 }
 }
 if ((iFilterLen & 1) > 0)
 bTemp = bTab[(iFilterLen - 1) / 2];
 else
 bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
 return bTemp;
}

void loop() {
  // Conecta ou reconecta o WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Atenção para conectar o SSID: ");
    Serial.println(ssid);
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, password);
      Serial.print(".");
      delay(5000);
    }
    Serial.println("\nConectado");
  }
  
 //Chama a rotina de leitura de temperatura
 temperatura = lerTemperatura();
 Serial.print("-------\n");
 Serial.printf("|Temperatura: %.2f\n", temperatura);

 
 //Chama a rotina de leitura de tds
 tds = lerTds();
 Serial.printf("|TDS: %.2f\n", tds);
  
 //Chama a rotina de leitura da turbidez
 int valorLido = analogRead(TURBIDEZ_SENSOR);
 float voltage = valorLido * (5.0 / 1024.0);
 int turbidity = map(valorLido, 0, 750, 100, 0);
 Serial.printf("NTU: %2f\n", turbidity);

 
 Serial.print("-------\n");
 delay(1000);

 if (millis() - lastTime > timerDelay) {

    // Configura os campos com os valores
    ThingSpeak.setField(1, temperatura);
    ThingSpeak.setField(2, tds);
    ThingSpeak.setField(3, turbidity);
    ThingSpeak.setField(4, voltage);

    // Escreve no canal do ThingSpeak 
    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if (x == 200) {
      Serial.println("Update realizado com sucesso");
    }
    else {
      Serial.println("Problema no canal - erro HTTP " + String(x));
    }

    lastTime = millis();
  }
  }
  
 
 
