#include <Arduino.h>
#include "scoring.h"
#include "diceRoll.h"
#include "StatusOutput.h"
#include "DisplayBoard.h"
#include "MenuSystem.h"
#include <vector>
extern int ballLocation;
extern int displayField(int ballLocation, TeamSide sideWithTheBall, int downMarkerLocation);

namespace {
PointAfterDecisionProvider g_pointAfterDecisionProvider = nullptr;
PointAfterMenuContext g_pointAfterMenuContext;
volatile bool g_pointAfterSelectionDone = false;
PointAfterChoice g_pointAfterChoice = PointAfterChoice::Kick;
}

void configurePointAfterMenuContext(const PointAfterMenuContext& ctx)
{
    g_pointAfterMenuContext = ctx;
}

PointAfterChoice promptPointAfterViaMenu(TeamSide side)
{
    if (!g_pointAfterMenuContext.menuForTeam || !g_pointAfterMenuContext.boardForTeam)
    {
        return PointAfterChoice::Kick;
    }

    MenuSystem* scoringMenu = g_pointAfterMenuContext.menuForTeam(side);
    DisplayBoard* scoringBoard = g_pointAfterMenuContext.boardForTeam(side);

    if (!scoringMenu || !scoringBoard)
    {
        return PointAfterChoice::Kick;
    }

    const bool actionButtonsWereEnabled = g_pointAfterMenuContext.areActionButtonsEnabled
                                              ? g_pointAfterMenuContext.areActionButtonsEnabled()
                                              : false;

    if (!actionButtonsWereEnabled && g_pointAfterMenuContext.enableActionButtons)
    {
        g_pointAfterMenuContext.enableActionButtons();
    }

    if (g_pointAfterMenuContext.resetActionButtonFlags)
    {
        g_pointAfterMenuContext.resetActionButtonFlags();
    }

    g_pointAfterSelectionDone = false;
    g_pointAfterChoice = PointAfterChoice::Kick;

    std::vector<MenuItem*> pointAfterMenu;
    pointAfterMenu.reserve(2);
    pointAfterMenu.push_back(new MenuItem("Kick Point After", MenuItem::ACTION, []()
                                          {
                                              g_pointAfterChoice = PointAfterChoice::Kick;
                                              g_pointAfterSelectionDone = true;
                                          }));
    pointAfterMenu.push_back(new MenuItem("Two-Point Conversion", MenuItem::ACTION, []()
                                          {
                                              g_pointAfterChoice = PointAfterChoice::TwoPoint;
                                              g_pointAfterSelectionDone = true;
                                          }));

    scoringMenu->setShowingDescription(false);
    scoringMenu->setRootMenu(pointAfterMenu);
    scoringMenu->begin();
    scoringMenu->redraw();
    scoringMenu->update();
    statusPrintLine(String((side == HOME) ? "HOME" : "VISITOR") + " scored!");
    // set the ball location to the 3 yard line for the team 'side' that scored, to indicate where the point after attempt will be made
    ballLocation= ((side == HOME) ? 97 : 3);
    int downMarkerLocation = ((side == HOME) ? 100 : 0); // set the down marker to the end zone for the team that scored
    // light up the LED for the team with the ball to indicate they are ready for the point after attempt
    displayField(ballLocation, side, downMarkerLocation);                                      

    Serial.printf("Select point after touchdown option on %s menu.\n",
                  teamSideToString(side).c_str());
    eventPrintLine("Btn1=Kick Btn2=2PT");

    const uint32_t startMs = millis();
    const uint32_t timeoutMs = (g_pointAfterMenuContext.timeoutMs > 0)
                                   ? g_pointAfterMenuContext.timeoutMs
                                   : 30000;
    int32_t lastShownRemainingSeconds = -1;
    while (!g_pointAfterSelectionDone)
    {
        if (g_pointAfterMenuContext.pumpUiWhileWaiting)
        {
            g_pointAfterMenuContext.pumpUiWhileWaiting();
        }

        if (g_pointAfterMenuContext.consumeActionButton1 && g_pointAfterMenuContext.consumeActionButton1())
        {
            g_pointAfterChoice = PointAfterChoice::Kick;
            g_pointAfterSelectionDone = true;
            statusPrintLine("Point after Kick");
            break;
        }

        if (g_pointAfterMenuContext.consumeActionButton2 && g_pointAfterMenuContext.consumeActionButton2())
        {
            g_pointAfterChoice = PointAfterChoice::TwoPoint;
            g_pointAfterSelectionDone = true;
            statusPrintLine("2PT conversion");
            break;
        }

        const uint32_t elapsedMs = millis() - startMs;
        const uint32_t remainingMs = (elapsedMs < timeoutMs) ? (timeoutMs - elapsedMs) : 0;
        const int32_t remainingSeconds = static_cast<int32_t>((remainingMs + 999U) / 1000U);
        if (remainingSeconds != lastShownRemainingSeconds)
        {
            lastShownRemainingSeconds = remainingSeconds;
            eventPrintLine("Time remaining: " + String(remainingSeconds));
        }

        if (remainingMs<=0)
        {
            statusPrintLine("Defaulting to Kick");
            g_pointAfterChoice = PointAfterChoice::Kick;
            break;
        }

        delay(10);
    }

    if (g_pointAfterMenuContext.restoreMenusAfterSelection)
    {
        g_pointAfterMenuContext.restoreMenusAfterSelection();
    }

    if (g_pointAfterMenuContext.resetActionButtonFlags)
    {
        g_pointAfterMenuContext.resetActionButtonFlags();
    }

    if (actionButtonsWereEnabled && g_pointAfterMenuContext.enableActionButtons)
    {
        g_pointAfterMenuContext.enableActionButtons();
    }
    else if (!actionButtonsWereEnabled && g_pointAfterMenuContext.disableActionButtons)
    {
        g_pointAfterMenuContext.disableActionButtons();
    }

    return g_pointAfterChoice;
}

