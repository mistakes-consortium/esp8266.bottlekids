#include "mem.h"
#include "c_types.h"
#include "user_interface.h"
#include "ets_sys.h"
#include "uart.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"



#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];

#define DHT_READ_PIN 2
#define DHT_GPIO_SETUP PERIPHS_IO_MUX_GPIO2_U
#define DHT_GPIO_FUNC FUNC_GPIO2
#define DHT_WAIT_PERIOD 100000
#define DHT_PULSES 10000
#define DHT_WAIT 20

static volatile os_timer_t dht_timer;
static void loop(os_event_t *events);

static void ICACHE_FLASH_ATTR
loop(os_event_t *events)
{
    os_printf("Hello\n\r");
    os_delay_us(10000);
    system_os_post(user_procTaskPrio, 0, 0 );
}

void ICACHE_FLASH_ATTR at_recvTask()
{
        //Called from UART.
}

void wifi_init(void){
    char ssid[32] = WIFI_USER;
    char password[64] = WIFI_PASS;
    
    struct station_config stconf;
        
    // station mode
    wifi_set_opmode(0x1);
    
    // this is necessary, see: http://41j.com/blog/2015/01/esp8266-wifi-doesnt-connect/
    stconf.bssid_set = 0;
    
    // yay auth stuff
    os_memcpy((char *)stconf.ssid,ssid, 32);
    os_memcpy((char *)stconf.password,password, 64);
    
    // set config
    wifi_station_set_config(&stconf);
    wifi_station_set_auto_connect(1);
}


static void ICACHE_FLASH_ATTR
read_sensor(void *arg) {
    uart0_sendStr("Reading\r\n");
    
    /// setup some vars
    // wait counter
    int i_wait = 0;
    // pulse counter
    int pc = 0;
    // data storage
    int return_data[DHT_PULSES];
    return_data[0] = return_data[1] = return_data[2] = return_data[3] = return_data[4] = 0;
    int j_storage = 0;
    // prevstate && a counter
    int prevstate = 1;
    int counter = 0;
    
    // results
    float res_temp = 0.0;
    float res_hum = 0.0;
    int checksum = 0;
    
    
    PIN_FUNC_SELECT(DHT_GPIO_SETUP, DHT_GPIO_FUNC);
    PIN_PULLUP_EN(DHT_GPIO_SETUP);
    // high
    GPIO_OUTPUT_SET(DHT_READ_PIN,1);
    // os_delay_us(500);
    os_delay_us(250000);
    // low & wait a bit
    GPIO_OUTPUT_SET(DHT_READ_PIN,0);
    // os_delay_us(20);
    os_delay_us(20000);
    // time to read
    GPIO_DIS_OUTPUT(2);
    os_delay_us(40);
    PIN_PULLUP_EN(DHT_GPIO_SETUP);
    
    
    // time to rest, until the DHT drops the pin
    
    while (GPIO_INPUT_GET(2) == 1 && i_wait<DHT_WAIT_PERIOD) {
          os_delay_us(1);
          i_wait++;
    }
    if(i_wait==DHT_WAIT_PERIOD) {
        // timeout, try again soon
        uart0_sendStr("timeout?\r\n");
        return;
    }
    
    // read, I guess
    
    for (pc = 0; pc < DHT_PULSES; pc++){
        counter = 0;
        while (GPIO_INPUT_GET(DHT_READ_PIN) == prevstate){
            counter++;
            os_delay_us(1);
            
            // this happens because we've reached the end and there is no fresh data coming in?
            if (counter == 1000){break;}
        }
        // same here
        prevstate = GPIO_INPUT_GET(DHT_READ_PIN);
        if (counter == 1000) {break;}
        
        
        // so we shove it in to the array, every other bit recieved is low so we don't store it
        // plus the 80us preambles?
        if (( pc >3 ) && ( pc % 2 == 0 ));
            return_data[j_storage/8] <<= 1;
            if (counter > DHT_WAIT)
                return_data[j_storage/8] |= 1;
            j_storage++;
    }
    
    // do calcs!?
    
    if (j_storage > 39) {
        checksum =  (return_data[0] + return_data[1] + return_data[2] + return_data[3]) & 0xFF;
        if (return_data[4] == checksum){
            // do  the calcs for humidity
            res_hum = return_data[0] * 256 + return_data[1];
            res_hum /= 10.0;
            
            // do the calcs for temp
            res_temp = (return_data[2] & 0x7F)* 256 + return_data[3];
            res_temp /= 10.0;
            
            // do calcs for negative temp
            if (return_data[2] & 0x80)
                res_temp *= -1;
            
            const char *ret_temp = "Temp: %f \r\n", res_temp;
            const char *ret_hum = "Hum: %f \r\n", res_hum; 
            uart0_sendStr(ret_temp);
            uart0_sendStr(ret_hum);
        }
        else {
            uart0_sendStr("Bad Checksum\r\n");
            
//             const char *cs = "checksum: %i \r\n", checksum;
//             const int ret4 = return_data[4];
//             const char *ret_c = "return_c: %d \r\n", ret4; 
//             const char *ret_a = "return_all: %s \r\n", return_data; 
//             uart0_sendStr(cs);
//             uart0_sendStr(ret4);
//             uart0_sendStr(ret_a);
        }
        
    }
    

    
    
    
}
//init
void ICACHE_FLASH_ATTR
user_init()
{
    uart_init(115200, 115200);
    wifi_init();
    
    uart0_sendStr("\r\nWifi Initialized\r\n");
    
    os_timer_disarm(&dht_timer);

    //Setup timer
    os_timer_setfn(&dht_timer, (os_timer_func_t *)read_sensor, NULL);

    // arm timer, 10s
    os_timer_arm(&dht_timer, 40000, 1);
    uart0_sendStr("\r\nTimer Rocking Out, should get data soon :)\r\n");

}