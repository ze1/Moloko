/*
 * Name:     moloko.ino          прошивка микроконтроллера для произведения действий отложенных во времени
 * Created:  2016-05-19 20:13:06 очередь заданий на отложенное срабатывание привода по сигналу от датчика
 * Modified: 
 * Author:  ao@ze1.org
**/

// To add/remove a button of a remote control for toggling relay on/off, press a button, then wait 5 sec, then press
// quickly 5 times during next 5 sec. The buttons config will be saved in EEPROM memory. 
//

#include "log_serial.h"
#include "cfg_eeprom.h"

#define PROGRAM_SIGN		0xA02E103A

#define LED_PIN				LED_BUILTIN

#define INPUT_PIN			3

#define SERVO_PIN			5
#define SERVO_DELAY_0		2000
#define SERVO_DELAY_1		500
#define SERVO_DELAY_2		200
#define SERVO_VALUE_1		100
#define SERVO_VALUE_2		200

#define SERVO_QUEUE_SIZE	8

volatile uint32_t servo_queue[SERVO_QUEUE_SIZE] = {0};
volatile uint8_t servo_queue_head = 0;
volatile uint8_t servo_queue_tail = 0;
volatile bool input_state = false;

uint32_t pre = 0;

CFG cfg(PROGRAM_SIGN);

void input_callback() {

	bool state = digitalRead(INPUT_PIN) == LOW;
	if (input_state == state) return;
	input_state = state;
	if (!state) return;

	uint32_t now = millis();
	uint32_t exe = now + SERVO_DELAY_0;
	uint8_t head = (servo_queue_head + 1) % SERVO_QUEUE_SIZE;
	if (head == servo_queue_tail) return; // { S("SIGNAL IGNORED. THE QUEUE IS FULL.", true); return; }

	servo_queue[servo_queue_head] = exe < now ? 100 : exe;
	servo_queue_head = head;
	// S("SIGNAL PROCESSED. EXECUTION IS SCHEDULED.", true);
}

void show_servo_queue() {

	N();
	uint32_t now = millis();
	for (uint8_t i = 0; i < SERVO_QUEUE_SIZE; ++i) {

		if (servo_queue_head > servo_queue_tail) {

			if (i < servo_queue_tail || i > servo_queue_head) { S(" ."); S(i); S("."); }
			else {

				if (now && now < servo_queue[i])                  { S(" ("); S(now); S(")"); now = 0; }
				if (i == servo_queue_tail)                        { S(" <"); S(i); S(":"); S(servo_queue[i]); S("|"); }
				if (i > servo_queue_tail && i < servo_queue_head) { S(" |"); S(i); S(":"); S(servo_queue[i]); S("|"); }
				if (i == servo_queue_head)                        { S(" |"); S(i); S(">"); }
			}
		}
		else {

			if (i > servo_queue_tail && i < servo_queue_head) { S(" ."); S(i); S("."); }
			else {

				if (now && now < servo_queue[i])                  { S(" ("); S(now); S(")"); now = 0; }
				if (i == servo_queue_head)                        { S(" |"); S(i); S(">"); }
				if (i < servo_queue_tail || i > servo_queue_head) { S(" |"); S(i); S(":"); S(servo_queue[i]); S("|"); }
				if (i == servo_queue_tail)                        { S(" <"); S(i); S(":"); S(servo_queue[i]); S("|"); }
			}
		}
	}
	N();
}

void setup() {

	Serial.begin(9600);

	pinMode(INPUT_PIN, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(INPUT_PIN), input_callback, CHANGE);

	pinMode(SERVO_PIN, OUTPUT);
}

void loop() {

	S(".");

	uint32_t now = millis();
	if (servo_queue_tail != servo_queue_head) {

		show_servo_queue();

		uint32_t exe = servo_queue[servo_queue_tail];
		if (now >= exe) {

			if (exe > pre) {

				Serial.println("EXECUTING SERVO WORK SEQUENCE");
				digitalWrite(SERVO_PIN, SERVO_VALUE_1);
				delay(SERVO_DELAY_1);
				digitalWrite(SERVO_PIN, SERVO_VALUE_2);
			}
			else {

				Serial.println("EXECUTION SKIPPED. BAD TIMING.");
			}
			servo_queue_tail = (servo_queue_tail + 1) % SERVO_QUEUE_SIZE;
		}
	}
	pre = now;
	delay(10);
}
