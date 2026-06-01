#include <SPI.h>
#include <LoRa.h>

#define SS 5
#define RST 2
#define DI0 17

void setup() {
  Serial.begin(115200);
  while (!Serial);

  LoRa.setPins(SS, RST, DI0);
  if (!LoRa.begin(433E6)) {
    Serial.println("Error LoRa Emisor");
    while (1);
  }
  Serial.println("Simulador de Parábola Perfecta Listo.");
  Serial.println("Envía la letra 'D' para iniciar el lanzamiento.");
}

void loop() {
  if (Serial.available()) {
    char comando = Serial.read();
    if (comando == 'D' || comando == 'd') {
      Serial.println("--- ¡INICIANDO PARÁBOLA CONTINUA (0 a 0)! ---");
      
      float t = 0.0;     
      float altitud = 0.0;
      float empuje = 0.0;
      float tiempo_total_vuelo = 6.0; // El vuelo dura exactamente 6 segundos
      float altura_maxima = 35.0;     // El apogeo teórico será a los 35 metros

      // Avanzamos en pasos de 0.4s para que la gráfica completa entre en el Serial Plotter
      while (t <= tiempo_total_vuelo + 0.01) { 
        
        // 1. Ecuación de Altitud: Parábola invertida perfecta que intersecta en t=0 y t=6
        // Formulación analítica: h(t) = -a * t * (t - ts)
        // Donde 'a' determina la altura máxima en el vértice (t = 3s)
        float a = (4.0 * altura_maxima) / (tiempo_total_vuelo * tiempo_total_vuelo);
        altitud = a * t * (tiempo_total_vuelo - t);

        // Control estricto de límites por redondeo flotante
        if (altitud < 0.05) altitud = 0.00;

        // 2. Ecuación de Empuje/Aceleración: Derivada física de la trayectoria
        // Durante el despegue simulamos el pico de presión; luego cae bajo la acción de la gravedad.
        if (t <= 0.4) {
          empuje = 45.0 - (t * 50.0); // Pico inicial de empuje neto positivo
        } else {
          empuje = -9.81;            // Fase inercial y caída libre (gravedad pura)
        }

        // Si el cohete ya tocó el suelo en el último paso, la aceleración vuelve a cero
        if (t >= tiempo_total_vuelo) {
          altitud = 0.00;
          empuje = 0.00;
        }

        // Construcción del paquete CSV
        String paquete = String(empuje, 2) + ",0.00,0.00," + String(altitud, 2);
        
        LoRa.beginPacket();
        LoRa.print(paquete);
        LoRa.endPacket();
        
        Serial.print("Simulado Enviado: "); Serial.println(paquete);
        
        t += 0.4;   // Paso del tiempo coordinado para el Plotter
        delay(400); // Retardo de estabilidad en milisegundos
      }
      Serial.println("--- FIN DE LA SIMULACIÓN ---");
    }
  }
}