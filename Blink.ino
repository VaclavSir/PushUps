#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <EEPROM.h>
#include <Time.h>

#define DEBUG false
#define DISTANCE_ECHOPIN 2        // Echo pin z HC-SC04 na pin 2
#define DISTANCE_TRIGPIN 3        // Trig pin z HC-SC04 na pin 3
#define BUZZER_SOURCE 9

#define BUTTON_1 19
#define BUTTON_2 18
#define BUTTON_3 17
#define BUTTON_4 16

const int THRESHOLD_MAX = 100;
const int THRESHOLD_DEFAULT_UP = 25;
const int THRESHOLD_DEFAULT_DOWN = 10;

enum Positions
{
	positionUp,
	positionDown
};

enum Screens
{
	mainScreen,
	configScreen
};

enum ConfigItems
{
	itemThresholdUp,
	itemThresholdDown
};

Positions currentPosition = positionUp;
Screens currentScreen = mainScreen;
int thresholdUp = 25;
int thresholdDown = 10;
int totalPushUpCount = 0;
int globalButtonsStates[4] = { false };
bool buttonsPressedNow[4] = { false };
int buttonPins[4] = {
	BUTTON_1,
	BUTTON_2,
	BUTTON_3,
	BUTTON_4
};
ConfigItems currentConfigItem;
bool isPaused = false;
int timeToResume = 0;
bool isWaitingForStart = true;



static const unsigned char PROGMEM buttonsMainUnpausedBitmap[] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x04, 0x00, 0x00,
	0x20, 0x00, 0x01, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x04, 0x00, 0x00, 0x20, 0x00, 0x01, 0x00, 0x00,
	0x0F, 0x33, 0x36, 0xE4, 0x03, 0x60, 0x20, 0x00, 0x01, 0x19, 0x25, 0x8F, 0x2A, 0x44, 0x44, 0x03,
	0x60, 0x20, 0x00, 0x01, 0x22, 0xA5, 0x0F, 0x33, 0x26, 0x44, 0x03, 0x60, 0x20, 0x00, 0x01, 0x22,
	0xB5, 0x8F, 0x2A, 0x14, 0x44, 0x03, 0x60, 0x20, 0x00, 0x01, 0x22, 0xAD, 0x0F, 0x2A, 0x14, 0x44,
	0x03, 0x60, 0x20, 0x00, 0x01, 0x22, 0xA5, 0x0F, 0x2B, 0x66, 0x44, 0x03, 0x60, 0x20, 0x00, 0x01,
	0x19, 0x25, 0x0F, 0x00, 0x00, 0x04, 0x00, 0x00, 0x20, 0x00, 0x01, 0x00, 0x00, 0x0F, 0x00, 0x00,
	0x04, 0x00, 0x00, 0x20, 0x00, 0x01, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x04, 0x00, 0x00, 0x20, 0x00,
	0x01, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};
static const unsigned char PROGMEM buttonsMainPausedBitmap[] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x04, 0x00, 0x00,
	0x20, 0x00, 0x01, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x04, 0x01, 0x00, 0x20, 0x00, 0x01, 0x00, 0x00,
	0x0F, 0x33, 0x36, 0xE4, 0x01, 0x80, 0x20, 0x00, 0x01, 0x19, 0x25, 0x8F, 0x2A, 0x44, 0x44, 0x01,
	0xC0, 0x20, 0x00, 0x01, 0x22, 0xA5, 0x0F, 0x33, 0x26, 0x44, 0x01, 0xE0, 0x20, 0x00, 0x01, 0x22,
	0xB5, 0x8F, 0x2A, 0x14, 0x44, 0x01, 0xE0, 0x20, 0x00, 0x01, 0x22, 0xAD, 0x0F, 0x2A, 0x14, 0x44,
	0x01, 0xC0, 0x20, 0x00, 0x01, 0x22, 0xA5, 0x0F, 0x2B, 0x66, 0x44, 0x01, 0x80, 0x20, 0x00, 0x01,
	0x19, 0x25, 0x0F, 0x00, 0x00, 0x04, 0x01, 0x00, 0x20, 0x00, 0x01, 0x00, 0x00, 0x0F, 0x00, 0x00,
	0x04, 0x00, 0x00, 0x20, 0x00, 0x01, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x04, 0x00, 0x00, 0x20, 0x00,
	0x01, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};


