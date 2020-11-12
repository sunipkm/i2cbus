#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <pthread.h>
#define __I2CBUS_INTERNAL
#include "i2cbus.h"
#undef __I2CBUS_INTERNAL

int i2cbus_open(i2cbus *dev, int id, int addr)
{
    // check 1: memory
    if (dev == NULL)
    {
        fprintf(stderr, "%s: Error: Device descriptor is NULL\n", __func__);
        return -1;
    }
    // check 2: id > __I2CBUS_MAX_NUM - 1
    if (!(id < __I2CBUS_MAX_NUM))
    {
        fprintf(stderr, "%s: System supports up to %d ID, you provided %d.\n", __func__, __I2CBUS_MAX_NUM - 1, id);
        return -1;
    }
    // check 3: addr valid range
    if (addr < 8 || addr & 0x7c)
    {
        fprintf(stderr, "%s: Address 0x%02x is invalid\n", __func__, addr);
        return -1;
    }

    // Try to open the file descriptor
    // step 1: Create file name
    char fname[256];
    if (snprintf(fname, 256, "/dev/i2c%d", id) < 0)
    {
        fprintf(stderr, "%s: Failed to generate device filename using snprintf. FATAL Error!\n", __func__);
        return -1;
    }
    if ((dev->fd = open(fname, O_RDWR)) < 0)
    {
        fprintf(stderr, "%s: Failed to open %s. Error %d\n", __func__, fname, dev->fd);
        return dev->fd; 
    }
    int status = 0;
    if ((status = ioctl(dev->fd, I2C_SLAVE, addr)) < 0)
    {
        fprintf(stderr, "%s: Failed to open I2C slave address 0x%02x on bus %s with error %d, returning...\n", __func__, addr, fname, status);
        close(dev->fd);
        return status;
    }
    // if we are here, then everything was successful
    dev->id = id; // assign device id
    dev->lock = &(__i2cbus_locks[id]); // assign lock
    dev->ctx = -1; // context has been cleared, it has to be set by the caller
    return dev->fd;
}

int i2cbus_close(i2cbus *dev)
{
    if (dev != NULL)
    {
        if (dev->fd > 0)
            return close(dev->fd);
        else
        {
            fprintf(stderr, "%s: %s %d\n", __func__, "Invalid file descriptor", dev->fd);
            return -2;
        }
    }
    else
    {
        fprintf(stderr, "%s: %s\n", __func__, "Invalid device descriptor");
        return -3;
    }
    // will never reach this point
    return -1;
}

#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)

int i2cbus_write(i2cbus *dev, const void *buf, ssize_t len)
{
    // usual checks
    if (unlikely(dev == NULL || dev->fd < 0))
    {
        fprintf(stderr, "%s: %s %p, %d\n", __func__, "Invalid device pointer or file descriptor", dev, dev->fd);
        return -1;
    }
    // check if it belongs to a context
    int locked = 1;
    int status = 0;
    if (!(dev->ctx < I2CBUS_CTX_0 || dev->ctx > I2CBUS_CTX_9))
    {
        status = pthread_mutex_trylock(dev->lock);
        // valid context found, check if there is a lock
        if (status == EBUSY || status == EINVAL)
            locked = 0;
    }
    // otherwise try to lock and block on it
    else
        status = pthread_mutex_lock(dev->lock);
    if (status == EDEADLK) // we already own the mutex
        locked = 0;
    status = write(dev->fd, buf, len);
    if (status != len)
    {
        fprintf(stderr, "%s: Failed to write %ld bytes, wrote %d bytes\n", __func__, len, status);
    }
    if (locked)
        pthread_mutex_unlock(dev->lock);
    return status;   
}

int i2cbus_read(i2cbus *dev, void *buf, ssize_t len)
{
    // usual checks
    if (unlikely(dev == NULL || dev->fd < 0))
    {
        fprintf(stderr, "%s: %s %p, %d\n", __func__, "Invalid device pointer or file descriptor", dev, dev->fd);
        return -1;
    }
    // check if it belongs to a context
    int locked = 1;
    int status = 0;
    if (!(dev->ctx < I2CBUS_CTX_0 || dev->ctx > I2CBUS_CTX_9))
    {
        status = pthread_mutex_trylock(dev->lock);
        // valid context found, check if there is a lock
        if (status == EBUSY || status == EINVAL)
            locked = 0;
    }
    // otherwise try to lock and block on it
    else
        status = pthread_mutex_lock(dev->lock);
    if (status == EDEADLK) // we already own the mutex
        locked = 0;
    status = read(dev->fd, buf, len);
    if (status != len)
    {
        fprintf(stderr, "%s: Failed to read %ld bytes, read %d bytes\n", __func__, len, status);
    }
    if (locked)
        pthread_mutex_unlock(dev->lock);
    return status;   
}

int i2cbus_lock(i2cbus *dev)
{
    if (likely(!(dev->ctx < I2CBUS_CTX_0 || dev->ctx > I2CBUS_CTX_9)))
    {
        return pthread_mutex_lock(dev->lock);
    }
    else
        return -EINVAL;
}

int i2cbus_trylock(i2cbus *dev)
{
    if (likely(!(dev->ctx < I2CBUS_CTX_0 || dev->ctx > I2CBUS_CTX_9)))
    {
        return pthread_mutex_trylock(dev->lock);
    }
    else
        return -EINVAL;
}

int i2cbus_unlock(i2cbus *dev)
{
    if (likely(!(dev->ctx < I2CBUS_CTX_0 || dev->ctx > I2CBUS_CTX_9)))
    {
        return pthread_mutex_unlock(dev->lock);
    }
    else
        return -EINVAL;
}