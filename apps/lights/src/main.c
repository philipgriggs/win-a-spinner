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

/*
* Event callback function for timer events. It toggles the led pin.
*/
static void spi_cb(struct os_event *ev)
{
	int i = 0;
	
	assert(ev != NULL);

	hal_gpio_toggle(LED_BLINK_PIN);
	
	console_printf("Sending SPI \n");
	
	for (i = 0; i < 10; i++)
	{
		hal_spi_tx_val(spi_num, 0x0f);
	}

	os_callout_reset(&spi_callout, OS_TICKS_PER_SEC);
}
 
int
main(int argc, char **argv)
{
	int rc;
	
	/* Initialize the OS */
	sysinit();
 
	/* Configure the LED GPIO as an output and HIGH (On) */
	hal_gpio_init_out(LED_BLINK_PIN, 0);
	
	// Set up SPI
	struct hal_spi_settings spi_settings;

    spi_settings.data_order = HAL_SPI_MSB_FIRST;
    spi_settings.data_mode = HAL_SPI_MODE0;
    spi_settings.baudrate = 125;							// kHz
    spi_settings.word_size = HAL_SPI_WORD_SIZE_8BIT;
	
	rc = hal_spi_config(spi_num, &spi_settings);
	
	if (rc != 0)
	{
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

    os_callout_reset(&spi_callout, OS_TICKS_PER_SEC);
 
	while (1) {
		/* Run the event queue to process background events */
		os_eventq_run(os_eventq_dflt_get());
	}
 
	return rc;
}