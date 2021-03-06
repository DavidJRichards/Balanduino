/* Copyright (C) 2013-2015 Kristian Lauszus, TKJ Electronics. All rights reserved.

 This software may be distributed and modified under the terms of the GNU
 General Public License version 2 (GPL2) as published by the Free Software
 Foundation and appearing in the file GPL2.TXT included in the packaging of
 this file. Please note that GPL2 Section 2[b] requires that all works based
 on this software must also be made publicly available under the terms of
 the GPL2 ("Copyleft").

 Contact information
 -------------------

 Kristian Lauszus, TKJ Electronics
 Web      :  http://www.tkjelectronics.dk
 e-mail   :  kristianl@tkjelectronics.dk
*/

void updatePID(float restAngle, float offset, float turning, float dt) {
  /* Brake */
  if (steerStop) {
    int32_t wheelPosition = getWheelsPosition();
    int32_t positionError = wheelPosition - targetPosition;
    if (cfg.backToSpot) {
      if (abs(positionError) > zoneA) // Inside zone A
        restAngle -= (float)positionError / positionScaleA;
      else if (abs(positionError) > zoneB) // Inside zone B
        restAngle -= (float)positionError / positionScaleB;
      else if (abs(positionError) > zoneC) // Inside zone C
        restAngle -= (float)positionError / positionScaleC;
      else // Inside zone D
        restAngle -= (float)positionError / positionScaleD;
    } else {
      if (abs(positionError) < zoneC)
        restAngle -= (float)positionError / positionScaleD;
      else
        targetPosition = wheelPosition;
    }
    restAngle -= (float)wheelVelocity / velocityScaleStop;

    restAngle = constrain(restAngle, cfg.targetAngle - 10, cfg.targetAngle + 10); // Limit rest Angle
  }
  /* Drive forward and backward */
  else {
    if ((offset > 0 && wheelVelocity < 0) || (offset < 0 && wheelVelocity > 0) || offset == 0) // Scale down offset at high speed - wheel velocity is negative when driving forward and positive when driving backward
      offset += (float)wheelVelocity / velocityScaleMove; // We will always compensate if the offset is 0, but steerStop is not set
    restAngle -= offset;
  }

  restAngle = constrain(restAngle, lastRestAngle - 1, lastRestAngle + 1); // Don't change restAngle with more than 1 degree in each loop
  lastRestAngle = restAngle;

  /* Update PID values */
  float error = restAngle - pitch;
  float pTerm = cfg.P * error;
  iTerm += cfg.I * 100.0f * error * dt; // Multiplication with Ki is done before integration limit, to make it independent from integration limit value
  iTerm = constrain(iTerm, -100.0f, 100.0f); // Limit the integrated error - prevents windup
  float dTerm = (cfg.D / 100.0f) * (error - lastError) / dt;
  lastError = error;
  float PIDValue = pTerm + iTerm + dTerm;

  /* Steer robot sideways */
  if (turning < 0) { // Left
    turning += abs((float)wheelVelocity / velocityScaleTurning); // Scale down at high speed
    if (turning > 0)
      turning = 0;
  }
  else if (turning > 0) { // Right
    turning -= abs((float)wheelVelocity / velocityScaleTurning); // Scale down at high speed
    if (turning < 0)
      turning = 0;
  }

  float PIDLeft = PIDValue + turning;
  float PIDRight = PIDValue - turning;

  PIDLeft *= cfg.leftMotorScaler; // Compensate for difference in some of the motors
  PIDRight *= cfg.rightMotorScaler;
  
  /* Set PWM Values */
  if (PIDLeft >= 0)
    moveMotor(left, forward, PIDLeft);
  else
    moveMotor(left, backward, -PIDLeft);
  if (PIDRight >= 0)
    moveMotor(right, forward, PIDRight);
  else
    moveMotor(right, backward, -PIDRight);
}

void moveMotor(Command motor, Command direction, float speedRaw) { // Speed is a value in percentage 0-100%
  if (speedRaw > 100.0f)
    speedRaw = 100.0f;
  setPWM(motor, speedRaw * (float)PWMVALUE / 100.0f); // Scale from 0-100 to 0-PWMVALUE
  if (motor == left) {
    if (direction == forward) {
      leftA::Clear();
      leftB::Set();
    }
    else {
      leftA::Set();
      leftB::Clear();
    }
  }
  else {
    if (direction == forward) {
      rightA::Set();
      rightB::Clear();
    }
    else {
      rightA::Clear();
      rightB::Set();
    }
  }
}

void stopMotor(Command motor) {
  setPWM(motor, PWMVALUE); // Set high
  if (motor == left) {
    leftA::Set();
    leftB::Set();
  }
  else {
    rightA::Set();
    rightB::Set();
  }
}

void setPWM(Command motor, uint16_t dutyCycle) { // dutyCycle is a value between 0-ICR1
  if (motor == left)
    OCR1A = dutyCycle;
  else
    OCR1B = dutyCycle;
}

void stopAndReset() {
  stopMotor(left);
  stopMotor(right);
  lastError = 0;
  iTerm = 0;
  targetPosition = getWheelsPosition();
  lastRestAngle = cfg.targetAngle;
}


int32_t readLeftEncoder() { // The encoders decrease when motors are traveling forward and increase when traveling backward
  return encoderL->getPosition();
}

int32_t readRightEncoder() {
  return encoderR->getPosition();
}

int32_t getWheelsPosition() {
  return encoderL->getPosition() + encoderR->getPosition();
}

int32_t getWheelsVelocity(void)
{
  return (encoderL->getRPM() + encoderR->getRPM() );
}

int32_t getWheelLVelocity(void)
{
  return encoderL->getRPM();
}

int32_t getWheelRVelocity(void)
{
  return encoderR->getRPM();
}
