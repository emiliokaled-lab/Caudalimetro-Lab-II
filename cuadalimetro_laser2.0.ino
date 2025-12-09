// Código para medir velocidad angular usando láser y LDR - Caudalímetro
// Autor: Basado en código de Oyvind N. Dahl
// Modificado para medición de velocidad angular

int laser = 9;           // Pin del láser
int ldr = 8;             // Pin del LDR (entrada analógica)
const int umbralLuz = 500; // Umbral para detectar luz (ajustar según necesidades)
int contadorPulsos = 0;    // Contador de pulsos/interrupciones del haz
unsigned long tiempoAnterior = 0;
unsigned long intervaloMedicion = 100000; // 1 segundo para cálculos

// Variables para cálculo de velocidad
float pi = 3.1416;
float radio = 3.50;       // Radio en metros (ajustar según tu sistema)
float velocidadLineal = 0.0;
float velocidadAngular = 0.0;
bool hazInterrumpido = false;

void setup() {
  pinMode(laser, OUTPUT); 
  pinMode(ldr, INPUT); 
  
  digitalWrite(laser, HIGH); // Encender láser continuamente
  
  Serial.begin(9600);
  Serial.println("=======================================");
  Serial.println("Iniciando Caudalimetro");
  Serial.println("=======================================");
  Serial.println("Esperando mediciones...");
  Serial.println();
  
  tiempoAnterior = millis();
}

void loop() {
  // Leer valor del LDR
  int valorLDR = analogRead(ldr);
  
  // Detectar si el haz está interrumpido
  if (valorLDR < umbralLuz && !hazInterrumpido) {
    // Haz acaba de ser interrumpido
    hazInterrumpido = true;
    contadorPulsos++;
    
    Serial.print("Pulso detectado! Total: ");
    Serial.println(contadorPulsos);
  }
  else if (valorLDR >= umbralLuz) {
    // Haz está presente
    hazInterrumpido = false;
  }
  
  // Calcular velocidad cada segundo
  unsigned long tiempoActual = millis();
  if (tiempoActual - tiempoAnterior >= intervaloMedicion) {
    calcularVelocidad();
    tiempoAnterior = tiempoActual;
  }
}

void calcularVelocidad() {
  // Evitar división por cero
  if (intervaloMedicion == 0) return;
  
  // Calcular frecuencia (Hz)
  float frecuencia = contadorPulsos / (intervaloMedicion / 1000.0);
  
  // Calcular velocidad angular (radianes/segundo)
  // Si tienes N aspas, cada pulso representa 1/N de revolución
  int numeroAspas = 4; // Cambiar según tu sistema (número de obstrucciones/aspas)
  float revolucionesPorSegundo = frecuencia / numeroAspas;
  velocidadAngular = revolucionesPorSegundo * 2 * pi; // rad/s
  
  // Calcular velocidad lineal (m/s)
  velocidadLineal = velocidadAngular * radio;
  
  // Mostrar resultados
  Serial.println("\n=== RESULTADOS DE MEDICIÓN ===");
  Serial.print("Pulsos en 1 segundo: ");
  Serial.println(contadorPulsos);
  Serial.print("Frecuencia: ");
  Serial.print(frecuencia);
  Serial.println(" Hz");
  Serial.print("Velocidad Angular: ");
  Serial.print(velocidadAngular);
  Serial.println(" rad/s");
  Serial.print("Velocidad Lineal: ");
  Serial.print(velocidadLineal);
  Serial.println(" m/s");
  Serial.print("RPM: ");
  Serial.println(revolucionesPorSegundo * 60);
  Serial.println("===============================\n");
  
  // Reiniciar contador para el próximo intervalo
  contadorPulsos = 0;
}