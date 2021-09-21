
#include <EEPROM.h>
//#include <WiFiUdp.h>
//#include <ArduinoOTA.h>
//#include <ESP8266mDNS.h>
#include <Arduino.h>
//#include <ArduinoJson.h>
//#include <FS.h>

#include <radiopult.h>
//#include "nRF24L01.h"
//#include "RF24.h"

RF24 radio(4, 5);

uint8_t pip[6];            // труба для связи присылает пульт
uint8_t pip0[6] = "setup"; // труба для настройки
String name = "Кораблик";
#define net_mod 1 // режим радиомодулей wifi или nrf24

//WiFiUDP udp;

//WiFiServer serverTCP(8888);

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
//const char auth[] = "AbOA2HM_PeZ9PAE5_z--k3KimAzNP6k1";

const char *ssid = "-Ship Kater";  //Имя точки доступа WIFI
const char *password = "11111111"; //пароль точки доступа WIFI

//IPAddress local_IP(192, 168, 4, 1); // не определнн IP адрес
//IPAddress gateway(192, 168, 4, 1);
//IPAddress subnet(255, 255, 255, 0);
//IPAddress dns(8, 8, 8, 8); // dfdf

void read_eprom();
void write_eprom();
void search_pult();

struct read_data
{
  byte x = 0;
  byte y = 0;
  byte gaz = 0;
  byte rul = 0;
  byte key1 = 00;
};
struct _menu
{
  String name;
  byte ch;
};

void ReadData(String data);

char ip[255];
int power = 10; // мощность двигателя - газ
byte rul = 50;  // угол поворота
byte np;
int rul_c = 50;
int gaz = 0;
//byte net_mod = 0;  // режим радиомодулей 0 - wifi  1 - nrf24
byte set_ch = 255; // канал связи для nrf24

read_data paket;

void servo(byte port, int t = 50)
{
  t = map(t, 0, 100, 540, 2400);
  digitalWrite(port, 1);
  delayMicroseconds(t);
  digitalWrite(port, 0);
  delayMicroseconds(20000 - t);
  //delay(5);
}

void setup()
{
  Serial.begin(115200);
  EEPROM.begin(10);
  read_eprom();

  /*
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D8, OUTPUT);
  pinMode(D7, OUTPUT);
*/
  //WiFi.mode(WIFI_AP);
  //WiFi.softAP(ssid, password);

  /*
  while (WiFi.status() != WL_CONNECTED)
  {
    // Serial.print(".");
    delay(1000);
  }
  */
  if (net_mod == 1)
  {
    search_pult();
  }

  //serverTCP.begin();
  // Serial.println(WiFi.softAPIP());

  //udp.begin(4220);

  // ArduinoOTA.setPort(8266);

  //ArduinoOTA.setPassword((const char *)"0000"); // Задаем пароль доступа для удаленной прошивки
  //ArduinoOTA.begin(); // Инициализируем OTA
  //  Blynk.begin(auth, ssid, password);
}

void loop()
{
  static byte pm = 0;
  //power = 0;
  String msg;
  if (radio.available())
  {
    radio.read(&msg, 32);
    Serial.println(msg);
  }

  //Serial.println( radio.isPVariant());

  //delay(5);
  int packetSize = 1;
  if (packetSize)
  {
    //Serial.printf("Received %d bytes from %s, port %d\n", packetSize, udp.remoteIP().toString().c_str(), udp.remotePort());
    //  "Получено %d байт от %s, порт %d%"

    ReadData(ip); // разложить пакет по каналам
    rul = paket.rul;
    np = paket.key1;
    power = paket.gaz;

    if (paket.gaz == 0) // для запска двигателей отключить газ
      pm = 1;
  }
  else // если нет пакетов от пульта
  {
    pm = 0;
    //if (WiFi.softAPgetStationNum() > 0)
    //      ESP.restart();
  }

  if (pm == 0)
    power = 0;

  if (rul_c > rul)
    rul_c -= 2;
  if (rul_c < rul)
    rul_c += 2;

  rul_c = constrain(rul_c, 0, 100);

  if (power == 0)
  {
    gaz = 0;
  }
  if (gaz > power)
    gaz -= 2;
  if (gaz < power)
    gaz += 2;
  gaz = constrain(gaz, 0, 100);
  /*
  servo(D5, rul_c);
  digitalWrite(D6, (np == 1));
  digitalWrite(D8, (np != 1));
  analogWrite(D7, map(gaz, 0, 100, 0, 1023));
  */
  //Serial.println(String(paket.gaz) + " , " + String(paket.rul));
  //Serial.println(String(gaz) + "," + String(rul) + "," + String(power) + "," + String(0) + "," + String(np) + ",");

  delay(25);
}

