#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_Sensor.h>
#include <ESP32Servo.h>  // Instalar "ESP32Servo" de Kevin Harrington desde el Gestor de Librerías

// ==================== CONFIGURACIÓN GENERAL ====================

// Poner en true para probar la máquina de estados en el escritorio,
// SIN sensores ni cohete: genera un perfil de vuelo sintético
// (rampa -> impulso -> coast -> apogeo -> caída) con ruido incluido.
// Poner en false para vuelo real con hardware conectado.
#define MODO_SIMULACION false

// Pines LoRa (según EMISOR.ino)
#define SS 5
#define RST 2
#define DI0 17

// Buzzer piezoeléctrico PASIVO (necesita tone(), no sirve con digitalWrite)
#define BUZZER_PIN 15
const unsigned int BUZZER_FRECUENCIA = 2000; // Hz
const unsigned int BUZZER_DURACION_MS = 3000; // cuánto suena al detectar el apogeo

// Servomotor (ej: liberación de paracaídas). Alimentarlo aparte del 3.3V
// de la ESP32 si va a mover algo con resistencia real.
#define SERVO_PIN 13
const int ANGULO_REPOSO = 0;
const int ANGULO_APOGEO = 90;
Servo servoRecuperacion;

Adafruit_MPU6050 mpu;
Adafruit_BMP280 bmp;

// ---------- Calibración ----------
float presion_base = 1013.25;
float offset_X = 0.0, offset_Y = 0.0, offset_Z = 0.0;

// ---------- Filtrado (media móvil exponencial) ----------
// alpha bajo = más suavizado pero más lento en reaccionar; alpha alto = más
// rápido pero deja pasar más ruido. Ajustá según lo que veas en pruebas reales.
const float ALPHA_ALTITUD = 0.25;
const float ALPHA_EMPUJE  = 0.35;
float altitud_filtrada = 0.0;
float empuje_filtrado  = 0.0;
bool filtro_inicializado = false;

// ---------- Umbrales de la máquina de estados ----------
const float UMBRAL_DESPEGUE   = 3.0;   // m/s² de empuje neto sostenido para detectar despegue
const int   MUESTRAS_DESPEGUE = 3;     // muestras consecutivas por encima del umbral (debounce)

const float HISTERESIS_APOGEO = 0.5;   // metros que debe caer desde el máximo para confirmar apogeo
const int   MUESTRAS_APOGEO   = 5;     // muestras consecutivas de caída sostenida (evita falsos positivos por ruido)

const float UMBRAL_ATERRIZAJE   = 1.0; // metros, cercanía al suelo
const int   MUESTRAS_ATERRIZAJE = 25;  // ~0.5s de estabilidad a 20ms/loop, para confirmar aterrizaje

// ---------- Telemetría (throttling para no saturar el enlace LoRa) ----------
const float UMBRAL_EMPUJE_TX  = 0.60;
const float UMBRAL_ALTITUD_TX = 0.50;
const unsigned long TIMEOUT_FORZADO = 2000;
float ultimo_empuje_enviado = 0.0;
float ultima_altitud_enviada = 0.0;
unsigned long ultimo_envio_tiempo = 0;

// ---------- Diagnóstico en vivo (para depurar la detección de apogeo) ----------
// Sacá este bloque o subí el intervalo una vez que esté todo calibrado.
const unsigned long DEBUG_INTERVALO_MS = 300;
unsigned long ultimo_debug_tiempo = 0;

// ==================== MÁQUINA DE ESTADOS ====================
enum EstadoVuelo {
  EN_RAMPA,     // en la rampa, esperando despegue
  ASCENSO,      // despegue detectado, subiendo
  DESCENSO,     // apogeo detectado, cayendo
  FINALIZADO    // aterrizaje confirmado
};

EstadoVuelo estado = EN_RAMPA;

float altitud_maxima = -9999.0;
float altitud_apogeo = 0.0;
int contador_despegue = 0;
int contador_apogeo = 0;
int contador_aterrizaje = 0;

const char* nombreEstado(EstadoVuelo e) {
  switch (e) {
    case EN_RAMPA:    return "EN_RAMPA";
    case ASCENSO:     return "ASCENSO";
    case DESCENSO:    return "DESCENSO";
    case FINALIZADO:  return "FINALIZADO";
  }
  return "?";
}

