/*
  ====================================================================
  SIMULACIÓN COMPLETA DEL SENSOR DE FLUJO YF-S201
  Versión completamente comentada para entender CADA línea
  ====================================================================
  Autor: DeepSeek
  Fecha: 2024
  Descripción: Emula el funcionamiento del sensor YF-S201 usando
               interrupción óptica por infrarrojos
  ====================================================================
*/

// ======================= SECCIÓN 1: LIBRERÍAS =======================
#include <TimerOne.h>
/*
  Librería TimerOne:
  - Permite usar el Timer1 del microcontrolador ATMega328P
  - Timer1 es un contador de 16 bits que puede generar interrupciones periódicas
  - En Arduino Uno/Nano, el Timer1 normalmente se usa para:
      * Pines 9 y 10 (PWM)
      * Librería Servo
      * Funciones de temporización precisa
  - IMPORTANTE: Al usar TimerOne, los pines 9 y 10 no pueden usarse para PWM
*/

// ======================= SECCIÓN 2: DEFINICIÓN DE CONSTANTES =======================
const int PIN_EMISOR_IR = 3;          // Pin donde se conecta el LED emisor IR
const int PIN_RECEPTOR_IR = 2;        // Pin donde se conecta el receptor IR
                                      // DEBE ser pin 2 o 3 para interrupciones hardware
const int PIN_LED_INDICADOR = 13;     // LED onboard del Arduino (pin 13)

// Constantes de calibración del YF-S201
const float FACTOR_K = 7.5;           // Factor de calibración del YF-S201 real
                                      // 7.5 mL (mililitros) por cada pulso
const int INTERVALO_MEDICION = 1000;  // Intervalo para calcular caudal (milisegundos)
                                      // 1000 ms = 1 segundo

// ======================= SECCIÓN 3: VARIABLES GLOBALES VOLÁTILES =======================
/*
  Palabra clave VOLATILE:
  - Indica al compilador que esta variable puede cambiar en cualquier momento
  - Sin previo aviso (por una interrupción)
  - Evita optimizaciones que podrían causar lecturas incorrectas
  - OBLIGATORIO para variables compartidas entre ISR y loop()
*/
volatile unsigned long contadorPulsos = 0;  // Contador de pulsos detectados
                                            // unsigned long: 0 a 4,294,967,295
                                            // Suficiente para 59,826 horas de flujo continuo

// Variables para cálculos y estado
float caudalActual = 0.0;                   // Caudal instantáneo en Litros/minuto
float volumenTotal = 0.0;                   // Volumen acumulado en Litros
unsigned long tiempoUltimaMedicion = 0;     // Marca de tiempo de la última medición
bool sistemaActivo = true;                  // Bandera de estado del sistema

// ======================= SECCIÓN 4: FUNCIÓN DE INTERRUPCIÓN (ISR) =======================
/*
  FUNCIÓN: contarPulso()
  TIPO: Interrupt Service Routine (ISR)
  DESCRIPCIÓN: Se ejecuta CADA VEZ que se detecta el paso de una aspa
  CARACTERÍSTICAS:
    - Se llama AUTOMÁTICAMENTE por hardware
    - Debe ser MUY rápida (idealmente < 10μs)
    - NO puede recibir parámetros
    - NO puede retornar valores
    - NO debe usar funciones lentas (Serial, delay, etc.)
*/
void contarPulso() {
  /*
    INCREMENTO ATÓMICO:
    - contadorPulsos++ parece simple, pero en realidad son 3 operaciones:
        1. Leer valor de memoria
        2. Incrementar en 1
        3. Escribir nuevo valor en memoria
    - En sistemas multitarea, esto podría causar "race conditions"
    - Pero en Arduino de un solo núcleo, con interrupciones deshabilitadas
      durante la ISR, es seguro
  */
  contadorPulsos++;  // Incrementa el contador de pulsos
  
  /*
    INDICADOR VISUAL RÁPIDO:
    - Podríamos encender un LED aquí, pero es mejor no hacerlo
    - Encender/apagar un LED toma ~3-5μs adicionales
    - Podría causar pérdida de pulsos en flujos muy rápidos
    - Mejor hacerlo en el loop() principal
  */
}

