# Documentation - OBC

Esta carpeta contiene documentación técnica complementaria del proyecto.

## Contenido esperado

- decisiones de diseño
- notas de ingeniería
- cambios importantes entre revisiones
- cálculos relevantes
- validaciones y pruebas
- observaciones de fabricación
- incidencias detectadas en laboratorio

## Objetivo

Mantener trazabilidad técnica del desarrollo del hardware y facilitar futuras revisiones o rediseños.

## Decisiones de Diseño:

- En OBC se necesita agregar el sensor de presión atmosférica BMP280, para ello, tanto el giróscopo MPU6050 como el sensor de presión atmosférica compartirán el bus I2C, y se detectan las direcciónes físicas de los sensores mediante el firmware I2C_scanner.ino presente en el apartado "firmware" de la documentación.
- Se conectaron en paralelo ambos sensores usando el mismo bus I2C.
