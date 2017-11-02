#ifndef CONFIGURATION_H
#define CONFIGURATION_H

//SW name & version
#define     VERSION                      "0.67"
#define     SW_NAME                      "Central heat"

//EEPROM config
#define CONFIG_START 0
#define CONFIG_VERSION "v08"
/*
--------------------------------------------------------------------------------------------------------------------------

Version history:
0.67 - 2.11.2017 - obsluha klavesnice, config
0.66 - 2.11.2017 - nastaveni hodin
0.64 - 11.9.2017 - obsluha displeje, build IDE 1.8.3
0.5             - i2c keyboard
0.4 - 23.2.2016 - add RTC, prenos teploty na satelit
0.3 - 16.1.2015

--------------------------------------------------------------------------------------------------------------------------
HW
Pro Mini 328
I2C display
1 Relays module
DALLAS
keyboard

Pro Mini 328 Layout
------------------------------------------
A0              - 
A1              - 
A2              - RX from CommUnit
A3              - TX to CommUnit
A4              - I2C display SDA 0x27, I2C Central heating unit 0x02, keypad 0x20, 
A5              - I2C display SCL 0x27, I2C Central heating unit 0x02, keypad 0x20
D0              - Rx
D1              - Tx
D2              - DALLAS
D3              - 
D4              - DALLAS
D5              - DALLAS
D6              - RELAY
D7              - 
D8              - LED
D9              - 
D10             - 
D11             - 
D12             - 
D13             - BUZZER
--------------------------------------------------------------------------------------------------------------------------
*/

#define SERIAL_SPEED    57600
#define mySERIAL_SPEED  9600

//pins for softwareserial
#define RX A2
#define TX A3

#define TEMP_ERR -127     

#define START_BLOCK       '#'
#define DELIMITER         ';'
#define END_BLOCK         '$'
#define END_TRANSMITION   '*'

//display
#define LCDADDRESS   0x27
#define LCDROWS      4
#define LCDCOLS      20

//wiring
#define ONE_WIRE_BUS_IN                     2
#define ONE_WIRE_BUS_OUT                    4
#define ONE_WIRE_BUS_UT                     5
#define RELAYPIN                            6
#define LEDPIN                              8
#define BUZZERPIN                           13

#define IN                                  0
#define OUT                                 1
//#define RELAY                               100
//#define RELAYTEMP                           101

#define TEMPERATURE_PRECISION 12

//keypad i2c address
#define I2CADDR       0x20
#define PRINT_SPACE           lcd.print(F(" "));


#endif
