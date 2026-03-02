/*
 * Sistema IoT - Monitoreo Calidad del Aire
 * Universidad de la Sabana - IoT 2026-1
 * Challenge #1
 * 
 * SIMULACIÓN WOKWI: 100% Potenciómetros
 * HARDWARE REAL: BME280 + PMS5003 + MQ-135
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HardwareSerial.h>
#include <Adafruit_BME280.h>
Adafruit_BME280 bme;
HardwareSerial pmsSerial(2);  // Usamos Serial2 del ESP32

// ===== CONFIGURACIÓN DISPLAY OLED =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define MQ135_PIN 32   // Pin analógico donde conectaste el AO del MQ-135
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ===== DEFINICIÓN DE PINES =====

// Sistema de Alertas
#define LED_VERDE 26
#define LED_AMARILLO 27
#define LED_ROJO 14
#define BUZZER 25

// ===== VARIABLES GLOBALES =====
// Variables de sensores
float temperatura = 25.0;
float humedad = 50.0;
float presion = 1013.25;
float pm25_ugm3 = 0;
float gas_ppm = 0;
bool pms_ok = false;


// Umbrales de calidad del aire (OMS/EPA para Colombia)
#define PM25_BUENO 12       // µg/m³
#define PM25_MODERADO 35    // µg/m³
#define PM25_MALO 55        // µg/m³

#define GAS_BUENO 70        // ppm
#define GAS_MODERADO 150    // ppm
#define GAS_MALO 300        // ppm

// Estados de calidad
enum CalidadAire {
  BUENA,
  MODERADA,
  MALA,
  MUY_MALA
};

CalidadAire estadoActual = BUENA;

// Contadores para estadísticas
unsigned long lecturas_totales = 0;
unsigned long alertas_criticas = 0;

// ===== SETUP =====

void setup() {
  Serial.begin(115200);

  // I2C
  Wire.begin(21, 22);

  // BME280
  if (!bme.begin(0x76)) {
    Serial.println("❌ No se encontró el BME280 (0x76)");
    while (1);
  }
  Serial.println("✓ BME280 detectado");

  // PMS5003 por UART2
  pmsSerial.begin(9600, SERIAL_8N1, 16, 17); // RX=16, TX=17
  Serial.println("✓ PMS5003 inicializado (RX=16, TX=17)");

  // Display OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ OLED no detectado en 0x3C");
    while (1);
  }
  Serial.println("✓ OLED OK");

  // Alertas
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_AMARILLO, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  testLEDs();

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Sistema IoT");
  display.println("Calidad del Aire");
  display.println("Listo.");
  display.display();
}

// ===== LOOP PRINCIPAL =====
void loop() {
  leerSensores();
  evaluarCalidadAire();
  activarAlertas();
  actualizarDisplay();
  mostrarDatosSerial();
  
  lecturas_totales++;
  
  delay(2000); // Actualizar cada 2 segundos
}

// ===== FUNCIÓN: LEER SENSORES =====
// ===== PMS5003 (PM2.5 real por UART) =====
void leerPMS5003() {
  static uint8_t buffer[32];

  while (pmsSerial.available() >= 32) {
    // Buscar cabecera 0x42 0x4D
    if (pmsSerial.peek() == 0x42) {
      pmsSerial.readBytes(buffer, 32);

      if (buffer[0] == 0x42 && buffer[1] == 0x4D) {
        pm25_ugm3 = buffer[12] * 256 + buffer[13];
        pms_ok = true;
        return;
      }
    } else {
      pmsSerial.read(); // descartar basura hasta alinear frame
    }
  }

  pms_ok = false;
}

void leerSensores() {
  // ===== PMS5003 (PM2.5 real por UART) =====
  leerPMS5003();

  // ===== BME280 (temperatura, humedad, presión reales) =====
  temperatura = bme.readTemperature();        // °C
  humedad     = bme.readHumidity();           // %
  presion     = bme.readPressure() / 100.0;   // hPa (presión absoluta)

  // ===== MQ-135 (lectura cruda escalada) =====
  int adc_gas = analogRead(MQ135_PIN);        // 0–4095
  gas_ppm = map(adc_gas, 0, 4095, 0, 500);    // Escala simple para demo
}

// ===== FUNCIÓN: EVALUAR CALIDAD DEL AIRE =====
void evaluarCalidadAire() {
  // ═══════════════════════════════════════════════
  // LÓGICA DE FUSIÓN DE SENSORES
  // Combina múltiples señales ambientales para
  // determinar la calidad del aire de forma robusta
  // ═══════════════════════════════════════════════
  
  // --- Evaluar PM2.5 ---
  bool pm25_critico = (pm25_ugm3 > PM25_MALO);
  bool pm25_moderado = (pm25_ugm3 > PM25_MODERADO && pm25_ugm3 <= PM25_MALO);
  bool pm25_leve = (pm25_ugm3 > PM25_BUENO && pm25_ugm3 <= PM25_MODERADO);
  
  // --- Evaluar Gas NO₂ ---
  bool gas_critico = (gas_ppm > GAS_MALO);
  bool gas_moderado = (gas_ppm > GAS_MODERADO && gas_ppm <= GAS_MALO);
  bool gas_leve = (gas_ppm > GAS_BUENO && gas_ppm <= GAS_MODERADO);
  
  // --- Evaluar Condiciones Meteorológicas ---
  // Condiciones que dificultan la dispersión de contaminantes:
  // 1. Alta temperatura + Baja humedad → Inversión térmica probable
  // 2. Baja presión → Aire estancado
  bool inversion_termica = (temperatura > 30 && humedad < 40);
  bool presion_baja = (presion < 730);  // Umbral realista para la altura de Chía
  bool condicion_desfavorable = inversion_termica || presion_baja;
  
  // ═══════════════════════════════════════════════
  // ALGORITMO DE FUSIÓN
  // Prioriza contaminación crítica, luego combina
  // múltiples factores para clasificación robusta
  // ═══════════════════════════════════════════════
  
  if (pm25_critico || gas_critico) {
    // NIVEL 4: MUY MALA - Contaminación crítica
    estadoActual = MUY_MALA;
    alertas_criticas++;
  } 
  else if ((pm25_moderado && gas_leve) || 
           (pm25_leve && gas_moderado) || 
           (pm25_moderado && condicion_desfavorable) ||
           (gas_moderado && condicion_desfavorable)) {
    // NIVEL 3: MALA - Múltiples factores adversos
    estadoActual = MALA;
  }
  else if (pm25_leve || gas_leve || condicion_desfavorable) {
    // NIVEL 2: MODERADA - Un factor adverso
    estadoActual = MODERADA;
  }
  else {
    // NIVEL 1: BUENA - Todos los parámetros normales
    estadoActual = BUENA;
  }
}

// ===== FUNCIÓN: ACTIVAR ALERTAS =====
void activarAlertas() {
  // Apagar todos los LEDs y buzzer
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_AMARILLO, LOW);
  digitalWrite(LED_ROJO, LOW);
  digitalWrite(BUZZER, LOW);
  
  switch(estadoActual) {
    case BUENA:
      // ✓ CALIDAD BUENA - LED Verde
      digitalWrite(LED_VERDE, HIGH);
      break;
      
    case MODERADA:
      // ⚠ CALIDAD MODERADA - LED Amarillo
      digitalWrite(LED_AMARILLO, HIGH);
      break;
      
    case MALA:
      // ⚠⚠ CALIDAD MALA - LED Rojo + Buzzer intermitente
      digitalWrite(LED_ROJO, HIGH);
      digitalWrite(BUZZER, HIGH);
      delay(100);
      digitalWrite(BUZZER, LOW);
      break;
      
    case MUY_MALA:
      // 🚨 CALIDAD MUY MALA - LED Rojo + Alarma continua
      digitalWrite(LED_ROJO, HIGH);
      // Patrón de alarma: beep-beep-pausa
      digitalWrite(BUZZER, HIGH);
      delay(200);
      digitalWrite(BUZZER, LOW);
      delay(100);
      digitalWrite(BUZZER, HIGH);
      delay(200);
      digitalWrite(BUZZER, LOW);
      break;
  }
}

// ===== FUNCIÓN: ACTUALIZAR DISPLAY =====
void actualizarDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Título con línea separadora
  display.setCursor(0, 0);
  display.println("CALIDAD DEL AIRE");
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  
  // Datos de contaminantes
  display.setCursor(0, 14);
  display.print("PM2.5: ");
  if (pms_ok) display.print(pm25_ugm3, 0);
  else display.print("--");
  display.println(" ug/m3");
  
  display.setCursor(0, 24);
  display.print("Gas:   ");
  display.print(gas_ppm, 0);
  display.println(" ppm");
  
  // Datos meteorológicos
  display.setCursor(0, 34);
  display.print("Temp:  ");
  display.print(temperatura, 1);
  display.print("C H:");
  display.print(humedad, 0);
  display.println("%");
  
  display.setCursor(0, 44);
  display.print("Pres:  ");
  display.print(presion, 0);
  display.println(" hPa");
  
  // Estado con indicador visual
  display.setCursor(0, 54);
  display.print("Estado: ");
  
  switch(estadoActual) {
    case BUENA:     
      display.print("BUENA");
      // Dibujar símbolo ✓
      display.drawCircle(120, 57, 3, SSD1306_WHITE);
      break;
    case MODERADA:  
      display.print("MODERADA");
      break;
    case MALA:      
      display.print("MALA");
      // Dibujar símbolo ⚠
      display.fillTriangle(118, 60, 122, 60, 120, 54, SSD1306_WHITE);
      break;
    case MUY_MALA:  
      display.print("CRITICA");
      // Dibujar símbolo 🚨
      display.fillRect(118, 54, 4, 6, SSD1306_WHITE);
      break;
  }
  
  display.display();
}

// ===== FUNCIÓN: MOSTRAR DATOS EN SERIAL =====
void mostrarDatosSerial() {
  Serial.println("╔════════════════════════════════════════════════╗");
  Serial.print("║  Lectura #"); 
  Serial.print(lecturas_totales);
  Serial.print(" | Alertas críticas: ");
  Serial.print(alertas_criticas);
  Serial.println("  ");
  Serial.println("╠════════════════════════════════════════════════╣");
  
  // Contaminantes
  Serial.println("║ CONTAMINANTES:");
  Serial.print("║  PM2.5:  "); 
  Serial.print(pm25_ugm3, 1); 
  Serial.print(" µg/m³");

  if (!pms_ok) {
    Serial.println(" ❌ PMS5003 sin datos");
  } else if (pm25_ugm3 > PM25_MALO) {
    Serial.println(" ⚠ CRÍTICO");
  } else if (pm25_ugm3 > PM25_MODERADO) {
    Serial.println(" ⚠ ALTO");
  } else if (pm25_ugm3 > PM25_BUENO) {
    Serial.println(" - Moderado");
  } else {
    Serial.println(" ✓ Bueno");
  }

  
  Serial.print("║  Gas:    "); 
  Serial.print(gas_ppm, 1); 
  Serial.print(" ppm");
  if (gas_ppm > GAS_MALO) Serial.println(" ⚠ CRÍTICO");
  else if (gas_ppm > GAS_MODERADO) Serial.println(" ⚠ ALTO");
  else if (gas_ppm > GAS_BUENO) Serial.println(" - Moderado");
  else Serial.println(" ✓ Bueno");
  
  // Meteorología
  Serial.println("║ ");
  Serial.println("║ METEOROLOGÍA:");
  Serial.print("║  Temp:   "); 
  Serial.print(temperatura, 1); 
  Serial.println(" °C");
  
  Serial.print("║  Hum:    "); 
  Serial.print(humedad, 1); 
  Serial.println(" %");
  
  Serial.print("║  Pres:   "); 
  Serial.print(presion, 1); 
  Serial.println(" hPa");
  
  // Diagnóstico de condiciones
  Serial.println("║ ");
  Serial.println("║ DIAGNÓSTICO:");
  if (temperatura > 30 && humedad < 40) {
    Serial.println("║  ⚠ Inversión térmica probable");
  }
  if (presion < 730) {
  Serial.println("║  ⚠ Presión baja para esta altura");
}
  
  // Estado final
  Serial.println("║ ");
  Serial.print("║ ESTADO GENERAL: ");
  
  switch(estadoActual) {
    case BUENA:    
      Serial.println("✓ BUENA [LED Verde]"); 
      Serial.println("║ Acción: Ninguna requerida");
      break;
    case MODERADA: 
      Serial.println("⚠ MODERADA [LED Amarillo]"); 
      Serial.println("║ Acción: Monitorear continuamente");
      break;
    case MALA:     
      Serial.println("⚠⚠ MALA [LED Rojo + Buzzer]"); 
      Serial.println("║ Acción: Reducir actividades al aire libre");
      break;
    case MUY_MALA: 
      Serial.println("🚨 MUY MALA [LED Rojo + Alarma]"); 
      Serial.println("║ Acción: ALERTA - Evitar exposición");
      break;
  }
  
  Serial.println("╚════════════════════════════════════════════════╝\n");
}

// ===== FUNCIÓN: TEST DE LEDs Y BUZZER =====
void testLEDs() {
  Serial.println("  → Probando LED Verde...");
  digitalWrite(LED_VERDE, HIGH);
  delay(300);
  digitalWrite(LED_VERDE, LOW);
  
  Serial.println("  → Probando LED Amarillo...");
  digitalWrite(LED_AMARILLO, HIGH);
  delay(300);
  digitalWrite(LED_AMARILLO, LOW);
  
  Serial.println("  → Probando LED Rojo...");
  digitalWrite(LED_ROJO, HIGH);
  delay(300);
  digitalWrite(LED_ROJO, LOW);
  
  Serial.println("  → Probando Buzzer...");
  digitalWrite(BUZZER, HIGH);
  delay(200);
  digitalWrite(BUZZER, LOW);
  delay(100);
  digitalWrite(BUZZER, HIGH);
  delay(200);
  digitalWrite(BUZZER, LOW);
  
  Serial.println("✓ Test de hardware completado - Todo OK\n");
}