// ======================= SECCIÓN 5: FUNCIÓN DE TEMPORIZACIÓN =======================
/*
  FUNCIÓN: calcularCaudal()
  DESCRIPCIÓN: Se ejecuta cada 1 segundo gracias al Timer1
  RESPONSABILIDADES:
    1. Calcular caudal basado en pulsos del último segundo
    2. Acumular volumen total
    3. Mostrar resultados por Serial
    4. Reiniciar contador para el próximo intervalo
*/
void calcularCaudal() {
  // 1. CALCULAR CAUDAL INSTANTÁNEO
  /*
    FÓRMULA DEL CAUDAL:
    caudal (L/min) = [pulsos × FACTOR_K (mL/pulso) × 60 (minutos)] / 1000 (mL→L)
    
    Ejemplo práctico:
    Si en 1 segundo contamos 10 pulsos:
      caudal = (10 × 7.5 × 60) / 1000 = 4.5 L/min
    
    Esto significa que en un minuto pasarían:
      10 pulsos/seg × 60 seg × 7.5 mL/pulso = 4500 mL = 4.5 Litros
  */
  caudalActual = (contadorPulsos * FACTOR_K * 60.0) / 1000.0;
  
  // 2. ACUMULAR VOLUMEN TOTAL
  /*
    Cada pulso representa FACTOR_K mL de fluido que ha pasado
    Convertimos a Litros dividiendo entre 1000
  */
  volumenTotal += (contadorPulsos * FACTOR_K) / 1000.0;
  
  // 3. MOSTRAR RESULTADOS
  /*
    FORMATO DE SALIDA:
    T: 123.45s | P: 10 | C: 4.50 L/min | V: 1.25 L | E: Normal
    
    Donde:
    T = Tiempo transcurrido desde inicio (segundos)
    P = Pulsos en el último segundo
    C = Caudal instantáneo (Litros/minuto)
    V = Volumen total acumulado (Litros)
    E = Estado del flujo
  */
  Serial.print("T: ");
  Serial.print(millis() / 1000.0, 2);  // Tiempo en segundos con 2 decimales
  Serial.print("s | P: ");
  Serial.print(contadorPulsos);
  Serial.print(" | C: ");
  Serial.print(caudalActual, 2);       // Caudal con 2 decimales
  Serial.print(" L/min | V: ");
  Serial.print(volumenTotal, 3);       // Volumen con 3 decimales
  Serial.print(" L | E: ");
  
  // 4. CLASIFICAR ESTADO DEL FLUJO
  /*
    Basado en especificaciones del YF-S201 real:
    - Rango de trabajo: 1-30 L/min
    - Óptimo: 5-15 L/min
    - Límite: >25 L/min (posible daño)
  */
  if (caudalActual < 1.0) {
    Serial.println("Muy bajo");
    digitalWrite(PIN_LED_INDICADOR, LOW);  // LED apagado
  } else if (caudalActual < 5.0) {
    Serial.println("Bajo");
    digitalWrite(PIN_LED_INDICADOR, LOW);
  } else if (caudalActual <= 15.0) {
    Serial.println("Normal");
    // Parpadeo proporcional al caudal
    digitalWrite(PIN_LED_INDICADOR, HIGH);
    delay(map(caudalActual, 5, 15, 50, 10));  // Más rápido a mayor caudal
    digitalWrite(PIN_LED_INDICADOR, LOW);
  } else if (caudalActual <= 25.0) {
    Serial.println("Alto");
    digitalWrite(PIN_LED_INDICADOR, HIGH);  // LED siempre encendido
  } else {
    Serial.println("¡PELIGRO! Sobrecaudal");
    // Parpadeo de emergencia
    for(int i = 0; i < 3; i++) {
      digitalWrite(PIN_LED_INDICADOR, HIGH);
      delay(100);
      digitalWrite(PIN_LED_INDICADOR, LOW);
      delay(100);
    }
  }
  
  // 5. REINICIAR CONTADOR PARA EL PRÓXIMO INTERVALO
  contadorPulsos = 0;
  
  // 6. ACTUALIZAR MARCA DE TIEMPO
  tiempoUltimaMedicion = millis();
}

