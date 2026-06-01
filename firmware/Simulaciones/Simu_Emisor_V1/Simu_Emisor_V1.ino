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
  Serial.println("Simulador de Vuelo Listo.");
  Serial.println("Envía la letra 'D' por el monitor serial para simular el lanzamiento.");
}

void loop() {
  if (Serial.available()) {
    char comando = Serial.read();
    if (comando == 'D' || comando == 'd') {
      Serial.println("--- ¡INICIANDO SIMULACIÓN DE LANZAMIENTO! ---");
      
      float t = 0.0;     
      float altitud = 0.0;
      float empuje = 0.0;

      // Simulación de un vuelo de 6 segundos en intervalos de 100ms
      while (t < 6.0) {
        if (t <= 0.4) {
          // Fase de propulsión de agua violenta (Primeros 400ms)
          empuje = 45.0 - (t * 50.0); 
          altitud += (empuje * 0.1) + (5.0 * t); 
        } else if (t > 0.4 && t <= 2.5) {
          // Fase de inercia hacia el apogeo (Frenado por gravedad y resistencia)
          empuje = -9.81 - (t * 0.5); 
          altitud += (25.0 * (2.5 - t) * 0.1); 
        } else {
          // Fase de descenso por gravedad
          empuje = -9.81;
          altitud -= (9.81 * (t - 2.5) * 0.1); 
          if (altitud < 0) altitud = 0;
        }

        // Construcción del paquete CSV idéntico al de los sensores reales
        String paquete = String(empuje, 2) + ",0.00,0.00," + String(altitud, 2);
        
        LoRa.beginPacket();
        LoRa.print(paquete);
        LoRa.endPacket();
        
        Serial.print("Simulado Enviado: "); Serial.println(paquete);
        
        t += 0.1; 
        delay(100); 
      }
      Serial.println("--- FIN DE LA SIMULACIÓN ---");
    }
  }
}