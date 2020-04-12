# Wemos Remote

Wemos remote - проект для керування авто моделлю з додатку на телефоні. 

Проект зроблено спеціально для моделеі ГАЗ13 з Youtube каналу Евгения Былеева https://www.youtube.com/channel/UC27d-7p-ovLAEvgUijC3Dwg

Усі етапи переобки моделі:
* Частина 1 https://www.youtube.com/watch?v=YaWISeNnW-Y
* Частина 2 https://www.youtube.com/watch?v=_9iZ8m-Jo94
* Частина 3 https://www.youtube.com/watch?v=CbGfEdLAH0U
* Частина 4 https://www.youtube.com/watch?v=SgVseJ60G0s
* Частина 5 https://www.youtube.com/watch?v=5hFmwyWWN0M
* Частина 6 https://www.youtube.com/watch?v=KGiXMOKt4lY
* Частина 7 https://www.youtube.com/watch?v=0yMU2JMWXjI
* Частина 8 
* Частина 9 
* Частина 10
* Частина 11
* Частина 12
* Тест драйв

Ідея додати підтримку mp3 та реалізація належить Alex https://github.com/zilibob4ik

Керування реалізовано на процесорі ESP8266. Цей процесор компактний, має вбудований Wi-Fi модуль і адаповане ядро під фреймворк Arduino. Прошивка реалізована у середовищі Visual Studio 2019 з використанням фреймворків:
* Arduino https://www.arduino.cc/en/Main/Software
* RemoteXY http://remotexy.com/ https://github.com/RemoteXY/RemoteXY-Arduino-library
* ESP8266 https://github.com/esp8266/Arduino
* jQuery https://jquery.com/
* Bootstrap https://getbootstrap.com/
* ESPAudio https://github.com/earlephilhower/ESP8266Audio

При необхідності скетч можна редагувати у середовищі Arduino IDE.


# Основні можливості
## Можливості прошивки


## Параметри, що налаштовуються



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
![Config](/img/config.png)
* SSID - назва вашої моделі у Wi-Fi мережі. Це може бути, наприклад, номерний знак, або ваш нікнейм...
* PASSWORD - пароль доступу. Типово встановлено 12345678

### Налаштування сервоприводу
![Servo](/img/servo.png)
* center - Положення сервоприводу при русі прямо, в градусах
* left - Положення сервопривду при вивороті коліс до упору в ліво, в градусах
* right - Положення сервопривду при вивороті коліс до упору в право, в градусах
* Stearing potenciometer linearity - лінійність керма
  * Linear - кермо лінійне. Відхилення керма на 1 градус повертає колеса на 1 градус.
  * Y = X^2/X кермо не лінійне. При позиціях, близьких до нуля, на один градус зміни положення керма колеса повертаються менше.
При позиціях, близьких до крайніх положень, колеса повертаються швидко. На високих швидкостях це дозволяє маневрувати плавніше.


## Підключення

1) Сервопривід Керуючий вивід => D5
2) Драйвер тягового мотора Вхід А => D7, Вхід Б => D6
3) Головне світло D4
4) Лівий поврот D2
5) Правий поворот D1
6) Задній хід D3
7) Стоп-сигнал D8
8) Габарит D0
9) Бузер RX


