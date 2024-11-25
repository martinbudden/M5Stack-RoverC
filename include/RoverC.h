#pragma once

#include <cstdint>

class RoverC {
public:
    enum { MIN_SPEED = -100, MAX_SPEED = 100 };
    enum control_mode_t { MECANUM_MODE, TANK_MODE };
public:
    RoverC(void);
private:
    enum { SDA_PIN = 0, SCL_PIN = 26 }; //!< Extended IO port: Pin 0 and 26
    enum { GROVE_SDA_PIN = 32, GROVE_SCL_PIN = 33 }; //!< // Grove-Connector: Pin 32 and 33
    enum : uint8_t { I2C_ADDRESS = 0x38 };
    enum : uint8_t { REGISTER_MOTOR_1 = 0x00, REGISTER_MOTOR_2 = 0x01, REGISTER_MOTOR_3 = 0x02, REGISTER_MOTOR_4 = 0x03 }; 
public:
    void stop(void);
    void move(float throttle, float roll, float pitch, float yaw, control_mode_t control_mode = MECANUM_MODE);
    float getSpeed(void) const { return _speed; }
    float getAngle(void) const { return _angle; }
    void setServoAngle(uint8_t servoChannel, int angle);
private:
    void moveMecanumMode(float throttle, float roll, float pitch, float yaw);
    void moveTankMode(float throttle, float roll, float pitch, float yaw);
    void setMotorSpeeds(int speedM1, int speedM2, int speedM3, int speedM4);
    void setMotorSpeed(uint8_t motor, int speed);
    void setServoPulse(uint8_t servoChannel, uint16_t pulseWidth);
protected:
    static int clip(int value, int min, int max) { return value < min ? min : value > max ? max : value; }
private:
    float _speed {0.0};
    float _angle {0.0};
};

