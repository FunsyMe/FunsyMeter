/* 
  FunsyMeter -  
  средний вольтметр на
  INA-219 и 7 сегментных
  дисплее с модулем 
  74HC595 на Arduino Nano,
  by Funsy, 2025
*/

#define DISP_SCLK 2 // Пин SCLK дисплея
#define DISP_RCLK 3 // Пин RCLK дисплея
#define DISP_DIO 4  // Пин DIO дисплея

#define BTN_UP 7 // Пин кнопки вверх
#define BTN_DOWN 8 // Пин кнопки вниз

#define INA_STEP 10 // Шаг калибровки INA-219

#define INFO_TIME 100 // Период обновления данных на дисплей в милисекундах
#define DISP_TIME 1500 // Период динамеческой индикации в микросекундах

#include <GyverINA.h> // Подключаем INA-219
#include <GyverSegment.h> // Покдлючаем дисплей
#include <GyverFilters.h> // Подключаем фильтры
#include <EncButton.h> // Подключаем кнопки

INA219 ina; // Класс INA-219
Disp595_8 disp(DISP_DIO, DISP_SCLK, DISP_RCLK); // Класс дисплея
GMedian3<float> voltFilter; // Класс фильтра напряжения
GMedian3<float> amperFilter; // Класс фильтра тока
Button up(BTN_UP); // Класс кнопки вверх
Button down(BTN_DOWN); // Класс кнопки вниз

uint32_t dispTimer; // Таймер дисплея
uint8_t dispBright = 15; // Яркость дисплея
bool serMode = false; // Флаг для сервисного режима

/* ===== НАСТРОЙКА МОДУЛЕЙ ===== */
void setup() {
  if (!ina.begin()) { // Если INA-219 не инициализировалась
    for (;;) { // Бесконечный цикл
      SegRunner runner(&disp); // Класс бегущей строки
      runner.setText("ERROR"); // Устанавливаем текст
      runner.start(); // Запускаем бегущую строку
      runner.waitEnd(); // Ждем окончания прокрутки
    }
  }

  if (!digitalRead(BTN_UP) && !digitalRead(BTN_DOWN)) { // Если зажаты обе кнопки
    serMode = true; // Флаг сервисного режима в true
    serviceMode(); // Включаем сервисный режим
  }

  ina.setResolution(INA219_VBUS, INA219_RES_12BIT_X4); // Устанавливаем напряжение в 12 битный режим, 4 кратное усреднение
  ina.setResolution(INA219_VSHUNT, INA219_RES_12BIT_X128); // Устанавливаем ток в 12 битный режим, 128 кратное усреднение
}

/* ==== ОБРАБОТАКА СОБЫТИЙ ==== */
void yield() {
  up.tick(); // Тик кнопки вверх
  down.tick(); // Тик кнопки вниз

  if (!serMode) { // Если обычные режим
    if (up.click()) { // Если кнопка вверх нажата
      dispBright += 1; // Увеличить яркость
      dispBright = constrain(dispBright, 0, 15); // Обрезать яркосить
      disp.brightness(dispBright); // Применить яркость на дисплей
    }

    else if (down.click()) { // Если кнопка вниз нажата 
      dispBright -= 1; // Уменьшить яркость
      dispBright = constrain(dispBright, 0, 15); // Обрезать яркость
      disp.brightness(dispBright); // Применить яркость на дисплей
    }
  }

  else { // Если сервисный режим
    if (up.click()) { // Если кнопка вверх нажата
      ina.setCalibration(ina.getCalibration() + INA_STEP); // Увеличиваем калибровочное значение
    }

    else if (down.click()) { // Если кнопка вниз нажата 
      ina.setCalibration(ina.getCalibration() - INA_STEP); // Уменьшаем калибровочное значение
    }
  }
}

/* ======= ГЛАВНЫЙ ЦИКЛ ======= */
void loop() {
  disp.tick(); // Тик дисплея

  if (millis() - dispTimer >= INFO_TIME) { // Если сработал таймер
    dispTimer = millis(); // Сбрасываем таймер
    
    float volts = ina.getVoltage(); // Получаем напряжение в милливольтах
    float ampers = ina.getCurrent(); // Получаем ток в миллиамперах

    volts = voltFilter.filtered(volts); // Фильтруем напряжение
    ampers = amperFilter.filtered(ampers); // Фильтруем ток

    showInfo(volts, ampers); // Выводим показания
  }
}

/* ======== ПОКАЗАНИЯ ======== */
void showInfo(float volts, float ampers) {
  disp.setCursorEnd(); // Ставим курсор в конец дисплея
  disp.printRight(true); // Устанавливаем флаг для печати с право на лево в true
  disp.fillChar('0'); // Устанавливаем символ заполнения

  if (volts >= 10.000) { // Если напряжение больше 10
    disp.print(volts, 2);  // Выводим напряжение на дисплей с 2-мя знаками после запятой
  } else {
    disp.print(volts, 3);  // Выводим напряжение на дисплей с 3-мя знаками после запятой
  }

  disp.print(ampers, 3);  // Выводим ток на дисплей с 3-мя знаками после запятой

  disp.update(); // Обновляем дисплей

  disp.delay(DISP_TIME); // Задержка дисплея
  disp.printRight(false); // Устанавливаем флаг для печати с право на лево в false
}

/* ===== СЕРВИСНЫЙ РЕЖИМ ===== */
void serviceMode() {
  SegRunner runner(&disp); // Класс бегущей строки
  runner.setText("SER MODE"); // Устанавливаем текст
  runner.start(); // Запускаем бегущую строку
  runner.waitEnd(); // Ждем окончания прокрутки

  for (;;) { // Бесконечный цикл
    disp.clear(); // Очищаем дисплей
    disp.setCursorEnd(); // Ставим курсор в конец дисплея 
    disp.printRight(true); // Устанавливаем флаг для печати с право на лево в true

    disp.print(ina.getCalibration()); // Выводим калибровочное значение

    disp.update(); // Обновляем дисплей
    disp.delay(DISP_TIME); // Задержка дисплея
  }
}