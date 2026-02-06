//Пины для сигналов с датчиков

const int LS1_PIN = 2;
const int LS2_PIN = 3;
const int LS3_PIN = 4;
const int LS4_PIN = 5;
const int LS5_PIN = 6;

//Пин для кнопки открытия выездных ворот

const int OPEN_BUTTON_PIN = 7;

// Светофор пока что моделирую тремя сигналами, протестирую сначала на трехсветодиодах из того комлпекта, который заказала. Далее уже совмещу с функционалом signal_reciever.c & signal_transmitter.c

const int TL_RED_PIN = 8;
const int TL_YELLOW_PIN = 9;
const int TL_GREEN_PIN = 10;

//Пины управления воротами

const int GATE_IN_OPEN_PIN = 11; 
const int GATE_IN_CLOSE_PIN = 12;
const int GATE_OUT_OPEN_PIN = A0;
const int GATE_OUT_CLOSE_PIN = A1;

// Константы для обеспечения антидребезга датчиков, а также установы по безопасности и времени закрытия/открытия ворот. Поменяю в зависимости от работы датчиков

const unsigned long GATE_OPERATION_TIME = 5000;
const unsigned long SENSOR_DEBOUNCE = 100;
const unsigned long BUTTON_DEBOUNCE = 50;
const unsigned long GATE_CLOSE_DELAY = 2000;
const unsigned long SAFETY_TIMEOUT = 30000;

//Объявление состояний системы

enum SystemState {
  IDLE,
  GATE_IN_OPENING,
  CAR_ENTERING,
  GATE_IN_CLOSING,
  CAR_INSIDE,
  GATE_OUT_OPENING,
  CAR_EXITING,
  GATE_OUT_CLOSING,
  ERROR_STATE
};

//Глобальные констаты и флаги состояния системы

SystemState currentState = IDLE;
SystemState previousState = IDLE;

bool ls1State = true;
bool ls2State = true;
bool ls3State = true;
bool ls4State = true;
bool ls5State = true;

bool ls1Prev = true;
bool ls2Prev = true;
bool ls3Prev = true;
bool ls4Prev = true;
bool ls5Prev = true;

unsigned long ls1LastChange = 0;
unsigned long ls2LastChange = 0;
unsigned long ls3LastChange = 0;
unsigned long ls4LastChange = 0;
unsigned long ls5LastChange = 0;

bool buttonPressed = false;
bool buttonPrevState = false;
unsigned long buttonLastChange = 0;

unsigned long stateStartTime = 0;
unsigned long gateOperationStartTime = 0;

bool gateInOpen = false;
bool gateOutOpen = false;

// Счетчик проездов (для отладки и статистики)

unsigned long carCounter = 0;


void setup() {

  // Для отладки в терминале

  Serial.begin(9600);
  Serial.println(F("=== Сar Wash Station System ==="));
  Serial.println(F("Ver. 1.0"));
  
  //Настройка работы пинов

  pinMode(LS1_PIN, INPUT_PULLUP);
  pinMode(LS2_PIN, INPUT_PULLUP);
  pinMode(LS3_PIN, INPUT_PULLUP);
  pinMode(LS4_PIN, INPUT_PULLUP);
  pinMode(LS5_PIN, INPUT_PULLUP);

  pinMode(OPEN_BUTTON_PIN, INPUT_PULLUP);

  pinMode(TL_RED_PIN, OUTPUT);
  pinMode(TL_YELLOW_PIN, OUTPUT);
  pinMode(TL_GREEN_PIN, OUTPUT);

  pinMode(GATE_IN_OPEN_PIN, OUTPUT);
  pinMode(GATE_IN_CLOSE_PIN, OUTPUT);
  pinMode(GATE_OUT_OPEN_PIN, OUTPUT);
  pinMode(GATE_OUT_CLOSE_PIN, OUTPUT);
  
  initializeSystem();
  
  Serial.println(F("System is ready"));
}

void loop() {

  readSensors();
  readButton();
  handleStateMachine();
  checkSafetyTimeouts();
  
  debugPrint();
  
  delay(10);
}

