
#include <LoRa_E220.h>

#include <SPI.h>
#include <SD.h>


//********************************** ФАЙЛОВЫЕ СИСТЕМЫ ************************
#include <EEPROM.h>  // библиотека памяти EEPROM
File f;              // пременная f типа File. перменная обеъкта файла

//********************************** ДАТЧИКИ *********************************
#include <DallasTemperature.h>              // библиотека датчиков
#include <OneWire.h>                        // библиотека для подключения по одной шине (линии, проводу)
#define DEVICES_PIN 9                     //Пин датчиков температуры
#define DEVICE_MAX_COUNT                 //максимальное количество датчиков
DeviceAddress addresses[DEVICE_MAX_COUNT];  // переменная-массив addresses для адресов датчиков типа DeviceAddress(каждый элемент массива тоже массив из 8ми элемнтов типа byte)
OneWire oneWire(DEVICES_PIN);               // переменная oneWire объекта oneWire для доступа к датчикам по одной линии с инициализацией на пине DEVICES_PIN
DallasTemperature sensors(&oneWire);        // переменная sensors объектов DallasTemperature  с инициализацией ссылкой& на переменную oneWire
byte device_count = DEVICE_MAX_COUNT;       // переменная типа byte для хранения числа датчиков на линии

//********************************** ЧАСЫ **********************************
#include <RTC_DS1307.h>
#include <RTC_DS3231.h>
//_________________________________________
//#include <buildTime.h>
//#include <microDS3231.h>
DS3231 cl;
//DateTime dt;
//******************************** ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ **********************************
byte lastDay = 1;  // переменная lastDay типа byte. В нее считываем значение последнего обновленного числа месяца
String strToWeb;   // переменная strToWeb типа String. В неё формируем строку с данными датчиков для вэб сервера

void setup() {
  Serial.begin(9600);
  Wire.begin();
  rtc.set_self_update(DISABLE);
  rtc.force_update();  // get date from RTC
  Wire.begin();
  //______________________________________________
  if (Serial.available())
    processSyncMessage();

  rtc.print_calendar();

  delay(1000);
  //_________________________________________________
  // cl.begin();
  //cl.setTime(BUILD_SEC, BUILD_MIN, BUILD_HOUR, 14, BUILD_MONTH, BUILD_YEAR);
  // cl.setHMSDMY(BUILD_HOUR, BUILD_MIN, BUILD_SEC, BUILD_DAY, BUILD_MONTH, BUILD_YEAR);

  sensors.begin();  // инициализация датчиков
  if (!SD.begin(7)) {
    while (1)
      ;
  }
  dt = cl.getTime();
  Serial.println(dt.date);
  Serial.println(dt.month);
  Serial.println(dt.year);
  Serial.println(dt.hour);
  Serial.println(dt.minute);
}

void loop() {
  dt = cl.getTime();
  if (lastDay != dt.date)  // если число последнего обновления не равно текущему числу месяца
  {
    readDayNum();  // обновим чило последнего обновления (если сбой по питанию или после последней записи в файл)
    Serial.print("lastDay: ");
    Serial.println(lastDay);
    if (lastDay != dt.date)  //если число последнего обновления опять не равно текущему числу месяца
    {
      updateDay();  // обновим файл новой записью
    }
  }
}

void sensors_request()  // функция обновляет адреса и температуру всех датчиков
{
  device_count = sensors.getDeviceCount();     //получаем количество устройств на шине
  sensors.requestTemperatures();               // запрос температуры датчиков
  for (byte i = 0; i < DEVICE_MAX_COUNT; i++)  // очистим массив адресов в addresses в цикле по макимальному количеству датчиков
  {                                            //
    for (byte j = 0; j < 8; j++)               // цикл по каждому байту в адресе (в адресе 8 байт)
    {
      addresses[i][j] = 0;  // обнулим j-й байт i-го адреса
    }
  }
  for (byte i = 0; i < device_count; i++)  // цикл по количеству датчиков на линии
  {
    sensors.getAddress(addresses[i], i);  // заполняем массив адресов в addresses
  }
}

String GetAddressToString(DeviceAddress deviceAddress)  //функция возвращает строку с адресом полученную из массива byte[8]
{
  String str = "";              // в этой переменной мы вернем строку с адресом
  for (byte i = 0; i < 8; i++)  // цикл по 8ми элементам массива с адресом
  {
    if (deviceAddress[i] < 16) {
      str += String(0, HEX);  //добавляем к строке 0 в 16-ричном счислении
    }
    str += String(deviceAddress[i], HEX);  //добавляем к строке i-й байт адреса в 16-ричном счислении
  }
  return str;  // результат выполнения функции. На местевызова функции появится занчение из строки str
}

void writeDayNum()  // функция записывает в eeprom число месяца последнего обновления файла
{
  dt = cl.getTime();         // получим время в структуру t
  EEPROM.write(0, dt.date);  // запиешем в eeprom число месяца(1 байт) по адресу 0
  EEPROM.end();              // прекратим доступ к eeprom
}

void readDayNum() {        // функция считывает из eeprom число месяца последнего обновления
  EEPROM.get(0, lastDay);  // получим из eeprom число месяца(1 байт) по адресу 0 в переменную lastDay
  EEPROM.end();            // прекратим доступ к eeprom
}

void updateDay()  // функция формирует и записывает в файл две строки с данными id и температур всех датчиков
{
  sensors_request();                                                                    //обновим адреса в массиве и иемпературы датчиков
  String str = String(dt.date) + "." + String(dt.month) + "." + String(dt.year) + ";";  //формируем новую строчку с датой и ";"
  for (byte i = 0; i < DEVICE_MAX_COUNT; i++)                                           // цикл по максимальному количеству датчиков
  {
    str += GetAddressToString(addresses[i]);  //добавляем в строку каждый адрес
    str += ";";                               //разделим адреса символом ";"
  }
  str += ";";                                  // закончили первую строку. Строка кончется символами  ";;"
  str += char(10);                             //добавим перенос строки. Служебный код '/n' имеет код 10
  str += "temp";                               // заполним первую колонку второй строки для сохранения структуры файла
  for (byte i = 0; i < DEVICE_MAX_COUNT; i++)  // цикл по максимальному количеству датчиков
  {
    str += sensors.getTempC(addresses[i]);  //добавляем в строку температуру каждого адреса
    str += ";";                             //разделим температуры символом ";"
  }
  str += ";";       //закончили вторую строку
  str += char(10);  //добавим перенос строки

  if (dt.date == 1)  // если первое число месяца,
  {
    f = SD.open("termo.txt");
    String old;
    while (f.available()) {
      old += f.read();
    }
    f.close();
    SD.remove("result.txt");
    f = SD.open("result.txt", FILE_WRITE);
    f.print(old);
    f.close();
    SD.remove("termo.txt");
    f = SD.open("termo.txt", FILE_WRITE);
  } else {
    f = SD.open("termo.txt", FILE_WRITE);
  }
  f.print(str);   //запись строки
  f.close();      //закроем файл
  writeDayNum();  // запишем в eeprom число месяца последнего обновления файла
}

void processSyncMessage() {
  unsigned long pctime;
  const unsigned long DEFAULT_TIME = 1357387200;  // Jan 1 2017

  if (Serial.find(TIME_HEADER)) {
    pctime = Serial.parseInt();
    if (pctime > DEFAULT_TIME)  // check the integer is a valid time (greater than Jan 1 2017)
      rtc.set_date(pctime);     // Sync Arduino clock to the time received on the serial port
  }
