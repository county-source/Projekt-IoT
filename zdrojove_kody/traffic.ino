// Definice pinů
const int pinVerejneOsvetleni = 13;
const int pinCervenaChodci = 8;
const int pinZelenaChodci = 9;
const int pinOranzovaAuta = 10;
const int pinCervenaAuta = 11;
const int pinZelenaAuta = 12;
const int pinTlacitko = 2;
const int pinFotorezistor = A0;

// Nastavení pro osvětlení
const int prahoveNapetiProOsvetleni = 512; // Předpokládáme polovinu rozsahu ADC

void setup() {
  pinMode(pinVerejneOsvetleni, OUTPUT);
  pinMode(pinCervenaChodci, OUTPUT);
  pinMode(pinZelenaChodci, OUTPUT);
  pinMode(pinCervenaAuta, OUTPUT);
  pinMode(pinOranzovaAuta, OUTPUT);
  pinMode(pinZelenaAuta, OUTPUT);
  pinMode(pinTlacitko, INPUT);
}

void loop() {
  // Kontrola fotorezistoru pro zapnutí/vypnutí veřejného osvětlení
  int hodnotaSvetla = analogRead(pinFotorezistor);
  bool jeNoc = hodnotaSvetla < prahoveNapetiProOsvetleni; // Pokud je hodnota světla pod prahem, považujeme to za noc

  digitalWrite(pinVerejneOsvetleni, jeNoc);

  // Noční blikání semaforu
  if (jeNoc) {
    digitalWrite(pinCervenaChodci, LOW);
    digitalWrite(pinZelenaAuta, LOW);
    digitalWrite(pinOranzovaAuta, HIGH);
    delay(1000);
    digitalWrite(pinOranzovaAuta, LOW);
    delay(1000);
  } else {
    //Normální stav světel
    digitalWrite(pinZelenaAuta, HIGH);
    digitalWrite(pinCervenaChodci, HIGH);

    // Sekvence přechodu
    if (digitalRead(pinTlacitko) == HIGH) {
      digitalWrite(pinOranzovaAuta, HIGH);
      digitalWrite(pinZelenaAuta, LOW);
      delay(1500);
      digitalWrite(pinCervenaAuta, HIGH);
      digitalWrite(pinOranzovaAuta, LOW);
      delay(1500);
      digitalWrite(pinCervenaChodci, LOW);
      digitalWrite(pinZelenaChodci, HIGH);
      delay(5000);
      digitalWrite(pinCervenaChodci, HIGH);
      digitalWrite(pinZelenaChodci, LOW);
      delay(1500);
      digitalWrite(pinOranzovaAuta, HIGH);
      delay(1500);
      digitalWrite(pinCervenaAuta, LOW);
      digitalWrite(pinOranzovaAuta, LOW);
    }
  }
}
