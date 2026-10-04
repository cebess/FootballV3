#include <Arduino.h>
#include "diceRoll.h"
long diceRoll(int count, int sides) { // default to 6 sides if not specified
    if (count <= 0 || sides <= 0) {
        return -1; // Invalid parameters, return -1
    }
    long total = 0;
    for (int i = 0; i < count; i++) {
        total += random(1, sides + 1); // Uses the provided 'sides' variable
    }
    return total;
}
/**
 * Overloaded version of diceRoll with default value for simplicity in calls.
 * This is the standard way to handle default parameters in C++.
 */
long diceRoll(int count) {
    return (int)diceRoll(count, 6);
}