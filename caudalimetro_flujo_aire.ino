/*
  ====================================================================
  CAUDALÍMETRO DE AIRE ÓPTICO INFRARROJO
  Específico para medición de flujo de aire/gases
  ====================================================================
  Características:
  - Turbina liviana de bajo rozamiento
  - Factor K ajustado para aire
  - Compensación de temperatura (opcional)
  - Unidades: L/min, m³/h, CFM
  ====================================================================
*/

#include <TimerOne.h>  // Para mediciones precisas

// ================= CONFIGURACIÓN DE HARDWARE =================
const int PIN_EMISOR_IR = 3;       // LED IR emisor (siempre encendido)
const int PIN_RECEPTOR_IR = 2;     // Receptor IR (pin de interrupción)
const int PIN_LED_STATUS = 13;     // LED indicador de estado
const int PIN_TEMP = A1;           // Sensor de temperatura (opcional)

// ================= PARÁMETROS DE CALIBRACIÓN AIRE =================
/*
  FACTOR_K_AJUSTABLE: Litros por pulso
  Para una turbina típica de aire:
  - Diámetro 50mm: ~0.5 L/pulso
  - Diámetro 30mm: ~0.2 L/pulso
  - Diámetro 100mm: ~2.0 L/pulso
  
  CALIBRAR: Medir volumen conocido y ajustar
*/
float factorK = 0.5;              // Litros por pulso (AJUSTAR!)
const float DIAMETRO_TUBERIA = 0.05;  // Diámetro en metros (50mm)

// ================= VARIABLES GLOBALES =================
volatile unsigned long pulsosAire = 0;     // Contador de pulsos
float caudalAire = 0.0;                    // Caudal en L/min
float caudalM3H = 0.0;                     // Caudal en m³/h
float caudalCFM = 0.0;                     // Caudal en CFM (pies³/min)
float volumenTotalAire = 0.0;              // Volumen total en Litros
float temperatura = 25.0;                  // Temperatura en °C (asumida o medida)

// ================= PROTOTIPOS DE FUNCIONES =================
float calcularVelocidadAire(float caudalLmin);
float calcularCaudalMasico(float caudalLmin, float temperaturaC);
void calibrarAutomaticamente();
void mostrarMenuUnidades();

// ================= INTERRUPCIÓN PARA PULSOS =================
void contarPulsoAire() {
  /*
    Para aire, podemos necesitar filtro anti-rebote software
    porque las turbinas pueden vibrar más
  */
  static unsigned long ultimaInterrupcion = 0;
  unsigned long ahora = micros();
  
  // Filtro anti-rebote: ignorar pulsos menores a 1000μs (1ms)
  if (ahora - ultimaInterrupcion > 1000) {
    pulsosAire++;
    ultimaInterrupcion = ahora;
    
    // Indicación visual rápida
    digitalWrite(PIN_LED_STATUS, HIGH);
    delayMicroseconds(50);
    digitalWrite(PIN_LED_STATUS, LOW);
  }
}

