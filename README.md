# Wemos Remote

Wemos remote - проект для автоматичного керування освітлювальними приборами радіомоделі. 

Керування реалізовано на процесорі ESP8266. Цей процесор компактний, має вбудований Wi-Fi модуль і адаповане ядро під фреймворк Arduino. Прошивка реалізована у середовищі Visual Studio 2019 з використанням фреймворків:
* Arduino https://www.arduino.cc/en/Main/Software
* ESP8266 https://github.com/esp8266/Arduino
* jQuery https://jquery.com/
* Bootstrap https://getbootstrap.com/
* ESPArudio https://github.com/earlephilhower/ESP8266Audio

При необхідності скетч можна редагувати у середовищі Arduino IDE.


# Основні можливості
* Автоматичне включення поворотів в залежності від положення руля
* Автоматичне включення стопсигналу під час скидання швидкості
* Автоматичне включення лампочки заднього ходу при русі назад
* Канал 3 
    * 1 натискання - габарити
    * 2 натискання - ближній
    * 3 натичкання - дальній
    * 4 натискання - виключаємо світло
	* Утримання кнопки довше ніж 2 секунди - вмикає протитуманні вогні.
	* Повторне утримання кнопки довше ніж 2 секунди - вимикає протитуманні вогні.
* Канал 4 - Керування склоочисниками
    * 1 положення - склоочисники стоять
    * 2 положення - перша швидкість
    * 3 положення - друга швидкість
 


# Мені вже все подобається! З чого розпочати?
## Що необхідно?
Зробити радіокерування для моделі дуже просто.

