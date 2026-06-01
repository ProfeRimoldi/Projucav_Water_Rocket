#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_Sensor.h>

// Pines LoRa según EMISOR.ino
#define SS 5
#define RST 2
#define DI0 17

Adafruit_MPU6050 mpu;
Adafruit_BMP280 bmp;

// Variables de calibración
float presion_base = 1013.25;
float offset_X = 0.0, offset_Y = 0.0, offset_Z = 0.0;
// Umbrales de cambio significativo (Ajustables)
const float UMBRAL_EMPUJE = 0.60;  // m/s² (Disparará el envío si varía más de 0.6)
const float UMBRAL_ALTITUD = 0.50; // metros (Disparará el envío si sube/baja más de 50cm)

// Historial para comparar
float ultimo_empuje_enviado = 0.0;
float ultima_altitud_enviada = 0.0;

// Tiempo máximo de respaldo (Heartbeat)
unsigned long ultimo_envio_tiempo = 0;
const unsigned long TIMEOUT_FORZADO = 2000; // Si pasan 2 segundos sin cambios, envía igual por seguridad

void setup() {
  Serial.begin(115200); // Subimos la velocidad para mejor rendimiento en telemetría
  while (!Serial);

  Serial.println("\n--- Inicializando Aviónica del Cohete + LoRa ---");

  // 1. Inicializar LoRa
  LoRa.setPins(SS, RST, DI0);
  if (!LoRa.begin(433E6)) {
    // Configuración de radio para asegurar enlace fuerte en banco de pruebas
    LoRa.setSignalBandwidth(125E3);    // 125 kHz estándar
    LoRa.setSpreadingFactor(7);        // SF7 por defecto (rápido)
    LoRa.setTxPower(17);               // Aumentamos la potencia de transmisión a 17dBm
    Serial.println("¡Fallo crítico en la inicialización de LoRa!");
    while (1);
  }
  Serial.println("-> Transmisor LoRa listo.");

  // 2. Inicializar Sensores I2C
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

  // --- PROCESO DE CALIBRACIÓN ---
  Serial.println("Calibrando sensores... NO MOVER LA PLACA");
  delay(1000); 

  // Tomar presión base (Altitud = 0)
  float suma_presion = 0;
  for(int i = 0; i < 20; i++) {
    suma_presion += bmp.readPressure() / 100.0F;
    delay(30);
  }
  presion_base = suma_presion / 20.0; 

  // Tomar offsets del acelerómetro
  sensors_event_t a, g, temp;
  float suma_X = 0, suma_Y = 0, suma_Z = 0;
  for(int i = 0; i < 20; i++) {
    mpu.getEvent(&a, &g, &temp);
    suma_X += a.acceleration.x;
    suma_Y += a.acceleration.y;
    suma_Z += a.acceleration.z;
    delay(30);
  }
  
  offset_X = (suma_X / 20.0) - 9.81; 
  offset_Y = (suma_Y / 20.0);
  offset_Z = (suma_Z / 20.0);

  Serial.println("¡Calibración completada!");
  Serial.println("----------------------------------------------");
}

void loop() {
  sensors_event_t a, g, temp_mpu;
  mpu.getEvent(&a, &g, &temp_mpu);

  // Aplicar calibración física
  float acc_x_calibrada = a.acceleration.x - offset_X; 
  float acc_y_calibrada = a.acceleration.y - offset_Y;
  float acc_z_calibrada = a.acceleration.z - offset_Z;
  float empuje_neto = acc_x_calibrada - 9.81; // Eje de empuje vertical

  float presion = bmp.readPressure() / 100.0F;
  float altitud_relativa = bmp.readAltitude(presion_base); 

  // --- LÓGICA DE FILTRADO POR CAMBIO SIGNIFICATIVO ---
  
  // Calcular la diferencia absoluta respecto al último envío
  float delta_empuje = abs(empuje_neto - ultimo_empuje_enviado);
  float delta_altitud = abs(altitud_relativa - ultima_altitud_enviada);
  
  unsigned long tiempo_actual = millis();

  // Condición: ¿Hubo un cambio real o ya pasó mucho tiempo desde el último reporte?
  if (delta_empuje >= UMBRAL_EMPUJE || delta_altitud >= UMBRAL_ALTITUD || (tiempo_actual - ultimo_envio_tiempo >= TIMEOUT_FORZADO)) {
    
    // Actualizamos el historial con los nuevos valores enviados
    ultimo_empuje_enviado = empuje_neto;
    ultima_altitud_enviada = altitud_relativa;
    ultimo_envio_tiempo = tiempo_actual;

    // Construcción del paquete CSV
    String paqueteData = String(empuje_neto, 2) + "," + 
                         String(acc_y_calibrada, 2) + "," + 
                         String(acc_z_calibrada, 2) + "," + 
                         String(altitud_relativa, 2);

    // 1. Envío por Serial local
    Serial.print("EVENTO DETECTADO -> Enviando: ");
    Serial.println(paqueteData);

    // 2. Transmisión por ráfaga de Radio LoRa
    LoRa.beginPacket();
    LoRa.print(paqueteData);
    LoRa.endPacket();
  }

  // Muestreo interno rápido. La ESP32 revisa el bus a 50Hz (cada 20ms) 
  // para no perderse el instante exacto del despegue, pero solo transmite si hace falta.
  delay(20); 
}