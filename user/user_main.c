#include "mem.h"
#include "c_types.h"
#include "user_interface.h"
#include "ip_addr.h"
#include "ets_sys.h"
#include "uart.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "osapi.h"
#include "espconn.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];

#define DHT_READ_PIN 2
#define DHT_GPIO_SETUP PERIPHS_IO_MUX_GPIO2_U
#define DHT_GPIO_FUNC FUNC_GPIO2
#define DHT_WAIT_PERIOD 100000
#define DHT_PULSES 10000
#define DHT_WAIT 20

#define TICKHZ 400

struct env_reading {
	bool ok;
	char err_msg[64];
	float temp;
	float hum;
};
struct env_reading read_dht22();


static volatile os_timer_t loop_timer;
static void loop(os_event_t *events);

// wifi state
uint8 station_status = STATION_IDLE;

// udp connection state
bool udp_setup = false;
struct espconn udp_conn;
esp_udp net_udp;

// metadata
uint32 loops;
uint32 messages_sent;
uint32 messages_dropped;

static void ICACHE_FLASH_ATTR
loop(os_event_t *events)
{
    os_delay_us(10000);
    system_os_post(user_procTaskPrio, 0, 0 );
}

void ICACHE_FLASH_ATTR at_recvTask()
{
        //Called from UART.
}

