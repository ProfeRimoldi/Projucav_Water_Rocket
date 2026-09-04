#include <SPI.h>
#include <LoRa.h>

// ==========================================================
// CONFIGURACIÓN LoRa
// ==========================================================

#define SS 5
#define DI0 17

// Tiempo máximo sin recibir datos antes de considerar
// que se perdió la comunicación con el emisor.
const unsigned long TIMEOUT_COMUNICACION = 5000;

// Momento en que se recibió el último paquete válido.
unsigned long ultimo_paquete_recibido = 0;

// Estado de comunicación
bool comunicacion_activa = false;


// ==========================================================
// SETUP
// ==========================================================

void setup() {

  Serial.begin(115200);
  while (!Serial);

  Serial.println();
  Serial.println("==============================================");
  Serial.println("       ESTACION TERRENA ProJuCAv");
  Serial.println("==============================================");
  Serial.println();

  // --------------------------------------------------------
  // Inicialización LoRa
  // --------------------------------------------------------

  LoRa.setPins(SS, -1, DI0);
  // -1 = no se utiliza RESET en el receptor

  if (!LoRa.begin(433E6)) {

    Serial.println("LoRa: ERROR");
    Serial.println("¡Fallo crítico en la inicialización de LoRa!");

    while (1);
  }

  // IMPORTANTE:
  // La configuración debe hacerse DESPUÉS de LoRa.begin()

  LoRa.setSignalBandwidth(125E3);
  LoRa.setSpreadingFactor(7);
  LoRa.setTxPower(17);

  Serial.println("LoRa: OK");
  Serial.println("Configuracion:");
  Serial.println("  Frecuencia: 433 MHz");
  Serial.println("  Bandwidth: 125 kHz");
  Serial.println("  Spreading Factor: 7");
  Serial.println("  Potencia: 17 dBm");

  Serial.println();
  Serial.println("Emisor: SIN COMUNICACION");
  Serial.println("Esperando telemetria...");
  Serial.println("----------------------------------------------");

  ultimo_paquete_recibido = millis();
}


// ==========================================================
// LOOP
// ==========================================================

