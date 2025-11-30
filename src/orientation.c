#include "orientation.h"
#include <math.h>


struct orientation_angles orientation_compute(struct sensor_value accel[3],
                                              struct sensor_value mag[3])
{
    struct orientation_angles out;

    float ax = sensor_value_to_float(&accel[0]);
    float ay = sensor_value_to_float(&accel[1]);
    float az = sensor_value_to_float(&accel[2]);

    float mx = sensor_value_to_float(&mag[0]);
    float my = sensor_value_to_float(&mag[1]);
    float mz = sensor_value_to_float(&mag[2]);

    /* Roll and pitch from accelerometer */
    out.roll  = atan2f(ay, az) * (180.0f / M_PI);
    out.pitch = atan2f(-ax, sqrtf(ay*ay + az*az)) * (180.0f / M_PI);

    /* Tilt compensation for magnetometer */
    float roll_rad  = atan2f(ay, az);
    float pitch_rad = atan2f(-ax, sqrtf(ay*ay + az*az));

    float mx_comp = mx * cosf(pitch_rad) + mz * sinf(pitch_rad);
    float my_comp = mx * sinf(roll_rad) * sinf(pitch_rad)
                  + my * cosf(roll_rad)
                  - mz * sinf(roll_rad) * cosf(pitch_rad);

    float heading = atan2f(-my_comp, mx_comp) * (180.0f / M_PI);
    if (heading < 0) heading += 360.0f;

    out.heading = heading;

    return out;
}
