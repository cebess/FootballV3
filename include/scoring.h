#pragma once

#include "teamside.h"

class DisplayBoard;
class MenuSystem;

enum class PointAfterChoice {
	Kick,
	TwoPoint
};

using PointAfterDecisionProvider = PointAfterChoice (*)(TeamSide side);

struct PointAfterMenuContext {
	MenuSystem* (*menuForTeam)(TeamSide side) = nullptr;
	DisplayBoard* (*boardForTeam)(TeamSide side) = nullptr;
	bool (*areActionButtonsEnabled)() = nullptr;
	bool (*consumeActionButton1)() = nullptr;
	bool (*consumeActionButton2)() = nullptr;
	void (*disableActionButtons)() = nullptr;
	void (*enableActionButtons)() = nullptr;
	void (*resetActionButtonFlags)() = nullptr;
	void (*pumpUiWhileWaiting)() = nullptr;
	void (*restoreMenusAfterSelection)() = nullptr;
	uint32_t timeoutMs = 30000;
};

/**
 * @brief Configure runtime hooks for the menu-driven point-after chooser.
 *
 * This allows scoring.cpp to own PAT menu flow while main.cpp provides
 * hardware/menu integration callbacks.
 */
void configurePointAfterMenuContext(const PointAfterMenuContext& ctx);

/**
 * @brief PAT decision provider backed by on-screen menu selection.
 */
PointAfterChoice promptPointAfterViaMenu(TeamSide side);

/**
 * @brief Register a runtime provider for PAT decision input.
 *
 * When set, pointAfter() calls this provider to choose Kick or Two-Point.
 * If not set, pointAfter() falls back to Serial input behavior.
 *
 * @param provider Callback invoked during pointAfter().
 */
void registerPointAfterDecisionProvider(PointAfterDecisionProvider provider);

/**
 * @brief Resolve a post-touchdown attempt.
 *
 * Prompts for kick or two-point conversion and executes the selected attempt.
 * If input is missing or invalid, it defaults to a kick attempt.
 *
 * @param side Team attempting the conversion.
 * @return Points awarded for the attempt.
 */
int pointAfter(TeamSide side);

/**
 * @brief Attempt a kicked point-after.
 *
 * @param side Team attempting the kick.
 * @return 1 if good, otherwise 0.
 */
int kickPointAfter(TeamSide side);

/**
 * @brief Attempt a two-point conversion.
 *
 * @param side Team attempting the conversion.
 * @return 1 if good, otherwise 0.
 */
int tryTwoPointConversion(TeamSide side);