// Управляющие функции. Все принты - исключительно для дебаггинга и вывода в терминал. В финальном варианте можно убрать, либо оставить для возможной будущей отладки.

void initializeSystem() {

  closeGateIn();
  closeGateOut();
  setTrafficLight(0, 0, 1);
  
  currentState = IDLE;
  stateStartTime = millis();
  
  Serial.println(F("System state: IDLE"));
}

void readSensors() {

  unsigned long currentTime = millis();
  
  // LS1
  bool ls1Raw = digitalRead(LS1_PIN);
  if (ls1Raw != ls1Prev && (currentTime - ls1LastChange > SENSOR_DEBOUNCE)) {
    ls1State = ls1Raw;
    ls1Prev = ls1Raw;
    ls1LastChange = currentTime;
    Serial.print(F("LS1: "));
    Serial.println(ls1State ? F("Uncovered") : F("Covered"));
  }
  
  // LS2
  bool ls2Raw = digitalRead(LS2_PIN);
  if (ls2Raw != ls2Prev && (currentTime - ls2LastChange > SENSOR_DEBOUNCE)) {
    ls2State = ls2Raw;
    ls2Prev = ls2Raw;
    ls2LastChange = currentTime;
    Serial.print(F("LS2: "));
    Serial.println(ls2State ? F("Uncovered") : F("Covered"));
  }
  
  // LS3
  bool ls3Raw = digitalRead(LS3_PIN);
  if (ls3Raw != ls3Prev && (currentTime - ls3LastChange > SENSOR_DEBOUNCE)) {
    ls3State = ls3Raw;
    ls3Prev = ls3Raw;
    ls3LastChange = currentTime;
    Serial.print(F("LS3: "));
    Serial.println(ls3State ? F("Uncovered") : F("Covered"));
  }
  
  // LS4
  bool ls4Raw = digitalRead(LS4_PIN);
  if (ls4Raw != ls4Prev && (currentTime - ls4LastChange > SENSOR_DEBOUNCE)) {
    ls4State = ls4Raw;
    ls4Prev = ls4Raw;
    ls4LastChange = currentTime;
    Serial.print(F("LS4: "));
    Serial.println(ls4State ? F("Uncovered") : F("Covered"));
  }
  
  // LS5
  bool ls5Raw = digitalRead(LS5_PIN);
  if (ls5Raw != ls5Prev && (currentTime - ls5LastChange > SENSOR_DEBOUNCE)) {
    ls5State = ls5Raw;
    ls5Prev = ls5Raw;
    ls5LastChange = currentTime;
    Serial.print(F("LS5: "));
    Serial.println(ls5State ? F("Uncovered") : F("Covered"));
  }
}

void readButton() {
  bool buttonRaw = !digitalRead(OPEN_BUTTON_PIN);
  unsigned long currentTime = millis();
  
  if (buttonRaw != buttonPrevState && (currentTime - buttonLastChange > BUTTON_DEBOUNCE)) {
    buttonPressed = buttonRaw;
    buttonPrevState = buttonRaw;
    buttonLastChange = currentTime;
    
    if (buttonPressed) {
      Serial.println(F("Button pressed"));
    }
  }
}

void setTrafficLight(int red, int yellow, int green) {
  digitalWrite(TL_RED_PIN, red);
  digitalWrite(TL_YELLOW_PIN, yellow);
  digitalWrite(TL_GREEN_PIN, green);
}


void openGateIn() {
  if (!gateInOpen) {
    Serial.println(F("Gates In Are Opening"));
    digitalWrite(GATE_IN_CLOSE_PIN, LOW);
    digitalWrite(GATE_IN_OPEN_PIN, HIGH);
    gateOperationStartTime = millis();
    gateInOpen = true;
  }
}

void closeGateIn() {
  if (gateInOpen) {
    Serial.println(F("Gates In Are Closing"));
    digitalWrite(GATE_IN_OPEN_PIN, LOW);
    digitalWrite(GATE_IN_CLOSE_PIN, HIGH);
    gateOperationStartTime = millis();
    gateInOpen = false;
  }
}

