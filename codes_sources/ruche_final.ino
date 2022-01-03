#include <HX711.h>
#include <DHT.h>
#include <DHT_U.h>

#define MAXIMWIRE_EXTERNAL_PULLUP
#include <MaximWire.h>
#define PIN_BUS1 4
#define PIN_BUS2 8
#define PIN_LED 13

MaximWire::Bus bus1(PIN_BUS1);
MaximWire::DS18B20 device;
/*******ds18b20**********/
MaximWire::Bus bus2(PIN_BUS2);
/*********weight sensor*****************/
HX711 scale; /*weight sensor*/
uint8_t dataPin = 10;
uint8_t clockPin = 9;
float w1, w2 = 0;
/*Valeurs calibration*/
float offset1=133208.59; //59920.00;
float scale1=30066.24; //23332.56;

/****DHT22-1***/
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#define DHTPIN1 2     // Digital pin connected to the DHT sensor
DHT dht1(DHTPIN1, DHTTYPE); /*temperature and humidity sensor dht22*/

/****DHT22-2***/
#define DHTPIN2 3     // Digital pin connected to the DHT sensor
DHT dht2(DHTPIN2, DHTTYPE); /*temperature and humidity sensor dht22*/

/**********dht22 variables********************/
int temp_dht, hum_dht = 0;
int temp_dht2, hum_dht2;
/*Initialisation pin batterie*/
int analogPin = A4;
float battery_level = 0;

/*Initialisation pin luminosity*/
int luxPin = A0;
float lux = 0;

/**************functions************** */
void get_temp_ds18b20(int* tab1) {
  int i = 0;
  MaximWire::Discovery discovery = bus1.Discover();
  do {
    MaximWire::Address address;
    if (discovery.FindNextDevice(address)) {
        Serial.print(" (DS18B20)");
        MaximWire::DS18B20 device(address);
        float temp = device.GetTemperature<float>(bus1);
        tab1[i] = (int)temp;
        Serial.print(" temp=");
        Serial.print(temp);
        Serial.println();
        device.Update(bus1);
        i++;
    } else {
      Serial.println("NOTHING FOUND");
    }
  } while (discovery.HaveMore());
  delay(1000);
}

void get_temp(int* temp_out){
    MaximWire::Discovery discovery = bus2.Discover();
    MaximWire::Address address;
    discovery.FindNextDevice(address);         
    MaximWire::DS18B20 device(address);
    float temp = device.GetTemperature<float>(bus2);
    *temp_out = (int) temp;
    Serial.print(" (DS18B20)_2");    
    Serial.print(" temp_3=");
    Serial.print(temp);
    Serial.println();
    device.Update(bus2);
    delay(1000);
}

float get_weight() {
  // read until stable
  w1 = scale.get_units(10);
  delay(100);
  w2 = scale.get_units();
  while (abs(w1 - w2) > 10)
  {
    w1 = w2;
    w2 = scale.get_units();
    delay(100);
  }

  Serial.print("UNITS: ");
  Serial.println(w1);
  return w1;
  delay(5000);

}
/**battery*/
float battery_level_read() {
  int val = 0;
  float bat;
  analogReadResolution(10);
  val = analogRead(analogPin);
  bat = 0.457 * val - 360.79;
  return bat;
}

