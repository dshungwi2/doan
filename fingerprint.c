/*
 * fingerprint.c
 *
 *  Created on: Nov 18, 2021
 *      Author: Steven
 */
// TODO clean up and migrate to new model

#include "fingerprint.h"
#include "ssd1306.h"
#include "fonts.h"
#include <stdio.h>
#include <stdbool.h>
F_Packet fpacket = {
    .start_code = FINGERPRINT_STARTCODE,
    .address = 0xFFFFFFFF,
    .type = FINGERPRINT_COMMANDPACKET,
};

F_Packet rpacket;

uint16_t status_reg = 0x0;    ///< The status register (set by getParameters)
uint16_t system_id = 0x0;     ///< The system identifier (set by getParameters)
uint16_t capacity = 64;       ///< The fingerprint capacity (set by getParameters)
uint16_t security_level = 0;  ///< The security level (set by getParameters)
uint32_t device_addr =
    0xFFFFFFFF;              ///< The device address (set by getParameters)
uint16_t packet_len = 64;    ///< The max packet length (set by getParameters)
uint16_t baud_rate = 57600;  ///< The UART baud rate (set by getParameters)

void init_fingerprint() {
    uint8_t temp = FINGERPRINT_READSYSPARAM;
    setup_packet(&temp, 1);
    receive();
    // insert init;
}

void setup_packet(uint8_t *data, uint8_t size) {
    // setup the length, datasize, and checksum
    if (size > 64)
        return;

    // length
    uint16_t packet_length = 2 + size;
    fpacket.length = size;  // packet size

    // data and checksum
    uint16_t sum =
        ((packet_length) >> 8) + ((packet_length)&0xFF) + fpacket.type;

    for (int i = 0; i < size; i++) {
        fpacket.data[i] = data[0];
        sum += *data;
        data++;
    }
    fpacket.checksum = sum;
    //
}

void setup_received(uint8_t *data) {
    // Data format
    /*
     * 2 bytes of header
     * 4 bytes of address
     * 1 byte of package identifier (type)
     * 2 byte of length
     * n amount of data
     * 2 checksum (which technically u dunnit)
     *
     * */
    rpacket.start_code = ((uint16_t)data[0] << 8) + ((uint16_t)data[1]);
    rpacket.address = ((uint32_t)data[2] << 24) + ((uint32_t)data[3] << 16) +
                      ((uint32_t)data[4] << 8) + ((uint32_t)data[5]);
    rpacket.type = data[6];
    rpacket.length = ((uint16_t)data[7] << 8) + ((uint16_t)data[8]);

    // get all the data
    for (int i = 0; i < rpacket.length - 2; i++) {
        rpacket.data[i] = data[9 + i];
    }
}

void send() {
    // first, send start command
    uint8_t value;

    for (int i = 0; i < 2; i++) {
        value = (fpacket.start_code >> (8 * (1 - i)));
        HAL_UART_Transmit(&UART_HANDLER, &value, 1, 100);
    }

    // second, send chip addr
    for (int i = 0; i < 4; i++) {
        value = 0xFF;
        HAL_UART_Transmit(&UART_HANDLER, &value, 1, 100);
    }

    // third, send package identifier
    value = fpacket.type;
    HAL_UART_Transmit(&UART_HANDLER, &value, 1, 100);

    // fourth, send package length
    uint16_t packet_length = fpacket.length + 2;
    for (int i = 0; i < 2; i++) {
        value = (packet_length >> (8 * (1 - i)));
        HAL_UART_Transmit(&UART_HANDLER, &value, 1, 100);
    }

    // fifth, send package data
    for (int i = 0; i < fpacket.length; i++) {
        value = fpacket.data[i];
        HAL_UART_Transmit(&UART_HANDLER, &value, 1, 100);
    }

    // lastly checksum
    for (int i = 0; i < 2; i++) {
        value = (fpacket.checksum >> (8 * (1 - i)));
        HAL_UART_Transmit(&UART_HANDLER, &value, 1, 100);
    }
}


void receive() {
    uint8_t recv_buffer[100] = {0};
    send();

		HAL_UART_Receive(&UART_HANDLER, recv_buffer, 100, 500);
		setup_received(recv_buffer);

    //	//get data and checksum
    //	HAL_UART_Receive(&UART_HANDLER, &(rpacket->data), rpacket->length - 2 ,
    // timeout);

    //	//set received checksum
    //	rpacket->checksum = rpacke
}

