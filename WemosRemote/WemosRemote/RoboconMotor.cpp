#include "RoboconMotor.h"
#include "Json.h"

void RoboEffects::begin()
{
	start = millis();
	halfProgress = fullProgress / 2;
}

int RoboEffects::softStart()
{
	if (duration == 0) return fullProgress;
	long current = millis();//поточний час
	span = (current - start);//минуло від початку руху
	if (span > duration) return fullProgress;//якщо минуло більше часу ніж триває розгін - повертаємо максимальний прогрес
	long progress = span * fullProgress / duration;
	return (progress * progress) / fullProgress;
}
int RoboEffects::softEnd()
{
	if (duration == 0) return fullProgress;
	long current = millis();//поточний час
	span = (current - start);//минуло від початку руху
	if (span > duration) return fullProgress;//якщо минуло більше часу ніж триває розгін - повертаємо максимальний прогрес
	long progress = span * fullProgress / duration;
	return -((progress - fullProgress) * (progress - fullProgress)) / fullProgress + fullProgress;
}
int RoboEffects::softStartSoftEnd()
{
	if (duration == 0) return fullProgress;
	long current = millis();//поточний час
	span = (current - start);//минуло від початку руху
	if (span > duration) return fullProgress;//якщо минуло більше часу ніж триває розгін - повертаємо максимальний прогрес
	long progress = span * fullProgress / duration;
	if (progress < halfProgress) {//початок розгону
		return (progress * progress) / fullProgress * 2L;
	}
	else {//завершення розгону
		return -((progress - fullProgress) * (progress - fullProgress)) / fullProgress * 2L + fullProgress;
	}
}




MotorBase::MotorBase(String name, RoboEffects * effect)
{
	this->name = name;
	this->effect = effect;
}

void MotorBase::setWeight(long weight)
{
	//запамятовуємо вагу (не обовязково, але може в подальшому знадобиться для інформації)
	this->weight = weight;
	//На основі ваги - визначаємо еталонну тривалість розгону (в мілісекундах) мотора від 0 до максимальних обертів. 
	//Це умовний показник, і він немає прямого відношення до реального світу, 
	//він лише для того, щоб не перевантаждувати мотори в момент старту, а плавно розганяти їх.
	this->etalonDuration = /*500 + */(weight / 20);//тривалість розгону від 0 до максимума = 0,5 секунди + 0,5сеунди на кожні 10 кг ваги
	//Де взялась така формула?
	//500мс + (Вага(грам) / 10000грам * 500мс) => скорочуємо одиниці виміру (а саме грами)
	//500мс + (Вага / 10000 * 500мс) => Виносимо одиниці за дужки
	//500мс + (Вага / 10000 * 500)мс => Скорочуємо дроби
	//500мс + (Вага / 100 * 5)мс => => Скорочуємо дроби
	//500мс + (Вага / (100/5) * (5/5))мс => 500мс + (Вага / 20 * 1)мс = 500мс + (Вага / 20)мс
	//Формула спрощена, щоб не тратити час процесора на розрахунки і в одну секунду зробити більше ітерацій в Loop()
	//Приклад: Якщо робот важить 10 кг то до максималки він розганяєтиметься 1 секунду. 
	//Ця характеристика буде мінятись експерементально
}

void MotorBase::setSpeed(int speed)
{
	if (targetSpeed == speed) return;
	targetSpeed = speed;//Запамятовуємо цільову швидкість
	delta = targetSpeed - factSpeed;//на скільки змінилася швидкість відносно фактичної
	int d = delta;
	if (d < 0) d *= -1;
	effect->duration = map(d, 0, 255, 0, etalonDuration);//Визначаємо тривалість зміни швидкості
	//Маємо тривалість розгону від 0 до максималки, коректуємо час пропорційно до дельти.
	//Використовуємо функцію map();
	Serial.print("effect->duration");
	Serial.println(effect->duration);
	effect->begin();//Скидємо точку відліку фізики. 
}

void MotorBase::reset()
{
	targetSpeed = 0;
	factSpeed = 0;
	delta = 0;
	effect->duration = 0;
}

