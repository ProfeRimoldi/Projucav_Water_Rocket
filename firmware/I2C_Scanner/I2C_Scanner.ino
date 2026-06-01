#include <Wire.h>

void setup() {
  // Inicializamos la comunicación serial a alta velocidad para telemetría
  Serial.begin(115200);
  while (!Serial); 
  
  Serial.println("\n--- I2C Scanner para Cohete de Agua ---");
  
  // En ESP32, Wire.begin() usa GPIO21 y GPIO22 por defecto
  Wire.begin();
}

void loop() {
  byte error, address;
  int nDevices;

  Serial.println("Escaneando bus...");

  nDevices = 0;
  for (address = 1; address < 127; address++) {
    // El i2c_scanner usa el valor de retorno de
    // Write.endTransmission para saber si un dispositivo reconoció la dirección.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Dispositivo I2C encontrado en la dirección 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      
      // Identificación rápida basada en lo que tienes conectado
      if (address == 0x68) Serial.println(" (Probable MPU6050)");
      else if (address == 0x76 || address == 0x77) Serial.println(" (Probable BMP280)");
      else Serial.println(" (Dispositivo desconocido)");

      nDevices++;
    } 
    else if (error == 4) {
      Serial.print("Error desconocido en la dirección 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }

  if (nDevices == 0)
    Serial.println("No se encontraron dispositivos I2C\n");
  else
    Serial.println("Escaneo finalizado.\n");

  delay(5000); // Espera 5 segundos para el siguiente escaneo
}