#include <p32xxxx.h>
#include <plib.h>


#if defined (__32MX340F512H__) || (__32MX360F512L__) || (__32MX460F512L__) || (__32MX795F512L__) || (__32MX430F064L__) || (__32MX450F256L__) || (__32MX470F512L__)
// Configuration Bit settings
// SYSCLK = 80 MHz (8MHz Crystal / FPLLIDIV * FPLLMUL / FPLLODIV)
// PBCLK = 80 MHz (SYSCLK / FPBDIV)
// Primary Osc w/PLL (XT+,HS+,EC+PLL)
// WDT OFF
// Other options are don't care
#pragma config FPLLMUL = MUL_20, FPLLIDIV = DIV_2, FPLLODIV = DIV_1, FWDTEN = OFF
#pragma config POSCMOD = HS, FNOSC = PRIPLL, FPBDIV = DIV_1
#define SLAVE1_I2C_ADDRESS 0x90
#define SYS_FREQ (80000000L)

#endif

#define	GetPeripheralClock()		(SYS_FREQ/(1 << OSCCONbits.PBDIV))
#define	GetInstructionClock()		(SYS_FREQ)

#if defined (__32MX340F512H__)

#define UART_MODULE_ID UART1 // use first UART.  ChipKit uC32 USB is on UART1

#endif


// Function Prototypes
typedef unsigned char BYTE;   
signed int temperature;
BYTE read_temperature();
void Delay(int amount);


void SendDataBuffer(const char *buffer, UINT32 size);
UINT32 GetMenuChoice(void);
UINT32 GetDataBuffer(char *buffer, UINT32 max_size);


void Delay (int amount){
    int r;
    for (r = 0; r<amount; r++){}
}

const char mainMenu[] =
{
    "\n"\
    "Project Individual Task 1: \r\n"\
    "Here are the main menu choices\r\n"\
    "1. Option 1: Celsius Reading\r\n"\
    "2. Option 2: Fahrenheit Reading\r\n"\
    "\r\n\r\nPlease Choose a number\r\n"\
};



int main (void){
    UINT32  menu_choice;
    UINT8   buf[1024];

    SYSTEMConfigPerformance(SYS_FREQ);
    
    UARTConfigure(UART_MODULE_ID, UART_ENABLE_PINS_TX_RX_ONLY);
    UARTSetFifoMode(UART_MODULE_ID, UART_INTERRUPT_ON_TX_NOT_FULL | UART_INTERRUPT_ON_RX_NOT_EMPTY);
    UARTSetDataRate(UART_MODULE_ID, GetPeripheralClock(), 57600);
    UARTEnable(UART_MODULE_ID, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
    
    SendDataBuffer(mainMenu, sizeof(mainMenu));
    TRISE = 0x00;
    LATE = 0;
    float Temp_C;
    float Temp_F;
    
    
    while (1){
        Temp_C = read_temperature();
        Delay(1000);
        Temp_C = Temp_C/2;
        Temp_F = ((float)Temp_C*9/5) + 32;
        
        OpenI2C1(I2C_EN, 44);

        menu_choice = GetMenuChoice();
        
        switch(menu_choice){
        case 1:
            sprintf (buf, "Temperature in Celsius is: %.2f\r\n", Temp_C);
            SendDataBuffer(buf, strlen(buf));
            
            break;
            
        case 2:   
            sprintf (buf, "Temperature in Fahrenheit is: %.2f\r\n", Temp_F);
            SendDataBuffer(buf, strlen(buf));
            
            break;
            
        default:
            SendDataBuffer(mainMenu, sizeof(mainMenu));
        }
    }
    return -1;
}

BYTE read_temperature()
{
    IdleI2C1();
    StartI2C1();
    IdleI2C1();
    MasterWriteI2C1(SLAVE1_I2C_ADDRESS); 
    if(I2C1STATbits.ACKSTAT ==1) 
        goto read_temperature_fail;
    IdleI2C1();
    MasterWriteI2C1(0x00);    
    while(I2C1STATbits.TRSTAT) {}; 
    if(I2C1STATbits.ACKSTAT == 1)
        goto read_temperature_fail;
    IdleI2C1();
    RestartI2C1();
    while(I2C1CONbits.SEN) {};
    IdleI2C1();
    MasterWriteI2C1(SLAVE1_I2C_ADDRESS | 0x01);  
    while(I2C1STATbits.TRSTAT) {}; 
    if(I2C1STATbits.ACKSTAT == 1)
        goto read_temperature_fail;  
    IdleI2C1();
    temperature = ((signed int)MasterReadI2C1() << 1);
    if (temperature & 0x0100)
       temperature |= 0xFE00;
    IdleI2C1();
    AckI2C1();
    while(I2C1CONbits.ACKEN) {}; 
    IdleI2C1();
    if (MasterReadI2C1() & 0x80)
        temperature |= 1;
    IdleI2C1();
    NotAckI2C1();
    while(I2C1CONbits.ACKEN) {}; 
    IdleI2C1();
    StopI2C1();
    while(I2C1CONbits.PEN) {}; 
    return(temperature);
    
    read_temperature_fail:
    IdleI2C1();StopI2C1();
    while(I2C1CONbits.PEN) {};
    temperature = 0;
    return(0);
}



void SendDataBuffer(const char *buffer, UINT32 size)
{
    while(size)
    {
        while(!UARTTransmitterIsReady(UART_MODULE_ID))
            ;

        UARTSendDataByte(UART_MODULE_ID, *buffer);

        buffer++;
        size--;
    }

    while(!UARTTransmissionHasCompleted(UART_MODULE_ID))
        ;
}


UINT32 GetDataBuffer(char *buffer, UINT32 max_size)
{
    UINT32 num_char;

    num_char = 0;

    while(num_char < max_size)
    {
        UINT8 character;

        while(!UARTReceivedDataIsAvailable(UART_MODULE_ID))
            ;

        character = UARTGetDataByte(UART_MODULE_ID);

        if(character == '\r')
            break;

        *buffer = character;

        buffer++;
        num_char++;
    }

    return num_char;
}

UINT32 GetMenuChoice(void)
{
    UINT8  menu_item;

    while(!UARTReceivedDataIsAvailable(UART_MODULE_ID))
        ;

    menu_item = UARTGetDataByte(UART_MODULE_ID);

    menu_item -= '0';

    return (UINT32)menu_item;
}