void MotorBase::loop()
{
	//Фактичну швидкість визначаємо згідно із ядром фізики (softStartSoftEnd - плавний старт<->плавний стоп)
	//Це означає, що на початку ми збільшуємо шим малими порціями, потім щораз більшими і більшими,
	//на половині шляху - порції досягають максимума, а тоді плавно йдуть на спад за протилежним алгоритмом, 
	//спочатку великими порціями, потм щораз меншими і меншими
	//Получається аналог синусоїди.
	//Але це не чистий сінус, це його імітація, шляхом накладення двох парабул (y=x*x).
	//Обчислення синуса - то значно складнішша задача для процесора ніж множення двох змінних, 
	//тому синус замінено на відносно прості операції множення та ділення
	int newSpeed = targetSpeed - delta + (((long)delta) * effect->softStartSoftEnd() / effect->fullProgress);
	if (newSpeed != factSpeed) {
		JsonString ret;
		ret.beginObject();
		ret.AddValue(name, String(newSpeed));
		ret.endObject();
		responder->println(ret);
	}
	write(newSpeed);
}

void MotorBase::write(int newSpeed)
{
	factSpeed = newSpeed;
}


HBridge::HBridge(String name, int pwmPin, int reley1Pin, int reley2Pin, RoboEffects *effect) : MotorBase(name, effect)
{
	//Запамятовуємо піни мотора
	this->pwmPin = pwmPin;
	this->motorPinA = reley1Pin;
	this->motorPinB = reley2Pin;
	//Налаштовуємо пни на вихід
	if (pwmPin != 0) pinMode(pwmPin, OUTPUT);
	pinMode(reley1Pin, OUTPUT);
	pinMode(reley2Pin, OUTPUT);
	//Виводимо нулі на всі пни (шим мовчить, релюшки виключені)
	if (pwmPin != 0) digitalWrite(pwmPin, LOW);
	digitalWrite(reley1Pin, LOW);
	digitalWrite(reley2Pin, LOW);
}

HBridge::HBridge(String name, int pinA, int pinB, RoboEffects * effect) :HBridge(name, 0, pinA, pinB, effect)
{
}

void HBridge::write(int newSpeed)
{
	MotorBase::write(newSpeed);
	if (pwmPin != 0) {
		if (isEnabled && (factSpeed > 0)) {
			digitalWrite(motorPinA, LOW);
			digitalWrite(motorPinB, HIGH);
			analogWrite(pwmPin, factSpeed);
		}
		else if (isEnabled && (factSpeed < 0))
		{
			digitalWrite(motorPinA, HIGH);
			digitalWrite(motorPinB, LOW);
			analogWrite(pwmPin, -factSpeed);
		}
		else {
			//Стоїмо
			digitalWrite(motorPinA, LOW);
			digitalWrite(motorPinB, LOW);
			analogWrite(pwmPin, factSpeed);
		}
	}
	else {
		if (isEnabled && (factSpeed > 0)) {
			//їдемо в перед
			digitalWrite(motorPinA, LOW);
			analogWrite(motorPinB, factSpeed);
		}
		else if (isEnabled && (factSpeed < 0)) {
			//Їдемо назад
			analogWrite(motorPinA, -factSpeed);
			digitalWrite(motorPinB, LOW);
		}
		else {
			//Стоїмо
			digitalWrite(motorPinA, LOW);
			digitalWrite(motorPinB, LOW);
		}
	}
}

SpeedController::SpeedController(String name, int pin, RoboEffects * effect) : MotorBase(name, effect)
{
	this->pin = pin;
	pinMode(pin, OUTPUT);
	servo = new Servo();
}

SpeedController::~SpeedController()
{
	delete servo;
	servo = nullptr;
}

void SpeedController::write(int newSpeed)
{
	if (isEnabled) {
		if (!servo->attached()) servo->attach(pin);
		servo->write(map(factSpeed, -255, 255, 0, 180));
	}
	else
	{
		if (servo->attached()) servo->detach();
	}
	MotorBase::write(newSpeed);
}
