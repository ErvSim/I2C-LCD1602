#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

int main() //this code to get a ping from the correct address which is active.
{
    stdio_init_all();
    gpio_set_function(16,GPIO_FUNC_I2C);
    gpio_set_function(17,GPIO_FUNC_I2C);
    i2c_init(i2c0,100000);

    sleep_ms(1000);
    printf("Starting Scan\n");

    for(int addr = 0; addr <127;addr++)
    {
        uint8_t dummy = 0;
        int ret = i2c_write_blocking(i2c0, addr, &dummy, 1 ,false);

        if(ret >= 0)
        {
            printf("Found device at 0x%02x\n", addr);
        }
    }
    printf("Scan done\n");
    while(true)
    
    {
        tight_loop_contents();
    }
    return 0;
}