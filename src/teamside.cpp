#include <Arduino.h>
#include "teamside.h"
// function to convert TeamSide enum to string for printing
// This function takes a TeamSide enum value and returns a human-readable string representation of it.
// side: The TeamSide enum value to convert (HOME or VISITOR).
// returns: A String representing the team side ("Home", "Visitor", or "Unknown"
String teamSideToString(TeamSide side) {
    switch (side) {
        case HOME:    return "Home";
        case VISITOR: return "Visitor";
        default:      return "Unknown";
    }
}

TeamSide switchTeamSide(TeamSide side) {
    if (side == HOME) {
        return VISITOR;
    } else {
        return HOME;
    }
}