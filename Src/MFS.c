/*
 * MFS.c
 *
 *  Created on: Aug 31, 2021
 *      Author: rickweil
 */
#include "stm32l476xx.h"
#include "MFS.h"
#include "ctype.h"
#include "stdio.h"
//#include "tim.h"

#ifdef FreeRTOS_REFRESH
#include "FreeRTOS.h"
#include "timers.h"
#endif

//////////////////////////////////////////
// private variables

/// GPIO stuff
static GPIO_OUT_t leds[] =
{
	{ .port=GPIOA, .pin=5, .otype=0, .ospeed=0, .init_value=1},	// place holder
	{ .port=GPIOA, .pin=5, .otype=0, .ospeed=0, .init_value=1},	// LED 1 d13 PA5
	{ .port=GPIOA, .pin=6, .otype=0, .ospeed=0, .init_value=1},	// LED 2 d12 PA6
	{ .port=GPIOA, .pin=7, .otype=0, .ospeed=0, .init_value=1},	// LED 3 d11 PA7
	{ .port=GPIOB, .pin=6, .otype=0, .ospeed=0, .init_value=1},	// LED 4 d10 PB6
};

static GPIO_IN_t buttons[] =
{
	{ .port=GPIOA, .pin=1, .pupd=0},	// place holder
	{ .port=GPIOA, .pin=1, .pupd=0},	// Button 1 a2 PA1
	{ .port=GPIOA, .pin=4, .pupd=0},	// Button 2 a2 PA4
	{ .port=GPIOB, .pin=0, .pupd=0},	// Button 3 a3 PB0
};

static GPIO_OUT_t seg7[] = {
	{ .port=GPIOA, .pin=9, .otype=0, .ospeed=0, .init_value=0},	// data	 d8  PA9
	{ .port=GPIOA, .pin=8, .otype=0, .ospeed=0, .init_value=0},	// shift d7  PA8
	{ .port=GPIOB, .pin=5, .otype=0, .ospeed=0, .init_value=0},	// latch d12 PB5
};

/// 7-segment stuff

// these char arrays map digits or letters to segments to light up
static const uint8_t SEGMENT_MAP[] = {0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90}; // 0-9
static const uint8_t SEGMENT_MAP_ALPHA[] = {136, 131, 167, 161, 134, 142, 144, 139 ,207, 241, 182, 199, 182, 171, 163, 140, 152, 175, 146, 135, 227, 182, 182, 182, 145, 182}; // a-z
static const uint8_t SEGMENT_BLANK = 0xFF;
static const uint8_t SEGMENT_MINUS = 0xBF;
static const uint8_t SEGMENT_SELECT[] = {0xF1,0xF2,0xF4,0xF8};  // low selects the letter (0-3)

static char segments[4] = {0, 0, 0, 0};	/// bit map of segments to light up for each of the 4 letters - refresh from here

//////////////////////////////////////////
// private functions

static void gpio_config_input(GPIO_IN_t *gpio)
{
	GPIO_TypeDef *port = gpio->port;
	uint32_t pin = gpio->pin;

	// First, configure as input
    port->MODER &= ~(0x3 << (pin*2)) ;      // clear the two MODE bits for this pin
    port->MODER |=  0 << (pin*2)  ;        	// 0 => input

	port->PUPDR &= ~(0x3 << (pin*2)) ;      // clear the two PUPDR bits for this pin
	port->PUPDR |=  gpio->pupd << (pin*2)  ;
}

static void gpio_config_output(GPIO_OUT_t *gpio)
{
	GPIO_TypeDef *port = gpio->port;
	uint32_t pin = gpio->pin;

	// First, configure as an output
    port->MODER &= ~(0x3 << (pin*2)) ;      // clear the two MODE bits for this pin
    port->MODER |=  1 << (pin*2)  ;        	// 1 => output

	// ...and then the selected drive
	port->OTYPER &= ~(0x1 << pin) ;
	port->OTYPER |= (gpio->otype << pin) ;

	// ...with selected speed
	port->OSPEEDR &= ~(0x3 << (pin*2)) ; 	// clear the two OSPEED bits for this pin
	port->OSPEEDR |= gpio->ospeed << (pin*2) ;

	// ...set initial value
	port->ODR &= ~(0x1 << pin);
	port->ODR |= (gpio->init_value << pin);
}


/// write 8 bits of `value` to shift register
static void shiftOut(uint8_t value) {

	GPIO_TypeDef *clock_port = seg7[1].port;
	uint32_t clock_bit = 1 << (seg7[1].pin);

	GPIO_TypeDef *data_port = seg7[0].port;
	uint32_t data_bit = 1 << (seg7[0].pin);

	for(int ii=0x80; ii; ii>>=1) {
		clock_port->ODR &= ~clock_bit;		// clear clock pin
		if(ii & value)						// if this bit in `value` is set
			data_port->ODR |= data_bit;		//   set it in shift register
		else
			data_port->ODR  &= ~data_bit;	//   else clear it
		clock_port->ODR |= clock_bit;		// set clock pin
	}
}

