#ifndef MY_COUNT_DOWN_TIMER_H
#define MY_COUNT_DOWN_TIMER_H

#include <Arduino.h>

/**
 * @brief Lightweight countdown timer for Arduino loop-based updates.
 *
 * The timer uses millis() and is intended to be updated from loop().
 * Call setDuration(), start(), then update() each loop iteration.
 */
class myCountDownTimer {
  private:
    unsigned long startTime;
    unsigned long duration;
    bool running;
    bool finished;

  public:
    /**
     * @brief Construct a timer with zero duration and stopped state.
     */
    myCountDownTimer();

    /**
     * @brief Set countdown duration in seconds.
     * @param seconds Duration before the timer finishes.
     */
    void setDuration(int seconds);

    /**
     * @brief Start or restart the countdown using the configured duration.
     */
    void start();

    /**
     * @brief Set duration and start countdown in one call.
     * @param durationMs Countdown duration in milliseconds.
     */
    void start(unsigned long durationMs);

    /**
     * @brief Advance timer state; call this repeatedly from loop().
     */
    void update();

    /**
     * @brief Get remaining whole seconds until completion.
     * @return Remaining seconds, clamped to 0.
     */
    int getRemainingSeconds();

    /**
     * @brief Get remaining time as MM:SS.
     *
     * This allows existing call sites like String(gameTimer.getRemainingTime())
     * to render formatted timer text.
     * @return Formatted remaining time string.
     */
    String getRemainingTime();

    /**
     * @brief Check whether the countdown has reached zero.
     * @return True when the timer is finished.
     */
    bool isFinished();

    /**
     * @brief Get remaining time in MM:SS format.
     * @return Formatted remaining time string.
     */
    String getFormattedRemainingTime();
};

#endif
