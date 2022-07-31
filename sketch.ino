#include <TimerOne.h> //Укључивање библиотеке која нам омогућава рад са Timer1
#define ERROR_LED_PIN 8 //Дефинисање пина диоде која означава стање грешке
#define EXTERNAL_INTERUPT_BUTTON_PIN 2 //Дефинисање пина тастера екстерног прекида
#define MAX_GAME_DIODE_COUNT 100 //Дефинисање максималног нивоа (100 диода у секвенци)
#define NUMBER_OF_DIODES 4 //Дефинисање броја диода

const byte ledPins[NUMBER_OF_DIODES] = {7, 6, 5, 4}; //Дефинисање низа пинова диода
const byte buttonPins[NUMBER_OF_DIODES] = {13, 12, 11, 10}; //Дефинисање низа пинова тастера (редослед одговара редоследу диода)
byte gameSequence[MAX_GAME_DIODE_COUNT]; //Дефинисање низа који чува секвенцу диода (максималне дужине 100)
byte lastCorrectDiodeInSequenceIndex, currentSequenceDiodeCount, triesCount;
/* Дефинисање промењљивих који чувају индекс последње погођене диоде у секвенци,
  тренутног броја диода у секвенци и број покушаја погађања */
bool isUserTimeoutAllowed; //Дефинисање промењљиве која проверава да ли је дозвољено активирати временски прекид

enum {
  IDLE_STATE,
  BEFORE_GAME_START,
  NEW_LEVEL_STARTED,
  WAITING_FOR_USER_SEQUENCE,
  BEFORE_NEXT_LEVEL,
  ON_GAME_END,
} state; //Дефинисање промењљиве enum типа која означава могућа стања током игре

void blinkDiode(byte diodePin){ //Функција која омогућава трептај једне диоде
  digitalWrite(diodePin, HIGH); //Паљење диоде
  delay(500); //Пауза
  digitalWrite(diodePin, LOW); //Гашење диоде
  delay(500); //Пауза
}

void playCurrentSequence() { //Функција кoја пролази кроз секвенцу и пали појединачне диоде
  for (int i = 0; i < currentSequenceDiodeCount; i++) { //Петља за пролазак кроз секвенцу
    blinkDiode(ledPins[gameSequence[i]]);
  }
  state = WAITING_FOR_USER_SEQUENCE; //Промена стања у стање чекања да корисник понови секвенцу
}

void startGame() { //Функција екстерног прекида за враћање првобитних вредности приликом покретања нове игре
  triesCount = 0;
  lastCorrectDiodeInSequenceIndex = 0;
  memset(gameSequence, 0, sizeof(gameSequence));
  gameSequence[0] = random(0, NUMBER_OF_DIODES); //Генерисање прве вредности у низу секвенци...
  currentSequenceDiodeCount = 1; //...и померање индекса у низу секвенци за један јер се почетна секвенца састоји од 2 диоде
  digitalWrite(ERROR_LED_PIN, LOW);
  digitalWrite(EXTERNAL_INTERUPT_BUTTON_PIN, LOW);
  detachInterrupt(digitalPinToInterrupt(EXTERNAL_INTERUPT_BUTTON_PIN)); //Уклањамо екстерни прекид јер нам више није потребан
  startNextLevel();
}

void startNextLevel(){ //Функција екстреног прекида за прелазак на следећи ниво
  delay(5*1000*1000);
  state = NEW_LEVEL_STARTED; //Прелазак у стање почетка новог нивоа
}

void wrongButtonPressed() { //Функција која се позива услед притиска погрешног тастера или путем временског прекида
  lastCorrectDiodeInSequenceIndex = 0; //Ресетовање бројача тренутне диоде коју је корисник последње погодио
  Serial.print(", почните секвенцу испочетка - Имате још "); //Наредне три линије исписују поруку о преосталом броју покушаја
  Serial.print(3 - ++triesCount);
  Serial.println(" покушај(а)! -");
  for (int i = 0; i < triesCount; i++) { //Диода која представља грешку, а она трепће онолико пута колико је покушаја искоришћено
    blinkDiode(ERROR_LED_PIN);
  }
  isUserTimeoutAllowed = false; //Забрана активирања временског прекида (активираће се поново када корисник притисне први исправан тастер поново)
  if (triesCount >= 3) state = ON_GAME_END; //Уколико је истекао број покушаја систем прелази у стање завршетка
}