/// write `segments` to drive letter `n`
static void set_segment(uint8_t n, uint8_t segments)
{
	GPIO_TypeDef *latch_port = seg7[2].port;
	uint32_t latch_bit = 1 << (seg7[2].pin);

	latch_port->ODR &= ~latch_bit;		// clear latch pin low
	shiftOut(segments);					// write 8 bits for this digit
	shiftOut(SEGMENT_SELECT[n%4]);		// and select which digit
	latch_port->ODR |= latch_bit;		// set latch pin
}

// convert any ascii value to bit map of segments
uint8_t ascii_to_segment (char ascii) {
  uint8_t segmentValue = 182;
  ascii = tolower(ascii);

  if (ascii >= '0' && ascii <= '9')
    segmentValue = SEGMENT_MAP[ascii - '0'];
  else if (ascii >= 'a' && ascii <='z')
    segmentValue = SEGMENT_MAP_ALPHA[ascii - 'a'];
  else if (ascii == '-')
        segmentValue = SEGMENT_MINUS;
  else if (ascii == '.')
        segmentValue = 127;
  else if (ascii == '_')
        segmentValue = 247;
  else if (ascii == ' ')
        segmentValue = SEGMENT_BLANK;
  return segmentValue;
}

#ifdef FreeRTOS_REFRESH
/**
 * It is called from FreeRTOS timer task periodically to refresh the 7-segment display.
 */
static void refresh_display(xTimerHandle hRefreshTimer) {
	MFS_7seg_refresh();
}
#endif

//////////////////////////////////////////
// public functions

void MFS_init(void)
{
    // Enable the clock to GPIO Ports A, and B
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;    // enable clock some MFS buttons / LEDs
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;    // enable clock some MFS buttons / LEDs

	// configure the LEDs as GPIO outputs
	for(int ii=1; ii<=4; ii++) {
		gpio_config_output(&leds[ii]);
	}

	// configure the buttons as GPIO inputs
	for(int ii=1; ii<=3; ii++) {
		gpio_config_input(&buttons[ii]);
	}

	// configure data, clock and latch of the 7-segment display as GPIO outputs
	for(int ii=0; ii<3; ii++) {
		gpio_config_output(&seg7[ii]);
	}

	// then blank all 4 digits
	for(int ii=0; ii<4; ii++) {
		set_segment(ii, SEGMENT_BLANK);
	}

#ifdef FreeRTOS_REFRESH
	// create and start a timer to refresh 7-segment display
	static TimerHandle_t MFS_refresh_timer;
	MFS_refresh_timer = xTimerCreate( "MFS_Refresh", 10, pdTRUE, NULL, refresh_display );
	assert(NULL != MFS_refresh_timer);
	assert(pdPASS == xTimerStart(MFS_refresh_timer, 10));

	HAL_TIM_Base_Start_IT(&htim16);

#endif

	(void)SEGMENT_MINUS;// suppress compiler warning
}

// Turns LED `num` to 'on' if non-zero, or off if zero
void MFS_set_led( uint8_t num, uint32_t on )
{
	while(num > 4);	// hang if invalid input

	GPIO_TypeDef *port = leds[num].port;
	uint32_t bit = 1 << leds[num].pin;

	if ( on )
		port->ODR &= ~bit ;	// active low
	else
		port->ODR |= bit ;
}

// Toggles LED `num`
void MFS_toggle_led( uint8_t num )
{
	while(num > 4);	// hang if invalid input

	GPIO_TypeDef *port = leds[num].port;
	uint32_t bit = 1 << leds[num].pin;

	if ( port->IDR & bit )
		port->ODR &= ~bit ;
	else
		port->ODR |= bit ;
}

// Returns 1 if button `num` if pressed, 0 otherwise
uint8_t MFS_button_pressed( uint8_t num )
{
	while(num > 3);	// hang if invalid input

	GPIO_TypeDef *port = buttons[num].port;
	uint32_t bit = 1 << buttons[num].pin;

	return (port->IDR & bit) == 0;  // invert active low
}

// print ascii string to 7-segment display
void MFS_print_str(char *ascii) {
	for(int ii=0; ii<4; ii++)
		segments[ii] = ascii_to_segment(ascii[ii]);
}

// print integer to 7-segment display
void MFS_print_int(int integer) {
	if(integer >= -999 && integer <= 9999) {
		char ascii[20];
		sprintf(ascii, "%4d", integer);	// turn integer into letters
		MFS_print_str(ascii);				// turn letters into segments
	}
	else
		MFS_print_str("   E");
}

// call repeatedly to refresh the 7-segment display, 1 char each call
// could be called from a FreeRTOS thread
void MFS_7seg_refresh() {
	static uint8_t x = 0;
	set_segment(x, segments[x]);  // light up LED x with selected for segments
	x = (x+1) % 4;
}