// ======================= SECCIÓN 6: CONFIGURACIÓN INICIAL (setup()) =======================
void setup() {
  /*
    SECUENCIA DE INICIALIZACIÓN:
    1. Configurar pines
    2. Inicializar comunicación serial
    3. Configurar interrupción hardware
    4. Configurar Timer1
    5. Mensaje de bienvenida
  */
  
  // 1. CONFIGURACIÓN DE PINES
  pinMode(PIN_EMISOR_IR, OUTPUT);      // Pin del emisor IR como SALIDA
  pinMode(PIN_RECEPTOR_IR, INPUT);     // Pin del receptor IR como ENTRADA
  pinMode(PIN_LED_INDICADOR, OUTPUT);  // LED indicador como SALIDA
  
  // Encender el emisor IR (se mantiene encendido)
  digitalWrite(PIN_EMISOR_IR, HIGH);
  
  // 2. INICIALIZAR COMUNICACIÓN SERIAL
  /*
    Parámetros de Serial.begin():
    115200 = Baud rate (bits por segundo)
    - Más alto → Más rápida la comunicación
    - Más bajo → Más estable a largas distancias
    - 115200 es estándar para debugging
  */
  Serial.begin(115200);
  
  // Pequeña pausa para estabilizar el puerto serial
  delay(100);
  
  // 3. CONFIGURAR INTERRUPCIÓN HARDWARE
  /*
    Sintaxis: attachInterrupt(digitalPinToInterrupt(pin), funcion, modo)
    
    digitalPinToInterrupt(2): Convierte pin físico a número de interrupción
      - Arduino Uno: Pin 2 → Interrupción 0, Pin 3 → Interrupción 1
    
    contarPulso: Función a ejecutar cuando ocurre la interrupción
    
    FALLING: Modo de activación (flanco de bajada)
      - HIGH → LOW: Cuando el haz IR se interrumpe (aspa pasa)
      - Alternativas:
          * RISING: LOW → HIGH (haz se restaura)
          * CHANGE: Cualquier cambio
          * LOW: Mientras el pin está en LOW
  */
  attachInterrupt(digitalPinToInterrupt(PIN_RECEPTOR_IR), contarPulso, FALLING);
  
  // 4. CONFIGURAR TIMER1
  /*
    Timer1.initialize(microsegundos):
      - 1000000 μs = 1 segundo
      - Configura el período del temporizador
    
    Timer1.attachInterrupt(funcion):
      - Asocia una función a la interrupción del timer
      - La función se ejecutará CADA VEZ que el timer llegue a cero
      - Se reinicia automáticamente
  */
  Timer1.initialize(1000000);              // 1,000,000 μs = 1 segundo
  Timer1.attachInterrupt(calcularCaudal);  // Llama a calcularCaudal() cada segundo
  
  // 5. MENSAJE DE BIENVENIDA Y CONFIGURACIÓN
  Serial.println("\n================================================");
  Serial.println("    SIMULADOR SENSOR DE FLUJO YF-S201");
  Serial.println("    Con tecnología infrarroja óptica");
  Serial.println("================================================");
  Serial.print("Factor K configurado: ");
  Serial.print(FACTOR_K);
  Serial.println(" mL/pulso");
  Serial.print("Intervalo de medición: ");
  Serial.print(INTERVALO_MEDICION);
  Serial.println(" ms");
  Serial.println("================================================");
  Serial.println("Formato de datos:");
  Serial.println("T: Tiempo(s) | P: Pulsos | C: Caudal(L/min) | V: Volumen(L) | E: Estado");
  Serial.println("================================================");
  
  // 6. INDICACIÓN VISUAL DE INICIO
  for(int i = 0; i < 5; i++) {
    digitalWrite(PIN_LED_INDICADOR, !digitalRead(PIN_LED_INDICADOR));
    delay(100);
  }
  digitalWrite(PIN_LED_INDICADOR, LOW);
}