void checkUserSequenceTimeout() { //Прекидна рутина која настаје након истека времена тајмера
  if (isUserTimeoutAllowed && triesCount < 3) { //Провера да ли је дозвољено активирати временски прекид
    Serial.print("- Сувише сте дуго чекали да притиснете следећи тастер");
    wrongButtonPressed(); //Позив функције грешке
  } //Овај временски прекид се активира када корисник чека сувише дуго (3.5 секунди) између понављања две узастопне диоде у секвенци
}

void setup() { //Функција која се увек позива приликом покретања ардуина
  Serial.begin(9600); //Иницијализација серијског приказа са брзином преноса података од 9600 bps.
  for(int i = 0; i < NUMBER_OF_DIODES; i++){
    pinMode(ledPins[i], OUTPUT); //Постављање дигиталних пинова диода као излазне
    pinMode(buttonPins[i], INPUT_PULLUP); //https://create.arduino.cc/projecthub/Hack-star-Arduino/push-buttons-and-arduino-a-simple-guide-wokwi-simulator-c2281f?ref=user&ref_id=1743724&offset=0
  }
  pinMode(ERROR_LED_PIN, OUTPUT);
  pinMode(EXTERNAL_INTERUPT_BUTTON_PIN, INPUT_PULLUP); //Користи се интерни pull-up отпорник
  
  attachInterrupt(digitalPinToInterrupt(EXTERNAL_INTERUPT_BUTTON_PIN), startGame, FALLING); //Додељује се функција покретања нове игре екстерног прекида зеленом тастеру која се активира детектовањем силазне ивице напона (1 --> 0)
  randomSeed(analogRead(A0)); //Иницијализација генератора псеудо сличајних бројева коришћењем вредности прочитане на аналогном пину А0 као seed
  Timer1.initialize(3500000); //Иницијализација тајмера за времеснки прекид приликом дугог чекања на 3.5 секунди
  Timer1.attachInterrupt(checkUserSequenceTimeout); //Везивање функције као прекидну рутину тајмера
  Timer1.stop(); //Прекид тајмера до почетка игре
  Serial.println("*** Добродошли у игру ***"); //Приказ поруке добродошлице
  state = BEFORE_GAME_START; //Промена стања на стање почетка нове игре
}

