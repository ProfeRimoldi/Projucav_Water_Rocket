# 📊 Módulo de Simulación de Vuelo (Inyector de Datos)

Este directorio contiene las herramientas de simulación de software diseñadas para validar y ensayar la lógica de la estación terrena (`Receptor_V1.ino`) en banco de pruebas, sin necesidad de realizar lanzamientos físicos ni exponer los sensores a desgastes mecánicos.

---

## 📐 Modelo Matemático Continúo (Parábola 0 a 0)

Para lograr una visualización óptima en el **Serial Plotter** del IDE de Arduino (el cual posee un búfer horizontal limitado a 500 muestras), el firmware del emisor simulado implementa una función cuadrática continua en el tiempo. Esto permite modelar de forma simétrica el ascenso y descenso del cohete de agua, asegurando que la trayectoria inicie exactamente en `0.00 metros` y finalice en `0.00 metros`.

La ecuación analítica implementada para la altitud $h(t)$ en función del tiempo $t$ es:

$$h(t) = a \cdot t \cdot (t_{total} - t)$$

Donde:
* $t_{total} = 6.0 \text{ segundos}$ (Tiempo total estimado de vuelo).
* $h_{max} = 35.0 \text{ metros}$ (Apogeo teórico parametrizado en el vértice $t = 3.0\text{s}$).
* $a = \frac{4 \cdot h_{max}}{t_{total}^2}$ (Constante de curvatura para ajustar el apogeo).

---

## ⏱️ Optimización del Espectro y Tasa de Muestreo

A diferencia del firmware de vuelo real (`Emisor_V1.ino`) que adquiere datos a 50 Hz, el emisor de simulación opera con un diferencial de tiempo comprimido:
* **Paso de tiempo ($\Delta t$):** $0.4 \text{ segundos}$.
* **Retardo del lazo (`delay`):** $400 \text{ milisegundos}$.

Esta tasa de refresco reducida permite que todo el perfil de vuelo de 6 segundos se resuelva en **16 puntos de datos** de telemetría. Gracias a esto, la curva parabólica completa se dibuja de forma estática en la pantalla del Serial Plotter del receptor, evitando que el gráfico se desplace hacia la izquierda (*scrolling*) y permitiendo un análisis visual completo del despegue y aterrizaje.

---

## 📑 Perfil de Telemetría Generado (Vectores de Fuerza)

El simulador discrimina los estados físicos del cohete de la siguiente manera:

1. **Fase de Propulsión ($0.0\text{s} \le t \le 0.4\text{s}$):** Simula la expulsión violenta de agua a presión. Presenta un pico de empuje positivo inicial de $45.0 \text{ m/s}²$ que decrece rápidamente.
2. **Fase Inercial y Caída Libre ($t > 0.4\text{s}$):** El empuje cesa. El acelerómetro pasa a registrar instantáneamente la aceleración de la gravedad pura ($-9.81 \text{ m/s}²$), mientras la altitud barométrica continúa su ascenso inercial hacia el vértice y su posterior descenso.
3. **Fase de Impacto ($t \ge 6.0\text{s}$):** Al tocar tierra de forma controlada, tanto la altitud como el empuje neto se fuerzan analíticamente a `0.00` para estabilizar las líneas del gráfico.

---

## 💻 Procedimiento de Ensayo en Banco

1. Flashear la placa de la Estación Terrena con el código receptor analítico.
2. Flashear la placa del Cohete con el código del **Emisor Simulado** ubicado en esta carpeta.
3. Conectar ambas placas por USB, abrir el **Serial Plotter** en el puerto del receptor (configurado a **115200 baudios**).
4. Abrir el Monitor Serie común en el puerto del emisor, escribir la letra `D` (Despegue) y presionar *Enter*.
5. **Resultado esperado:** El receptor graficará en tiempo real la parábola de altitud en sincronía con los cambios de estado de la aviónica, permitiendo verificar la sensibilidad del algoritmo de detección de apogeo.