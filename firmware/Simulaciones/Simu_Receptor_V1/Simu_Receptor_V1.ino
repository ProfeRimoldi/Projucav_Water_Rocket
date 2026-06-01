#include <SPI.h>
#include <LoRa.h>

#define SS 5
#define DI0 17

// Variables para análisis de apogeo
float max_altitud = 0.0;
int lecturas_descendientes = 0;
bool apogeo_detectado = false;
String estado_vuelo = "EN RAMPA";

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("Iniciando Estación Terrena con Analizador de Apogeo...");
  
  LoRa.setPins(SS, -1, DI0);

  if (!LoRa.begin(433E6)) {
    Serial.println("¡Fallo crítico en LoRa!");
    while (1);
  }
  Serial.println("Estación Terrena escuchando telemetría...");
  Serial.println("----------------------------------------------------------------");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  
  if (packetSize) {
    String datosRecibidos = "";
    while (LoRa.available()) {
      datosRecibidos += (char)LoRa.read();
    }
    
    // Parseo CSV simplificado
    int primeraComa = datosRecibidos.indexOf(',');
    int segundaComa = datosRecibidos.indexOf(',', primeraComa + 1);
    int terceraComa = datosRecibidos.indexOf(',', segundaComa + 1);
    
    if (primeraComa != -1 && segundaComa != -1 && terceraComa != -1) {
      float empuje = datosRecibidos.substring(0, primeraComa).toFloat();
      float latY = datosRecibidos.substring(primeraComa + 1, segundaComa).toFloat();
      float latZ = datosRecibidos.substring(segundaComa + 1, terceraComa).toFloat();
      float altitud = datosRecibidos.substring(terceraComa + 1).toFloat();
      
      // ---- MÁQUINA DE ESTADOS DEL VUELO ----
      if (estado_vuelo == "EN RAMPA" && empuje > 15.0) {
        estado_vuelo = "PROPULSIÓN / ASCENSO";
      }
      
      if (estado_vuelo == "PROPULSIÓN / ASCENSO") {
        // Guardar la máxima altura alcanzada
        if (altitud > max_altitud) {
          max_altitud = altitud;
          lecturas_descendientes = 0; 
        } else if (altitud < max_altitud - 0.5) { 
          // Si bajó más de 50cm respecto al pico máximo, sospechamos apogeo
          lecturas_descendientes++;
        }
        
        // Si confirma 3 lecturas consecutivas hacia abajo, es el apogeo real
        if (lecturas_descendientes >= 3 && !apogeo_detectado) {
          estado_vuelo = "¡APOGEO DETECTADO!";
          apogeo_detectado = true;
        }
      }
      
      if (apogeo_detectado && altitud < max_altitud - 2.0) {
        estado_vuelo = "DESCENSO";
      }

      // Imprimir telemetría con el estado actual del cohete
      Serial.print("["); Serial.print(estado_vuelo); Serial.print("] ");
      Serial.print("Empuje:"); Serial.print(empuje); Serial.print(" m/s² | ");
      Serial.print("Altitud:"); Serial.print(altitud); Serial.print(" m | ");
      Serial.print("Max_Alt:"); Serial.print(max_altitud); Serial.println(" m");
      
      // Alerta visual en el instante exacto del apogeo
      if (estado_vuelo == "¡APOGEO DETECTADO!") {
        Serial.println("\n====================================================");
        Serial.print("¡¡ALERTA APOGEO!! Altura máxima de eyección: "); 
        Serial.print(max_altitud); Serial.println(" metros.");
        Serial.println("====================================================\n");
        estado_vuelo = "DESCENSO"; 
      }
    }
  }
}