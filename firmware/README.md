# Aviónica y Telemetría en Tiempo Real - Projucav Water Rocket

Este módulo del repositorio contiene el firmware de la aviónica embarcada y de la estación terrena para el desarrollo del cohete de agua. El sistema adquiere dinámicamente datos de aceleración triaxial y altitud barométrica, implementa algoritmos de calibración y filtrado por cambio significativo en el emisor, y transmite la telemetría vía radioenlace LoRa hacia la estación terrena.

---

## 🚀 Arquitectura del Sistema

El proyecto está compuesto por dos nodos basados en microcontroladores ESP32:

1. **Nodo Emisor (Emisor_V1.ino):** Firmware embarcado en el PCB del cohete. Se encarga de la inicialización de los sensores I2C, el cálculo de offsets, el filtrado inercial/barométrico y el empaquetado de datos en formato CSV para su transmisión.
2. **Nodo Receptor (Receptor_V1.ino):** Firmware de la Estación Terrena. Escucha el espectro de RF, decodifica las ráfagas entrantes en tiempo real, procesa una máquina de estados para el análisis dinámico del vuelo y la detección automática del Apogeo.

---

## 🛠️ Especificaciones de Hardware y Conectividad

### Asignación de Pines (Pinout)

Ambos PCBs dedicados implementan transceptores LoRa basados en la tecnología Semtech (SX1276/SX1278) interconectados mediante el bus SPI nativo (VSPI) de la ESP32:

| Señal LoRa | Pin ESP32 (Emisor) | Pin ESP32 (Receptor) |
| :--- | :---: | :---: |
| **MOSI** | GPIO 23 | GPIO 23 |
| **MISO** | GPIO 19 | GPIO 19 |
| **SCK** | GPIO 18 | GPIO 18 |
| **SS (CS)** | GPIO 5 | GPIO 5 |
| **DI0 (IRQ)**| GPIO 17 | GPIO 17 |
| **RST** | GPIO 2 | *No utilizado (-1)* |

### Direccionamiento I2C (Sensores Embarcados)
* **Acelerómetro / Giroscopio (MPU6050):** `0x68`
* **Sensor de Presión / Altímetro (BMP280):** `0x76` *(Dirección física corregida)*

> ⚠️ **Nota de Orientación del PCB:** El MPU6050 se encuentra montado de forma **vertical** en el PCB del cohete. Por consiguiente, el **Eje X** ha sido mapeado por software como el eje de Empuje Vertical Primario.

---

## ⚙️ Características Principales del Firmware

### 1. Calibración Automática en Rampa
Al inicializar el Emisor, el programa ejecuta una rutina de promediado estático (20 muestras) para aislar las desviaciones térmicas y mecánicas (*offsets*) de ambos sensores:
* Establece una **Línea de Base Barométrica** dinámica para fijar la altitud inicial en `0.00 metros` relativos al suelo de lanzamiento.
* Resta la aceleración inercial en reposo en los ejes laterales y compensa el eje X respecto a la gravedad local ($9.81 m/s^2$) para obtener la lectura de **Empuje Neto**.

### 2. Filtrado por Cambio Significativo (*Deadband*)
Con el fin de mitigar el ruido de fase en la rampa, economizar batería y optimizar el uso del espectro de RF de LoRa, el emisor implementa reportes por umbral dinámico:
* **Umbral de Empuje:** $\ge 0.60 m/s^2$
* **Umbral de Altitud:** $\ge 0.50 metros$
* **Transmisión de Respaldo (*Heartbeat*):** Si las variables no superan los umbrales (cohete estático), se fuerza un envío cíclico de seguridad cada `2000 ms` para validar la continuidad del enlace.

### 3. Analizador de Estados de Vuelo y Detección de Apogeo
El firmware de la estación terrena decodifica la cadena CSV recibida y evalúa los vectores de telemetría a través de una máquina de estados:
* **`EN RAMPA`**: Estado inicial de espera.
* **`PROPULSIÓN / ASCENSO`**: Se gatilla automáticamente al detectar un $Empuje > 15.0 m/s^2$.
* **`¡APOGEO DETECTADO!`**: Se procesa mediante la derivada temporal de la altitud. Al registrar `3 lecturas consecutivas descendentes` tras haber alcanzado la altura máxima, la estación confirma el apogeo y emite una alerta crítica visual. El algoritmo está diseñado para actuar como el disparador lógico del sistema de eyección del paracaídas.
* **`DESCENSO`**: Monitoreo de velocidad terminal hasta el aterrizaje.

---

## 💾 Dependencias Requeridas

Para compilar y flashear estos programas desde el IDE de Arduino, es necesario instalar las siguientes librerías desde el Gestor de Bibliotecas:

* **LoRa** (por *Sandeep Mistry*)
* **Adafruit MPU6050** (por *Adafruit*)
* **Adafruit BMP280 Library** (por *Adafruit*)
* **Adafruit Unified Sensor** (por *Adafruit*)

---

## 📊 Formato del Paquete de Datos (Telemetría Raw)

Las ráfagas transmitidas por el módulo de radio viajan de forma compacta en una única cadena de texto plana separada por comas (CSV), reduciendo el tiempo de aire al mínimo:

```csv
[Empuje_Neto_X],[Aceleracion_Y],[Aceleracion_Z],[Altitud_Relativa]