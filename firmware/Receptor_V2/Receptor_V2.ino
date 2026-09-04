#include <SPI.h>
#include <LoRa.h>

// Pines LoRa según tu configuración funcional
#define SS 5
#define DI0 17

void setup() {
  Serial.begin(115200); // Velocidad sincronizada para la telemetría
  while (!Serial);

  Serial.println("Iniciando estación terrena LoRa...");
  
  LoRa.setPins(SS, -1, DI0); // -1 indica que no se usa pin RESET en el receptor

  if (!LoRa.begin(433E6)) {
    // Configuración de radio para asegurar enlace fuerte en banco de pruebas
    LoRa.setSignalBandwidth(125E3);    // 125 kHz estándar
    LoRa.setSpreadingFactor(7);        // SF7 por defecto (rápido)
    LoRa.setTxPower(17);               // Aumentamos la potencia de transmisión a 17dBm
    Serial.println("¡Fallo crítico en la inicialización de LoRa!");
    while (1);
  }

  Serial.println("Estación terrena escuchando correctamente...");
  Serial.println("----------------------------------------------");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  
  if (packetSize) {
    String datosRecibidos = "";
    
    // Leer el paquete completo
    while (LoRa.available()) {
      datosRecibidos += (char)LoRa.read();
    }

    // ==========================================================
    // MENSAJES ESPECIALES
    // ==========================================================

    // Por si en futuras versiones el emisor envía un mensaje
    // específico de APOGEO o DEPLOY.
    if (datosRecibidos.startsWith("APOGEO,")) {
      String alturaApogeo = datosRecibidos.substring(7);

      Serial.println();
      Serial.println("==============================================");
      Serial.println("          !!! APOGEO DETECTADO !!!");
      Serial.print("Altura del apogeo: ");
      Serial.print(alturaApogeo);
      Serial.println(" m");
      Serial.println("==============================================");
      Serial.println();

      return;
    }

    if (datosRecibidos.startsWith("DEPLOY,")) {
      String alturaDeploy = datosRecibidos.substring(7);

      Serial.println();
      Serial.println("==============================================");
      Serial.println("       !!! DESPLIEGUE PARACAIDAS !!!");
      Serial.print("Altura de despliegue: ");
      Serial.print(alturaDeploy);
      Serial.println(" m");
      Serial.println("==============================================");
      Serial.println();

      return;
    }
    
    // ==========================================================
    // PARSEO DE TELEMETRÍA CSV
    // ==========================================================

    // Ahora esperamos 5 campos:
    // empuje, accY, accZ, altitud, estado

    int primeraComa = datosRecibidos.indexOf(',');
    int segundaComa = datosRecibidos.indexOf(',', primeraComa + 1);
    int terceraComa = datosRecibidos.indexOf(',', segundaComa + 1);
    int cuartaComa = datosRecibidos.indexOf(',', terceraComa + 1);
    
    if (primeraComa != -1 &&
        segundaComa != -1 &&
        terceraComa != -1 &&
        cuartaComa != -1) {

      String empuje = datosRecibidos.substring(0, primeraComa);
      String latY = datosRecibidos.substring(primeraComa + 1, segundaComa);
      String latZ = datosRecibidos.substring(segundaComa + 1, terceraComa);
      String altitud = datosRecibidos.substring(terceraComa + 1, cuartaComa);
      String estado = datosRecibidos.substring(cuartaComa + 1);

      // Imprime el formato formateado y legible en el monitor serie de la PC
      Serial.print("Empuje_Neto(X):");
      Serial.print(empuje);
      Serial.print(" m/s² | ");

      Serial.print("Lat_Y:");
      Serial.print(latY);
      Serial.print(" m/s² | ");

      Serial.print("Lat_Z:");
      Serial.print(latZ);
      Serial.print(" m/s²");

      Serial.print("  ||  ");

      Serial.print("Altitud_Relativa:");
      Serial.print(altitud);
      Serial.print(" m");

      // NUEVO: Estado de vuelo
      Serial.print("  ||  ");

      Serial.print("Estado:");
      Serial.println(estado);

    } else {
      // Si el paquete llega incompleto o con ruido,
      // se muestra directo en bruto (Raw)
      Serial.print("Paquete Raw Recibido: ");
      Serial.println(datosRecibidos);
    }
  }
}