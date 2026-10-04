#pragma once
#include <Arduino.h>
#include <FastLED.h>
#include "teamside.h"
#include "scoring.h"
int ballMovement(int startLocation, TeamSide side, int distance);
void animateBallMovement(int startLocation, int endLocation, TeamSide offenseSide);
int teamBallLocation(TeamSide side, int location);
int firstAndTen(int ballLocation, TeamSide teamWithTheBall);
TeamSide playExecution(TeamSide teamWithTheBall, const String& offense_play, const String& defense_play);
CRGB& ballOwnerColor(TeamSide teamWithTheBall);