void stopGateIn() {
  digitalWrite(GATE_IN_OPEN_PIN, LOW);
  digitalWrite(GATE_IN_CLOSE_PIN, LOW);
}

void openGateOut() {
  if (!gateOutOpen) {
    Serial.println(F("Gates Out Are Opening"));
    digitalWrite(GATE_OUT_CLOSE_PIN, LOW);
    digitalWrite(GATE_OUT_OPEN_PIN, HIGH);
    gateOperationStartTime = millis();
    gateOutOpen = true;
  }
}

void closeGateOut() {
  if (gateOutOpen) {
    Serial.println(F("Gates Out Are Closing"));
    digitalWrite(GATE_OUT_OPEN_PIN, LOW);
    digitalWrite(GATE_OUT_CLOSE_PIN, HIGH);
    gateOperationStartTime = millis();
    gateOutOpen = false;
  }
}

void stopGateOut() {
  digitalWrite(GATE_OUT_OPEN_PIN, LOW);
  digitalWrite(GATE_OUT_CLOSE_PIN, LOW);
}

void changeState(SystemState newState) {
  if (currentState != newState) {
    previousState = currentState;
    currentState = newState;
    stateStartTime = millis();
    
    Serial.print(F("Changing state: "));
    printState(newState);
  }
}

void printState(SystemState state) {
  switch (state) {
    case IDLE:
      Serial.println(F("IDLE"));
      break;
    case GATE_IN_OPENING:
      Serial.println(F("GATE_IN_OPENING"));
      break;
    case CAR_ENTERING:
      Serial.println(F("CAR_ENTERING"));
      break;
    case GATE_IN_CLOSING:
      Serial.println(F("GATE_IN_CLOSING"));
      break;
    case CAR_INSIDE:
      Serial.println(F("CAR_INSIDE"));
      break;
    case GATE_OUT_OPENING:
      Serial.println(F("GATE_OUT_OPENING"));
      break;
    case CAR_EXITING:
      Serial.println(F("CAR_EXITING"));
      break;
    case GATE_OUT_CLOSING:
      Serial.println(F("GATE_OUT_CLOSING"));
      break;
    case ERROR_STATE:
      Serial.println(F("ERROR_STATE"));
      break;
  }
}