void loop() {
  switch (state) { //Провера стања у коме се тренутно налази систем
    case BEFORE_GAME_START: //Случај када је систем у стању пре почетка нове игре
      Serial.println("** Можете почети нову игру притиском зеленог тастер. Када игра почне будите спремни да испратите прву секвенцу која се састоји од две упаљене диоде. Срећно! **");
      state = IDLE_STATE; //Прелазак у стање мировања
      break;
    case NEW_LEVEL_STARTED: //Случај када је систем у стању почетка новог нивоа
      Serial.print("* НИВО "); //Наредне 4 линије исписују поруку која показује који је тренутни ниво
      Serial.print(currentSequenceDiodeCount);
      Serial.print(" ЈЕ ДОСТИГНУТ - ");
      Serial.println("ГЛЕДАЈТЕ ПАЖЉИВО ДИОДЕ! *");
      gameSequence[currentSequenceDiodeCount++] = random(0, NUMBER_OF_DIODES); //Додавање нове вредности индекса диоде у секвенцу и инкремент бројача укупне количине диода у секвенци за један 
      state = IDLE_STATE; //Прелазак у стање мировања
      playCurrentSequence(); //Позив функције за трептање диода по секвенци
      break;
    case WAITING_FOR_USER_SEQUENCE: //Случај када је систем у стању чекања на понављање секвенце од стране корисника
      Timer1.start(); //Покретање тајмера
      do {
        for (byte i = 0; i < NUMBER_OF_DIODES; i++) { //Петља која пролази кроз све тастере и проверава да ли је неки од њих притиснут
          if (digitalRead(buttonPins[i]) == LOW) { //Уколико је тастер притиснут дешава се провера
            blinkDiode(ledPins[i]);  
            if (i != gameSequence[lastCorrectDiodeInSequenceIndex] && triesCount < 3) { //Уколико та диода није тренутна у секвенци...
              Serial.print("- Погрешна диода"); //...исписујемо поруку и...
              wrongButtonPressed(); //...позивамо функцију грешке
            }
            else {
              Timer1.restart(); //Рестартовање бројача тајмера
              isUserTimeoutAllowed = true; //Давање дозволе за временски прекид
              lastCorrectDiodeInSequenceIndex++; //Инкремент бројача погођених диода у секвенци
            }
          }
        }
        delay(1); //Неопходна пауза приликом провере
      } while (lastCorrectDiodeInSequenceIndex < currentSequenceDiodeCount && triesCount < 3); //Петња се понавља докле год се не понови цела секвенца или истекне број покушаја
      Timer1.stop(); //Прекидање временског бројача
      if (state == WAITING_FOR_USER_SEQUENCE) state = BEFORE_NEXT_LEVEL; //Прелазак у стање пре почетка следећег нивоа (уколико се идаље налазимо у овом стању*)
      break; //* Могуће је да се нађемо и у стању завршетка игре уколико је искоришћен максимални број покушаја, а није погођена секвенца
    case BEFORE_NEXT_LEVEL: //Случај када је систем у стању пре почетка следећег нивоа
      Serial.println("+ ЧЕСТИТКЕ СЕКВЕНЦА ЈЕ ТАЧНА * Када будете спремни притисните зелени тастер за наставак +"); //Испис поруке честитке
      triesCount = 0; //Ресетовање броја покушаја
      lastCorrectDiodeInSequenceIndex = 0; //Ресетовање бројача погођених диода у секвенци
      if (currentSequenceDiodeCount >= MAX_GAME_DIODE_COUNT) { //Провера да ли смо погодили све диоде целокупне секвенце
        isUserTimeoutAllowed = false;
        state = ON_GAME_END; //Прелазак у стање завршетка игре
      } else {
        attachInterrupt(digitalPinToInterrupt(EXTERNAL_INTERUPT_BUTTON_PIN), startNextLevel, FALLING); //Додељује се функција промене нивоа екстерног прекида зеленом тастеру која се активира детектовањем силазне ивице напона (1 --> 0)
        state = IDLE_STATE; //Прелазак у стање мировања
      }
      break;
    case ON_GAME_END: //Случај када је систем у стању завршетка игре
      digitalWrite(ERROR_LED_PIN, HIGH); //Диода грешке остаје заувек упаљена
      if (triesCount >= 3) { //Провера да ли смо у ово стање дошли услед истека броја покушаја
        Serial.print("*** Игра је завршена услед истека броја покушаја! Достигли сте ниво: ");
        Serial.print(currentSequenceDiodeCount - 2); //Последњи достигнути ниво одговара тренутном броју диода у секвенци умањен за 2, јер не рачунамо последњи ниво и због чињенице да низ креће од нуле нуле (јер се иницијално секвенца састоји од две диоде)
        Serial.println(" ***");
      } //Или смо успешно прешли све нивое
      else Serial.println("*** Игра је успешно завршена! Успешно сте поновили целокупну секвенцу. Све честитке! ***");
      attachInterrupt(digitalPinToInterrupt(EXTERNAL_INTERUPT_BUTTON_PIN), startGame, FALLING); //Додељује се функција покретања нове игре екстерног прекида зеленом тастеру која се активира детектовањем силазне ивице напона (1 --> 0)
      state = IDLE_STATE; //Прелазак у стање мировања (у овом случају ће то бити крајње стање)
      break;
    case IDLE_STATE: break; //Случај када је систем у стању мировања (ништа се не дешава већ се понавља празна петља)
    default: //Непознато стање (највероватније систем никада неће доћи у ово стање)
      for(int i = 0; i < NUMBER_OF_DIODES; i++){
        digitalWrite(ledPins[i], HIGH);
      }
      Serial.println("**** Грешка: Систем се налази у непознатом стању! Потребно је поновно покретање система ****");
  }
}