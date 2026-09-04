#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

Adafruit_BMP280 bmp;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("\n--- Iniciando prueba aislada de BMP280 ---");

  // Forzar pines I2C por defecto del ESP32 (SDA = 21, SCL = 22)
  Wire.begin(21, 22);

  // Intentar inicializar en 0x76, si falla probar 0x77
  if (!bmp.begin(0x76)) {
    Serial.println("Fallo en 0x76. Probando 0x77...");
    if (!bmp.begin(0x77)) {
      Serial.println("Error: No se encontró ningún BMP280. Revisa el cableado.");
      while (1) delay(10);
    } else {
      Serial.println("BMP280 encontrado en la dirección 0x77.");
    }
  } else {
    Serial.println("BMP280 encontrado en la dirección 0x76.");
    
  }

  // Configuración conservadora para prueba
  bmp.setSampling(Adafruit_BMP280::MODE_FORCED,
                  Adafruit_BMP280::SAMPLING_X2,
                  Adafruit_BMP280::SAMPLING_X16,
                  Adafruit_BMP280::FILTER_X16,
                  Adafruit_BMP280::STANDBY_MS_500);

  Serial.println("BMP280 inicializado. Imprimiendo datos...\n");
}

void loop() {
  // Forzar una nueva medición en cada ciclo
  if (bmp.takeForcedMeasurement()) {
    Serial.print("Presión RAW: ");
    Serial.print(bmp.readPressure());
    Serial.println(" Pa");
  } else {
    Serial.println("Error al tomar medición forzada.");
  }

  delay(500);
}
 /* if (bmp.takeForcedMeasurement()) {
  Serial.print("Presión RAW: ");
  Serial.println(bmp.readPressure());
}
 /* Serial.print("Temperatura: ");
  Serial.print(bmp.readTemperature());
  Serial.print(" °C | ");

  Serial.print("Presión RAW: ");
  Serial.print(bmp.readPressure());
  Serial.print(" Pa | ");

  Serial.print("Altitud Aprox: ");
  Serial.print(bmp.readAltitude(1013.25));
  Serial.println(" m");*/

  //delay(500);
//}