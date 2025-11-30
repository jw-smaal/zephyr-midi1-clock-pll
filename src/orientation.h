#ifndef ORIENTATION_H_
#define ORIENTATION_H_

#include <zephyr/drivers/sensor.h>

struct orientation_angles {
    float pitch;   /* Tilt forward/backward (-90..+90) */
    float roll;    /* Tilt sideways (-180..+180) */
    float heading; /* Compass heading (0..360) */
};


/* Compute orientation angles from accelerometer + magnetometer */
struct orientation_angles orientation_compute(struct sensor_value accel[3],
                                              struct sensor_value mag[3]);

#endif /* ORIENTATION_H_ */