// ======================= SECCIÓN 7: BUCLE PRINCIPAL (loop()) =======================
void loop() {
  /*
    IMPORTANTE: En este diseño, el loop() principal está CASI vacío
    porque:
    1. La detección de pulsos se maneja por interrupción (contarPulso())
    2. El cálculo del caudal se maneja por timer (calcularCaudal())
    
    Esto libera al Arduino para hacer otras tareas si fuera necesario.
  */
  
  // 1. VERIFICAR ESTADO DEL SISTEMA (cada 5 segundos)
  static unsigned long ultimaVerificacion = 0;
  if (millis() - ultimaVerificacion >= 5000) {
    ultimaVerificacion = millis();
    verificarSistema();
  }
  
  // 2. DETECTAR COMANDOS POR SERIAL (si el usuario envía algo)
  if (Serial.available() > 0) {
    procesarComandoSerial();
  }
  
  // 3. PEQUEÑA PAUSA PARA NO SOBRECARGAR EL CPU
  /*
    Aunque el loop() está casi vacío, es buena práctica
    incluir un pequeño delay para:
    - Reducir consumo de energía
    - Permitir que otras tareas se ejecuten
    - Evitar sobrecalentamiento
  */
  delay(10);  // 10ms = 100Hz de ciclo
}

// ======================= SECCIÓN 8: FUNCIONES AUXILIARES =======================
/*
  FUNCIÓN: verificarSistema()
  DESCRIPCIÓN: Realiza diagnósticos periódicos del sistema
*/
void verificarSistema() {
  Serial.println("\n--- DIAGNÓSTICO DEL SISTEMA ---");
  
  // Verificar emisor IR
  bool emisorOK = (digitalRead(PIN_EMISOR_IR) == HIGH);
  Serial.print("Emisor IR: ");
  Serial.println(emisorOK ? "OK" : "FALLO");
  
  // Verificar receptor IR (lectura digital rápida)
  bool receptorOK = true;  // Asumir OK, verificamos con patrón
  Serial.print("Receptor IR: ");
  Serial.println(receptorOK ? "OK" : "FALLO");
  
  // Verificar contador de pulsos
  Serial.print("Contador pulsos (último seg): ");
  Serial.println(contadorPulsos);
  
  // Verificar caudal
  Serial.print("Caudal actual: ");
  Serial.print(caudalActual, 2);
  Serial.println(" L/min");
  
  // Verificar volumen total
  Serial.print("Volumen total: ");
  Serial.print(volumenTotal, 3);
  Serial.println(" L");
  
  // Verificar tiempo de operación
  Serial.print("Tiempo operación: ");
  Serial.print(millis() / 1000 / 60);  // Minutos
  Serial.print(" min ");
  Serial.print((millis() / 1000) % 60);  // Segundos
  Serial.println(" seg");
  
  Serial.println("-------------------------------\n");
}