void ReadData(String data_s) // разбор пакета данных по каналам
{
  String data;
  byte canal[10];
  byte n = 0;
  data_s.trim();
  //Serial.println(data);
  while (data_s != "")
  {
    data = data_s.substring(0, data_s.indexOf(","));
    data_s = data_s.substring(data_s.indexOf(",") + 1);
    //  порядок данных в протоколе передачи с пульта
    //  gaz, rul, x, y, key1
    canal[n] = data.toInt();
    n++;
  }

  //Serial.println(canal[0]);

  paket.gaz = canal[0];
  paket.rul = canal[1];
  paket.x = canal[2];
  paket.y = canal[3];
  paket.key1 = canal[4];
}

void search_pult() // поиск каналоа и ожидание подключения
{
  radio.begin(); // Инициируем работу модуля nRF24L01+.
  radio.setAutoAck(true);
  radio.setDataRate(RF24_1MBPS);  // Указываем скорость передачи данных (RF24_250KBPS, RF24_1MBPS, RF24_2MBPS), RF24_1MBPS - 1Мбит/сек.
  radio.setPALevel(RF24_PA_HIGH); // Указываем мощность передатчика (RF24_PA_MIN=-18dBm, RF24_PA_LOW=-12dBm, RF24_PA_HIGH=-6dBm, RF24_PA_MAX=0dBm).
  radio.openReadingPipe(1, pip0);
  radio.powerUp();

  byte ch = 2;
  while (ch <= 125)
  {
    int scanal = 0;
    int i = 100;
    //radio.flush_tx();
    pinMode(39, OUTPUT);
    while (i--)
    {
      radio.setChannel(ch);
      radio.startListening();
      delayMicroseconds(200);
      radio.stopListening();
      if (radio.testCarrier())
        scanal++;
    }
    Serial.println(String(ch) + " - " + String(scanal));

    if (scanal < 2) // нашел чистый канал
    {
      break;
    }
    ch++;
  }
  Serial.println(ch);
  char s[20] = "#Машинка2 "; // буфер ответа
  radio.setAutoAck(true);
  //radio.setRetries(1, 15);
  radio.enableAckPayload();
  radio.openReadingPipe(1, pip0);
  radio.startListening(); // включаем прослушку
  //radio.openWritingPipe(pip0);
  //radio.setChannel(ch); // устанавливаем очередной канал работы модуля
  radio.writeAckPayload(1, &s, sizeof(s)); // Помещаем данные всего массива ackData в буфер FIFO для их отправки на следующее получение данных от передатчика на 1 трубе.
  Serial.println("Поиск пульта");

dd:

  if (radio.available())
  {
    // Если в буфере приёма имеются принятые данные от передатчика, то ...
    char msg[32] = "";
    radio.read(&msg, sizeof(msg));           // Читаем данные из буфера приёма в массив myData указывая сколько всего байт может поместиться в массив.
    radio.writeAckPayload(1, &s, sizeof(s)); // Помещаем данные всего массива ackData в буфер FIFO для их отправки на следующее получение данных от передатчика на 1 трубе.

    //Serial.println(String(radio.getChannel()) + " - " + msg);

    if (String(msg).startsWith("#pip#"))
    {
      pip[0] = msg[5];
      pip[1] = msg[6];
      pip[2] = msg[7];
      pip[3] = msg[8];
      pip[4] = msg[9];
      set_ch = radio.getChannel();
      radio.openReadingPipe(1, pip);
      //Сохранить данные в памяти
      write_eprom();
      return;
    }
  }
  //delay(3);

  goto dd;
}

void read_eprom()
{
  set_ch = EEPROM.read(1); //  канал связи по NRF24
  pip[0] = EEPROM.read(2);
  pip[1] = EEPROM.read(3);
  pip[2] = EEPROM.read(4);
  pip[3] = EEPROM.read(5);
  pip[4] = EEPROM.read(6);
}
void write_eprom()
{
  EEPROM.write(1, set_ch);
  EEPROM.write(2, pip[0]);
  EEPROM.write(3, pip[1]);
  EEPROM.write(4, pip[2]);
  EEPROM.write(5, pip[3]);
  EEPROM.write(6, pip[4]);

  EEPROM.commit();
}