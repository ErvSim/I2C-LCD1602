#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <string.h>

// Bitmask definitions for LCD control lines connected via PCF8574
#define LCD_RS        0x01  // Register Select (0 = command, 1 = data)
#define LCD_RW        0x02  // Read/Write (0 = write, 1 = read) [we always write for LCD]
#define LCD_ENABLE    0x04  // Enable bit (used to latch data)
#define LCD_BACKLIGHT 0x08  // Backlight control

// Sends a single byte over I2C to the LCD module through PCF8574
static void pcf_write_byte(uint8_t val)
{
    i2c_write_blocking(i2c0, 0x27, &val, 1, false); // Send the byte to address 0x27
    sleep_us(100); // Short delay to ensure signal is registered
}

// Sends a full command byte to the LCD in 4-bit mode (upper nibble then lower nibble)
void lcd_send_command(uint8_t cmd)
{
    uint8_t high = (cmd & 0xF0);           // Get high nibble (upper 4 bits)
    uint8_t low  = (cmd << 4) & 0xF0;      // Get low nibble (lower 4 bits shifted to upper position)

    // Send high nibble with Enable pulse
    pcf_write_byte(high | LCD_BACKLIGHT | LCD_ENABLE);
    pcf_write_byte(high | LCD_BACKLIGHT);

    // Send low nibble with Enable pulse
    pcf_write_byte(low | LCD_BACKLIGHT | LCD_ENABLE);
    pcf_write_byte(low | LCD_BACKLIGHT);
}

// Initializes the LCD in 4-bit mode following the official LCD startup sequence
void lcd_init_sequence()
{
    sleep_ms(50); // Wait for LCD power-up

    // Send 0x30 three times to reset LCD into 8-bit mode first
    for (int i = 0; i < 3; i++)
    {
        pcf_write_byte(0x30 | LCD_BACKLIGHT | LCD_ENABLE);
        pcf_write_byte(0x30 | LCD_BACKLIGHT);
        sleep_ms(5);
    }

    // Switch to 4-bit mode
    pcf_write_byte(0x20 | LCD_BACKLIGHT | LCD_ENABLE);
    pcf_write_byte(0x20 | LCD_BACKLIGHT);
    sleep_ms(5);

    // Function set: 4-bit mode, 2-line display, 5x8 dots
    lcd_send_command(0x28);

    // Display ON, cursor OFF, blink OFF
    lcd_send_command(0x0C);

    // Clear display
    lcd_send_command(0x01);

    // Entry mode set: move cursor to right, no display shift
    lcd_send_command(0x06);

    sleep_us(1000); // Short delay after init
}

// Sends a single character to the LCD (with RS = 1)
void lcd_send_char(char c)
{
    uint8_t high = (c & 0xF0);           // Get high nibble
    uint8_t low  = (c << 4) & 0xF0;      // Get low nibble

    // Send high nibble with RS set (data mode)
    pcf_write_byte(high | LCD_BACKLIGHT | LCD_ENABLE | LCD_RS);
    pcf_write_byte(high | LCD_BACKLIGHT | LCD_RS);

    // Send low nibble with RS set (data mode)
    pcf_write_byte(low | LCD_BACKLIGHT | LCD_ENABLE | LCD_RS);
    pcf_write_byte(low | LCD_BACKLIGHT | LCD_RS);
}

// Sends a full string to the LCD, wraps to second line after 16 characters
void lcd_send_string(char *character)
{
    int counter = 0;

    while (*character != '\0')
    {
        if (counter == 16)
        {
            lcd_send_command(0xC0); // Set DDRAM address to beginning of second line
            if(*character == ' ')
            {
                character++;
            }
            
        }
        // If printable ASCII, send as is; otherwise send a space
        if (*character >= 32 && *character <= 126)
        {
            lcd_send_char(*character);
        }
        else
        {
            lcd_send_char(' ');
        }

        // After 16 characters, move to second line
        

        counter++;
        character++; // Move to next character
    }
}

int main()
{
    stdio_init_all();         // Initialize USB serial for printf/scanf
    sleep_ms(1000);           // Wait for terminal connection to connect

    gpio_set_function(16, GPIO_FUNC_I2C); // Set GP16 as I2C SDA
    gpio_set_function(17, GPIO_FUNC_I2C); // Set GP17 as I2C SCL
    i2c_init(i2c0, 100000);               // Init I2C0 at 100kHz

    char c[33]; // Buffer for 32 characters + null terminator

    lcd_init_sequence(); // Run the LCD startup commands

    printf("Please type out what you want to say\n");
    getchar(); // Clear leftover newline from terminal buffer

    while (true)
    {
        fgets(c, sizeof(c), stdin);          // Get full input line from serial
        c[strcspn(c, "\n")] = '\0';          // Replace newline with null terminator to remove strange dashed lines
                                             // that were always showing up after writing

        lcd_send_command(0x01);              // Clear display
        sleep_ms(5);                         // Wait for command to complete

        lcd_send_string(c);                  // Send the user-entered string to the LCD
    }
}
