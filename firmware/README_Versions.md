# Aviónica de Cohete de Agua — ESP32 + LoRa

File Name: EMISOR_apogeo_V2_Test
Firmware para la placa de aviónica embarcada en un cohete de agua. Lee acelerómetro y presión barométrica, detecta el despegue y el apogeo mediante una máquina de estados, y transmite telemetría por LoRa a una estación en tierra.

## Hardware

| Componente | Modelo | Notas |
|---|---|---|
| Microcontrolador | ESP32 (devkit genérica) | |
| Radio | Módulo LoRa (SX127x) a 433 MHz | |
| IMU | MPU6050 | I2C, dirección `0x68` |
| Barómetro | BMP280 | I2C, dirección `0x76` |
| Buzzer | Piezoeléctrico **pasivo** | Requiere `tone()`, no sirve con `digitalWrite` |
| Servo | Genérico (ej. liberación de paracaídas) | Alimentar aparte del 3.3V si mueve algo con resistencia |
| LED | Integrado de la placa | Verificar el pin real en tu devkit específica |

### Conexión de pines

| Señal | GPIO |
|---|---|
| LoRa SS | 5 |
| LoRa RST | 2 |
| LoRa DIO0 | 17 |
| I2C (MPU6050 + BMP280) | 21 (SDA) / 22 (SCL), por defecto |
| Buzzer | 15 |
| Servo | 13 |
| LED | 2 (fallback si la placa no define `LED_BUILTIN`) |

> ⚠️ En algunas devkits el LED integrado está en el mismo GPIO2 que el `RST` del LoRa. Confirmá el pin real de tu placa antes de volar.

## Dependencias (Arduino Library Manager)

- `LoRa` (sandeepmistry)
- `Adafruit MPU6050`
- `Adafruit BMP280`
- `Adafruit Unified Sensor`
- `ESP32Servo` (Kevin Harrington)

## Máquina de estados

```
EN_RAMPA → ASCENSO → DESCENSO → FINALIZADO
```

- **EN_RAMPA → ASCENSO**: empuje neto sostenido por encima de `UMBRAL_DESPEGUE` durante `MUESTRAS_DESPEGUE` lecturas consecutivas.
- **ASCENSO → DESCENSO (apogeo)**: la altitud cae `HISTERESIS_APOGEO` metros por debajo del máximo alcanzado, sostenido por `MUESTRAS_APOGEO` lecturas (evita falsos positivos por ruido).
- **DESCENSO → FINALIZADO**: altitud por debajo de `UMBRAL_ATERRIZAJE` sostenida por `MUESTRAS_ATERRIZAJE` lecturas.

Las lecturas de altitud y empuje se suavizan con un filtro de media móvil exponencial (`ALPHA_ALTITUD`, `ALPHA_EMPUJE`) antes de evaluarse en la máquina de estados.

## Al detectar el apogeo

1. Se enciende el LED y suena el buzzer (no bloqueante).
2. El servo se mueve a `ANGULO_APOGEO`.
3. Se envía por LoRa un paquete inmediato `APOGEO,<altura>`.
4. Se espera hasta `ACK_TIMEOUT_MS` una respuesta `ACK_APOGEO,<rssi>,<snr>` de la estación en tierra.
5. Se imprime por Serial y se retransmite por LoRa un cartel con la altura y, si llegó a tiempo, el RSSI/SNR medidos en tierra.

> El RSSI/SNR de un paquete solo los conoce quien lo recibe. Para que estos valores no aparezcan siempre como "sin confirmación", la estación en tierra debe responder con `ACK_APOGEO,<rssi>,<snr>` al recibir un paquete `APOGEO,...` (usando `LoRa.packetRssi()` / `LoRa.packetSnr()`).

## Telemetría regular

Además del aviso de apogeo, se envía telemetría periódica por LoRa (formato CSV: empuje, acc_y, acc_z, altitud, estado) cada vez que hay un cambio significativo en empuje o altitud, o cada `TIMEOUT_FORZADO` ms como respaldo.

## Modo simulación

Para probar toda la lógica de la máquina de estados en el escritorio, sin sensores, cohete ni radio conectados:

```cpp
#define MODO_SIMULACION true
```

Esto genera un perfil de vuelo sintético (reposo → impulso → coast → apogeo → caída → aterrizaje) con ruido incluido, útil para validar umbrales antes de una prueba real. Volver a `false` para vuelo real.

## Calibración

