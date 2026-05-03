# 🚀 ProJuCAv – Water Rocket with Avionics

## 📡 Descripción del Proyecto

**ProJuCAv (Proyecto de Cohete de Agua con Aviónica)** es una iniciativa educativa basada en metodologías STEM que integra **física, electrónica, programación y diseño mecánico** en el desarrollo de un sistema aeroespacial a escala.

El proyecto propone el diseño, construcción y validación de un **cohete de agua instrumentado**, capaz de medir, registrar y transmitir datos de vuelo en tiempo real.

📘 Inspirado en programas aeroespaciales reales, el enfoque busca trasladar conceptos de ingeniería de alta complejidad a un entorno educativo práctico.

---

## 🎯 Objetivos

* Comprender los principios físicos de la **propulsión por reacción** (Tercera Ley de Newton) 
* Diseñar y construir un vehículo de lanzamiento funcional
* Desarrollar un sistema de **aviónica embarcada**
* Implementar una **estación terrena de telemetría**
* Analizar datos reales de vuelo
* Fomentar el trabajo colaborativo y multidisciplinario

📊 Según la planificación del proyecto, se estructura en etapas progresivas de diseño, integración y validación.

---

## 🧩 Arquitectura del Sistema

El sistema se divide en tres subsistemas principales:

### 🔧 Estructura (Mecánica)

* Botellas PET (fuselaje)
* Aletas aerodinámicas
* Sistema de presurización
* Plataforma de lanzamiento

### ⚡ Aviónica (Electrónica)

* Microcontrolador (ESP32)
* Sensores:

  * Altímetro barométrico (BMP280)
  * Acelerómetro (MPU6050)
* Sistema de almacenamiento local (microSD)
* Módulo de comunicación (LoRa)
* Sistema de alimentación (LiPo + regulación)

📌 La arquitectura electrónica completa se describe en 

---

### 📡 Estación Terrena

* Receptor de telemetría
* Computadora de procesamiento
* Software de visualización en tiempo real

---

## 📡 Telemetría y Adquisición de Datos

El sistema permite:

* Medición de **altura, aceleración y velocidad**
* Registro de datos en tiempo real y post-vuelo
* Transmisión inalámbrica a estación terrena

---

## 🚀 Propulsión

El cohete utiliza propulsión hidráulica basada en:

* Aire comprimido (energía potencial)
* Agua (masa de reacción)

✔ Basado en la **Tercera Ley de Newton**
✔ Expulsión de masa → generación de empuje

---

## 🪂 Sistema de Recuperación

El sistema de recuperación es crítico para la misión:

* Detección de apogeo mediante aviónica.
* Despliegue de paracaídas en apogeo.

---

## 🧪 Etapas del Proyecto

Según la planificación general:

1. **Diseño y fundamentos**
2. **Construcción del cohete**
3. **Integración de aviónica**
4. **Telemetría y pruebas**
5. **Lanzamiento final**

---

## 🛰️ Aviónica – Enfoque del Desarrollo

La aviónica evoluciona en niveles:

* 🔹 Nivel básico: sensores esenciales
* 🔹 Nivel intermedio: registro de datos (caja negra)
* 🔹 Nivel avanzado: telemetría en tiempo real
* 🔹 Nivel completo: sistema autónomo

---

## ⚙️ Tecnologías Utilizadas

* ESP32 (control principal)
* Sensores MEMS
* Comunicación inalámbrica

---

## 📁 Estructura del Repositorio

```bash
Projucav_Water_Rocket/
│
├── firmware/        # Código del sistema embebido
├── hardware/        # Diseño electrónico y PCB
├── docs/            # Documentación técnica y académica
└── README.md        # Descripción general del proyecto
```

---

## 👨‍🏫 Contexto Educativo

* Institución: Instituto Juan XXIII
* Modalidad: Aprendizaje basado en proyectos (PBL)
* Duración: ~5 meses
* Trabajo en equipos multidisciplinarios

---

## 🎓 Enfoque del Proyecto

Este proyecto busca replicar, a escala educativa, un sistema aeroespacial real, integrando:

* Ingeniería electrónica
* Física aplicada
* Programación embebida
* Diseño mecánico
* Análisis de datos

---

## 🚀 Autores / Desarrollo

**Diego Otero**; **Damián Rimoldi**
* Ingeniería en Electrónica – Docentes – Investigadores

Quien escribe:
Responsable del desarrollo de la **aviónica y sistema electrónico** del proyecto.

---

## 📌 Estado del Proyecto

🟡 En desarrollo
🔧 Etapa: Diseño e integración de sistemas

