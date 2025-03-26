We are using the LCD1602 I2C module, which is a basic HD44780 LCD display with a PCF8574 I/O expander "backpack" soldered onto it. This backpack converts I2C serial data into parallel signals required by the LCD, reducing the number of GPIOs we need.

The first step is wiring. Since we're using I2C, we need two data lines: SDA (Serial Data) and SCL (Serial Clock). For this project, we're using the Raspberry Pi Pico W with the Raspberry Pi Pico extension in VSCode. We chose GPIO16 for SDA and GPIO17 for SCL. We also connect GND on the LCD module to a ground pin on the Pico and connect VCC on the LCD to the VBUS pin (which provides 5V). It’s important to use VBUS instead of 3.3V because the LCD module requires 5V.

With physical connections in place, we move to the code. First, we initialize USB serial communication (to use CoolTerm) and enable the hardware I2C functionality in our CMakeLists.txt file. This includes linking the hardware_i2c library.

<img src="https://github.com/user-attachments/assets/4ce840ba-229b-4093-85f5-fcf0f650af28" height="50%" width="100%" alt="Disk Sanitization Steps"/>

In the code, we configure GPIO16 and GPIO17 to function in I2C mode using gpio_set_function(). Then we initialize I2C0 with i2c_init() at a baud rate of 100 kHz (standard I2C speed). The Raspberry Pi Pico W has two separate I2C buses: i2c0 and i2c1. For this project, we are using i2c0.

Once I2C is configured, we need to determine the slave address of our LCD module. While the datasheet lists the default I2C address as either 0x27 or 0x3F, it’s best to scan the bus to confirm. To do this, we create a simple C program (I2C_LCD_Scan_Code.c) that attempts to write to all 127 possible I2C addresses and checks which device acknowledges (ACKs) the communication. The address that responds is our LCD’s I2C address.

In our case, the scan confirmed the address was 0x27, so we use that in our program. With the address known, we're ready to start coding for display control.

<img src="https://github.com/user-attachments/assets/5ea4a16a-bee1-4be5-af6b-39ebc27471b0" height="20%" width="50%" alt="Disk Sanitization Steps"/>

---------------------------------





