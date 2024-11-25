#include "RoverC.h"
#include <Wire.h>


RoverC::RoverC() // NOLINT(hicpp-use-equals-default,modernize-use-equals-default) false positive
{
    // initialize I2C
    Wire.begin(SDA_PIN, SCL_PIN);
}

void RoverC::stop()
{
    setMotorSpeeds(0, 0, 0, 0);
}

void RoverC::setMotorSpeeds(int speedM1, int speedM2, int speedM3, int speedM4)
{
    setMotorSpeed(REGISTER_MOTOR_1, speedM1);
    setMotorSpeed(REGISTER_MOTOR_2, speedM2);
    setMotorSpeed(REGISTER_MOTOR_3, speedM3);
    setMotorSpeed(REGISTER_MOTOR_4, speedM4);
}

void RoverC::setMotorSpeed(uint8_t motorRegister, int speed)
{
    speed = clip(speed, MIN_SPEED, MAX_SPEED);

    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(motorRegister);
    Wire.write(speed);
    Wire.endTransmission();
}

void RoverC::setServoAngle(uint8_t servoChannel, int angle)
{
    angle = clip(angle, 0, 90);

    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(servoChannel | 0x10);
    Wire.write(angle);
    Wire.endTransmission();
}

// cppcheck-suppress unusedFunction
void RoverC::setServoPulse(uint8_t servoChannel, uint16_t pulseWidth)
{
    pulseWidth = clip(pulseWidth, 500, 2500);

    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(servoChannel | 0x20);
    Wire.write(pulseWidth);
    Wire.endTransmission();
}

void RoverC::move(float throttle, float roll, float pitch, float yaw, control_mode_t control_mode)
{
    if (control_mode == MECANUM_MODE) {
        moveMecanumMode(throttle, roll, pitch, yaw);
    } else {
        moveTankMode(throttle, roll, pitch, yaw);
    }
}

/*
 motor numbers:
1 ------ 2
|        |
|        |
|   M5   |
3 ------ 4
*/

void RoverC::moveMecanumMode(float throttle, float roll, float pitch, float yaw)
{
    const int servoAngle = 90 * abs(throttle);
    setServoAngle(0, servoAngle);
    setServoAngle(1, servoAngle);

    const int speedX = round(roll * MAX_SPEED);
    const int speedY = round(pitch * MAX_SPEED);
    const int rotation  = round(yaw * MAX_SPEED);

    _speed = (speedX + speedY)/2.0F;
    _angle = rotation;

    const int frontLeft  = speedY + speedX + rotation;
    const int frontRight = speedY - speedX - rotation;
    const int backLeft   = speedY - speedX + rotation;
    const int backRight  = speedY + speedX - rotation;

    setMotorSpeeds(frontLeft, frontRight, backLeft, backRight);
}

void RoverC::moveTankMode(float throttle, [[maybe_unused]] float roll, float pitch, float yaw)
{
    const int speedLeft = round(throttle * MAX_SPEED);
    const int speedRight = round(pitch * MAX_SPEED);
    _speed = speedLeft;
    _angle = speedRight;

    const int servoAngle = 90 * abs(yaw);
    // set both servos, so it doesn't matter which one the user plugged in
    setServoAngle(0, servoAngle);
    setServoAngle(1, servoAngle);

    setMotorSpeeds(speedLeft, speedRight, speedLeft, speedRight);
}