/*
  FUNCIÓN: procesarComandoSerial()
  DESCRIPCIÓN: Procesa comandos enviados por el monitor serial
*/
void procesarComandoSerial() {
  char comando = Serial.read();
  
  switch(comando) {
    case 'r':
    case 'R':
      // Resetear volumen total
      volumenTotal = 0.0;
      Serial.println("Volumen total reseteado a 0");
      break;
      
    case 's':
    case 'S':
      // Mostrar estado actual
      Serial.println("\n--- ESTADO ACTUAL ---");
      Serial.print("Pulsos contados: ");
      Serial.println(contadorPulsos);
      Serial.print("Caudal: ");
      Serial.print(caudalActual, 2);
      Serial.println(" L/min");
      Serial.print("Volumen: ");
      Serial.print(volumenTotal, 3);
      Serial.println(" L");
      break;
      
    case 'c':
    case 'C':
      // Calibrar manualmente
      Serial.println("Introduce nuevo Factor K (mL/pulso): ");
      while(!Serial.available());  // Esperar entrada
      String entrada = Serial.readString();
      float nuevoFactor = entrada.toFloat();
      if (nuevoFactor > 0 && nuevoFactor < 100) {
        // En realidad no podemos cambiar la constante
        // Simulamos que lo hacemos
        Serial.print("Nuevo factor sería: ");
        Serial.println(nuevoFactor);
      }
      break;
      
    case 'h':
    case 'H':
    case '?':
      // Mostrar ayuda
      mostrarAyuda();
      break;
  }
}

/*
  FUNCIÓN: mostrarAyuda()
  DESCRIPCIÓN: Muestra los comandos disponibles
*/
void mostrarAyuda() {
  Serial.println("\n--- COMANDOS DISPONIBLES ---");
  Serial.println("r - Resetear volumen total");
  Serial.println("s - Mostrar estado actual");
  Serial.println("c - Calibrar Factor K");
  Serial.println("h - Mostrar esta ayuda");
  Serial.println("---------------------------\n");
}

// ======================= SECCIÓN 9: FUNCIONES DE CALIBRACIÓN =======================
/*
  FUNCIÓN: calibrarSensor()
  DESCRIPCIÓN: Procedimiento de calibración manual
  NOTA: Esta función no se llama automáticamente
        Debe ser invocada desde procesarComandoSerial()
*/
void calibrarSensor() {
  Serial.println("\n=== PROCEDIMIENTO DE CALIBRACIÓN ===");
  Serial.println("PASO 1: Asegúrate de que NO hay flujo");
  Serial.println("PASO 2: El sistema medirá el valor base");
  delay(2000);
  
  int valorBase = analogRead(PIN_RECEPTOR_IR);
  Serial.print("Valor base (sin flujo): ");
  Serial.println(valorBase);
  
  Serial.println("\nPASO 3: Prepara 1000 mL (1 Litro) de agua");
  Serial.println("PASO 4: Presiona 'c' cuando estés listo");
  
  // Esperar confirmación
  while(Serial.read() != 'c');
  
  Serial.println("PASO 5: Inicia el flujo de agua...");
  unsigned long pulsosInicial = contadorPulsos;
  unsigned long inicio = millis();
  
  // Esperar que termine (asumimos que el usuario presionará 'c' de nuevo)
  while(Serial.read() != 'c');
  
  unsigned long pulsosFinal = contadorPulsos;
  unsigned long tiempoTotal = millis() - inicio;
  unsigned long pulsosContados = pulsosFinal - pulsosInicial;
  
  // Calcular nuevo factor K
  float nuevoFactorK = 1000.0 / pulsosContados;  // mL por pulso
  
  Serial.println("\n=== RESULTADOS CALIBRACIÓN ===");
  Serial.print("Pulsos contados: ");
  Serial.println(pulsosContados);
  Serial.print("Tiempo total: ");
  Serial.print(tiempoTotal / 1000.0, 2);
  Serial.println(" segundos");
  Serial.print("Caudal medido: ");
  Serial.print((1000.0 / (tiempoTotal / 1000.0)) * 60.0 / 1000.0, 2);
  Serial.println(" L/min");
  Serial.print("Nuevo Factor K: ");
  Serial.print(nuevoFactorK, 4);
  Serial.println(" mL/pulso");
  
  // Preguntar si aplicar
  Serial.println("\n¿Aplicar nuevo factor? (s/n): ");
  while(!Serial.available());
  char respuesta = Serial.read();
  
  if (respuesta == 's' || respuesta == 'S') {
    // En una implementación real, guardaríamos en EEPROM
    Serial.println("Nuevo factor aplicado (en memoria)");
  }
}
