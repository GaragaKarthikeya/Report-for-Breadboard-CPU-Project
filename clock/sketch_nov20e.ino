void setup() {
  // put your setup code here, to run once:
pinMode(13,OUTPUT);
digitalWrite(13,LOW);//initially set the pin to low
delay(1);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(13,HIGH);
  delay(1000);
  digitalWrite(13,HIGH);
  delay(1000);

}