// ================= CÁLCULO DE CAUDAL (cada segundo) =================
void calcularCaudalAire() {
  // 1. CAUDAL EN LITROS/MINUTO (base)
  caudalAire = (pulsosAire * factorK * 60.0);  // L/min
  
  // 2. CAUDAL EN METROS CÚBICOS/HORA
  caudalM3H = caudalAire * 0.06;  // L/min × 0.001 m³/L × 60 min/h = 0.06
  
  // 3. CAUDAL EN CFM (Pies Cúbicos por Minuto)
  caudalCFM = caudalAire * 0.0353147;  // 1 L = 0.0353147 ft³
  
  // 4. VELOCIDAD DEL AIRE (m/s)
  float velocidad = calcularVelocidadAire(caudalAire);
  
  // 5. CAUDAL MÁSICO (g/s) - útil para aplicaciones industriales
  float caudalMasico = calcularCaudalMasico(caudalAire, temperatura);
  
  // 6. ACUMULAR VOLUMEN TOTAL
  volumenTotalAire += (pulsosAire * factorK) / 60.0;  // Litros
  
  // 7. MOSTRAR RESULTADOS
  Serial.println("\n=== MEDICIÓN DE AIRE ===");
  Serial.print("Pulsos/s: ");
  Serial.println(pulsosAire);
  Serial.print("Caudal: ");
  Serial.print(caudalAire, 2);
  Serial.print(" L/min | ");
  Serial.print(caudalM3H, 3);
  Serial.print(" m³/h | ");
  Serial.print(caudalCFM, 3);
  Serial.println(" CFM");
  Serial.print("Velocidad: ");
  Serial.print(velocidad, 2);
  Serial.println(" m/s");
  Serial.print("Área sección: ");
  Serial.print(PI * (DIAMETRO_TUBERIA/2) * (DIAMETRO_TUBERIA/2) * 10000, 2);
  Serial.println(" cm²");
  Serial.print("Volumen total: ");
  Serial.print(volumenTotalAire / 1000.0, 3);
  Serial.println(" m³");
  
  // 8. CLASIFICAR FLUJO DE AIRE
  Serial.print("Clasificación: ");
  if (caudalAire < 10) {
    Serial.println("Brisa suave");
  } else if (caudalAire < 50) {
    Serial.println("Ventilación típica");
  } else if (caudalAire < 200) {
    Serial.println("Viento moderado");
  } else if (caudalAire < 500) {
    Serial.println("Viento fuerte");
  } else {
    Serial.println("¡Flujo muy alto!");
  }
  
  // 9. REINICIAR CONTADOR
  pulsosAire = 0;
}

// ================= FUNCIONES DE CÁLCULO =================
float calcularVelocidadAire(float caudalLmin) {
  /*
    Fórmula: Velocidad = Caudal / Área
    
    caudalLmin → m³/s: caudalLmin × 0.001 / 60
    Área = π × r²
    r = DIAMETRO_TUBERIA / 2
  */
  float caudalM3s = (caudalLmin * 0.001) / 60.0;
  float radio = DIAMETRO_TUBERIA / 2.0;
  float area = PI * radio * radio;
  
  if (area > 0) {
    return caudalM3s / area;
  }
  return 0.0;
}

float calcularCaudalMasico(float caudalLmin, float temperaturaC) {
  /*
    Fórmula: ṁ = ρ × Q
    
    ρ (densidad del aire) = P / (R × T)
    Donde:
      P = Presión (101325 Pa, asumimos atmosférica)
      R = Constante gases (287.05 J/(kg·K) para aire seco)
      T = Temperatura en Kelvin (Celsius + 273.15)
    
    Q = caudal en m³/s
  */
  float temperaturaK = temperaturaC + 273.15;
  float densidad = 101325.0 / (287.05 * temperaturaK);  // kg/m³
  
  float caudalM3s = (caudalLmin * 0.001) / 60.0;  // Convertir a m³/s
  float caudalMasico = densidad * caudalM3s * 1000.0;  // g/s
  
  return caudalMasico;
}

// ================= CONFIGURACIÓN INICIAL =================
void setup() {
  // 1. Configurar pines
  pinMode(PIN_EMISOR_IR, OUTPUT);
  pinMode(PIN_RECEPTOR_IR, INPUT);
  pinMode(PIN_LED_STATUS, OUTPUT);
  pinMode(PIN_TEMP, INPUT);
  
  // 2. Encender emisor IR
  digitalWrite(PIN_EMISOR_IR, HIGH);
  
  // 3. Configurar interrupción
  attachInterrupt(digitalPinToInterrupt(PIN_RECEPTOR_IR), 
                  contarPulsoAire, FALLING);
  
  // 4. Configurar Timer1 para mediciones cada segundo
  Timer1.initialize(1000000);  // 1 segundo
  Timer1.attachInterrupt(calcularCaudalAire);
  
  // 5. Iniciar comunicación serial
  Serial.begin(115200);
  delay(100);
  
  // 6. Mensaje de bienvenida
  Serial.println("\n========================================");
  Serial.println("   CAUDALÍMETRO DE AIRE INFRARROJO");
  Serial.println("========================================");
  Serial.print("Diámetro tubería: ");
  Serial.print(DIAMETRO_TUBERIA * 100);
  Serial.println(" cm");
  Serial.print("Factor K inicial: ");
  Serial.print(factorK, 4);
  Serial.println(" L/pulso");
  Serial.println("Unidades: L/min, m³/h, CFM, m/s");
  Serial.println("========================================\n");
  
  // 7. Secuencia de inicio
  for(int i = 0; i < 3; i++) {
    digitalWrite(PIN_LED_STATUS, HIGH);
    delay(200);
    digitalWrite(PIN_LED_STATUS, LOW);
    delay(200);
  }
}

