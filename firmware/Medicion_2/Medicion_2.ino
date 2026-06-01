#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_Sensor.h>

Adafruit_MPU6050 mpu;
Adafruit_BMP280 bmp;

// Variables de calibración
float presion_base = 1013.25;
float offset_X = 0.0, offset_Y = 0.0, offset_Z = 0.0;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("\n--- Inicializando Aviónica con Calibración ---");

  if (!mpu.begin(0x68)) {
    Serial.println("¡Error MPU6050!");
    while (1);
  }
  if (!bmp.begin(0x76)) {
    Serial.println("¡Error BMP280!");
    while (1);
  }

  // Configuración de rangos y filtros
  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,
                  Adafruit_BMP280::SAMPLING_X16,
                  Adafruit_BMP280::FILTER_X16,
                  Adafruit_BMP280::STANDBY_MS_1);

  // --- PROCESO DE CALIBRACIÓN (Dejar la placa quieta) ---
  Serial.println("Calibrando sensores... NO MOVER LA PLACA");
  delay(1000); // Esperar a que se estabilice

  // 1. Tomar presión base para Altitud = 0 metros
  float suma_presion = 0;
  for(int i=0; i<20; i++) {
    suma_presion += bmp.readPressure() / 100.0F;
    delay(30);
  }
  presion_base = suma_presion / 20.0; // Promedio

  // 2. Tomar offsets del acelerómetro en reposo
  sensors_event_t a, g, temp;
  float suma_X = 0, suma_Y = 0, suma_Z = 0;
  for(int i=0; i<20; i++) {
    mpu.getEvent(&a, &g, &temp);
    suma_X += a.acceleration.x;
    suma_Y += a.acceleration.y;
    suma_Z += a.acceleration.z;
    delay(30);
  }
  
  // Guardamos las desviaciones. Al eje X (vertical) le restamos 9.81 
  // para capturar solo el error neto del chip aislado de la gravedad.
  offset_X = (suma_X / 20.0) - 9.81; 
  offset_Y = (suma_Y / 20.0);
  offset_Z = (suma_Z / 20.0);

  Serial.println("¡Calibración completada con éxito!");
  Serial.print("Presión base registrada: "); Serial.print(presion_base); Serial.println(" hPa");
  Serial.println("----------------------------------------------");
}

void loop() {
  sensors_event_t a, g, temp_mpu;
  mpu.getEvent(&a, &g, &temp_mpu);

  // Aplicar calibración de aceleración
  // Restamos el offset físico para limpiar el ruido del sensor
  float acc_x_calibrada = a.acceleration.x - offset_X; 
  float acc_y_calibrada = a.acceleration.y - offset_Y;
  float acc_z_calibrada = a.acceleration.z - offset_Z;

  // Calculamos la aceleración neta restando la gravedad (9.81) del eje vertical (X)
  // De esta forma, quietos en el suelo marcará 0 m/s², y al despegar marcará solo el empuje puro.
  float empuje_neto = acc_x_calibrada - 9.81;

  // Calcular altitud relativa usando la presión base de tu escritorio
  float presion = bmp.readPressure() / 100.0F;
  float altitud_relativa = bmp.readAltitude(presion_base); 

  // --- Mostrar datos ---
  Serial.print("Empuje_Neto(X):"); Serial.print(empuje_neto); Serial.print(" m/s² | ");
  Serial.print("Lat_Y:"); Serial.print(acc_y_calibrada); Serial.print(" m/s² | ");
  Serial.print("Lat_Z:"); Serial.print(acc_z_calibrada); Serial.print(" m/s²");
  Serial.print("  ||  ");
  Serial.print("Altitud_Relativa:"); Serial.print(altitud_relativa); Serial.println(" m");

  delay(100);
}