float luminosity_level_read(){
  int val=0;
  float lux;
  analogReadResolution(10);
  val= analogRead(luxPin);
  lux =100-(val *100/1024);
  Serial.print("luminosity value=");
  Serial.println(lux);
  return lux;    
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600); /*9600 bits per second or 960 bytes per second */
  Serial1.begin(9600);
  digitalWrite(LED_PWR, LOW);
  digitalWrite(PIN_LED, HIGH);
  scale.begin(dataPin, clockPin);
  scale.set_offset(offset1);
  scale.set_scale(scale1);
  delay(100);
  dht1.begin();
  delay(100);
  dht2.begin();
  delay(20000);
  digitalWrite(PIN_LED, LOW);
}
//envoyer les données qui sont en entrée(Faly :p ) et qui sont déja traitées
void envoyer(int poids, int Humidite_ext, int batterie, int Temp_ext, int Humidite_int, int Temp_int1,
             int Temp_int2, int Temp_int3,int lux) {
  //on utilise les 32 bits
  int variable = 0;
  unsigned int  message1 = 0;
  message1 = (poids & 0x3ff)<< 6; //decalage pour avoir de la place pour Humidité
  message1 = (message1 ^ (Humidite_ext & 0x3f)) << 7;
  message1 = (message1 ^ (batterie & 0x7f)) << 9;
  message1 = (message1 ^ (Temp_ext & 0x1ff));
  //on utilise 27/32 bits
  unsigned int message2 = 0;
  message2 = (message2 ^ (Humidite_int & 0x3f)) << 9; //6
  message2 = (message2 ^ (Temp_int1 & 0x1ff)) << 9; //9
  message2 = (message2 ^ (Temp_int2 & 0x1ff)) << 8; //9
  message2 = message2 ^ (variable & 0xff);

  //on utilise 27/32 bits
  unsigned int message3 = 0;
  message3 = (message3 ^ (Temp_int3 & 0x1ff)) << 7;
  message3 = (message3 ^ (lux & 0x7f)) << 8;
  message3 = (message3 ^ (variable & 0xff)) << 8;
  message3 = message3 ^ (variable & 0xff);
  String text = String("AT$SF=" + String(message1, HEX) + String(message2, HEX) + String(message3, HEX) + "\r\n");

  char buff[33];
  for (int i = 0; i < 33; i++) {
    buff[i] = '0';
  }
  text.toCharArray(buff, 33);
  Serial1.write(buff);
  for (int i = 0; i < 33; i++) {
    Serial.print(buff[i]);
  }
  Serial.println();
  delay(6000);
}
//lire les données des capteurs /traitement des données/ envoie de données à l'aide de fonction envoyer
void lire() {
  /////lire les données des capteurs
  int tab[2];
  int temp_3;
  // traitement de données
  int int_poids =100;
  
  if (get_weight() > 0) {
    int_poids = (int)(get_weight() * 10 + 100);
  }
  
  get_temp_ds18b20(tab);
  get_temp(&temp_3);
  int batterie=(int)battery_level_read();
  //int batterie=50;
  int int_Humidite_ext = (int)(dht1.readHumidity());
  int int_Temp_ext = (int)(dht1.readTemperature() * 10 + 100);
  int int_Humidite_int = (int)(dht2.readHumidity());
  int int_Temp_int1 = (int)(tab[0] * 10 + 100);
  int int_Temp_int2 = (int)(tab[1] * 10 + 100);
  int int_Temp_int3 = (int)( temp_3* 10 + 100);
  lux = (int)luminosity_level_read();
  //variable
  //envoyer les valeurs a l'aide d'une autre fonction envoyer 
  /*Serial.print("Hum_ext=");
  Serial.println(int_Humidite_ext);
  Serial.print("Temp_ext=");
  Serial.println(int_Temp_ext);
  Serial.print("Hum2=");
  Serial.println(int_Humidite_int);
  Serial.print("Poids=");
  Serial.println(int_poids);
  Serial.print("Batterie=");
  Serial.println(batterie);
  Serial.print("DS18B20_2=");
  Serial.println(int_Temp_int2);*/
  
  envoyer(int_poids, int_Humidite_ext, batterie, int_Temp_ext, int_Humidite_int, int_Temp_int1, int_Temp_int2, int_Temp_int3,lux);
  
}
void loop() {
  scale.power_up();
  delay(2000);
  lire();
  delay(5000);
  scale.power_down();
  delay(600000);//envoie toutes les 15min = 900 000
}
