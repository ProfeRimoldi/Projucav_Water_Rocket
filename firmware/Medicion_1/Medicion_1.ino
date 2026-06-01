#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_Sensor.h>

Adafruit_MPU6050 mpu;
Adafruit_BMP280 bmp; // Usará el bus I2C por defecto

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("\n--- Inicializando Aviónica del Cohete ---");

  // 1. Inicializar MPU6050 (0x68)
  if (!mpu.begin(0x68)) {
    Serial.println("¡Error! No se pudo encontrar el MPU6050. Revisa el cableado.");
    while (1) delay(10);
  }
  Serial.println("-> MPU6050 listo (Dirección 0x68)");

  // 2. Inicializar BMP280 (0x77)
  if (!bmp.begin(0x76)) {
    Serial.println("¡Error! No se pudo encontrar el BMP280. Revisa el cableado.");
    while (1) delay(10);
  }
  Serial.println("-> BMP280 listo (Dirección 0x77)");

  // Configuraciones óptimas para cohetería (Muestreo rápido)
  mpu.setAccelerometerRange(MPU6050_RANGE_16_G); // Rango de hasta 16G para soportar la aceleración inicial
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ); // Filtro digital para reducir ruido de vibraciones
  
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Modo de operación. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Presión oversampling (Mayor precisión de altitud) */
                  Adafruit_BMP280::FILTER_X16,      /* Filtro IIR para suavizar picos por viento */
                  Adafruit_BMP280::STANDBY_MS_1);   /* Tiempo de espera mínimo */
}

void loop() {
  // --- Lectura del MPU6050 ---
  sensors_event_t a, g, temp_mpu;
  mpu.getEvent(&a, &g, &temp_mpu);

  // --- Lectura del BMP280 ---
  float presion = bmp.readPressure() / 100.0F; // Convertimos a hPa
  float altitud = bmp.readAltitude(1013.25);   // Altitud basada en presión estándar al nivel del mar (ajustable)

  // --- Imprimir Datos en Consola ---
  Serial.print("AGX:"); Serial.print(a.acceleration.x); Serial.print(" m/s² | ");
  Serial.print("AGY:"); Serial.print(a.acceleration.y); Serial.print(" m/s² | ");
  Serial.print("AGZ (Empuje):"); Serial.print(a.acceleration.z); Serial.print(" m/s²");
  Serial.print("  ||  ");
  Serial.print("Presion:"); Serial.print(presion); Serial.print(" hPa | ");
  Serial.print("Altitud:"); Serial.print(altitud); Serial.println(" m");

  delay(100); // Muestreo cada 100ms para pruebas en banco. En vuelo será menor.
}