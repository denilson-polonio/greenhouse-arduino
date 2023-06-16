#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <TimeLib.h>
#include <DHT.h>






// INIZIO IMPOSTAZIONI OBBLIGATORIE (per un corretto funzionamento)

// CONSIGLIO: Per compilare l'ora imposta circa 10 secondi prima di copilarlo
const int CODE_HOUR = 17;                    // Ora di quando viene compilato il codice nell'arduino
const int CODE_MINUTE = 38;                  // Minuti di quando viene compilato il codice nell'arduino
const int CODE_SECOND = 00;                  // Secondi di quando viene compilato il codice nell'arduino

const int CODE_DAY = 14;                     // Giorno di quando viene compilato il codice nell'arduino
const int CODE_MONTH = 06;                   // Mese di quando viene compilato il codice nell'arduino
const int CODE_YEAR = 2023;                  // Anno di quando viene compilato il codice nell'arduino

// FINE IMPOSTAZIONI OBBLIGATORIE






// INIZIO IMPOSTAZIONI BASE

const int SOIL_MOISTURE_THRESHOLD = 20;      // Se il valore dell'igrometro scende sotto a questa soglia, 
                                             // da la possibilità alla pompa di avviarsi, rispettando il 
                                             // ritardo impostato (percentuale)
int PUMP_DURATION_SECONDS = 7;               // Durata di accensione della pompa (in secondi)

const float PUMP_DELAY = 10.30;              // Imposta un ritardo tra ogni irrigazione (in ore e minuti, esempio: 10.30)
                                             // (Opzioni consigliata per chi non ha un'igrometro di ottima qualità)

// !!! ATTENZIONE !!!
// Fin dall'avvio ci sarà il ritardo della pompa,
// quindi la prima irrigazione avverrà dopo l'ora impostata su "PUMP_DELAY",
// se ovviamente l'igrometro e sotto alla soglia impostata in "SOIL_MOISTURE_THRESHOLD"

const float TEMPERATURE_THRESHOLD = 32.00;   // Soglia di temperatura per prolungare la durata della pompa (in gradi Celsius)
const int EXTRA_PUMP_DURATION_SECONDS = 5;   // Tempo da aggiungere alla durata della pompa se la temperatura supera la soglia (in secondi)
                                             // [SE NON VUOI IMPOSTARE NESSUNA VARIABILE DELLA POMPA IMPOSTA 0]

const float HUMIDITY_THRESHOLD = 80.0;       // Soglia di umidità per attivare la ventola (percentuale)
int fanMaxDurationHours = 2;                 // Tempo massimo giornaliero per la ventola (in ore)
                                             // (Opzione consigliata per chi ha una ventola di non ottima qualità)

// FINE IMPOSTAZIONI BASE






// INIZIO IMPOSTAZIONI AVANZATE

const int LCD_ADDRESS = 0x27;                // Pin del modulo LCD I2C
const int LCD_ROWS = 4;                      // Numero di righe
const int LCD_COLS = 20;                     // Numero di casella per riga

const int DHT_PIN = 2;                       // Pin del sensore di temperatura/umidità DHT11
const int PUMP_RELAY_PIN = 3;                // Pin del relè per la pompa di irrigazione
const int WATER_SENSOR_PIN = A0;             // Pin del sensore di livello dell'acqua
const int HUMIDITY_RELAY_PIN = 4;            // Pin del relè per il controllo dell'umidità alta
const int SOIL_MOISTURE_PIN = A1;            // Pin del sensore dell'igrometro

// MODIFICA LE IMPOSTAZZIONI DEL SENSORE DI LIVELLO DELL'ACQUA IN BASE ALLA POSIZZIONE DEL SENSORE STESSO
const int WATER_HIGH_THRESHOLD = 56;         // Soglia di quando vuoi che sia mostato il 100% dell'acqua.
                                             // Questo valore non è obbligatorio, ma se si vuole impostare un proprio valore
                                             // è consigliato modificarlo: per esempio ho fatto sì che il 100% dell'acqua venga
                                             // mostrato a circa la metà del sensore dell'acqua (percentuale).
                                             // [SE NON HAI PREFERENZE IMPOSTA IL VALORE 100]
const int WATER_0_THRESHOLD = -50;           // Soglia di quanto mostrare quando l'acqua arriva allo 0%.
                                             // E' possibile impostare un valore personalizzato quando l'acqua
                                             // arriva allo 0% (percentuale).
                                             // [SE NON HAI PREFERENZE IMPOSTA IL VALORE 0]
const int WATER_LOW_THRESHOLD = 20;          // Soglia di livello dell'acqua basso. (percentuale)
                                             // [CONSIGLIO UN VALORE CHE VADA DA 5 A 30]

// FINE IMPOSTAZIONI AVANZATE






LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);

DHT dht(DHT_PIN, DHT11); 

