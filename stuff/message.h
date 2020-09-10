/*
 * USB Tutorial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * Message sent via USB connection
 */

#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

#define MESSAGE_MAGIC 0xd3


struct message
{
    uint8_t magic; // magic byte 0xd3
    uint8_t len; // length incl. CRC32 (in bytes)
    uint8_t type;
    uint8_t subtype;
    uint8_t data[0];
    // uint8_t crc8;

    /**
     * @brief Returns a pointer to the message in the buffer.
     * 
     * Checks if the message is valid. If not, `nullptr` is returned.
     * The message must start at the beginning of the buffer, but it
     * might not occupy the entire buffer.
     * 
     * The message is not copied. It continues to be in the buffer.
     * 
     * @param buf pointer to buffer
     * @param buf_len length of valid data in buffer
     * @return message, or `nullptr` if no valid message is contained
     */
    static message* get_message(uint8_t* buf, int buf_len);

    /**
     * @brief Removes the invalid message at the start of the buffer.
     * 
     * The remaining bytes in the buffer are moved to the front and the
     * buffer length is updated.
     * 
     * @param buf pointer to buffer
     * @param buf_len length of valid data in buffer
     */
    static void remove_invalid_message(uint8_t* buf, int& buf_len);

} __attribute__((packed));

#endif

