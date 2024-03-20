/*
  Электронный предсказатель погоды по изменению давления
  Измеряет давление каждые 10 минут, находит разницу с давлением час назад
  При расчёте давления использовано среднее арифметическое из 10 измерений
  для уменьшения шумности сигнала с датчика
*/


//-----------------------НАСТРОЙКИ---------------------
#define servo_invert 1    
//-----------------------НАСТРОЙКИ---------------------

#define servo_Vcc 12           

//------ИНВЕРСИЯ------
#if servo_invert == 1
#define servo_180 0
#define servo_0 180
#else
#define servo_0 0
#define servo_180 180
#endif
//------ИНВЕРСИЯ------

//------БИБЛИОТЕКИ------
#include <Servo.h>             // библиотека серво
#include <Wire.h>              // вспомогательная библиотека датчика
#include <Adafruit_BMP085.h>   // библиотека датчика
#include <LowPower.h>          // библиотека сна
//------ИНВЕРСИЯ------

boolean wake_flag, move_arrow;
int sleep_count, angle, delta, last_angle = 90;
float k = 0.8;
float my_vcc_const = 1.080;    
unsigned long pressure, aver_pressure, pressure_array[6], time_array[6];
unsigned long sumX, sumY, sumX2, sumXY;
float a, b;

Servo servo;
Adafruit_BMP085 bmp; //объявить датчик с именем bmp

void setup() {
  Serial.begin(9600);
  pinMode(servo_Vcc, OUTPUT);
  pinMode(A3, OUTPUT);
  pinMode(A2, OUTPUT);
  digitalWrite(servo_Vcc, 1);      // подать питание на серво
  digitalWrite(A3, 1);             // подать питание на датчик
  digitalWrite(A2, 0);
  delay(500);
  bmp.begin(BMP085_ULTRAHIGHRES);  // включить датчик
  servo.attach(2);                 // подключить серво
  servo.write(servo_0);            // увести серво в крайнее левое положение
  delay(1000);      
  servo.write(90);                 // поставить серво в центр
  delay(2000);
  digitalWrite(servo_Vcc, 0);      // отключить серво
  pressure = aver_sens();          // найти текущее давление по среднему арифметическому
  for (byte i = 0; i < 6; i++) {   // счётчик от 0 до 5
    pressure_array[i] = pressure;  // забить весь массив текущим давлением
    time_array[i] = i;             // забить массив времени числами 0 - 5
  }
}

void loop() {
  if (wake_flag) {
    delay(500);
    pressure = aver_sens();                          // найти текущее давление по среднему арифметическому
    for (byte i = 0; i < 5; i++) {                   // счётчик от 0 до 5 (да, до 5. Так как 4 меньше 5)
      pressure_array[i] = pressure_array[i + 1];     // сдвинуть массив давлений КРОМЕ ПОСЛЕДНЕЙ ЯЧЕЙКИ на шаг назад
    }
    pressure_array[5] = pressure;                    // последний элемент массива теперь - новое давление

    sumX = 0;
    sumY = 0;
    sumX2 = 0;
    sumXY = 0;
    for (int i = 0; i < 6; i++) {                    // для всех элементов массива
      sumX += time_array[i];
      sumY += (long)pressure_array[i];
      sumX2 += time_array[i] * time_array[i];
      sumXY += (long)time_array[i] * pressure_array[i];
    }
    a = 0;
    a = (long)6 * sumXY;             // расчёт коэффициента наклона приямой
    a = a - (long)sumX * sumY;
    a = (float)a / (6 * sumX2 - sumX * sumX);
    delta = a * 6;                   // расчёт изменения давления

    angle = map(delta, -250, 250, servo_0, servo_180);  // пересчитать в угол поворота сервы
    angle = constrain(angle, 0, 180);                   // ограничить диапазон

    // если угол несильно изменился с прошлого раза, то нет смысла лишний раз включать серву
    // и тратить энергию. Так что находим разницу, и если изменение существенное - то поворачиваем стрелку    
    if (abs(angle - last_angle) > 7) move_arrow = 1;

    if (move_arrow) {
      last_angle = angle;
      digitalWrite(servo_Vcc, 1);      // подать питание на серво
      delay(300);                      // задержка для стабильности
      servo.write(angle);              // повернуть серво
      delay(1000);                     // даём время на поворот
      digitalWrite(servo_Vcc, 0);      // отключить серво
      move_arrow = 0;
    }

    if (readVcc() < battery_min) LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); 
    wake_flag = 0;
    delay(10);                       // задержка для стабильности
  }

  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);      // спать 8 сек. mode POWER_OFF, АЦП выкл
  sleep_count++;            // +1 к счетчику просыпаний
  if (sleep_count >= 70) {  // если время сна превысило 10 минут (75 раз по 8 секунд - подгон = 70)
    wake_flag = 1;          // рарешить выполнение расчета
    sleep_count = 0;        // обнулить счетчик
    delay(2);               // задержка для стабильности
  }
}

// среднее арифметичсекое от давления
long aver_sens() {
  pressure = 0;
  for (byte i = 0; i < 10; i++) {
    pressure += bmp.readPressure();
  }
  aver_pressure = pressure / 10;
  return aver_pressure;
}