time_t lastIrrigationTime;
unsigned long irrigationInterval = (unsigned long)(PUMP_DELAY * 3600);
String lastIrrigationTimeString;

unsigned long fanElapsedTime = 0;
int fanMaxDurationMinutes = fanMaxDurationHours * 60;

boolean pumpStarted = false;
boolean humidityRelayActivated = false;






void setup() {
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  pinMode(HUMIDITY_RELAY_PIN, OUTPUT);
  pinMode(SOIL_MOISTURE_PIN, INPUT);

  dht.begin();
  digitalWrite(PUMP_RELAY_PIN, LOW);
  digitalWrite(HUMIDITY_RELAY_PIN, LOW);

  analogReference(DEFAULT);

  setTime(CODE_HOUR, CODE_MINUTE, CODE_SECOND, CODE_DAY, CODE_MONTH, CODE_YEAR);

  lcd.begin();
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("  Serra Automatica");
  lcd.setCursor(0, 2);
  lcd.print("   Versione: 1.0");
  delay(3000);

  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("      Creata da     ");
  lcd.setCursor(0, 2);
  lcd.print("  Denilson Polonio  ");
  delay(3000);

  lcd.clear();

  lastIrrigationTime = now();

  fanElapsedTime += millis() / 1000;
}






void loop() {
  int currentHour = hour();
  int currentMinute = minute();

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  time_t currentTime = now();
  unsigned long elapsedSeconds = (unsigned long)(currentTime - lastIrrigationTime);

  time_t fanStartTime = now();

  int soilMoisture = analogRead(SOIL_MOISTURE_PIN);
  int soilMoisturePercentage = 0;
  soilMoisturePercentage = map (soilMoisture, 100, 990, 100, 0);






  if (soilMoisturePercentage < SOIL_MOISTURE_THRESHOLD) {
      if (elapsedSeconds >= irrigationInterval) {
        lastIrrigationTime = currentTime;
        lastIrrigationTimeString = String(hour()) + ":" + String(minute()) + ":" + String(second());

      int waterLevel = analogRead(WATER_SENSOR_PIN);
      int waterPercentage = map(waterLevel, 0, 1023, 0, 100);

      digitalWrite(PUMP_RELAY_PIN, HIGH);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("IRRIGAZIONE | ");
      lcd.print(currentHour);
      lcd.print(":");
      lcd.print(currentMinute);

      if (waterPercentage < WATER_LOW_THRESHOLD) {
        lcd.setCursor(0, 1);
        lcd.print("Attenzione:");
        lcd.setCursor(0, 2);
        lcd.print("Acqua in esaurimento");
      }

      int pumpDuration = PUMP_DURATION_SECONDS;
      if (temperature > TEMPERATURE_THRESHOLD) {
        pumpDuration += EXTRA_PUMP_DURATION_SECONDS;
      }

      int secondsRemaining = pumpDuration;

      while (secondsRemaining > 0) {
        lcd.setCursor(0, 3);
        lcd.print("Tempo rimanente: ");
        lcd.print(secondsRemaining);
        lcd.print("s ");
        delay(1000);
        secondsRemaining--;
      }

      digitalWrite(PUMP_RELAY_PIN, LOW);
      pumpStarted = true;

      lastIrrigationTime = currentTime;
      
    }
  } else {
    pumpStarted = false;
  }






 if (humidity >= HUMIDITY_THRESHOLD && fanElapsedTime < (fanMaxDurationHours * 3600)) {
    if (!humidityRelayActivated) {
      digitalWrite(HUMIDITY_RELAY_PIN, HIGH);
      humidityRelayActivated = true;
    }
  } else {
    digitalWrite(HUMIDITY_RELAY_PIN, LOW);
    humidityRelayActivated = false;
  }






  int waterLevel = analogRead(WATER_SENSOR_PIN);
  int waterPercentage = map(waterLevel, 0, 1023, 0, 100);

  if (waterPercentage > WATER_HIGH_THRESHOLD) {
    waterPercentage = 100;
  } else if (waterPercentage == 0) {
    waterPercentage = WATER_0_THRESHOLD;
  }






  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(currentHour);
  lcd.print(":");
  lcd.print(currentMinute);
  lcd.print(":");
  lcd.print(second());
  lcd.print(" | ");

  lcd.print(lastIrrigationTimeString);

  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print("C");
  if(temperature > TEMPERATURE_THRESHOLD) {
    lcd.print(" +");
    lcd.print(EXTRA_PUMP_DURATION_SECONDS);
    lcd.print("s");
  }

  lcd.setCursor(0, 2);
  lcd.print("Umid: ");
  lcd.print(humidity);
  lcd.print("%");
  if(humidityRelayActivated == true) {
    lcd.print(" FAN");
  }

  lcd.setCursor(0, 3);
  lcd.print("Acqua: ");
  lcd.print(waterPercentage);
  lcd.print("% | T ");
  lcd.print(soilMoisturePercentage);
  lcd.print("%");

  delay(1000);
}
