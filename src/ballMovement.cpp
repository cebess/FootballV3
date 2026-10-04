#include <Arduino.h>
#include "ballMovement.h"
#include "teamside.h"
#include "diceRoll.h"
#include "StatusOutput.h"
#include <myFastLED.h>

extern int ballLocation;
extern int downMarkerLocation;
extern int downCounter;
extern int points; // number of points associated with the last scoring play
extern MyFastLED* myStrip;
extern size_t getPlayDataRowCount();
extern String getPlayDataValue(size_t rowIndex, const String& columnName);
extern void updateCenteredLCDDisplay(const String& line1, const String& line2);

// ballOwnerColor function returns the color of the team that currently has possession of the ball
CRGB &ballOwnerColor(TeamSide teamWithTheBall)
{
    static CRGB color = CRGB::Blue;
    if (teamWithTheBall == HOME)
    {
        color = CRGB::Blue; // Home team color
    }
    else
    {
        color = CRGB::Red; // Visitor team color
    }
    return color;
}

int ballMovement(int startLocation, TeamSide offenseSide, int distance) {
    int newLocation = startLocation;
    // Serial.printf("Ball movement: start=%d offensive side=%s distance to move=%d\n",
    //               startLocation,
    //               teamSideToString(offenseSide).c_str(),
    //               distance);
    // home team + moves to the right, visitor team + moves to the left
    if (offenseSide == HOME) {
        newLocation += distance; // Move right for home team
    } else {
        newLocation -= distance; // Move left for visitor team
    }
    // Ensure the ball location stays within field bounds (0 to 100)
    if (newLocation < 0) {
        newLocation = 0;
    } else if (newLocation > 100) {
        newLocation = 100;
    }
    if (abs(newLocation - startLocation) > 9) { // Only animate if the ball moved more than 9 yards
        animateBallMovement(startLocation, newLocation, offenseSide);
    }
    return newLocation;
}

void animateBallMovement(int startLocation, int endLocation, TeamSide offenseSide) {
    // Animate the ball movement from startLocation to endLocation
    int step = (endLocation > startLocation) ? 1 : -1; // Determine the direction of movement
    myStrip->logicalLEDsequence(startLocation, endLocation, 50, ballOwnerColor(offenseSide), false);
}

int teamBallLocation(TeamSide side,int location) {
    // This function can be used to determine the current location of the ball based on which team has it
    // the 20 yard line for the home team is at location 20, and the 20 yard line for the visitor team is at location 80
    if (location < 0) {
        location = 0;
    } else if (location > 100) {
        location = 100;
    }
    if (side == HOME) {
        return location; // field location for home team
    } else {
        return 100-location; // field location for visitor team
    }
}

// this function returns the new down marker location after a new set of downs is established.
int firstAndTen(int ballLocation, TeamSide teamWithTheBall)
{
    // This function will determine the new down marker location after a new set of downs is established. It will be 10 yards ahead of the current ball location, but cannot exceed the opponent's end zone at 100 yards.
    int downLocation = 0; // initialized the down location variable that will be returned at the end of the function
    if (ballLocation < 0 || ballLocation > 100)
    {
        Serial.println("Invalid ball location");
        return ballLocation; // Return current location if invalid
    }
    else
    {
        // for the home team the downlocation is defined +10 yards, for the visitor team the ball is defined -10 yards, since they are moving in opposite directions on the field
        if (teamWithTheBall == HOME)
        {
            downLocation = ballLocation + 10; // Move the ball 10 yards ahead for the home team
            if (downLocation > 100)
            {
                downLocation = 100; // Cap the ball location at the opponent's end zone if it exceeds 100 yards
            }
        }
        else
        {
            downLocation = ballLocation - 10; // Move the ball 10 yards ahead for the visitor team (which is -10 yards in the opposite direction)
            if (downLocation < 0)
            {
                downLocation = 0; // Cap the ball location at the opponent's end zone if it goes below 0 yards
            }
        }
        downCounter = 1; // Reset the down counter to 1 for the new set of downs
        return downLocation;
    }
}

