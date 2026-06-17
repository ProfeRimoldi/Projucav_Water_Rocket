# 🚀 Sistema de Aviónica y Telemetría - Cohete de Agua

Este repositorio contiene las iteraciones de firmware desarrolladas para la computadora de a bordo y la estación terrena del proyecto del cohete de agua. El ecosistema de software está diseñado para abarcar desde las pruebas iniciales de hardware en laboratorio hasta el análisis dinámico de telemetría inalámbrica en tiempo real durante el vuelo.

---

## 🛠️ Estructura del Firmware y Características Técnicas

El ecosistema de código se divide en cuatro módulos principales con objetivos específicos:

### 1. `Medicion_1.ino` (Fase de Diagnóstico y Banco de Pruebas)
* [cite_start]**Propósito:** Actúa como un script básico de validación y diagnóstico inicial para comprobar la conectividad física de los periféricos en el bus $I^2C$[cite: 139].
* [cite_start]**Direccionamiento de Hardware:** Inicializa de forma directa el acelerómetro/giroscopio MPU6050 en la dirección fija `0x68` y el barómetro BMP280 en la dirección `0x76`[cite: 139, 150, 151].
* **Tratamiento Físico:** Carece de rutinas de calibración activa. [cite_start]Calcula la altitud utilizando una presión de referencia fija estandarizada al nivel del mar de $1013.25 \text{ hPa}$[cite: 140]. No descuenta el vector de la gravedad terrestre de los ejes analizados.
* [cite_start]**Tasa de Adquisición:** Configura un retraso estricto de `100 ms` ($10 \text{ Hz}$) en el bucle de ejecución[cite: 174], ideal únicamente para la inspección visual inicial en el monitor serie.

### 2. `Medicion_2.ino` (Computadora de A Bordo - Monitoreo Local)
* **Propósito:** Firmware de telemetría local enfocado en la estabilidad de datos, mitigación de ruido mecánico y análisis analítico en rampa.
* **Calibración del Origen ($0\text{ m}$):** Durante el primer segundo del arranque (`setup`), toma un promedio de 20 muestras de la presión atmosférica del entorno para fijar la presión base real ($P_{\text{base}}$), garantizando que la altitud relativa inicie exactamente en $0 \text{ metros}$ sin importar las condiciones climáticas.
* **Cálculo de Empuje Neto:** Integra una rutina para calcular los offsets de los sensores en reposo. Al eje vertical (eje X) se le sustrae el vector de la aceleración de la gravedad terrestre ($g = 9.81 \text{ m/s}^2$) para aislar el error neto del chip y medir el empuje puro del motor.
* **Filtrado Analógico/Digital:** * **MPU6050:** Activa el filtro digital pasabajo (DLPF) a una banda de `21 Hz` para amortiguar las vibraciones estructurales de alta frecuencia del cohete.
  * **BMP280:** Configura un filtro IIR $\times16$ junto a un sobremuestreo de presión de $\times16$ para suavizar picos espurios provocados por corrientes dinámicas de viento durante el ascenso.

### 3. `Emisor_V1.ino` (Computadora de A Bordo - Firmware de Vuelo Principal)
* **Propósito:** Es el código definitivo integrado en el hardware del cohete. [cite_start]Fusiona las funciones analíticas de calibración con la ráfaga de transmisión inalámbrica LoRa[cite: 139].
* [cite_start]**Lógica de Filtrado por Cambio Significativo (Smart Telemetry):** Para no saturar el canal inalámbrico (debido a la baja tasa de transferencia del enlace LoRa de largo alcance), el bucle interroga internamente a los sensores a una velocidad de $50 \text{ Hz}$ (`20 ms`) [cite: 173, 174] para capturar el instante exacto del despegue, pero solo transmite vía radio si se cumplen los siguientes criterios:
  * [cite_start]$\Delta_{\text{Empuje}} \ge 0.60 \text{ m/s}^2$ (Variación brusca de aceleración en el eje vertical X)[cite: 141, 142].
  * [cite_start]$\Delta_{\text{Altitud}} \ge 0.50 \text{ m}$ (Desplazamientos verticales superiores a 50 cm)[cite: 142, 143].
  * [cite_start]**Heartbeat de Seguridad:** Transmisión forzada por timeout si transcurren `2000 ms` sin cambios significativos [cite: 144, 145][cite_start], permitiendo validar el estado de la aviónica en tierra antes del lanzamiento[cite: 145].
* [cite_start]**Configuración RF LoRa:** Opera a una frecuencia de `433 MHz` [cite: 147][cite_start], con un ancho de banda de `125 kHz` [cite: 147, 148][cite_start], Spreading Factor `SF7` para optimizar velocidad [cite: 148] [cite_start]y una potencia máxima de transmisión configurada en `17 dBm`[cite: 148, 149].

### 4. `Receptor_V1.ino` (Estación Terrena - Módulo Receptor)
* **Propósito:** Firmware exclusivo de la placa receptora de telemetría que permanece en la base de lanzamiento conectada a la computadora terrena.
* **Operación Inalámbrica:** Configura el módulo LoRa en modo escucha asíncrona a `433 MHz` utilizando interrupciones de hardware en el pin digital `DI0` de la placa.
* [cite_start]**Parseo Analítico de Datos:** Recibe la cadena compacta en formato CSV generada por el emisor[cite: 131], procesa la posición de los delimitadores por comas y extrae ordenadamente los valores de Empuje Neto (X), Aceleraciones Laterales (Y, Z) y Altitud Relativa para mostrarlos de forma legible en el terminal serie de la PC.
* **Modo Tolerancia a Fallos:** Si la trama de datos sufre degradación de señal o interferencias mecánicas en el aire, el código omite el parseo y despliega la cadena de texto cruda en bruto (`Raw`) para evitar bloqueos del software.

---

## 📊 Matriz de Comparación Operativa

| Especificación Técnica | `Medicion_1.ino` | `Medicion_2.ino` | `Emisor_V1.ino` | `Receptor_V1.ino` |
| :--- | :---: | :---: | :---: | :---: |
| **Ubicación Física** | Cohete (Banco) | Cohete (Local) | Cohete (Vuelo) | Estación Terrena |
| **Buses de Interfaz** | $I^2C$ | $I^2C$ | $I^2C$ + SPI (LoRa) | SPI (Módulo LoRa) |
| **Frecuencia del Bucle** | $10 \text{ Hz}$ (`100ms`) | $10 \text{ Hz}$ (`100ms`) | $50 \text{ Hz}$ (`20ms` interno) | Asíncrono (RX Polling) |
| **Calibración Dinámica** | No | Sí (Fija Origen $0\text{m}$) | Sí (Fija Origen $0\text{m}$) | No aplica |
| **Filtrado Activo (DLPF)** | No | Sí (Banda a `21 Hz`) | Sí (Banda a `21 Hz`) | No aplica |
| **Criterio de Disparo** | Transmisión Continua | Transmisión Continua | Umbrales dinámicos / Heartbeat | Recepción de paquete válido |
| **Formato de Salida** | Texto Serie | Texto Serie | String CSV Compacto | Formateado / Raw en consola |

---

## 📡 Protocolo y Estructura de la Trama CSV

[cite_start]La comunicación inalámbrica entre la aviónica del cohete (`Emisor_V1.ino`) y la estación terrena (`Receptor_V1.ino`) utiliza un formato de cadena comprimido delimitado por comas[cite: 131]:

```text
Formato: <Empuje_Neto_X>,<Acc_Lateral_Y>,<Acc_Lateral_Z>,<Altitud_Relativa>
Ejemplo: 12.34,-0.45,0.12,38.50