/**
 * @file i2cbus.h
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2020-11-11
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef __I2CBUS_H
#define __I2CBUS_H
#include <stdio.h>
#include <pthread.h>
/**
 * @brief Structure describing an I2C bus.
 * 
 */
#ifdef __I2CBUS_INTERNAL
#define __I2CBUS_MAX_NUM 2 /// Maximum 2 /dev/i2cX
/**
 * @brief Set of mutexes for the I2C bus
 * 
 */
pthread_mutex_t __i2cbus_locks[__I2CBUS_MAX_NUM];
/**
 * @brief Set of the contexts for the lock
 * 
 */
int __i2cbus_lock_ctx[__I2CBUS_MAX_NUM] = {
    0,
};
#endif
typedef enum
{
    I2CBUS_CTX_0 = 0xa,
    I2CBUS_CTX_1,
    I2CBUS_CTX_2,
    I2CBUS_CTX_3,
    I2CBUS_CTX_4,
    I2CBUS_CTX_5,
    I2CBUS_CTX_6,
    I2CBUS_CTX_7,
    I2CBUS_CTX_8,
    I2CBUS_CTX_9,
} i2cbus_ctxs;

typedef struct
{
    int fd;                /// I2C device file descriptor
    int id;                /// I2C device file id (X in /dev/i2cX)
    pthread_mutex_t *lock; /// Lock corresponding to the /dev/i2cX file, assigned from the locks array indexed by id
    int ctx;               /// Context for the I2C device file.
                           /// Multiple devices can share the same context if they are
                           /// parts of the same module and require time-sensitive access to
                           /// the i2c bus. In that case even if the mutex is locked for the
                           /// bus, the system will proceed to provide read/write access for
                           /// devices sharing the same context.
                           /// Note: Context has to be set AFTER i2cbus_init() has been called.
} i2cbus;
/**
 * @brief Open an I2C bus file descriptor using the supplied parameters.
 * 
 * Note: This function does NOT set the context for the same block of
 * devices. Please set the ctx in dev manually as needed. Use the members
 * of the i2cbus_ctxs enum to populate the dev->ctx variable AFTER this 
 * function has been called.
 * 
 * @param dev i2c device descriptor
 * @param id i2c device file ID (X in /dev/i2cX)
 * @param addr i2c slave address
 * @return int fd, non-negative on success, negative on error. See open() for details.
 */
int i2cbus_open(i2cbus *dev, int id, int addr);
/**
 * @brief Close the file descriptor for the I2C device. 
 * 
 * @param dev i2c device descriptor. Note: Memory for dev is not freed by this function and it has to be freed by the allocator.
 * @returns (int) Return code from close() or negative on error.
 */
int i2cbus_close(i2cbus *dev);
/**
 * @brief Write bytes to the i2c device.
 * 
 * Note: The write will proceed only when the corresponding mutex is
 * unlocked, OR if the device belongs to the same context in which
 * the bus was locked in the first place. In case of a pre-lock context,
 * this function WILL NOT unlock the mutex.
 * 
 * Note 2: It is logical to use read() or write() calls directly if
 * the execution is particularly time sensitive after acquiring a context
 * based buslock that can be retained between multiple read/write calls.
 * 
 * @param dev i2c device descriptor
 * @param buf Pointer to byte array to write (MSB first)
 * @param len Length of byte array
 * @return int Length of bytes written on success, -1 on failure
 */
int i2cbus_write(i2cbus *dev, const void *buf, ssize_t len);
/**
 * @brief Read bytes from the i2c device
 * 
 * Note: The read will proceed only when the corresponding mutex is
 * unlocked, OR if the device belongs to the same context in which
 * the bus was locked in the first place. In case of a pre-lock context,
 * this function WILL NOT unlock the mutex.
 * 
 * @param dev i2c device descriptor
 * @param buf Pointer to byte array to read to (MSB first)
 * @param len Length of byte array
 * @return int Length of bytes read on success, -1 on failure
 */
int i2cbus_read(i2cbus *dev, void *buf, ssize_t len);
/**
 * @brief Function to do a write, and get the reply in one operation
 * in order to avoid read/write mangling with multiple threads 
 * accessing the bus. Avoid using this function if you have read or
 * write buffer lengths zero.
 * 
 * @param dev i2c device descriptor
 * @param outbuf Pointer to byte array to write (MSB first)
 * @param outlen Length of output byte array
 * @param inbuf Pointer to byte array to read to (MSB first)
 * @param inlen Length of input byte array
 * @param timeout_usec Timeout between read and write (in microseconds)
 * @return int Length of bytes read on success, -1 on failure
 */
int i2cbus_xfer(i2cbus *dev,
                void *outbuf, ssize_t outlen,
                void *inbuf, ssize_t inlen,
                unsigned long timeout_usec);
/**
 * @brief Lock the mutex for the I2C bus ID. Use this only if you 
 * need continuous access to a bus for time sensitive reads. DO NOT
 * forget to call i2cbus_unlock() after this call.
 * 
 * @param dev i2c device descriptor
 * @return int pthread_mutex_lock()
 */
int i2cbus_lock(i2cbus *dev);
/**
 * @brief Try to lock the mutex for the I2C bus ID. Use this only if you 
 * need continuous access to a bus for time sensitive reads. DO NOT
 * forget to call i2cbus_unlock() after this call.
 * 
 * @param dev i2c device descriptor
 * @return int pthread_mutex_lock()
 */
int i2cbus_trylock(i2cbus *dev);
/**
 * @brief Unlock the mutex for the I2C bus
 * 
 * @param dev i2c device descriptor
 * @return int pthread_mutex_unlock()
 */
int i2cbus_unlock(i2cbus *dev);
#endif