static char last_msg[64] = "";
void clear_last_message() {
    last_msg[0] = '\0';
}
void display_message( char* msg, uint8_t x, uint8_t y, FontDef_t* font) {

    static uint8_t last_x = 255;
    static uint8_t last_y = 255;

    // Neu thông diep hoac vi trí khác voi lan truoc
    if (strcmp(last_msg, msg) != 0 || x != last_x || y != last_y) {
        SSD1306_Clear(); // Ch? xóa khi có thay d?i
        SSD1306_GotoXY(x, y);
        SSD1306_Puts((char*)msg, font, 1);
        SSD1306_UpdateScreen();

        // Cap nhat lai 
        strcpy(last_msg, msg);
        last_x = x;
        last_y = y;
    }
}

uint8_t enroll_fingerprint(uint8_t id) {
    uint8_t data[4];
    uint8_t result;

    // Step 1: L?y ?nh vân tay d?u tiên
    SSD1306_Clear();
    //display_message("Enrolling...", 0, 0);
    do {
        uint8_t cmd = FINGERPRINT_GETIMAGE;
        setup_packet(&cmd, 1);
        receive();

        result = rpacket.data[0];
        //HAL_Delay(500);
    } while (result != FINGERPRINT_OK);
    display_message("Image 1 OK", 0, 20, &Font_7x10);

    // Step 2: Chuy?n ?nh thành template buffer 1
    data[0] = FINGERPRINT_IMAGE2TZ;
    data[1] = 1;
    setup_packet(data, 2);
    receive();

    if (rpacket.data[0] != FINGERPRINT_OK) {
        display_message("Conv 1 Fail", 0, 40, &Font_7x10);
        return rpacket.data[0];
    }
    display_message("Conv 1 OK ", 0, 40, &Font_7x10);


    // Step 3: Yêu c?u b? tay ra
    SSD1306_Clear();
    display_message("bo tay ra", 0, 0, &Font_7x10);
    //HAL_Delay(500);

    do {
        uint8_t cmd = FINGERPRINT_GETIMAGE;
        setup_packet(&cmd, 1);
        receive();
    } while (rpacket.data[0] != FINGERPRINT_NOFINGER);

    // Step 4: Yêu c?u d?t l?i tay
    SSD1306_Clear();
    display_message("dat tay lai...", 0, 0, &Font_7x10);


    do {
        uint8_t cmd = FINGERPRINT_GETIMAGE;
        setup_packet(&cmd, 1);
        receive();

        result = rpacket.data[0];
        //HAL_Delay(500);
    } while (result != FINGERPRINT_OK);


    display_message("Image 2 OK", 0, 20, &Font_7x10);


    // Step 5: Chuy?n ?nh th? 2 thành template buffer 2
    data[0] = FINGERPRINT_IMAGE2TZ;
    data[1] = 2;
    setup_packet(data, 2);
    receive();

    if (rpacket.data[0] != FINGERPRINT_OK) {

        display_message("Conv 2 Fail", 0, 40, &Font_7x10);

        return rpacket.data[0];
    }


    display_message("Conv 2 OK ", 0, 40, &Font_7x10);


    // Step 6: Ghép 2 template thành model
    uint8_t cmd = FINGERPRINT_REGMODEL;
    setup_packet(&cmd, 1);
    receive();

    SSD1306_Clear();
    if (rpacket.data[0] != FINGERPRINT_OK) {

        display_message("Model Fail", 0, 0, &Font_7x10);
        return rpacket.data[0];
    }


    display_message("Model OK", 0, 0, &Font_7x10);

    // Step 7: Luu template vào ID
    data[0] = FINGERPRINT_STORE;
    data[1] = 1;  // buffer s? 1
    data[2] = (id >> 8) & 0xFF;
    data[3] = id & 0xFF;
    setup_packet(data, 4);
    receive();

    if (rpacket.data[0] != FINGERPRINT_OK) {
        display_message("Store Fail", 0, 20, &Font_7x10);
        HAL_Delay(500);
        return rpacket.data[0];
    }

    // Thông báo luu thành công

    char idStr[30];
    sprintf(idStr, "Stored ID:%d", id);
    display_message(idStr, 0, 21, &Font_7x10);
    HAL_Delay(500);
    return rpacket.data[0];
}