TeamSide playExecution(TeamSide teamWithTheBall, const String& offense_play, const String& defense_play) {
    auto parseIntOrDefault = [](const String& value, int fallback) -> int {
        if (value.length() == 0) {
            return fallback;
        }
        return static_cast<int>(value.toInt());
    };

    String defenseColumn = defense_play;
    //Serial.printf("playExecution: offense='%s' defense='%s'\n", offense_play.c_str(), defense_play.c_str());
   
    int matchedRow = -1;
    for (size_t i = 0; i < getPlayDataRowCount(); ++i) {
        //Serial.printf("playExecution: checking row %zu, CSV offense='%s'\n", i, getPlayDataValue(i, "Name").c_str());
        if (getPlayDataValue(i, "Name") == offense_play) {
            matchedRow = static_cast<int>(i);
            break;
        }
    }

    if (matchedRow < 0) {
        Serial.printf("playExecution: no CSV row found for offense play '%s'\n", offense_play.c_str());
        return teamWithTheBall; // No change in possession if play not found
    }

    const size_t row = static_cast<size_t>(matchedRow);
    const int success = parseIntOrDefault(getPlayDataValue(row, "Success"), 0);
    const int impact = parseIntOrDefault(getPlayDataValue(row, "Impact"), 0);
    const int defenseRating = parseIntOrDefault(getPlayDataValue(row, defenseColumn), 0);

    int yardsGained = 0;
    const int successRoll = static_cast<int>(esp_random() % 11); // [0..10]
    bool isSuccess = (successRoll <= success);
    eventPrintLine(offense_play);
    Serial.printf("playExecution: offense='%s' defense='%s' success=%d impact=%d def=%d successRoll=%d isSuccess=%s\n",
                  offense_play.c_str(),
                  defense_play.c_str(),
                  success,
                  impact,
                  defenseRating,
                  successRoll,
                  isSuccess ? "true" : "false");    
    if (!isSuccess) {
        // Failed play: mostly no/loss gain.    
        const int failRoll = static_cast<int>(esp_random() % 11); // [0..10]
        if (failRoll < 4) {
            yardsGained = 0;
            statusPrintLine("No gain");
        } else if (failRoll >= 10) {
        // big failure turnover
            if (offense_play == "Punt") {
                eventPrintLine("Punt blocked!");
            } else if (offense_play == "Field goal") {
                eventPrintLine("Field goal blocked!");
            } else {
                eventPrintLine("Failed play!");
                statusPrintLine("Turnover");
            }
            teamWithTheBall = switchTeamSide(teamWithTheBall);
            downMarkerLocation = firstAndTen(ballLocation, teamWithTheBall); // Reset the down marker for the new offense
            delay(2000); // Wait for 2 seconds to allow the user to see the turnover message
            return teamWithTheBall; // Turnover on downs, return the new team with the ball
        }
        else {
            yardsGained = -static_cast<int>(esp_random() % 6); // [0..-5]
            statusPrintLine("Yards lost " + String(-yardsGained));
        }
    } else {
        // Successful play: impact governs upside, defense can shave result.
        yardsGained = diceRoll(impact);
        const int defenseRoll = static_cast<int>(esp_random() % 11); // [0..10]
        if (defenseRoll <= defenseRating) {
            yardsGained -= diceRoll(1);
        }
        if (yardsGained>0) {
            statusPrintLine("Yards gained " + String(yardsGained));
        }
        else if (yardsGained<0) {
            statusPrintLine("Yards lost " + String(-yardsGained));
        } else {
            statusPrintLine("No gain");
        }
    }
    Serial.printf("ballLocation=%d, yardsGained=%d team=%s\n", ballLocation, yardsGained, teamSideToString(teamWithTheBall).c_str());
    const int newBallLocation = ballMovement(ballLocation, teamWithTheBall, yardsGained);
    ballLocation = newBallLocation;
    if (offense_play == "Punt") {
        teamWithTheBall = switchTeamSide(teamWithTheBall);
        if (ballLocation < 0 || ballLocation > 100) {
            Serial.println("Looks like the punted into the end zone, so the receiving team will take possession at their 25 yard line.");
            ballLocation = teamBallLocation(teamWithTheBall, 25); // Reset the ball location to the receiving team's 20 yard line
            statusPrintLine("Punt into end zone");
        }        
        // Looks like the punt took place so turn over the ball
        downMarkerLocation = firstAndTen(ballLocation, teamWithTheBall); // Reset the down marker for the new offense
        Serial.printf(" After punt ballLocation=%d, team=%s\n", ballLocation, teamSideToString(teamWithTheBall).c_str());
        return (teamWithTheBall);
    }
    if (offense_play == "Field goal") {
        int distance_to_goal = (teamWithTheBall == HOME) ? (100 - ballLocation) : ballLocation;
        Serial.printf("Field goal attempt from %d yards to go.\n", distance_to_goal);
        long initial_roll = diceRoll(2); // Roll a 20-sided die for the field goal attempt
        if (initial_roll <3) {
            statusPrintLine("Field Goal missed.");
            delay(2000); // Wait for 2 seconds to allow the user to see the missed field goal message
            teamWithTheBall = switchTeamSide(teamWithTheBall);
            downMarkerLocation = firstAndTen(ballLocation, teamWithTheBall); // Reset the down marker for the new offense
            return (teamWithTheBall);
        }
        initial_roll = diceRoll(4); // Roll four 6-sided dice for the field goal attempt
        int success = distance_to_goal - static_cast<int>(initial_roll) -25;
        if (success <= 0) {
            statusPrintLine("Field goal success!");
            myStrip->triggerEventByName("field goal");
            //delay(2000); // Wait for 2 seconds to allow the user to see the successful field goal message
            // Field goal is good, award points and change possession
            points = 3;

        } else {
            statusPrintLine("Field goal miss");
            delay(2000); // Wait for 2 seconds to allow the user to see the missed field goal message
            teamWithTheBall = switchTeamSide(teamWithTheBall);
            downMarkerLocation = firstAndTen(ballLocation, teamWithTheBall); // Reset the down marker for the new offense
        }
        // check for safety, when the ball at the goal line of the team with the ball. When there is a safety and the other team gets 2 points and possession of the ball
        if ((teamWithTheBall == HOME && ballLocation <= 0) || (teamWithTheBall == VISITOR && ballLocation >= 100)) {
            statusPrintLine("Safety!");
            points = 2;
            teamWithTheBall = switchTeamSide(teamWithTheBall);
            // there should now be a kickoff back to the team that just scored the safety, so reset the ball location to the 25 yard line of the team that just scored the safety
            ballLocation = teamBallLocation(teamWithTheBall, 25);
        }
        return (teamWithTheBall);
    }

    // now see if we made a first down or not
    const bool reachedMarker = (teamWithTheBall == HOME)
                                 ? (newBallLocation >= downMarkerLocation)
                                 : (newBallLocation <= downMarkerLocation);
    if (reachedMarker) {
        Serial.println("down marker reached!");
        // check to see if we scored a touchdown
        if ((teamWithTheBall == HOME && newBallLocation >= 100) ||
            (teamWithTheBall == VISITOR && newBallLocation <= 0)) {
            updateCenteredLCDDisplay(teamSideToString(teamWithTheBall), "Touchdown!");
            Serial.println("Touchdown scored!");
            eventPrintLine("Point after option?");
            myStrip->triggerEventByName("touchdown");
            points = 6 + pointAfter(teamWithTheBall);
            //delay(2000); // Wait for 2 seconds to allow the user to see the touchdown message
            return teamWithTheBall; // Return the team that scored
        }
        downCounter = 1;
        downMarkerLocation = firstAndTen(ballLocation, teamWithTheBall);
    } else {
        downCounter += 1;
    }

    return teamWithTheBall;
}