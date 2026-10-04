#include "myCountDownTimer.h"

#include <cstdio>

myCountDownTimer::myCountDownTimer()
    : startTime(0), duration(0), running(false), finished(false) {}

void myCountDownTimer::setDuration(int seconds) {
  duration = static_cast<unsigned long>(seconds) * 1000UL;
  finished = false;
}

void myCountDownTimer::start() {
  startTime = millis();
  running = true;
  finished = false;
}

void myCountDownTimer::start(unsigned long durationMs) {
  duration = durationMs;
  start();
}

void myCountDownTimer::update() {
  if (running && !finished) {
    if (millis() - startTime >= duration) {
      finished = true;
      running = false;
    }
  }
}

String myCountDownTimer::getRemainingTime() {
  return getFormattedRemainingTime();
}

int myCountDownTimer::getRemainingSeconds() {
  if (finished) {
    return 0;
  }

  unsigned long elapsed = millis() - startTime;
  long remaining = static_cast<long>(duration - elapsed) / 1000;
  return (remaining < 0) ? 0 : static_cast<int>(remaining);
}

bool myCountDownTimer::isFinished() {
  return finished;
}

String myCountDownTimer::getFormattedRemainingTime() {
  int remaining = getRemainingSeconds();
  int minutes = remaining / 60;
  int seconds = remaining % 60;
  char buffer[6];
  snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, seconds);
  return String(buffer);
}