uint8_t check_fingerprint() {
    uint8_t temp;
    // 1. GetImage
    temp = FINGERPRINT_GETIMAGE;
    setup_packet(&temp, 1);
    receive();

    if (rpacket.data[0])
        return rpacket.data[0];
		
    // 2. Conv image to value image2Tz
    uint8_t data[6] = {FINGERPRINT_IMAGE2TZ, 2};
    setup_packet(data, 2);
    receive();

    if (rpacket.data[0])
        return rpacket.data[0];

    // 3. Search for it
    data[0] = FINGERPRINT_SEARCH;
    data[1] = 2;
    data[2] = data[3] = 0x00;
    data[4] = (uint8_t)(capacity >> 8);
    data[5] = (uint8_t)(capacity & 0xFF);
    setup_packet(data, 6);
    receive();
		
    if (rpacket.data[0] == FINGERPRINT_OK) {  
				uint16_t fingerID = ((uint16_t)rpacket.data[1] << 8) | rpacket.data[2];
				
        return fingerID;

    } else {
        
        return 0xff;  // Không tìm th?y
    }
}

void led_mode(uint8_t control) {
    if ((control == 0) | (control == 1)) {
        uint8_t temp = control == 0 ? FINGERPRINT_LEDOFF : FINGERPRINT_LEDON;
        setup_packet(&temp, 1);
        receive();
    }
}
uint8_t Led_control(uint8_t control, uint8_t speed, uint8_t coloridx, uint8_t count){
		uint8_t data[5];
    data[0] = FINGERPRINT_AURALEDCONFIG;
    data[1] = control;   
    data[2] = speed;     
    data[3] = coloridx;  
    data[4] = count;
    setup_packet(data, 5);
    receive();
		return rpacket.data[0]; // 0 -ok
	
	
}
uint16_t get_template_number() {
    uint8_t temp;
    temp = FINGERPRINT_TEMPLATECOUNT;
    setup_packet(&temp, 1);
    receive();
    return ((uint16_t)rpacket.data[1] << 8) | rpacket.data[2];

}

void reset_database() {
    uint8_t temp;
    // 1. GetImage
    temp = FINGERPRINT_EMPTY;
    setup_packet(&temp, 1);
    send();
}
uint8_t delete_model(uint16_t location){
	    // delete
		uint8_t data[5];
    data[0] = FINGERPRINT_DELETE;
    data[1] = (uint8_t)(location )>> 8;
    data[2] = (uint8_t)(location & 0xFF);
    data[3] = 0x00;
		data[4] = 0x01;
    setup_packet(data, 5);
    receive();
		return rpacket.data[0]; // 0 -ok
}

uint8_t load_model(uint16_t location){
	    
		uint8_t data[4];
    data[0] = FINGERPRINT_LOAD;
    data[1] = 0x01;
    data[2] = (uint8_t)(location >> 8);
    data[3] = (uint8_t)(location & 0xFF);
    setup_packet(data, 4);
    receive();
		return rpacket.data[0]; // 0 -ok
}
uint8_t check_password(uint32_t thePassword){
	    
		uint8_t data[5];
    data[0] = FINGERPRINT_VERIFYPASSWORD;
    data[1] = (uint8_t)(thePassword >> 24);
    data[2] = (uint8_t)(thePassword >> 16);
    data[3] = (uint8_t)(thePassword >> 8);
		data[4] = (uint8_t)(thePassword & 0xFF);
    setup_packet(data, 5);
    receive();
		if (rpacket.data[0] == FINGERPRINT_OK){
			return FINGERPRINT_OK;
			}
		else{
    return FINGERPRINT_PACKETRECIEVEERR;
		}
}
uint8_t set_password(uint32_t password){
	    
		uint8_t data[5];
    data[0] = FINGERPRINT_SETPASSWORD;
    data[1] = (uint8_t)(password >> 24);
    data[2] = (uint8_t)(password >> 16);
    data[3] = (uint8_t)(password >> 8);
		data[4] = (uint8_t)(password & 0xFF);
    setup_packet(data, 5);
    receive();
		return rpacket.data[0];
		}