void loop() {

  int packetSize = LoRa.parsePacket();


  // ========================================================
  // ¿LLEGÓ UN PAQUETE?
  // ========================================================

  if (packetSize) {

    String datosRecibidos = "";

    // ------------------------------------------------------
    // Leer paquete completo
    // ------------------------------------------------------

    while (LoRa.available()) {
      datosRecibidos += (char)LoRa.read();
    }


    // ------------------------------------------------------
    // Actualizar estado de comunicación
    // ------------------------------------------------------

    ultimo_paquete_recibido = millis();

    if (!comunicacion_activa) {

      comunicacion_activa = true;

      Serial.println();
      Serial.println("==============================================");
      Serial.println("       COMUNICACION CON EMISOR OK");
      Serial.println("==============================================");
    }


    // ======================================================
    // INFORMACIÓN DE RADIO
    // ======================================================

    int rssi = LoRa.packetRssi();
    float snr = LoRa.packetSnr();


    // ======================================================
    // MENSAJES ESPECIALES
    // ======================================================

    // ------------------------------------------------------
    // APOGEO
    // ------------------------------------------------------

    if (datosRecibidos.startsWith("APOGEO,")) {

      String alturaApogeo = datosRecibidos.substring(7);

      Serial.println();
      Serial.println("**********************************************");
      Serial.println("           !!! APOGEO DETECTADO !!!");
      Serial.println("**********************************************");

      Serial.print("Altura del apogeo: ");
      Serial.print(alturaApogeo);
      Serial.println(" m");

      Serial.print("RSSI: ");
      Serial.print(rssi);
      Serial.println(" dBm");

      Serial.print("SNR: ");
      Serial.print(snr);
      Serial.println(" dB");

      Serial.println("**********************************************");
      Serial.println();

      return;
    }


    // ------------------------------------------------------
    // DEPLOY
    // ------------------------------------------------------

    if (datosRecibidos.startsWith("DEPLOY,")) {

      String alturaDeploy = datosRecibidos.substring(7);

      Serial.println();
      Serial.println("**********************************************");
      Serial.println("        !!! DESPLIEGUE PARACAIDAS !!!");
      Serial.println("**********************************************");

      Serial.print("Altura de despliegue: ");
      Serial.print(alturaDeploy);
      Serial.println(" m");

      Serial.print("RSSI: ");
      Serial.print(rssi);
      Serial.println(" dBm");

      Serial.print("SNR: ");
      Serial.print(snr);
      Serial.println(" dB");

      Serial.println("**********************************************");
      Serial.println();

      return;
    }


    // ======================================================
    // PARSEO DE TELEMETRÍA CSV
    // ======================================================

    /*
       Formato enviado actualmente por el emisor:

       empuje,
       accY,
       accZ,
       altitud,
       estado

       Ejemplo:

       24.53,0.12,-0.31,8.42,ASCENSO
    */

    int primeraComa =
      datosRecibidos.indexOf(',');

    int segundaComa =
      datosRecibidos.indexOf(',', primeraComa + 1);

    int terceraComa =
      datosRecibidos.indexOf(',', segundaComa + 1);

    int cuartaComa =
      datosRecibidos.indexOf(',', terceraComa + 1);


    // ======================================================
    // PAQUETE VÁLIDO
    // ======================================================

    if (primeraComa != -1 &&
        segundaComa != -1 &&
        terceraComa != -1 &&
        cuartaComa != -1) {


      // ----------------------------------------------------
      // Separar campos
      // ----------------------------------------------------

      String empuje =
        datosRecibidos.substring(
          0,
          primeraComa
        );

      String latY =
        datosRecibidos.substring(
          primeraComa + 1,
          segundaComa
        );

      String latZ =
        datosRecibidos.substring(
          segundaComa + 1,
          terceraComa
        );

      String altitud =
        datosRecibidos.substring(
          terceraComa + 1,
          cuartaComa
        );

      String estado =
        datosRecibidos.substring(
          cuartaComa + 1
        );


      // ====================================================
      // MOSTRAR TELEMETRÍA
      // ====================================================

      Serial.println();
      Serial.println("----------------------------------------------");

      Serial.println("COMUNICACION: OK");

      Serial.println("----------------------------------------------");

      Serial.print("Empuje_Neto(X): ");
      Serial.print(empuje);
      Serial.println(" m/s²");

      Serial.print("Aceleracion_Y:  ");
      Serial.print(latY);
      Serial.println(" m/s²");

      Serial.print("Aceleracion_Z:  ");
      Serial.print(latZ);
      Serial.println(" m/s²");

      Serial.print("Altitud:        ");
      Serial.print(altitud);
      Serial.println(" m");

      Serial.print("Estado:         ");
      Serial.println(estado);

      Serial.println("----------------------------------------------");

      Serial.print("RSSI:           ");
      Serial.print(rssi);
      Serial.println(" dBm");

      Serial.print("SNR:            ");
      Serial.print(snr);
      Serial.println(" dB");

      Serial.println("----------------------------------------------");
    }


    // ======================================================
    // PAQUETE NO RECONOCIDO
    // ======================================================

    else {

      Serial.println();
      Serial.println("Paquete recibido pero formato no reconocido.");

      Serial.print("Raw: ");
      Serial.println(datosRecibidos);

      Serial.print("RSSI: ");
      Serial.print(rssi);
      Serial.println(" dBm");

      Serial.print("SNR: ");
      Serial.print(snr);
      Serial.println(" dB");
    }
  }


  // ========================================================
  // CONTROL DE TIMEOUT DE COMUNICACIÓN
  // ========================================================

  if (comunicacion_activa &&
      millis() - ultimo_paquete_recibido >
      TIMEOUT_COMUNICACION) {

    comunicacion_activa = false;

    Serial.println();
    Serial.println("**********************************************");
    Serial.println("       !!! COMUNICACION PERDIDA !!!");
    Serial.println("**********************************************");

    Serial.println(
      "No se recibieron paquetes del emisor."
    );

    Serial.println("Esperando nuevamente...");
    Serial.println();
  }
}