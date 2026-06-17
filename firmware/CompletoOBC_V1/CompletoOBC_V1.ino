#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_Sensor.h>
#include <Servo.h> // Librería para el control del servomotor

// Pines LoRa según su PCB funcional
#define SS 5
#define RST 2
#define DI0 17

// Pin asignado para la señal de control del Servomotor (GPIO25)
#define PIN_SERVO 25 

Adafruit_MPU6050 mpu;
Adafruit_BMP280 bmp;
Servo servoParacaidas; // Objeto para controlar el servo

// Variables de calibración
float presion_base = 1013.25;
float offset_X = 0.0, offset_Y = 0.0, offset_Z = 0.0;

// Umbrales de cambio significativo para Telemetría (Ajustables)
const float UMBRAL_EMPUJE = 0.60;  // m/s²
const float UMBRAL_ALTITUD = 0.50; // metros

// --- VARIABLES PARA DETECCIÓN SEGURA DE APOGEO ---
float altitud_maxima = 0.0;
int muestras_descendientes = 0;
const int VENTANA_CONFIRMACION = 5;      // Cantidad de muestras consecutivas bajando requeridas
const float SEGURO_ALTITUD_MINIMA = 5.0; // El paracaídas NO se abre abajo de 5 metros
bool paracaidas_desplegado = false;     // Evita activaciones repetitivas

// Posiciones angulares del servomotor (Ajustar según su diseño mecánico con Diego)
const int ANGULO_BLOQUEADO = 0;   // Mecanismo cerrado reteniendo el paracaídas
const int ANGULO_DESPLEGADO = 90; // Mecanismo abierto liberando el paracaídas

// Historial de telemetría para comparar
float ultimo_empuje_enviado = 0.0;
float ultima_altitud_enviada = 0.0;

// Tiempo máximo de respaldo (Heartbeat)
unsigned long ultimo_envio_tiempo = 0;
const unsigned long TIMEOUT_FORZADO = 2000;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("\n--- Inicializando Aviónica del Cohete + LoRa + Servo ---");

  // Inicializar Servomotor en posición de bloqueo inmediatamente por seguridad física
  servoParacaidas.attach(PIN_SERVO);
  servoParacaidas.write(ANGULO_BLOQUEADO);
  Serial.println("-> Servomotor asegurado en GPIO25 (Posición CERRADO).");

  // 1. Inicializar LoRa
  LoRa.setPins(SS, RST, DI0);
  if (!LoRa.begin(433E6)) {
    Serial.println("¡Fallo crítico en la inicialización de LoRa!");
    while (1);
  }
  
  // CONFIGURACIÓN LORA (Fuera del bloque de error para garantizar su aplicación)
  LoRa.setSignalBandwidth(125E3);    // 125 kHz estándar
  LoRa.setSpreadingFactor(7);        // SF7 por defecto (rápido)
  LoRa.setTxPower(17);               // Potencia de transmisión a 17dBm
  Serial.println("-> Transmisor LoRa configurado y listo.");

  // 2. Inicializar Sensores I2C
  if (!mpu.begin(0x68)) {
    Serial.println("¡Error MPU6050!");
    while (1);
  }
  if (!bmp.begin(0x76)) {
    Serial.println("¡Error BMP280!");
    while (1);
  }

  // Configuración de rangos y filtros de los sensores
  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,
                  Adafruit_BMP280::SAMPLING_X16,
                  Adafruit_BMP280::FILTER_X16,
                  Adafruit_BMP280::STANDBY_MS_1);

  // --- PROCESO DE CALIBRACIÓN EN RAMPA ---
  Serial.println("Calibrando sensores... NO MOVER LA PLACA");
  delay(1000);

  float suma_presion = 0;
  for(int i = 0; i < 20; i++) {
    suma_presion += bmp.readPressure() / 100.0F;
    delay(30);
  }
  presion_base = suma_presion / 20.0; 

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

  Serial.println("¡Calibración completada con éxito!");
  Serial.println("----------------------------------------------");
}

void loop() {
  sensors_event_t a, g, temp_mpu;
  mpu.getEvent(&a, &g, &temp_mpu);

  // Aplicar calibración física de aceleración
  float acc_x_calibrada = a.acceleration.x - offset_X; 
  float acc_y_calibrada = a.acceleration.y - offset_Y;
  float acc_z_calibrada = a.acceleration.z - offset_Z;
  float empuje_neto = acc_x_calibrada - 9.81; // Longitudinal vertical (Eje X)

  float presion = bmp.readPressure() / 100.0F;
  float altitud_relativa = bmp.readAltitude(presion_base);

  // =================================================================
  // 🛡️ ALGORITMO SEGURO DE DETECCIÓN DE APOGEO Y APERTURA DE PARACAÍDAS
  // =================================================================
  if (!paracaidas_desplegado) {
    
    // 1. Monitorear y registrar el punto más alto del vuelo
    if (altitud_relativa > altitud_maxima) {
      altitud_maxima = altitud_relativa;
      muestras_descendientes = 0; // Se limpia el contador porque seguimos subiendo
    } 
    // 2. Si la altitud es menor que el pico máximo registrado...
    else if (altitud_relativa < altitud_maxima && altitud_relativa > SEGURO_ALTITUD_MINIMA) {
      muestras_descendientes++; // Sumamos una muestra potencial de caída
      
      // 3. Si se cumple la ventana de persistencia, se confirma el apogeo real
      if (muestras_descendientes >= VENTANA_CONFIRMACION) {
        servoParacaidas.write(ANGULO_DESPLEGADO); // Activar el servo en GPIO25
        paracaidas_desplegado = true;             // Bloquear lógica para el resto del vuelo
        
        Serial.println("!!! APOGEO DETECTADO !!! -> PARACAÍDAS DESPLEGADO.");
        
        // Alerta inmediata por telemetría LoRa notificando el evento
        LoRa.beginPacket();
        LoRa.print("APOGEO,");
        LoRa.print(altitud_maxima, 2);
        LoRa.endPacket();
      }
    } 
    // Si la lectura fluctúa hacia arriba de manera aislada por ruido, se reinicia el contador
    else {
      muestras_descendientes = 0;
    }
  }
  // =================================================================

  // --- LÓGICA DE FILTRADO POR CAMBIO SIGNIFICATIVO (Telemetría) ---
  float delta_empuje = abs(empuje_neto - ultimo_empuje_enviado);
  float delta_altitud = abs(altitud_relativa - ultima_altitud_enviada);
  unsigned long tiempo_actual = millis();

  if (delta_empuje >= UMBRAL_EMPUJE || delta_altitud >= UMBRAL_ALTITUD || (tiempo_actual - ultimo_envio_tiempo >= TIMEOUT_FORZADO)) {
    
    ultimo_empuje_enviado = empuje_neto;
    ultima_altitud_enviada = altitud_relativa;
    ultimo_envio_tiempo = tiempo_actual;

    // Construcción de la cadena CSV estándar
    String paqueteData = String(empuje_neto, 2) + "," + 
                         String(acc_y_calibrada, 2) + "," + 
                         String(acc_z_calibrada, 2) + "," + 
                         String(altitud_relativa, 2);

    // Estado del paracaídas al final del CSV (0 = Cerrado, 1 = Desplegado)
    paqueteData += "," + String(paracaidas_desplegado ? 1 : 0);

    // 1. Consola local
    Serial.print("TX -> ");
    Serial.println(paqueteData);

    // 2. Transmisión LoRa
    LoRa.beginPacket();
    LoRa.print(paqueteData);
    LoRa.endPacket();
  }

  delay(20); // Muestreo de sensores a 50Hz (Cada 20ms)
}