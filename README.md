We are using the LCD1602 I2C module, which is a basic HD44780 LCD display with a PCF8574 I/O expander "backpack" soldered onto it. This backpack converts I2C serial data into parallel signals required by the LCD, reducing the number of GPIOs we need.

The first step is wiring. Since we're using I2C, we need two data lines: SDA (Serial Data) and SCL (Serial Clock). For this project, we're using the Raspberry Pi Pico W with the Raspberry Pi Pico extension in VSCode. We chose GPIO16 for SDA and GPIO17 for SCL. We also connect GND on the LCD module to a ground pin on the Pico and connect VCC on the LCD to the VBUS pin (which provides 5V). It’s important to use VBUS instead of 3.3V because the LCD module requires 5V.

With physical connections in place, we move to the code. First, we initialize USB serial communication (to use CoolTerm) and enable the hardware I2C functionality in our CMakeLists.txt file. This includes linking the hardware_i2c library.

<img src="https://github.com/user-attachments/assets/4ce840ba-229b-4093-85f5-fcf0f650af28" height="50%" width="100%" alt="Disk Sanitization Steps"/>

In the code, we configure GPIO16 and GPIO17 to function in I2C mode using gpio_set_function(). Then we initialize I2C0 with i2c_init() at a baud rate of 100 kHz (standard I2C speed). The Raspberry Pi Pico W has two separate I2C buses: i2c0 and i2c1. For this project, we are using i2c0.

Once I2C is configured, we need to determine the slave address of our LCD module. While the datasheet lists the default I2C address as either 0x27 or 0x3F, it’s best to scan the bus to confirm. To do this, we create a simple C program (I2C_LCD_Scan_Code.c) that attempts to write to all 127 possible I2C addresses and checks which device acknowledges (ACKs) the communication. The address that responds is our LCD’s I2C address.

In our case, the scan confirmed the address was 0x27, so we use that in our program. With the address known, we're ready to start coding for display control.

<img src="https://github.com/user-attachments/assets/5ea4a16a-bee1-4be5-af6b-39ebc27471b0" height="20%" width="50%" alt="Disk Sanitization Steps"/>

<img src="https://github.com/user-attachments/assets/eef8ddb6-d16d-4f6c-bfbf-e2ca13bfe99e" height="20%" width="50%" alt="Disk Sanitization Steps"/>

---------------------------------

Just like the previous .c file, we begin our current I2C_LCD.c file with the same I2C initialization. However, before diving into full LCD configuration, we want to confirm that our I2C communication is working correctly.

We define four bitmasks using #define for RS (Register Select), RW (Read/Write), EN (Enable), and the backlight control. These are used to construct the control byte we'll send over I2C. Then, using the i2c_write_blocking function, we send a single byte to our LCD’s slave address (0x27). To test communication, we send 0x08, which turns on the backlight (bit 3), and if the LCD lights up, we know we’re successfully communicating over I2C.

Now that we have working communication, we create a helper function called void lcd_init_sequence() to initialize the LCD.

<img src="https://github.com/user-attachments/assets/19bf79a1-030f-4a80-a90b-e3d450cc0e82" height="20%" width="70%" alt="Disk Sanitization Steps"/>

The very first step is to ensure the LCD enters 8-bit mode. Although we will eventually use it in 4-bit mode, the datasheet and common initialization practices recommend sending the 8-bit mode command (0x30) three times in a row. This helps reset the internal state of the LCD and ensures reliable initialization.

After that, we switch the LCD into 4-bit mode by sending 0x20. This byte sets:

- DL (Data Length) = 0 (indicating 4-bit mode),

- N = 0 (1-line display),

- F = 0 (5x8 dot format), but we later modify these with the proper settings.

To latch the byte into the LCD, we OR 0x20 with LCD_ENABLE (bit 2) and LCD_BACKLIGHT (bit 3), giving us a control byte. We first send this byte with EN set (to latch), then send the same value again with EN cleared (to complete the pulse). This transition from HIGH to LOW on the EN pin is what triggers the LCD to read the data.

Once we are in 4-bit mode, we send:

- 0x28 for 4-bit mode, 2-line display, 5x8 character dots (DL=0, N=1, F=0),

- 0x0C to turn on the display (D=1), with cursor OFF (C=0) and blink OFF (B=0),

- 0x01 to clear the display, and

