// 1. Declaración de Pines y Configuración (¡Asegúrate de incluir esto!)
const int PIN_BUZZER = 15;
const int RESOLUCION = 8; 

// 2. Frecuencias agudas de alta resonancia (en Hz)
#define TONO_ALTO_1  2700  // Nota de alta eficiencia piezoeléctrica
#define TONO_ALTO_2  3300  
#define TONO_ALTO_3  4000  // Máxima resonancia típica para más volumen

// 3. Estructura de la nueva melodía rápida
int melodiaAguda[] = { TONO_ALTO_1, TONO_ALTO_2, TONO_ALTO_3 };
int duracionesAgudas[] = { 100, 100, 250 }; // Tonos cortos y penetrantes

void reproducirMelodia() {
  // Calcula el tamaño del array dinámicamente
  int totalNotas = sizeof(melodiaAguda) / sizeof(melodiaAguda[0]);

  for (int i = 0; i < totalNotas; i++) {
    int nota = melodiaAguda[i];
    int duracion = duracionesAgudas[i];

    // Envía la frecuencia al pin
    ledcWriteTone(PIN_BUZZER, nota);
    delay(duracion); 

    // Pequeño silencio para separar los tonos
    ledcWriteTone(PIN_BUZZER, 0);
    delay(20); 
  }
  
  // Apagado total por seguridad
  ledcWriteTone(PIN_BUZZER, 0);
}

void setup() {
  // Inicializa el pin con la nueva API de ESP32 v3.0+
  ledcAttach(PIN_BUZZER, 2000, RESOLUCION);
  
  // Ejecuta la melodía de encendido
  reproducirMelodia();
}

void loop() {
  // Tu código principal del cohete irá aquí
}