void registerPointAfterDecisionProvider(PointAfterDecisionProvider provider)
{
    g_pointAfterDecisionProvider = provider;
}

// handles the point after attempt after a touchdown
int pointAfter(TeamSide side)
{
    //Serial.println("Pre question about Point after attempt for " + teamSideToString(side));
    if (g_pointAfterDecisionProvider)
    {
        PointAfterChoice choice = g_pointAfterDecisionProvider(side);
        if (choice == PointAfterChoice::TwoPoint)
        {
            return tryTwoPointConversion(side);
        }
        return kickPointAfter(side);
    } else
    {
        statusPrintLine("No point after decision provider registered. Defaulting to Kick.");
        return kickPointAfter(side);
    }
}

int kickPointAfter(TeamSide side)
{
    int offenseRandomTemp = diceRoll(2);
    int defenseRandomTemp = diceRoll(1);
    int randomTemp = offenseRandomTemp - defenseRandomTemp;
    if (randomTemp > 2)
    {
        eventPrintLine("Point after is good!");
        delay(2000); // Wait for 2 seconds to allow the user to see the successful point after message
        return 1; // Extra point is good
    }
    else
    {
        eventPrintLine("Point after failed!");
        delay(2000); // Wait for 2 seconds to allow the user to see the successful point after message
        return 0; // Extra point failed
    }
}

int tryTwoPointConversion(TeamSide side)
{
    int offenseRandomTemp = diceRoll(2);
    int defenseRandomTemp = diceRoll(1);
    int randomTemp = offenseRandomTemp - defenseRandomTemp;
    if (randomTemp > 6)
    {
        eventPrintLine("Conversion is good!");
        delay(2000); // Wait for 2 seconds to allow the user to see the successful point after message
        return 2; // Extra point is good
    }
    else
    {
        eventPrintLine("Conversion failed!");
        delay(2000); // Wait for 2 seconds to allow the user to see the successful point after message
        return 0; // Extra point failed
    }
}