// ================= BUCLE PRINCIPAL =================
void loop() {
  // 1. Leer temperatura (si hay sensor)
  leerTemperatura();
  
  // 2. Procesar comandos seriales
  if (Serial.available() > 0) {
    procesarComandoAire();
  }
  
  // 3. Mostrar menú periódicamente
  static unsigned long ultimoMenu = 0;
  if (millis() - ultimoMenu > 30000) {  // Cada 30 segundos
    ultimoMenu = millis();
    mostrarMenuUnidades();
  }
  
  delay(10);
}

// ================= FUNCIONES AUXILIARES =================
void leerTemperatura() {
  // Para sensor LM35 en pin A1
  // 10mV por °C, resolución 4.88mV (1024 pasos, 5V)
  int lectura = analogRead(PIN_TEMP);
  temperatura = (lectura * 5.0 / 1024.0) * 100.0;  // Convertir a °C
}

void procesarComandoAire() {
  char comando = Serial.read();
  
  switch(comando) {
    case 'c': case 'C':
      calibrarAutomaticamente();
      break;
      
    case 'f': case 'F':
      // Cambiar factor K
      Serial.print("Factor K actual: ");
      Serial.println(factorK, 4);
      Serial.println("Nuevo valor (L/pulso): ");
      while(!Serial.available());
      factorK = Serial.parseFloat();
      Serial.print("Nuevo Factor K: ");
      Serial.println(factorK, 4);
      break;
      
    case 'u': case 'U':
      mostrarMenuUnidades();
      break;
      
    case 'v': case 'V':
      // Cambiar visualización de unidades
      Serial.println("\nSelecciona unidad principal:");
      Serial.println("1 - L/min");
      Serial.println("2 - m³/h");
      Serial.println("3 - CFM");
      Serial.println("4 - m/s");
      break;
      
    case 'r': case 'R':
      volumenTotalAire = 0;
      Serial.println("Volumen total reseteado");
      break;
      
    case 'd': case 'D':
      // Cambiar diámetro de tubería
      Serial.print("Diámetro actual: ");
      Serial.print(DIAMETRO_TUBERIA * 100);
      Serial.println(" cm");
      Serial.println("Nuevo diámetro (cm): ");
      while(!Serial.available());
      float nuevoDiametro = Serial.parseFloat() / 100.0;  // Convertir a metros
      Serial.print("Nuevo diámetro: ");
      Serial.print(nuevoDiametro * 100);
      Serial.println(" cm");
      break;
  }
}

void mostrarMenuUnidades() {
  Serial.println("\n--- UNIDADES DE MEDIDA ---");
  Serial.println("L/min  : Litros por minuto");
  Serial.println("m³/h   : Metros cúbicos por hora");
  Serial.println("CFM    : Pies cúbicos por minuto");
  Serial.println("m/s    : Metros por segundo");
  Serial.println("g/s    : Gramos por segundo (másico)");
  Serial.println("----------------------------");
  Serial.println("Comandos: c=calibrar, f=factorK");
  Serial.println("          u=unidades, r=reset");
  Serial.println("          d=diámetro");
  Serial.println("----------------------------\n");
}

// ================= CALIBRACIÓN AUTOMÁTICA =================
void calibrarAutomaticamente() {
  Serial.println("\n=== CALIBRACIÓN DE AIRE ===");
  Serial.println("Método 1: Volumen conocido");
  Serial.println("Método 2: Velocidad conocida");
  Serial.println("Selecciona método (1 o 2): ");
  
  while(!Serial.available());
  char metodo = Serial.read();
  
  if (metodo == '1') {
    calibrarPorVolumen();
  } else if (metodo == '2') {
    calibrarPorVelocidad();
  }
}