static const unsigned char PROGMEM buttonsConfigBitmap[] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x04, 0x00, 0x00,
	0x20, 0x00, 0x01, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x04, 0x01, 0xC0, 0x20, 0x00, 0x01, 0x00, 0x70,
	0x0F, 0x0C, 0x95, 0x84, 0x01, 0xC0, 0x20, 0x00, 0x01, 0x00, 0x70, 0x0F, 0x10, 0x95, 0x04, 0x01,
	0xC0, 0x20, 0x3F, 0x81, 0x01, 0xFC, 0x0F, 0x09, 0x55, 0x84, 0x07, 0xF0, 0x20, 0x3F, 0x81, 0x01,
	0xFC, 0x0F, 0x05, 0xD5, 0x04, 0x03, 0xE0, 0x20, 0x3F, 0x81, 0x01, 0xFC, 0x0F, 0x05, 0x49, 0x04,
	0x01, 0xC0, 0x20, 0x00, 0x01, 0x00, 0x70, 0x0F, 0x19, 0x49, 0x84, 0x00, 0x80, 0x20, 0x00, 0x01,
	0x00, 0x70, 0x0F, 0x00, 0x00, 0x04, 0x00, 0x00, 0x20, 0x00, 0x01, 0x00, 0x00, 0x0F, 0x00, 0x00,
	0x04, 0x00, 0x00, 0x20, 0x00, 0x01, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x04, 0x00, 0x00, 0x20, 0x00,
	0x01, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static const unsigned char PROGMEM configThresholdUpBitmap[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x20, 0x00, 0x06, 0x70, 0x00, 0x0C, 0xA8, 0x00,
	0x18, 0x20, 0x00, 0x30, 0x20, 0x00, 0x60, 0x20, 0x00, 0xC0, 0x20, 0x01, 0x80, 0x20, 0x03, 0x00,
	0x20, 0x06, 0x00, 0xA8, 0x0C, 0x00, 0x70, 0x18, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char PROGMEM configThresholdDownBitmap[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x20, 0x00, 0x0E, 0x70, 0x00, 0x18, 0xA8, 0x00, 0x70, 0x20, 0x01, 0xC0,
	0x20, 0x03, 0x00, 0xA8, 0x0E, 0x00, 0x70, 0x18, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

Adafruit_PCD8544 display = Adafruit_PCD8544(7, 6, 5, 4, 10);

void setupLcd() {
	Serial.begin(9600);
	display.begin();
	display.setContrast(30);

	renderMain();
}


void setup()
{
	setupLcd();
	//Nastav� s�riovou komunikaci
	Serial.begin(9600);
	
	pinMode(DISTANCE_ECHOPIN, INPUT);
	pinMode(DISTANCE_TRIGPIN, OUTPUT);
	pinMode(BUZZER_SOURCE, OUTPUT);
	pinMode(BUTTON_1, INPUT);
	pinMode(BUTTON_2, INPUT);
	pinMode(BUTTON_3, INPUT);
	pinMode(BUTTON_4, INPUT);

	EEPROM.get(0, thresholdUp);
	EEPROM.get(sizeof(thresholdUp), thresholdDown);
	if (
		thresholdUp < 0
		|| thresholdUp > THRESHOLD_MAX
		|| thresholdDown < 0
		|| thresholdDown > THRESHOLD_MAX
		|| thresholdDown > thresholdUp
		) {
		DEBUG && Serial.println("Threshold fallback to defaults " + String(thresholdUp) + " " + thresholdDown);
		thresholdUp = THRESHOLD_DEFAULT_UP;
		thresholdDown = THRESHOLD_DEFAULT_DOWN;
	}

	renderSplashScreen();

	setTime(0);
}

void beep()
{
	digitalWrite(BUZZER_SOURCE, HIGH);
	delay(75);
	digitalWrite(BUZZER_SOURCE, LOW);
	delay(50);
}

void drawTextAlignedToRight(String text, int y = 0)
{
	int textLength = text.length();
	display.setTextSize(2);
	display.setTextColor(BLACK);
	display.setCursor(84 - (textLength * 12), y);

	for (int i = 0; i < textLength; i++) {
		display.write(text.charAt(i));
	}

}

void renderSplashScreen()
{
	display.clearDisplay();
	String text = "PushUps\n#czhackathon\n2015\nVaclav Sir\nTomas Michna";
	int textLength = text.length();
	display.setTextSize(1);
	display.setTextColor(BLACK);
	display.setCursor(0, 0);
	for (int i = 0; i < textLength; i++) {
		display.write(text.charAt(i));
	}
	display.display();
	delay(1000);
}

void drawTime()
{
	String time = getFormattedTime();
	int textLength = time.length();
	display.setTextSize(1);
	display.setTextColor(BLACK);
	display.setCursor(84 - (textLength * 6), 16);

	for (int i = 0; i < textLength; i++) {
		display.write(time.charAt(i));
	}
}

void renderMain()
{
	display.clearDisplay();
	display.drawBitmap(0, 36, isPaused ? buttonsMainPausedBitmap : buttonsMainUnpausedBitmap, 88, 16, BLACK);
	drawTextAlignedToRight(String(totalPushUpCount));
	drawTime();
	display.display();
}

void drawMenuTriangle(int y = 0)
{
	display.fillTriangle(
		2, 2 + y,
		7, 7 + y,
		2, 12 + y,
		BLACK
		);
}

void renderConfig()
{
	display.clearDisplay();
	drawMenuTriangle(currentConfigItem == itemThresholdUp ? 0 : 16);
	display.drawBitmap(10, 0, configThresholdUpBitmap, 24, 16, BLACK);
	display.drawBitmap(10, 16, configThresholdDownBitmap, 24, 16, BLACK);
	display.drawBitmap(0, 36, buttonsConfigBitmap, 88, 16, BLACK);
	drawTextAlignedToRight(String(thresholdUp));
	drawTextAlignedToRight(String(thresholdDown), 16);
	display.display();
}

void onSwitchPositionUpwards()
{
	DEBUG && Serial.println("Hodne vysoko miris.");
	if (!isWaitingForStart) {
		++totalPushUpCount;
		renderMain();
		beep();
		if (totalPushUpCount % 10 == 0) {
			delay(20);
			beep();
		}
	}
}

void onSwitchPositionDownwards()
{
	DEBUG && Serial.println("Jdeme dolu.");
	beep();
}

float measureSonarDistance() 
{
	// Vy�le impuls do modulu HC-SR04
	digitalWrite(DISTANCE_TRIGPIN, LOW);
	delayMicroseconds(2);
	digitalWrite(DISTANCE_TRIGPIN, HIGH);
	delayMicroseconds(10);
	digitalWrite(DISTANCE_TRIGPIN, LOW);

	// Spo��t� vzd�lenost
	return pulseIn(DISTANCE_ECHOPIN, HIGH) *0.017315f;
	
}


void loopMain()
{
	float distance = measureSonarDistance();
	
	// ode�le informace na s�riv� port
	DEBUG && Serial.print(distance) && Serial.print("cm\n");

	switch (currentPosition) {
	case positionUp:
		if (distance <= thresholdDown && !isPaused) {
			if (isWaitingForStart) {
				setTime(timeToResume);
				isWaitingForStart = false;
			}
			currentPosition = positionDown;
			onSwitchPositionDownwards();
		}
		break;
	case positionDown:
		if (distance >= thresholdUp && !isPaused) {
			currentPosition = positionUp;
			onSwitchPositionUpwards();
		}
		break;
	}

	renderMain();

	if (buttonsPressedNow[0]) { // reset
		totalPushUpCount = 0;
		setTime(0);
		timeToResume = 0;
		renderMain();
	}

	if (buttonsPressedNow[1]) { // pause / resume
		isPaused ? endPause() : startPause();
	}

	if (buttonsPressedNow[3]) { // config
		switchToConfigScreen();
		startPause();
	}

	delay(DEBUG ? 100 : 20);
}

void endPause()
{
	setTime(timeToResume);
	isWaitingForStart = true;
	isPaused = false;
}

void startPause()
{
	timeToResume = now();
	isPaused = true;
}

String getFormattedTime() {
	if (isPaused || isWaitingForStart) {
		setTime(timeToResume);
	}
	String clock = formatDigits(hour()) 
		+ ':' + formatDigits(minute()) 
		+ ':' + formatDigits(second());

	DEBUG && Serial.println(clock);

	return clock;
} 
String formatDigits(int digits) {
	String result = "";
	if (digits < 10) {
		result += '0';
	}
	result += digits;
	return result;
}

void switchToConfigScreen()
{
	currentConfigItem = itemThresholdUp;
	currentScreen = configScreen;
	renderConfig();
}

void switchToMainScreen()
{
	currentScreen = mainScreen;
	renderMain();
	EEPROM.put(0, thresholdUp);
	EEPROM.put(sizeof(thresholdUp), thresholdDown);
}

void loopConfig()
{
	if (buttonsPressedNow[0]) { // save & exit
		endPause();
		switchToMainScreen();
	}
	if (buttonsPressedNow[1]) { // next item
		currentConfigItem = currentConfigItem == itemThresholdDown
			? itemThresholdUp
			: itemThresholdDown;
		renderConfig();
	}
	if (buttonsPressedNow[2]) { // minus
		switch (currentConfigItem)
		{
		case itemThresholdUp:
			if (thresholdUp > 0 && thresholdUp > thresholdDown) {
				thresholdUp--;
			}
			break;
		case itemThresholdDown:
			if (thresholdDown > 0) {
				thresholdDown--;
			}
			break;
		}
		renderConfig();
	}
	if (buttonsPressedNow[3]) { // plus
		switch (currentConfigItem)
		{
		case itemThresholdUp:
			if (thresholdUp < THRESHOLD_MAX) {
				thresholdUp++;
			}
			break;
		case itemThresholdDown:
			if (thresholdDown < THRESHOLD_MAX && thresholdUp > thresholdDown) {
				thresholdDown++;
			}
			break;
		}
		renderConfig();
	}
}


void loop() {
	bool currentButtonsState[4];
	for (int buttonState, i = 0; i < 4; i++) {
		buttonState = digitalRead(buttonPins[i]);
		buttonsPressedNow[i] = buttonState == 0 && globalButtonsStates[i] == 1;
		globalButtonsStates[i] = buttonState;
	}
	
	currentScreen == mainScreen ? loopMain() : loopConfig();
}
