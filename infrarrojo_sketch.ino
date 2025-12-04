// Sensor de luz infrarroja

#define LED 9 // led que parpadea al detectar luz IR
#define IRRe A0 // receptor IR conectado en la entrada A5
#define IREm 2  // emisor IR conectado en la entrada 2

int valor = 0; // guardo el valor obtenido por IRRe con el emisor encendido
// cuanto más cerca, mayor debería ser el valor


void setup()
{
  // configuro entradas y salidas de los pines
  pinMode(LED, OUTPUT);
  pinMode(IRRe, INPUT);
  pinMode(IREm, OUTPUT);

  Serial.begin(9600);
}

void loop()
{
 
  digitalWrite(IREm, HIGH); // Enciendo el emisor IR

  valor = analogRead(IRRe); // Leo el valor recogido por el receptor

  if ( valor > 0) { //si el valor es mayor a 0 hago parpadear el LED

    digitalWrite(LED, HIGH);
    Serial.print("\tCANTIDAD DE LUZ IR: ");
    Serial.println(valor);
    delay(100);   
    digitalWrite(LED, LOW);
    delay(100);
  }
 
  else { //si el valor es 0 el LED queda apagado
    
    digitalWrite(LED, LOW);
    Serial.print("\tCANTIDAD DE LUZ IR: ");
    Serial.println(valor);
  }
}