void calibrarPorVolumen() {
  Serial.println("\n--- Calibración por Volumen ---");
  Serial.println("1. Conecta un volumen conocido (ej: 100L)");
  Serial.println("2. Asegura flujo constante");
  Serial.println("3. Presiona 'v' para empezar");
  
  while(Serial.read() != 'v');  // Esperar comando
  
  unsigned long pulsosInicio = pulsosAire;
  unsigned long tiempoInicio = millis();
  
  Serial.println("Medición en curso... Pulsa 'v' de nuevo al terminar");
  
  while(Serial.read() != 'v');  // Esperar fin
  
  unsigned long pulsosFin = pulsosAire;
  unsigned long tiempoFin = millis();
  float tiempoSeg = (tiempoFin - tiempoInicio) / 1000.0;
  unsigned long totalPulsos = pulsosFin - pulsosInicio;
  
  // Pedir volumen conocido
  Serial.println("Introduce volumen conocido (Litros): ");
  while(!Serial.available());
  float volumenConocido = Serial.parseFloat();
  
  // Calcular nuevo factor K
  float nuevoFactorK = volumenConocido / totalPulsos;
  
  Serial.println("\n=== RESULTADOS CALIBRACIÓN ===");
  Serial.print("Pulsos contados: ");
  Serial.println(totalPulsos);
  Serial.print("Tiempo: ");
  Serial.print(tiempoSeg, 2);
  Serial.println(" segundos");
  Serial.print("Volumen: ");
  Serial.print(volumenConocido, 2);
  Serial.println(" Litros");
  Serial.print("Caudal medido: ");
  Serial.print((volumenConocido / tiempoSeg) * 60.0, 2);
  Serial.println(" L/min");
  Serial.print("Nuevo Factor K: ");
  Serial.print(nuevoFactorK, 6);
  Serial.println(" L/pulso");
  
  // Preguntar si aplicar
  Serial.println("\n¿Aplicar nuevo factor? (s/n): ");
  while(!Serial.available());
  char respuesta = Serial.read();
  
  if (respuesta == 's' || respuesta == 'S') {
    factorK = nuevoFactorK;
    Serial.println("Nuevo factor aplicado");
  }
}

void calibrarPorVelocidad() {
  Serial.println("\n--- Calibración por Velocidad ---");
  Serial.println("Usa un anemómetro de referencia");
  Serial.print("Diámetro actual: ");
  Serial.print(DIAMETRO_TUBERIA * 100);
  Serial.println(" cm");
  Serial.println("Introduce velocidad conocida (m/s): ");
  
  while(!Serial.available());
  float velocidadRef = Serial.parseFloat();
  
  // Calcular caudal teórico
  float radio = DIAMETRO_TUBERIA / 2.0;
  float area = PI * radio * radio;  // m²
  float caudalTeoricoM3s = velocidadRef * area;
  float caudalTeoricoLmin = caudalTeoricoM3s * 1000.0 * 60.0;
  
  // Medir pulsos por 10 segundos
  Serial.println("Midiendo pulsos por 10 segundos...");
  unsigned long pulsosInicio = pulsosAire;
  delay(10000);  // 10 segundos
  unsigned long pulsosFin = pulsosAire;
  unsigned long totalPulsos = pulsosFin - pulsosInicio;
  
  // Calcular factor K
  float caudalMedidoLmin = (totalPulsos * factorK * 60.0) / 10.0;  // Por 10 segundos
  float nuevoFactorK = (caudalTeoricoLmin * 10.0) / (totalPulsos * 60.0);
  
  Serial.println("\n=== RESULTADOS ===");
  Serial.print("Velocidad referencia: ");
  Serial.print(velocidadRef, 2);
  Serial.println(" m/s");
  Serial.print("Caudal teórico: ");
  Serial.print(caudalTeoricoLmin, 2);
  Serial.println(" L/min");
  Serial.print("Pulsos en 10s: ");
  Serial.println(totalPulsos);
  Serial.print("Caudal medido: ");
  Serial.print(caudalMedidoLmin, 2);
  Serial.println(" L/min");
  Serial.print("Nuevo Factor K: ");
  Serial.print(nuevoFactorK, 6);
  Serial.println(" L/pulso");
  
  factorK = nuevoFactorK;
  Serial.println("Factor K actualizado");
}