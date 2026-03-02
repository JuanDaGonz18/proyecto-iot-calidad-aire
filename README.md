# Sistema IoT – Monitoreo de Calidad del Aire

Este proyecto implementa un sistema IoT para el monitoreo de la calidad del aire utilizando un microcontrolador ESP32 y sensores ambientales. El sistema permite medir material particulado (PM2.5), gases, temperatura, humedad y presión atmosférica, y clasificar el estado del ambiente en diferentes niveles de calidad.

El objetivo principal es demostrar la integración de sensores físicos, el procesamiento de datos en tiempo real y la generación de alertas visuales y sonoras según el estado ambiental.


## Funcionalidad principal

El sistema realiza las siguientes tareas:

- Lectura periódica de sensores ambientales.
- Procesamiento de los datos mediante una lógica de fusión.
- Clasificación del estado de la calidad del aire.
- Visualización de resultados en un display OLED.
- Activación de alertas mediante LEDs y buzzer.
- Registro de las lecturas por el monitor serial.


## Hardware utilizado

- ESP32  
- Sensor BME280 (temperatura, humedad y presión)  
- Sensor PMS5003 (PM2.5)  
- Sensor MQ-135 (gases)  
- Display OLED SSD1306  
- LEDs (verde, amarillo y rojo)  
- Buzzer  


## Estructura básica del software

El software está desarrollado en Arduino para ESP32 y se organiza en funciones que permiten:

- Inicializar sensores y periféricos.
- Leer datos de los sensores.
- Evaluar la calidad del aire.
- Mostrar información en pantalla.
- Activar alertas según el estado del sistema.

## Ejecución del sistema

El programa se ejecuta de forma cíclica en el ESP32. En cada iteración:

1. Se leen los valores de los sensores.  
2. Se evalúa la calidad del aire con base en umbrales definidos.  
3. Se actualiza el display OLED.  
4. Se activan los LEDs y el buzzer según el estado.  
5. Se imprimen los resultados en el monitor serial.


## Uso del proyecto

1. Conectar los sensores y actuadores al ESP32 según el esquema definido en el proyecto.  
2. Cargar el código en el ESP32 desde el entorno Arduino.  
3. Abrir el monitor serial para observar las lecturas y el diagnóstico del sistema.  
4. Verificar la información en el display OLED y el comportamiento de los LEDs y el buzzer.



## Repositorio

Todo el código fuente del proyecto se encuentra disponible en este repositorio.  
Para reproducir el sistema, basta con clonar el repositorio, instalar las librerías indicadas en el código y cargar el programa en el ESP32.