void wifi_status_show(void){
	struct ip_info ip;
	uint8 mac[6];
	if (wifi_get_macaddr(STATION_IF, &(mac[0]))) {
		char mac_str[25];
		os_sprintf((char*)(&(mac_str[0])), "mac: %x:%x:%x:%x:%x:%x\r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		
		uart0_sendStr(&(mac_str[0]));
	} else {
		uart0_sendStr("No mac addr available...\r\n");
	}

	station_status = wifi_station_get_connect_status();

	switch(station_status){
		case STATION_IDLE:
			uart0_sendStr("Connection: IDLE\r\n");
			break;
		case STATION_CONNECTING:
			uart0_sendStr("Connection: CONNECTING\r\n");
			break;
		case STATION_WRONG_PASSWORD:
			uart0_sendStr("Connection: WRONG_PASSWORD\r\n");
			break;
		case STATION_NO_AP_FOUND:
			uart0_sendStr("Connection: NO_AP_FOUND\r\n");
			break;
		case STATION_CONNECT_FAIL:
			uart0_sendStr("Connection: CONNECT_FAIL\r\n");
			break;
		case STATION_GOT_IP:
			uart0_sendStr("Connection: GOT_IP\r\n");
			if (wifi_get_ip_info(STATION_IF, &ip)) {
				char ip_str[22];
				os_sprintf((char*)(&(ip_str[0])), "ip: %d.%d.%d.%d\r\n", IP2STR(&(ip.ip)));
				uart0_sendStr(&(ip_str[0]));
			} else {
				uart0_sendStr("No ip info available...\r\n");
			}

			ip_addr_t ipa;
			ipa = dns_getserver(0);

			char ip_str[23];
			os_sprintf((char*)(&(ip_str[0])), "dns: %d.%d.%d.%d\r\n", IP2STR(&(ipa)));
			uart0_sendStr(&(ip_str[0]));

			break;
	}

	// set the udp setup flag to false, so we set it up next time we have a good station status
	if (station_status != STATION_GOT_IP) {
		udp_setup = false;
	}

}

void wifi_init(void){
    char ssid[32] = WIFI_SSID;
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

void udp_connect_maybe() {
	if (udp_setup)
		return;
	
	char temp[23];
	unsigned long ip = 0;
	udp_conn.type = ESPCONN_UDP;
	udp_conn.state = ESPCONN_NONE;
	udp_conn.proto.udp = &net_udp;
	udp_conn.proto.udp->local_port = espconn_port();
	uart0_sendStr("UDP Source :");
	os_sprintf(temp, "%d", udp_conn.proto.udp->local_port);
	uart0_sendStr(temp);
	udp_conn.proto.udp->remote_port = TBB_PORT;
	ip = ipaddr_addr(TBB_IP);
	uart0_sendStr(" Target: ");
	uart0_sendStr(TBB_IP);
	os_sprintf(temp, ":%d\r\n", TBB_PORT);
	uart0_sendStr(temp);
	os_memcpy(udp_conn.proto.udp->remote_ip, &ip, 4);
	if (espconn_create(&udp_conn) == ESPCONN_OK) {
		udp_setup = true;
	} else {
		uart0_sendStr("Failed to create UDP connection...\r\n");
	}
}

// fixme support > 15 size
// this only starts the list. You are expected to write the elements of the list and do the right number of elements.
void write_list(size_t *ou, char* obuf, uint8 lsize) {
	obuf[(*ou)++] = 0b10010000 + lsize;
}

// fixme support > 15 size
// this only starts the map. You are expected to write the (k,v), (k,v) pairs and do the right number of them.
void write_map(size_t *ou, char* obuf, uint8 msize) {
	obuf[(*ou)++] = 0b10000000 + msize;
}

void write_nil(size_t *ou, char* obuf) {
	obuf[(*ou)++] = 0xc0;
}

//fixme support > 255 sizes, this won't work
void write_string(size_t *ou, char* obuf, char* str) {
	size_t sl = os_strlen(str);
	if (sl < 32) {
		obuf[(*ou)++] = 0b10100000 + (uint8)sl;
	} else {
		obuf[(*ou)++] = 0xd9;
		obuf[(*ou)++] = (uint8)sl;
	}
	os_memcpy(obuf + *ou, str, sl);
	(*ou) += sl;
}


void write_float(size_t *ou, char* obuf, float num) {
	const size_t sl = sizeof(num);
	if (sl == 4) {
		obuf[(*ou)++] = 0xca;
	} else if (sl == 8) {
		obuf[(*ou)++] = 0xcb;
	}
	// msgpack is big endian, this CPU is little endian.. fun times.
	size_t i;
	for (i = 0; i < sl; i++) {
		obuf[(*ou)++] = ((char*)(&num))[sl - 1 - i];
	}
}

void write_int(size_t *ou, char* obuf, uint32 num) {
	const size_t sl = sizeof(num);
	obuf[(*ou)++] = 0xce;
	size_t i;
	for (i = 0; i < sl; i++) {
		obuf[(*ou)++] = ((char*)(&num))[sl - 1 - i];
	}
}


static void ICACHE_FLASH_ATTR
poll_loop(void *arg) {
	wifi_status_show();
	
	if (station_status == STATION_GOT_IP) {
		udp_connect_maybe();
	}

	loops++;

	if (udp_setup) {
		messages_sent++;
		
		size_t ou = 0;
		// careful not to overflow this, you absolutely can if you put in too much data :)
		char obuf[1024];

		// these are some really minimally implemented msgpack writer functions. They are not complete or bug free. careful. Read the msgpack spec.
		write_list(&ou, obuf, 4);
		write_string(&ou, obuf, TBB_NAMESPACE);
		write_nil(&ou, obuf); // options are nil or floating point unix epoch - we don't have an RTC or NTP
		
		uint8 num_fields = 1;

		struct env_reading reading;
		reading = read_dht22();

		if (reading.ok) {
			num_fields += 2;
		} else {
			num_fields += 1;
		}
		// send data
		write_map(&ou, obuf, num_fields);
		
		if (reading.ok) {
			write_string(&ou, obuf, "temp");
			write_float(&ou, obuf, reading.temp);
			write_string(&ou, obuf, "hum");
			write_float(&ou, obuf, reading.hum);
		} else {
			write_string(&ou, obuf, "dht22_error");
			write_string(&ou, obuf, reading.err_msg);
		}

		// meta data
		write_string(&ou, obuf, "metadata");
		write_map(&ou, obuf, 3);
		write_string(&ou, obuf, "loops");
		write_int(&ou, obuf, loops);
		write_string(&ou, obuf, "msgs_sent");
		write_int(&ou, obuf, messages_sent);
		write_string(&ou, obuf, "msgs_dropped");
		write_int(&ou, obuf, messages_dropped);

		// send documentation url
		write_string(&ou, obuf, "https://github.com/eastein/esp8266.dht22");
	

		char ret = espconn_sent(&udp_conn, obuf, ou);
		if (ret != ESPCONN_OK){
			uart0_sendStr("Error Sending UDP Packet...\r\n");
			messages_dropped++;
			messages_sent--; // here we decrement the messages sent number, because we assumed success before sending
		} else {
			uart0_sendStr("Sent packet...\r\n");
		}
	} else {
		messages_dropped++;
	}
}

struct env_reading read_dht22() {
	uart0_sendStr("Reading\r\n");
    
    /// setup some vars
    // wait counter
    int i_wait = 0;
    // pulse counter
    int pc = 0;
    // data storage
    uint8 return_data[100];
    return_data[0] = return_data[1] = return_data[2] = return_data[3] = return_data[4] = 0;
    int j_storage = 0;
    // prevstate && a counter
    int prevstate = 1;
    int counter = 0;
    
    // results
    struct env_reading reading;
	reading.ok = false;
	// avoid nasty uninit string stuff
	reading.err_msg[0] = 0;

    uint8 checksum = 0;
    
    //uart0_sendStr
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
    GPIO_OUTPUT_SET(DHT_READ_PIN,1);
    // time to read
    os_delay_us(40);
    GPIO_DIS_OUTPUT(2);
    PIN_PULLUP_EN(DHT_GPIO_SETUP);
    
    
    // time to rest, until the DHT drops the pin
    
    while ((GPIO_INPUT_GET(2) == 1) && (i_wait<DHT_WAIT_PERIOD)) {
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
            if (counter == 1000){
				uart0_sendStr("giving up inside main read loop for DHT22.\r\n");
				break;
			}
        }
        // same here
        prevstate = GPIO_INPUT_GET(DHT_READ_PIN);
        if (counter == 1000) {
			uart0_sendStr("giving up inside external check for DHT22.\r\n");
			break;
		}
        
        
        // so we shove it in to the array, every other bit recieved is low so we don't store it
        // plus the 80us preambles?
        if (( pc >3 ) && ( pc % 2 == 0 ));
            return_data[j_storage/8] <<= 1;
            if (counter > DHT_WAIT)
                return_data[j_storage/8] |= 1;
            j_storage++;
    }
	
	char data_str[26];
	os_sprintf((char*)(&(data_str[0])), "read bits: %d\r\n", j_storage);
	uart0_sendStr(&(data_str[0]));
    
    // do calcs!?
    
    if (j_storage > 39) {
		char data_str[128];
		os_sprintf((char*)(&(data_str[0])), "DHT22 data: %x:%x:%x:%x:%x\r\n", return_data[0], return_data[1], return_data[2], return_data[3], return_data[4]);
		uart0_sendStr(&(data_str[0]));
		
		checksum =  (return_data[0] + return_data[1] + return_data[2] + return_data[3]) & 0xFF;
        if (return_data[4] == checksum){
            // do  the calcs for humidity
            reading.hum = return_data[0] * 256 + return_data[1];
            reading.hum /= 10.0;
            
            // do the calcs for temp
            reading.temp = (return_data[2] & 0x7F)* 256 + return_data[3];
            reading.temp /= 10.0;
            
            // do calcs for negative temp
            if (return_data[2] & 0x80)
                reading.temp *= -1;
            
			reading.ok = true;
            os_sprintf((char*)(&(data_str[0])), "Temp: %f Hum: %f \r\n", reading.temp, reading.hum);
			uart0_sendStr(&(data_str[0]));
        }
        else {
			os_sprintf((char*)(&(reading.err_msg[0])), "bad checksum, %s", data_str);
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

	return reading;
}

//init
void ICACHE_FLASH_ATTR
user_init()
{
    uart_init(115200, 115200);
    wifi_init();
    
    uart0_sendStr("\r\nWifi Initialized\r\n");
    
    os_timer_disarm(&loop_timer);

	// metadata init
	loops = 0;
	messages_sent = 0;
	messages_dropped = 0;

    //Setup timer
    os_timer_setfn(&loop_timer, (os_timer_func_t *)poll_loop, NULL);
    // arm timer, 5s
    os_timer_arm(&loop_timer, 5 * TICKHZ, 1);

    uart0_sendStr("\r\nTimer Rocking Out, should get data soon :)\r\n");

}
