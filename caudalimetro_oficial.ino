int led=8,sensor=9; //Decimos a que pines del arduino van
int entrada=0;
int contador=0;
float velocidad=0.0 ;
bool sensorActivado=false;
float pi=3.1416;
void setup() {
  // put your setup code here, to run once:
  pinMode(led,OUTPUT);
  pinMode(sensor,INPUT);
  Serial.begin(9600);
  Serial.println("Iniciando Caudalimetro");
  Serial.print("===========================");
  Serial.println("Haciendo Cálculos");

}

void loop() {
  // put your main code here, to run repeatedly:
  entrada=digitalRead(sensor);
  if(entrada == HIGH){
    digitalWrite(led,HIGH);
    delay(200);
    digitalWrite(led,LOW);
    delay(200);
    if(!sensorActivado){
      contador++;
      velocidad = 2*pi*contador;

      sensorActivado = true;
      Serial.print("Activación #");
      Serial.println(contador);
      Serial.println(velocidad,"m/s");
      for (int i = 0; i < 15; i++){
        digitalWrite(led, HIGH);
        delay(200);
        digitalWrite(led, LOW);
        delay(200);
      }
    }
    delay(200);
    digitalWrite(led,LOW);
    delay(200);
  }
  else{
  digitalWrite(led,LOW);
  sensorActivado = false;
  }
}
