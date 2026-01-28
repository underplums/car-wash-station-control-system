#include <IRremote.h>

const int RECV_PIN = 11;
IRrecv irrecv(RECV_PIN);
decode_results results;

void setup() {
  Serial.begin(9600);
  Serial.println("IR decoder is ready.");
  irrecv.enableIRIn();
}

void loop() {
  if (irrecv.decode(&results)) {
    Serial.print("HEX code: 0x");
    Serial.println(results.value, HEX);
    
    Serial.print("Used protocol: ");
    Serial.println(results.decode_type);
    
    Serial.print("Bits: ");
    Serial.println(results.bits);
    Serial.println("---");
    
    irrecv.resume();
  }
}