// Arma el cartel de apogeo, lo imprime por Serial y lo retransmite por LoRa.
void notificarApogeo(float altura) {
  const char* linea = "**********************************************";

  String cartel = String(linea) + "\n";
  cartel += "           !!! APOGEO DETECTADO !!!\n";
  cartel += String(linea) + "\n";
  cartel += "Altura del apogeo: " + String(altura, 2) + " m\n";
  cartel += String(linea);

  Serial.println(cartel);

#if !MODO_SIMULACION
  LoRa.beginPacket();
  LoRa.print(cartel);
  LoRa.endPacket();
#endif
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  while (!Serial);
  pinMode(BUZZER_PIN, OUTPUT);

  servoRecuperacion.setPeriodHertz(50);
  servoRecuperacion.attach(SERVO_PIN, 500, 2400);
  servoRecuperacion.write(ANGULO_REPOSO);

  Serial.println("\n--- Inicializando Aviónica del Cohete + LoRa ---");

#if !MODO_SIMULACION
  // 1. LoRa
  LoRa.setPins(SS, RST, DI0);
  if (!LoRa.begin(433E6)) {
    Serial.println("¡Fallo crítico en la inicialización de LoRa!");
    while (1);
  }
  LoRa.setSignalBandwidth(125E3);
  LoRa.setSpreadingFactor(7);
  LoRa.setTxPower(2);
  Serial.println("-> Transmisor LoRa listo.");

  // 2. Sensores I2C
  if (!mpu.begin(0x68)) {
    Serial.println("¡Error MPU6050!");
    while (1);
  }
  if (!bmp.begin(0x76)) {
    Serial.println("¡Error BMP280!");
    while (1);
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,
                  Adafruit_BMP280::SAMPLING_X8,   // antes X16: menos sobremuestreo, lecturas más rápidas
                  Adafruit_BMP280::FILTER_X4,     // antes X16: mucho menos retraso para un vuelo tan corto
                  Adafruit_BMP280::STANDBY_MS_1);

  // --- Calibración ---
  Serial.println("Calibrando sensores... NO MOVER LA PLACA");
  delay(1000);

  float suma_presion = 0;
  for (int i = 0; i < 20; i++) {
    suma_presion += bmp.readPressure() / 100.0F;
    delay(30);
  }
  presion_base = suma_presion / 20.0;

  sensors_event_t a, g, temp;
  float suma_X = 0, suma_Y = 0, suma_Z = 0;
  for (int i = 0; i < 20; i++) {
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
#else
  randomSeed(analogRead(0));
  Serial.println(">>> MODO_SIMULACION activo: generando vuelo sintético, sin hardware real <<<");
#endif

  Serial.println("----------------------------------------------");
  ultimo_envio_tiempo = millis();
}

// ==================== LECTURA DE SENSORES ====================

#if MODO_SIMULACION
// Genera un perfil de vuelo falso para validar la máquina de estados en el
// escritorio, sin cohete ni sensores. Perfil: reposo -> impulso -> coast
// hasta apogeo (~24m) -> caída -> aterrizaje. Incluye ruido para simular
// las lecturas reales de un BMP280/MPU6050.
void leerSensoresSimulados(float &empuje_neto, float &altitud_relativa) {
  static unsigned long t0 = millis();
  float t = (millis() - t0) / 1000.0; // segundos desde el arranque

  float ruido_alt = random(-20, 20) / 100.0; // ±0.20 m de ruido
  float ruido_acc = random(-30, 30) / 100.0; // ±0.30 m/s² de ruido

  if (t < 1.0) {
    // en la rampa, quieto
    empuje_neto = 0.0 + ruido_acc;
    altitud_relativa = 0.0 + ruido_alt;
  } else if (t < 1.5) {
    // impulso de agua/aire
    empuje_neto = 25.0 + ruido_acc;
    altitud_relativa = (t - 1.0) * 8.0 + ruido_alt;
  } else if (t < 4.0) {
    // coast ascendente, desacelerando por gravedad
    float t_coast = t - 1.5;
    float v0 = 20.0;
    float des = 9.81;
    altitud_relativa = 4.0 + v0 * t_coast - 0.5 * des * t_coast * t_coast + ruido_alt;
    empuje_neto = -des + ruido_acc;
  } else if (t < 9.0) {
    // caída libre
    float t_caida = t - 4.0;
    float alt_en_apogeo = 24.0; // aprox, solo para esta simulación
    altitud_relativa = max(0.0f, alt_en_apogeo - 0.5f * 9.81f * t_caida * t_caida) + ruido_alt;
    empuje_neto = -9.81 + ruido_acc;
  } else {
    // aterrizado
    altitud_relativa = 0.0 + ruido_alt * 0.3;
    empuje_neto = 0.0 + ruido_acc * 0.3;
  }
}
#endif

// ==================== LOOP ====================
void loop() {
  float empuje_neto, altitud_relativa;
  float acc_y_calibrada = 0, acc_z_calibrada = 0;

#if MODO_SIMULACION
  leerSensoresSimulados(empuje_neto, altitud_relativa);
#else
  sensors_event_t a, g, temp_mpu;
  mpu.getEvent(&a, &g, &temp_mpu);

  float acc_x_calibrada = a.acceleration.x - offset_X;
  acc_y_calibrada = a.acceleration.y - offset_Y;
  acc_z_calibrada = a.acceleration.z - offset_Z;
  empuje_neto = acc_x_calibrada - 9.81; // Eje de empuje vertical (verificá orientación de montaje)

  //float presion = bmp.readPressure() / 100.0F;
  altitud_relativa = bmp.readAltitude(presion_base);
#endif

  // --- Filtrado (media móvil exponencial) para reducir ruido ---
  if (!filtro_inicializado) {
    altitud_filtrada = altitud_relativa;
    empuje_filtrado = empuje_neto;
    filtro_inicializado = true;
  } else {
    altitud_filtrada += ALPHA_ALTITUD * (altitud_relativa - altitud_filtrada);
    empuje_filtrado  += ALPHA_EMPUJE  * (empuje_neto - empuje_filtrado);
  }

  // ============ MÁQUINA DE ESTADOS (evaluada SIEMPRE, cada loop) ============
  switch (estado) {

    case EN_RAMPA: {
      if (empuje_filtrado >= UMBRAL_DESPEGUE) {
        contador_despegue++;
        if (contador_despegue >= MUESTRAS_DESPEGUE) {
          estado = ASCENSO;
          altitud_maxima = altitud_filtrada;
          contador_apogeo = 0;
          Serial.println(">>> DESPEGUE DETECTADO <<<");
        }
      } else {
        contador_despegue = 0; // se rompió la racha, reinicia el debounce
      }
      break;
    }

    case ASCENSO: {
      if (altitud_filtrada > altitud_maxima) {
        altitud_maxima = altitud_filtrada;
        contador_apogeo = 0; // seguimos subiendo, reiniciamos el contador de caída
      } else if (altitud_maxima - altitud_filtrada >= HISTERESIS_APOGEO) {
        contador_apogeo++;
        if (contador_apogeo >= MUESTRAS_APOGEO) {
          altitud_apogeo = altitud_maxima;
          estado = DESCENSO;
          contador_aterrizaje = 0;

          tone(BUZZER_PIN, BUZZER_FRECUENCIA, BUZZER_DURACION_MS); // no bloqueante
          servoRecuperacion.write(ANGULO_APOGEO);

          notificarApogeo(altitud_apogeo);
        }
      }
      break;
    }

    case DESCENSO: {
      if (altitud_filtrada <= UMBRAL_ATERRIZAJE) {
        contador_aterrizaje++;
        if (contador_aterrizaje >= MUESTRAS_ATERRIZAJE) {
          estado = FINALIZADO;
          Serial.println(">>> ATERRIZAJE CONFIRMADO <<<");
        }
      } else {
        contador_aterrizaje = 0;
      }
      break;
    }

    case FINALIZADO: {
      // Nada más que hacer: acá se podría apagar el LoRa, entrar en sleep, etc.
      break;
    }
  }

  // ============ DIAGNÓSTICO EN VIVO ============
  // Se imprime SIEMPRE, sin depender del throttling de telemetría, para ver
  // en tiempo real si el estado avanza y qué tan cerca está de disparar el
  // apogeo (útil mientras se calibra HISTERESIS_APOGEO / MUESTRAS_APOGEO).
  if (millis() - ultimo_debug_tiempo >= DEBUG_INTERVALO_MS) {
    ultimo_debug_tiempo = millis();
    Serial.print("[DEBUG] estado=");
    Serial.print(nombreEstado(estado));
    Serial.print(" | empuje_filtrado=");
    Serial.print(empuje_filtrado, 2);
    Serial.print(" | altitud_filtrada=");
    Serial.print(altitud_filtrada, 2);
    Serial.print(" | altitud_maxima=");
    Serial.print(altitud_maxima, 2);
    Serial.print(" | caida_desde_max=");
    Serial.print(altitud_maxima - altitud_filtrada, 2);
    Serial.print(" | contador_apogeo=");
    Serial.println(contador_apogeo);
  }

  // ============ TELEMETRÍA POR LoRa (throttling por cambio significativo) ============
  float delta_empuje = fabs(empuje_filtrado - ultimo_empuje_enviado);
  float delta_altitud = fabs(altitud_filtrada - ultima_altitud_enviada);
  unsigned long tiempo_actual = millis();

  bool debe_enviar = (delta_empuje >= UMBRAL_EMPUJE_TX) ||
                      (delta_altitud >= UMBRAL_ALTITUD_TX) ||
                      (tiempo_actual - ultimo_envio_tiempo >= TIMEOUT_FORZADO);

  if (debe_enviar) {
    ultimo_empuje_enviado = empuje_filtrado;
    ultima_altitud_enviada = altitud_filtrada;
    ultimo_envio_tiempo = tiempo_actual;

    String paqueteData = String(empuje_filtrado, 2) + "," +
                          String(acc_y_calibrada, 2) + "," +
                          String(acc_z_calibrada, 2) + "," +
                          String(altitud_filtrada, 2) + "," +
                          nombreEstado(estado);

    Serial.print("[");
    Serial.print(nombreEstado(estado));
    Serial.print("] Enviando: ");
    Serial.println(paqueteData);

#if !MODO_SIMULACION
    LoRa.beginPacket();
    LoRa.print(paqueteData);
    LoRa.endPacket();
#endif
  }

  delay(20);
}