- 0x06 to set entry mode (I/D=1 to move cursor right, S=0 for no shift).

After calling this lcd_init_sequence() function, the LCD is fully ready to receive and display data.

----------------------

Our next step is to create two similar functions but with different purposes.

The first function is void lcd_send_command(uint8_t cmd). This function mirrors what we did during initialization with bitmasking and the pcf_write_byte() function, but it is specifically designed to send commands to the LCD. It’s used for operations such as clearing the screen, toggling the cursor, enabling blinking, or moving the cursor. In fact, this is the same function called by lcd_init_sequence() to configure the LCD at startup.

![image](https://github.com/user-attachments/assets/08d4f707-966c-4b4d-8dc2-e73e092baab5)

-------------

The second user-defined function is void lcd_send_char(char c). This function also builds a command using bitmasking and sends it via pcf_write_byte(). The only difference from lcd_send_command() is that we OR the data with LCD_RS (Register Select) to indicate we are sending data, not a command. This tells the LCD to interpret the byte as a printable character rather than an instruction.

![image](https://github.com/user-attachments/assets/46aa9bf4-43ed-4d72-845b-4679a89cebde)

--------------

Since sending characters one by one isn’t efficient when working with full strings, we write another helper function:
void lcd_send_string(char *character).
This function accepts a pointer to a character array (a C string), iterates through it, and prints each valid ASCII character to the LCD. It skips non-printable characters (like tabs or backspaces) by checking their ASCII value range. It also tracks how many characters have been sent and moves to the second line after 16 characters by calling lcd_send_command(0xC0) (the command to move the cursor to the beginning of the second row). This way, even if the user types more than 16 characters, they’re not lost — they wrap to the second line cleanly.

![image](https://github.com/user-attachments/assets/8a834980-e45d-4247-b4e4-fb32726c7cba)

-----------------------------

Now that all of this is implemented, our main() becomes very clean and readable. Inside the main loop, we:

- Use fgets() to collect a full line of input from the user through serial (CoolTerm),

- Store it into a character array,

- Clear the LCD screen, and

- Pass the string to lcd_send_string() for printing.

Since all the core logic is wrapped inside our user-defined functions, main() contains minimal code and clearly describes the logic flow. We also added two small but important lines during debugging:

- getchar(); 
- c[strcspn(c, "\n")] = '\0';

![image](https://github.com/user-attachments/assets/8ab3ed6b-4b81-412d-8192-bbc5a2d2006d)

The reason for this is that when the program prints to the terminal (printf()), it leaves a trailing newline (\n). If we don’t clear that newline before reading input with fgets(), the first line of actual user input is ignored. So we call getchar() once after the initial prompt to "consume" the leftover newline.

Next, when we typed something and hit Enter, the LCD was showing strange ghost characters (boxes or lines). That happened because fgets() captures the newline character from the Enter key. So even though we only typed "Hello", the string stored was "Hello\n" — and that \n was being interpreted as a non-printable character on the LCD.

To fix this, we used: c[strcspn(c, "\n")] = '\0';
This line finds the position of the first newline character in the string (if present), and replaces it with a null terminator ('\0'). This trims the string cleanly, preventing ghost characters from being printed on the display.

--------------------------------

Now that everything is set up, we can connect to CoolTerm and type anything and it will appear cleanly on the LCD!

As a final note: scrolling (like the effect you see on digital billboards where text slides left or right) is not yet implemented, but it’s something I plan to add in the future. It will require sending shifting commands repeatedly while maintaining the cursor or memory address offset.

<img src="https://github.com/user-attachments/assets/5cc19a3e-ad06-4843-811b-214e5fb3aa94" height="20%" width="70%" alt="Disk Sanitization Steps"/>

<img src="https://github.com/user-attachments/assets/6d94d24b-92a2-4afd-aead-b9fc61b3e0f9" height="20%" width="100%" alt="Disk Sanitization Steps"/>

-----------------------------

<img src="https://github.com/user-attachments/assets/f5aa03e1-5f59-4ba8-bb8a-c786e6685dfc" height="20%" width="70%" alt="Disk Sanitization Steps"/>

<img src="https://github.com/user-attachments/assets/e24cf6b9-fa0e-4a4c-8ee9-356b39938f37" height="20%" width="100%" alt="Disk Sanitization Steps"/>
