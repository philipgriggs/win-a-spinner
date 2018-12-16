#include <assert.h>
#include <string.h>
 
#include "os/os.h"
#include "bsp/bsp.h"
#include "hal/hal_gpio.h"
#include "hal/hal_spi.h"
#include "sysinit/sysinit.h"
#include "console/console.h"
 
/* The timer callout */
static struct os_callout spi_callout;

const int spi_num = 0;

const int CLOCK_INTERVAL = 1;
const int N_LIGHTS = 144;

typedef enum {RAVE, CHASER}mode_t;
const mode_t MODE = CHASER;

// baudrate: 125, 250, 500, 1000, 2000, 4000
const int BAUDRATE = 4000;

uint8_t buf[584];

uint8_t red[4] = {0xff, 0, 0, 0xff};
uint8_t blue[4] = {0xff, 0xff, 0, 0};
uint8_t green[4] = {0xff, 0, 0xff, 0};
uint8_t zero[4] = {0xff, 0, 0, 0};

int time = 0;

static void rave();
static void chaser();
static void clearBuf();

/*
* Event callback function for timer events. It toggles the led pin.
*/
static void spi_cb(struct os_event *ev) {

	assert(ev != NULL);

	hal_gpio_toggle(LED_BLINK_PIN);
	
	// console_printf("Sending SPI \n");

	
	switch (MODE) {
		case RAVE:
			rave();
			break;
		case CHASER:
			chaser();
			break;
	
		default:
			break;
	}

	// end frame
	memset(&buf[580], 0xff, 4);	

	hal_spi_txrx(spi_num, buf, NULL, sizeof(buf));

	os_callout_reset(&spi_callout, CLOCK_INTERVAL);
	
	time++;
	if (time > 10000) {
		time = 0;
	}
}

static void rave() {
	int i = 0;
	int gap = 5;
	int slow = 20;

	clearBuf();

	for(i = 0; i < N_LIGHTS; i++) {
		if(time%(slow*(gap+1)) > slow*gap) {
			memcpy(&buf[4*i+4], red, sizeof(zero));
		}
	}
}

static void chaser() {
	int i = 0;
	int gap = 11;

	clearBuf();

	for(i = 0; i < N_LIGHTS; i++) {
		if(i%(3*(gap+1)) == time%(3*(gap+1))) {
			memcpy(&buf[4*i+4], red, sizeof(zero));
		} else if(i%(3*(gap+1)) == (time+gap)%(3*(gap+1))) {
			memcpy(&buf[4*i+4], blue, sizeof(zero));
		} else if(i%(3*(gap+1)) == (time+2*gap)%(3*(gap+1))) {
			memcpy(&buf[4*i+4], green, sizeof(zero));
		}
	}
}

static void clearBuf() {
	int i = 0;

	for(i = 0; i < N_LIGHTS; i++) {
		memcpy(&buf[4*i+4], zero, sizeof(zero));
	}
}

int main(int argc, char **argv) {
	int rc;
	
	/* Initialize the OS */
	sysinit();
 
	/* Configure the LED GPIO as an output and HIGH (On) */
	hal_gpio_init_out(LED_BLINK_PIN, 0);
	
	// Set up SPI
	struct hal_spi_settings spi_settings;

    spi_settings.data_order = HAL_SPI_MSB_FIRST;
    spi_settings.data_mode = HAL_SPI_MODE0;
    spi_settings.baudrate = BAUDRATE;							// kHz
    spi_settings.word_size = HAL_SPI_WORD_SIZE_8BIT;
	
	rc = hal_spi_config(spi_num, &spi_settings);
	
	if (rc != 0) {
		console_printf("Failed to configure SPI. Error %d \n", rc);
		return rc;
	}
	
	hal_spi_set_txrx_cb(spi_num, NULL, NULL);
	hal_spi_enable(spi_num);
	
	/*
     * Initialize the callout for a timer event.
     */
    os_callout_init(&spi_callout, os_eventq_dflt_get(),
                    spi_cb, NULL);

    os_callout_reset(&spi_callout, CLOCK_INTERVAL);
 
	while (1) {
		/* Run the event queue to process background events */
		os_eventq_run(os_eventq_dflt_get());
	}
 
	return rc;
}