Al arrancar (fuera de modo simulación), el firmware promedia 20 muestras de presión y aceleración con la placa quieta para fijar la presión de referencia (altitud 0) y los offsets del acelerómetro. **No mover la placa durante este proceso.**

File Name: EMISOR_apogeo_V3_Test
# Aviónica de Cohete de Agua — ESP32 + LoRa (sin LED / sin RSSI-SNR)

Firmware para la placa de aviónica embarcada en un cohete de agua. Lee acelerómetro y presión barométrica, detecta el despegue y el apogeo mediante una máquina de estados, y transmite telemetría por LoRa a una estación en tierra.

Esta es la versión simplificada: sin indicador LED y sin protocolo de confirmación de RSSI/SNR con la estación en tierra (por lo tanto, sin pausas ni esperas bloqueantes en ningún punto del `loop`).

## Hardware

| Componente | Modelo | Notas |
|---|---|---|
| Microcontrolador | ESP32 (devkit genérica) | |
| Radio | Módulo LoRa (SX127x) a 433 MHz | |
| IMU | MPU6050 | I2C, dirección `0x68` |
| Barómetro | BMP280 | I2C, dirección `0x76` |
| Buzzer | Piezoeléctrico **pasivo** | Requiere `tone()`, no sirve con `digitalWrite` |
| Servo | Genérico (ej. liberación de paracaídas) | Alimentar aparte del 3.3V si mueve algo con resistencia |

### Conexión de pines

| Señal | GPIO |
|---|---|
| LoRa SS | 5 |
| LoRa RST | 2 |
| LoRa DIO0 | 17 |
| I2C (MPU6050 + BMP280) | 21 (SDA) / 22 (SCL), por defecto |
| Buzzer | 15 |
| Servo | 13 |

## Dependencias (Arduino Library Manager)

- `LoRa` (sandeepmistry)
- `Adafruit MPU6050`
- `Adafruit BMP280`
- `Adafruit Unified Sensor`
- `ESP32Servo` (Kevin Harrington)

## Máquina de estados

```
EN_RAMPA → ASCENSO → DESCENSO → FINALIZADO
```

- **EN_RAMPA → ASCENSO**: empuje neto sostenido por encima de `UMBRAL_DESPEGUE` durante `MUESTRAS_DESPEGUE` lecturas consecutivas.
- **ASCENSO → DESCENSO (apogeo)**: la altitud cae `HISTERESIS_APOGEO` metros por debajo del máximo alcanzado, sostenido por `MUESTRAS_APOGEO` lecturas (evita falsos positivos por ruido).
- **DESCENSO → FINALIZADO**: altitud por debajo de `UMBRAL_ATERRIZAJE` sostenida por `MUESTRAS_ATERRIZAJE` lecturas.

Las lecturas de altitud y empuje se suavizan con un filtro de media móvil exponencial (`ALPHA_ALTITUD`, `ALPHA_EMPUJE`) antes de evaluarse en la máquina de estados.

## Al detectar el apogeo

1. Suena el buzzer (no bloqueante).
2. El servo se mueve a `ANGULO_APOGEO`.
3. Se imprime por Serial y se retransmite por LoRa un cartel con la altura del apogeo:

```
**********************************************
           !!! APOGEO DETECTADO !!!
**********************************************
Altura del apogeo: 153.72 m
**********************************************
```

No hay espera de confirmación de la estación en tierra: el `loop` sigue de largo sin pausas.

## Telemetría regular

Además del aviso de apogeo, se envía telemetría periódica por LoRa (formato CSV: empuje, acc_y, acc_z, altitud, estado) cada vez que hay un cambio significativo en empuje o altitud, o cada `TIMEOUT_FORZADO` ms como respaldo.

## Modo simulación

Para probar toda la lógica de la máquina de estados en el escritorio, sin sensores, cohete ni radio conectados:

```cpp
#define MODO_SIMULACION true
```

Esto genera un perfil de vuelo sintético (reposo → impulso → coast → apogeo → caída → aterrizaje) con ruido incluido, útil para validar umbrales antes de una prueba real. Volver a `false` para vuelo real.

## Calibración

Al arrancar (fuera de modo simulación), el firmware promedia 20 muestras de presión y aceleración con la placa quieta para fijar la presión de referencia (altitud 0) y los offsets del acelerómetro. **No mover la placa durante este proceso.**

File Name: EMISOR_apogeo_V3_Test
Se bajó el filtro del sensor BMP6050