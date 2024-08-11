#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <AccelStepper.h>
#include <HX711.h>
#include <DHT.h>
#include <Servo.h>

// LCD Display
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Motors (conveyor belts)
AccelStepper stepper1(AccelStepper::FULL4WIRE, 2, 3, 4, 5);  // M1
AccelStepper stepper2(AccelStepper::FULL4WIRE, 6, 7, 8, 9);  // M2
AccelStepper stepper3(AccelStepper::FULL4WIRE, 10, 11, 12, 13);  // M3
AccelStepper stepper4(AccelStepper::FULL4WIRE, 22, 23, 24, 25);  // M4
AccelStepper stepper5(AccelStepper::FULL4WIRE, 26, 27, 28, 29);  // M5
AccelStepper stepper6(AccelStepper::FULL4WIRE, 30, 31, 32, 33);  // M6

// Load Cells 
HX711 scale1;  // For Storage 1
HX711 scale2;  // For Storage 2

#define LOADCELL1_DOUT_PIN 34
#define LOADCELL1_SCK_PIN 35
#define LOADCELL2_DOUT_PIN 44
#define LOADCELL2_SCK_PIN 45

// Temperature Sensor
#define DHTPIN 36
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Servos for Valves
Servo valve1;
Servo valve2;
#define VALVE1_PIN 37
#define VALVE2_PIN 38

// LEDs
#define LED_START 39
#define LED_STOP 40
#define LED_PROCESS 41
#define LED_COMPLETE 42

// Buttons
#define BUTTON_START 50
#define BUTTON_STOP 51


bool systemRunning = false;
bool storage1Filled = false;
bool storage2Filled = false;

void setup() {
  
  lcd.begin(16, 2);
  lcd.backlight();

  stepper1.setMaxSpeed(1000);
  stepper2.setMaxSpeed(1000);
  stepper3.setMaxSpeed(1000);
  stepper4.setMaxSpeed(1000);
  stepper5.setMaxSpeed(1000);
  stepper6.setMaxSpeed(1000);

  scale1.begin(LOADCELL1_DOUT_PIN, LOADCELL1_SCK_PIN);
  scale2.begin(LOADCELL2_DOUT_PIN, LOADCELL2_SCK_PIN);

  dht.begin();

  valve1.attach(VALVE1_PIN);
  valve2.attach(VALVE2_PIN);

  pinMode(LED_START, OUTPUT);
  pinMode(LED_STOP, OUTPUT);
  pinMode(LED_PROCESS, OUTPUT);
  pinMode(LED_COMPLETE, OUTPUT);

  pinMode(BUTTON_START, INPUT_PULLUP);
  pinMode(BUTTON_STOP, INPUT_PULLUP);

  // Start system in stopped state
  systemRunning = false;
  digitalWrite(LED_STOP, HIGH);
  lcd.setCursor(0, 0);
  lcd.print("System Stopped");
}

void loop() {
  if (digitalRead(BUTTON_START) == LOW) {
    startSystem();
  }

  if (digitalRead(BUTTON_STOP) == LOW) {
    stopSystem();
  }

  if (systemRunning) {
    runProcess();
  }
}

void startSystem() {
  if (checkTank1()) { 
    systemRunning = true;
    digitalWrite(LED_START, HIGH);
    digitalWrite(LED_STOP, LOW);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("System Running");
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Tank 01 Empty!");
  }
}

void stopSystem() {
  systemRunning = false;
  digitalWrite(LED_START, LOW);
  digitalWrite(LED_STOP, HIGH);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Stopped");
}

void runProcess() {
  digitalWrite(LED_PROCESS, HIGH);

  // Step 1: Fill Storage 1
  if (!storage1Filled) {
    stepper1.moveTo(1000); // Motor 1 runs
    stepper1.runToPosition();
    delay(1000); // Simulate time to fill storage

    if (checkStorage1()) {
      storage1Filled = true;
      valve1.write(90); // Open Valve 1
      delay(1000);
      stepper2.moveTo(1000); // Motor 2 runs
      stepper2.runToPosition();
      stepper1.stop(); // Motor 1 stops
    }
  }

  // Step 2: Steaming Process
  if (storage1Filled) {
    float temp = dht.readTemperature();
    lcd.setCursor(0, 1);
    lcd.print("Steamer Temp: ");
    lcd.print(temp);

    if (temp >= 100) { // temperature
      stepper3.moveTo(1000); // Motor 3 runs
      stepper3.runToPosition();
      stepper2.stop(); // Motor 2 stops

      delay(10000); // drying time

      stepper3.moveTo(2000); // Motor 3 runs again to fill Storage 2
      stepper3.runToPosition();

      storage2Filled = true;
    }
  }

  // Step 3: Packing Process
  if (storage2Filled) {
    if (checkStorage2()) {
      valve2.write(90); // Open Valve 2
      delay(1000);

      for (int i = 0; i < 100; i++) {
        stepper4.moveTo(100); // Motor 4 runs
        stepper4.runToPosition();

        delay(500); //  filling time

        stepper5.moveTo(100); // Motor 5 runs to move filled packets
        stepper5.runToPosition();
      }

      stepper6.moveTo(1000); // Motor 6 runs to store packets
      stepper6.runToPosition();

      digitalWrite(LED_COMPLETE, HIGH);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Process Complete");
      delay(2000);
      digitalWrite(LED_COMPLETE, LOW);

      storage1Filled = false; // Reset for next process cycle
      storage2Filled = false;
    }
  }

  digitalWrite(LED_PROCESS, LOW);
  stopSystem(); // Stop system after process completes
}

bool checkTank1() {
  return true; 
}

bool checkStorage1() {
  // Check Storage 1 
  long weight = scale1.get_units(10);
  return weight > 1000; 
}

bool checkStorage2() {
  // Check Storage 2 
  long weight = scale2.get_units(10);
  return weight < 100; 
}