Базовий набір:
* Завантажена з GitHub копія проекту
* Модуль, або відлагоджувальна плата на базі процесора ESP8266 (Наприклад ESP12, ESP12-S, [Wemo D1 mini](https://wiki.wemos.cc/products:retired:d1_mini_v2.2.0), або інша.)
* Драйвер двигуна
* Сервопривід
* Комп'ютер з USB
* Веб-переглядач для налаштування

Додаткові інструменти, якщо ви захочете переробити прошивку під більш складні задачі:
* Arduino IDE
* Встановлена бібіліотека для ESP8266

## Прошивка базової конфігурації
У папці Tools є утиліта для прошивки та безпосередньо сам файл прошивки. Для більшості користувачів цього є цілком достатньо. Виконавши декілька простих кроків, ви зможете перетворити плату у радіоапаратуру для керування моделями.

Покрокова інструкція
1. Підключаєте плату до USB-порту вашого комп'ютера.
2. Встановлюєте драйвери у відповідності до інструкцій виробника плати
  * Драйвер CH340 з офіційного сайту можна скачати тут https://docs.wemos.cc/en/latest/ch340_driver.html
3. Заходите у диспечер пристроїів і перевіряєте, чи всі драйвери встановлено і чи ваша плата розпізнається системою. 
  * Відкриваєте панель керування комп'ютером. 
![Call device manager](/img/device-manager.png)
  * Переходите на пункт "Диспетчер пристроїв".
Ймовірно, ваша плата буде називатися 'USB-Serial CH340 (COM_)'
![Call device manager](/img/usb-serial-ch340.png)
  * Запамятовуєте, який номер порта отримала ваша плата (у моєму випадку - №3)
![Call device manager](/img/usb-serial-ch340-com3.png)
4. Запускаєте Tools/1 upload.bat
5. Після старту скрипт запитає номер порта, до якого під'єднано вашу плату
6. Вводите номер (тільки цифру), натискаєте Enter
7. Чекаєте, поки завершиться процеc завантаження

Все - плата прошита.

З цього моменту нею можна користуватись.

Якщо ж ви бажаєте змінити деякі налаштування (діапазон повороту сервоприводу, стартову швидкість, яскравість світла тощо), то додатково необхідно завантажити в пам'ять контроллера модуль налаштувань.

## Завантаження модуля налаштувань
На цьому кроці ви вже маєте підключену до вашого USB плату і знаєте який номер порта вона отримала.
Приступаємо
1. Запускаєте Tools/2 setup.bat
2. Після старту скрипт запитає номер порта, до якого під'єднано вашу плату
3. Вводите номер (тільки цифру), натискаєте Enter
4. Чекаєте, поки завершиться процеc завантаження

Все - плата поновлена.

## Налаштування
Для налаштування вам необхідно на телефоні під'єднатися до Wi-Fi мережі Wemos_00000000 (замість нулів буде серійний номер вашої плати).
Стандартний пароль - 12345678. Ви можете його змінити за вашим бажанням.

Після підключення відкриваєте веб-переглядач і переходите за адресою 192.168.4.1 - це адреса для налаштувань.

### Налаштування точки доступу
![SSID settings](/img/1.png)

* SSID - назва вашої моделі у Wi-Fi мережі. Це може бути, наприклад, номерний знак, або ваш нікнейм...
* PASSWORD - пароль доступу. Типово встановлено 12345678

### Моніторинг портів

Налаштування кожного з 4-ох входів каналів 

![Port monitoring](/img/2.png)


Керування обладнанням проводиться з стандартного приймача з допомогою імпульсів різної довжини. Типова довжина імпульсів коливається в діапазоні від 1000 до 2000 мікросекунд.

При підключенні приймача до входів приладу - постійно проводиться вимірювання довжини імпульсів. 

* Кнопка "Monitor" - Після натискання на кнопку Monitor - поточні виміри імпульсів кожного канлу будуть онвлюватись на екрані з періодом 3 секунди
* min - нижня межа діапазону
* max - вурхня межа діапазону
* current - виміряне значння довжини імпульса. 

Для кожного канаду можна звузити, або розширити діапазон дійсних значень з допомогою падаметрів min/max. Проте варто врахувати, що у випадку виходу діючого значення за межі діапазону - значення буде вважатись помилковим і спрацювання команди не відбудеться

### Налаштування освітлення
![Light settings](/img/3.png)
* Port address - Тип та конфігурація адреси мікросхеми розширення портів вводу/виводу (обираєтся на основі конфігурації підключення ножок A0, A1, A2). 
	* Для мікросхеми PCF8574
	  * 0x20 (PCF8574 pin A-&gt;000) - A0=0V; A1=0V; A2=0V;
	  * 0x21 (PCF8574 pin A-&gt;001) - A0=0V; A1=0V; A2=3V3; 
	  * 0x22 (PCF8574 pin A-&gt;010) - A0=0V; A1=33V3; A2=0V;
	  * 0x23 (PCF8574 pin A-&gt;011) - A0=0V; A1=3V3; A2=3V3;
	  * 0x24 (PCF8574 pin A-&gt;100) - A0=3V3; A1=0V; A2=0V;
	  * 0x25 (PCF8574 pin A-&gt;101) - A0=3V3; A1=0V; A2=3V3;
	  * 0x26 (PCF8574 pin A-&gt;110) - A0=3V3; A1=3V3; A2=0V;
	  * 0x27 (PCF8574 pin A-&gt;111) - A0=3V3; A1=3V3; A2=3V3;
	* Для мікросхеми PCF8574A
	  * 0x38 (PCF8574A* pin A-&gt;000) - A0=0V; A1=0V; A2=0V;
	  * 0x39 (PCF8574A* pin A-&gt;001) - A0=0V; A1=0V; A2=3V3; 
	  * 0x3A (PCF8574A* pin A-&gt;010) - A0=0V; A1=33V3; A2=0V;
	  * 0x3B (PCF8574A* pin A-&gt;011) - A0=0V; A1=3V3; A2=3V3;
	  * 0x3C (PCF8574A* pin A-&gt;100) - A0=3V3; A1=0V; A2=0V;
	  * 0x3D (PCF8574A* pin A-&gt;101) - A0=3V3; A1=0V; A2=3V3;
	  * 0x3E (PCF8574A* pin A-&gt;110) - A0=3V3; A1=3V3; A2=0V;
	  * 0x3F (PCF8574A* pin A-&gt;111) - A0=3V3; A1=3V3; A2=3V3;
* Stop light duration - Тривалість горіння стоп-сигналу Можливе значення від 500  до 5000 мілісекунд
* Back light timeout - Проміжок часу, псля зуписки, через який вимикається сигнал заднього ходу. Діапазон 0..1000 мілісекунд
* Reverce limit - Проміжок від 0-го положення після якої відбувається включення сигналу заднього ходу.
* Turn light limit - Проміжок від 0-го положення після якого відбувається включення поворотів


### Налаштування склоочистників
![Wipers settings](/img/4.png)
#### Range
* min - позиція парковки двірників
* max - позиція повного розміху двірників
#### Speed 1
* Run duration - Тривалість одного взмаху (від нуля до повного розмаху та назад у нульову позицію). Можливе значення від 500  до 5000 мілісекунд
* Pause - Пауза між взмахами. Можливе значення від 500  до 5000 мілісекунд
#### Speed 2
* Run duration - Тривалість одного взмаху (від нуля до повного розмаху та назад у нульову позицію). Можливе значення від 500  до 5000 мілісекунд
* Pause - Пауза між взмахами. Можливе значення від 500  до 5000 мілісекунд

## Підключення

1) Сервопривід очистників лобового вікна Керуючий вивід => D4
2) Регулятор +/-/D6
3) Розширювач портів +/-/D1(SCL)/D2(SDA)
	* Габарити P0
	* Ближнє P1
	* Дальнє P2
	* Протитуманки P3
	* Лівий поворот P4
	* Правий поворот P5
	* Задній хід P6
	* Стоп сигнал P7
4) Канал рольового сервопривода D5
5) Канал 3 (D7)
6) Канал 4 (D8)

## Схема

![Schematic](/img/schematic.png)

Схема передбачає наявність додаткового модуля розширення портів. Для перевірки чи правильно ви ого підключили - можна спочатку прошити Wemos тестовою прошивкою.
з допомогою скрипта **0 upload test.bat**. Тестова прошивка включає/виключає всі піни розширювача портів та вбудований світлодіод на платі wemos. Якщо вони блимають синхронно - значить все підключено правильно.