void handleStateMachine() {
  unsigned long currentTime = millis();
  
  switch (currentState) {
    
    case IDLE:

      // Ворота закрыты, светофор зеленый

      if (!gateInOpen && !gateOutOpen) {
        setTrafficLight(0, 0, 1);
      }
      
      // Если LS1 перекрыт - начать открытие въездных ворот

      if (!ls1State) {
        changeState(GATE_IN_OPENING);
        openGateIn();
        setTrafficLight(0, 1, 0);
      }
      break;
    

    case GATE_IN_OPENING:
      
      if (currentTime - gateOperationStartTime >= GATE_OPERATION_TIME) {
        stopGateIn();
        changeState(CAR_ENTERING);
      }
      
      // Eсли машина уехала - закрыть ворота

      if (ls1State && currentTime - stateStartTime > 3000) {
        closeGateIn();
        changeState(GATE_IN_CLOSING);
      }
      break;
    

    case CAR_ENTERING:

      // Ждем, пока машина проедет LS2, LS3, LS4
      
      if (!ls2State && ls3State && ls4State) {
      }
      
      if (!ls3State || !ls4State) {
      }
      
      // Если LS2, LS3, LS4 засвечены - закрыть въездные ворота

      if (ls2State && ls3State && ls4State && !ls1State) {
        changeState(GATE_IN_CLOSING);
        closeGateIn();
        setTrafficLight(1, 0, 0);
      }
      
      if (currentTime - stateStartTime > SAFETY_TIMEOUT) {
        Serial.println(F("TIMEOUT: Car entering"));
        changeState(GATE_IN_CLOSING);
        closeGateIn();
      }
      break;
    
    case GATE_IN_CLOSING:

      if (currentTime - gateOperationStartTime >= GATE_OPERATION_TIME) {
        stopGateIn();
        changeState(CAR_INSIDE);
        setTrafficLight(1, 0, 0);
        carCounter++;
        Serial.print(F("Car inside. Total car counter: "));
        Serial.println(carCounter);
      }
      break;
    

    case CAR_INSIDE:

      // Ждем нажатия кнопки открытия выездных ворот

      if (buttonPressed) {
        changeState(GATE_OUT_OPENING);
        openGateOut();
        setTrafficLight(0, 1, 0);
        buttonPressed = false;
      }
      break;
    
    case GATE_OUT_OPENING:

      // Ждем завершения открытия

      if (currentTime - gateOperationStartTime >= GATE_OPERATION_TIME) {
        stopGateOut();
        changeState(CAR_EXITING);
      }
      break;
    
    case CAR_EXITING:

      if (!ls5State) {
      }
      
      // Если LS5, LS3, LS4 засвечены - машина выезжает

      if (ls5State && ls3State && ls4State) {

        if (currentTime - stateStartTime > GATE_CLOSE_DELAY) {
          changeState(GATE_OUT_CLOSING);
          closeGateOut();
          setTrafficLight(0, 0, 1);
        }
      }
      
      // Таймаут безопасности

      if (currentTime - stateStartTime > SAFETY_TIMEOUT) {
        Serial.println(F("TIMEOUT: Car exiting"));
        changeState(GATE_OUT_CLOSING);
        closeGateOut();
      }
      break;
    

    case GATE_OUT_CLOSING:

      if (currentTime - gateOperationStartTime >= GATE_OPERATION_TIME) {
        stopGateOut();
        
        // Проверяем, есть ли следующий клиент

        if (!ls1State) {
          Serial.println(F("Next client is waiting"));
          changeState(GATE_IN_OPENING);
          openGateIn();
          setTrafficLight(0, 1, 0);
        } else {
          changeState(IDLE);
          setTrafficLight(0, 0, 1);
        }
      }
      break;
    
    // Аварийное состояние (доп. функция, далее ее доработаю по требованию)

    case ERROR_STATE:

      setTrafficLight(1, 0, 0);
      delay(500);
      setTrafficLight(0, 0, 0);
      delay(500);
      
      // Сброс в IDLE (ожидание) по кнопке

      if (buttonPressed) {
        Serial.println(F("System reboot"));
        initializeSystem();
        buttonPressed = false;
      }
      break;
  }
}

// Таймауты по безопасности

void checkSafetyTimeouts() {
  unsigned long currentTime = millis();
  
  if (gateInOpen && currentTime - gateOperationStartTime > SAFETY_TIMEOUT * 2) {
    Serial.println(F("SAFETY': Gates In Are Open Too Long "));
    closeGateIn();
  }
  
  if (gateOutOpen && currentTime - gateOperationStartTime > SAFETY_TIMEOUT * 2) {
    Serial.println(F("SAFETY': Gates Out Are Open Too Long"));
    closeGateOut();
  }
  
}

// Отладка и дебаггинг

void debugPrint() {
  static unsigned long lastDebugPrint = 0;
  unsigned long currentTime = millis();
  
  if (currentTime - lastDebugPrint > 2000) {
    Serial.println(F("--- DEBUG ---"));
    Serial.print(F("State: "));
    printState(currentState);
    Serial.print(F("LS1: ")); Serial.print(ls1State);
    Serial.print(F(" LS2: ")); Serial.print(ls2State);
    Serial.print(F(" LS3: ")); Serial.print(ls3State);
    Serial.print(F(" LS4: ")); Serial.print(ls4State);
    Serial.print(F(" LS5: ")); Serial.println(ls5State);
    Serial.print(F("Gate IN: ")); Serial.print(gateInOpen ? F("OPEN") : F("CLOSED"));
    Serial.print(F(" Gate OUT: ")); Serial.println(gateOutOpen ? F("OPEN") : F("CLOSED"));
    Serial.println(F("-------------"));
    
    lastDebugPrint = currentTime;
  }
}