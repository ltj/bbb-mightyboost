/* Moteino RFM69 gateway and MightyBoost control fusion
 * based on examples by Felix Rusu of LowPowerLab - felix@lowpowerlab.com
 * Listens for incoming RFM69 packets and relays them via serial 
 * in "JeeNode" (see jeelabs.org) format. It also manages MightyBoost 
 * button pushes and signals, but specifically tailored for use with
 * a BeagleBone Black.
 *
 * by Lars Toft Jacobsen (boxed.dk)
 */

#include <avr/wdt.h>
#include <RFM69.h>
#include <SPI.h>
#include <SPIFlash.h>

#define NODEID           1       //unique for each node on same network
#define NETWORKID        100     //the same on all nodes that talk to each other
#define FREQUENCY        RF69_868MHZ
#define ENCRYPTKEY       "_encryption_key_" //exactly the same 16 characters/bytes on all nodes!
#define ACK_TIME         30      // max # of ms to wait for an ack
#define LED              9       // Moteinos have LEDs on D9
#define BTNLED           5
#define BUTTON           3
#define SERIAL_BAUD      57600
#define SIG_SHUTOFF      6
#define SIG_BOOTOK       7
#define OUTPUT_5V        4
#define BATTSENSE        A7
#define LOBAT_THRESHOLD  3.7
#define BUTTON_HOLD      2000    // time to hold button before it registers
#define BUTTON_DEBOUNCE  20      // window to allow valid button reading
#define HALT_MIN_WAIT    6000    // how long to initially wait for bootOK to go LOW
#define HALT_MAX_WAIT    30000   // maximum waiting for bootOK to go LOW
#define CUTOFF_DELAY     5000    // final delay before cutting the power
#define CHECK_INTERVAL   10000   // interval for checking battery status

RFM69 radio;
SPIFlash flash(8, 0x1F45); //8Mbit Atmel chip

byte bootState, btnState, btnPrevState, btnPushed, bbb_powerstate;
unsigned long push_start, sig_start, dly_start, prev_check, prev_pwm;
float systemVoltage = 5.0;
int state;

enum STATE {
  ST_OFF,
  ST_POWERED,
  ST_BOOTED,
  ST_SHUTDOWN,
  ST_HALTED
};
  
void setup() {
  // serial setup
  Serial.begin(SERIAL_BAUD);
  delay(10);
  
  // pin setup
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(SIG_BOOTOK, INPUT);
  pinMode(SIG_SHUTOFF, OUTPUT);
  pinMode(BTNLED, OUTPUT);
  pinMode(OUTPUT_5V, OUTPUT);
  pinMode(BATTSENSE, INPUT);
  // start with power off and no shutdown signal
  digitalWrite(SIG_SHUTOFF, LOW);
  digitalWrite(OUTPUT_5V, LOW);
  
  // rf setup
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  radio.setHighPower();
  radio.encrypt(ENCRYPTKEY);
  radio.promiscuous(false);
  char buff[50];
  sprintf(buff, "\nListening at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);
  
  // flash setup
  if (flash.initialize())
    Serial.println("SPI Flash Init OK!");
  else
    Serial.println("SPI Flash Init FAIL! (is chip present?)");
    
  // initial state
  bootState = 0;     // BBB no bootOK signal
  btnState = 1;      // Button not pushed (pulled up)
  btnPrevState = 1;  // Prev. button state
  btnPushed = 0;     // Button has not been pushed
  bbb_powerstate = 0;// Power state is OFF
  state = ST_OFF;    // Initial state is OFF
    
  // enable WDT
  wdt_enable(WDTO_4S);
}

void loop() {
  
  wdt_reset();
  
  // simple state machine
  switch(state) {
    
    case ST_OFF:
      if(buttonPushed()) {
        bbb_powerstate = 1;
        digitalWrite(OUTPUT_5V, bbb_powerstate);
        state = ST_POWERED;
      }
      break;
      
    case ST_POWERED:
      bootState = digitalRead(SIG_BOOTOK);
      if(bootState == HIGH) {
        digitalWrite(BTNLED, bbb_powerstate);
        state = ST_BOOTED;
      }
      break;
      
    case ST_BOOTED:
      if(buttonPushed() || (systemVoltage <= LOBAT_THRESHOLD)) {
        sig_start = millis();
        digitalWrite(SIG_SHUTOFF, HIGH);
        state = ST_SHUTDOWN;
      }
      break;
      
    case ST_SHUTDOWN:
      if(millis()-sig_start >= HALT_MIN_WAIT) {
        bootState = digitalRead(SIG_BOOTOK);
        if(bootState == 0 && bbb_powerstate == 1) {
          bbb_powerstate = 0;
          digitalWrite(BTNLED, bbb_powerstate);
          digitalWrite(SIG_SHUTOFF, LOW);
          dly_start = millis();
          state = ST_HALTED;
        }
        else if(bootState == 1 && millis()-sig_start >= HALT_MAX_WAIT) {
          digitalWrite(SIG_SHUTOFF, LOW);
          state = ST_BOOTED;
        }
      }
      break;
      
    case ST_HALTED:
      if(millis()-dly_start >= CUTOFF_DELAY) {
        digitalWrite(OUTPUT_5V, bbb_powerstate);
        state = ST_OFF;
      }
      break;
      
  }
  
  // Get incoming radio packets and report to serial
  if (radio.receiveDone())
  {
    wdt_reset();
    Serial.print("OK ");
    Serial.print(radio.SENDERID, DEC);
    Serial.print(" ");
    for (byte i = 0; i < radio.DATALEN; i++) {
      Serial.print((word)radio.DATA[i]);
      Serial.print(" ");
    }
    Serial.print(radio.RSSI);

    if (radio.ACK_REQUESTED)
    {
      byte theNodeID = radio.SENDERID;
      radio.sendACK();
    }
    Serial.println();
    Blink(LED,3);
  }
  
  // Check voltage and initiate shutdown if battery goes below 3.5V
  unsigned long now = millis();
  if (now - prev_check >= CHECK_INTERVAL) {
    prev_check = now;
    systemVoltage = analogRead(BATTSENSE) * 0.00322 * 1.47;
    if (systemVoltage > 4.3) {
      Serial.println("MB LINE-OK");
    }
    else {
      Serial.print("MB BATTERY ");
      Serial.println(systemVoltage);
    }
  }
  
  wdt_reset();
}

// blink LED
void Blink(byte PIN, int DELAY_MS) {
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}

// check if button is pushed for BUTTON_HOLD ms
boolean buttonPushed() {
  btnState = digitalRead(BUTTON);
  // button pressed or released
  if(btnState != btnPrevState) {
    btnPrevState = btnState;
    // push begin
    if (btnState == 0) {
      push_start = millis(); // start the clock
    }
  }
  // still pushing
  else if(btnState == 0) {
    unsigned long delta = millis()-push_start;
    return ((delta >= BUTTON_HOLD) && (delta <= (BUTTON_HOLD + BUTTON_DEBOUNCE)));
  }
  return false;
}
  


