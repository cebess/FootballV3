#include "MenuConfig.h"
#include <cstddef>
#include <memory>

namespace {

// Node type in a descriptor row. The active fields in MenuNodeDef depend on kind.
enum class NodeKind {
    SUBMENU,
    VALUE,
};

/**
 * @brief Compile-time descriptor for one menu entry.
 *
 * Field usage by kind:
 * - SUBMENU: label, description, kind, children, childCount
 * - VALUE: label, description, kind (VALUE), children=nullptr, childCount=0
 * Unused fields for a given kind are ignored by the tree builder.
 */
struct MenuNodeDef {
    const char* label;
    String description;
    NodeKind kind;
    const MenuNodeDef* children;
    size_t childCount;
};

// Board 1 submenu descriptor table.
const MenuNodeDef SUBMENU_PASS[] = {
    {"Over the Middle", "A read-based passing play where a middle receiver crosses between linebackers. Its strength lies in its ability to create confusion for interior defenders who must react to a crossing runner; however, it requires precise timing and a clear path from the offensive side.", NodeKind::VALUE, nullptr, 0},
    {"Halfback option", "This is a hybrid run/pass play where the back can choose to tuck the ball if the intended pass isn't open. The strength of this play is keeping the defense guessing about whether it will be a handoff or a pass; its weakness is the high level of split-second decision-making required from the running back.", NodeKind::VALUE, nullptr, 0},
    {"Four Verticals", "A high-risk, high-reward play where four receivers sprint deep to stretch the secondary to its limit. Its strength is creating maximum space and potential for large gains, while its weakness is a higher risk of interception if the pass is not perfectly executed.", NodeKind::VALUE, nullptr, 0},
    {"Smash", "This is a high-low concept that uses a short hitch and a deeper corner route to stress a single defender's coverage. The strength lies in creating conflict for the defender who must choose between two targets; its weakness can be a slower reaction time if the pass isn't thrown quickly.", NodeKind::VALUE, nullptr, 0},
    {"Mesh", "This play features two receivers crossing in the middle at shallow depth to rub off defenders, making it difficult for them to maintain coverage. Its strength is creating physical obstacles for defenders; its weakness is that it can be effectively countered by a disciplined zone defense.", NodeKind::VALUE, nullptr, 0},
    {"Slant-Flat", "Combining an outside slant with an inside flat route, this play specifically targets and attacks a zone defense. The strength of this play is forcing the defender to choose between two options at different angles, while its weakness lies in the requirement for perfect synchronization from both receivers.", NodeKind::VALUE, nullptr, 0},
    {"Sail", "This pass flood involves three levels—deep, intermediate, and short—on one side of the field. Its strength is overwhelming a specific area with multiple targets; however, its complexity makes it more difficult to execute flawlessly compared to simpler routes.", NodeKind::VALUE, nullptr, 0},
    {"Stick", "A quick-hit play where a receiver runs upfield then stops abruptly as a target for the quarterback. The strength is catching linebackers before they can react to the stop; its weakness is that it requires an extremely sharp and well-timed cut from the runner.", NodeKind::VALUE, nullptr, 0},
    {"Flea Flicker", "This deceptive play involves tossing a handoff back to the QB, who then throws a long pass behind a defense expecting a run. Its strength is catching the defense off guard with a sudden switch; its weakness is the time required for the ball transfer.", NodeKind::VALUE, nullptr, 0},
    {"Hail Mary", "A desperate, long-distance pass into the end zone during the final moments of a game. The strength is its high payoff as a last resort when no other options exist, while its weakness is a low completion rate and high risk of turnover.", NodeKind::VALUE, nullptr, 0},
    {"Hook and Lateral", "A receiver catches a hook and immediately pitches it to a trailing teammate.", NodeKind::VALUE, nullptr, 0},

};

const MenuNodeDef SUBMENU_RUN[] = {
    {"Power", "This is a physically intense play where a guard pulls across the line to lead block through a gap in the defense. Its strength is its effectiveness in short-yardage situations; however, it can be stopped if the defenders anticipate the guard's movement.", NodeKind::VALUE, nullptr, 0},
    {"Counter", "A misdirection play where the runner cuts back behind a pulling guard to catch the defense off balance. The strength lies in creating confusion at the point of attack, while its weakness is that any lack of timing from the offensive line exposes the ball carrier.", NodeKind::VALUE, nullptr, 0},
    {"Trap", "An interior run where an offensive lineman intentionally leaves a defender unblocked for a teammate to hit them from the side. Its strength is the massive gain created by isolating and hitting defenders on their blind side; its weakness is the high level of coordination required between blockers.", NodeKind::VALUE, nullptr, 0},
    {"Isolation", "This is a straightforward running play where a fullback blocks one linebacker 1-on-1, allowing the ball carrier to break through. The strength is simplicity and ease of execution; however, it can be easily stopped if the defense successfully manages the lead blocker.", NodeKind::VALUE, nullptr, 0},
    {"Inside Zone", "In this maneuver, the offensive line moves laterally as a single unit, which allows the running back to find an internal seam in the defensive front. Its strength is its reliability and consistency across many plays; however, it can be countered by an aggressive interior defense.", NodeKind::VALUE, nullptr, 0},
    {"Outside Zone", "The offensive line flows toward the sideline as a collective unit to open up the perimeter for the runner. Its strength is the ability to stretch the defense horizontally across the field; its weakness is its vulnerability to defenders who can successfully contain the ball.", NodeKind::VALUE, nullptr, 0},
    {"Pitch", "A wide play where the quarterback tosses the ball to a running back sprinting toward the sideline of the field. The strength is utilizing the runner's speed to cross the line of scrimmage quickly; however, since the ball is in motion during the handoff, it is more susceptible to being stripped.", NodeKind::VALUE, nullptr, 0},
    {"Sweep", "A perimeter play where the offensive line creates a moving wall to protect a runner heading toward the outside. The strength of this play is the protection provided by the linemen as they form a corridor; its weakness is that it requires a high level of engagement from the defense's edge players.", NodeKind::VALUE, nullptr, 0},
    {"Read Option", "This dynamic play involves the QB reading an edge defender: if they crash inside, he runs outside; if not, he hands off. Its strength lies in forcing the defense into a difficult choice; its weakness is the extreme demand on the quarterback's awareness and speed.", NodeKind::VALUE, nullptr, 0},
    {"Jet Sweep", "A high-speed play where a receiver in motion takes a pitch from the QB to strike the sideline. The strength is the momentum gained by the runner, which often confuses defenders; however, it requires perfect coordination between the QB and the moving receiver.", NodeKind::VALUE, nullptr, 0},
    {"Short-Yardage Wedge", "A dense formation used for very short gains by pushing through the defense.", NodeKind::VALUE, nullptr, 0},
};

const MenuNodeDef SUBMENU_KICK[] = {
    {"Punt", "Used to move the ball away from the offensive team's goal line when scoring isn't likely.", NodeKind::VALUE, nullptr, 0},
    {"Field goal", "Attempt a kick through the uprights for 3 points, used when the offense is near the goal line.", NodeKind::VALUE, nullptr, 0},
};

// Board 1 root descriptor table.
const MenuNodeDef OFFENSE_MENU[] = {
    {"Run", "", NodeKind::SUBMENU, SUBMENU_RUN, sizeof(SUBMENU_RUN) / sizeof(SUBMENU_RUN[0])},
    {"Pass", "", NodeKind::SUBMENU, SUBMENU_PASS, sizeof(SUBMENU_PASS) / sizeof(SUBMENU_PASS[0])},
    {"Kick", "", NodeKind::SUBMENU, SUBMENU_KICK, sizeof(SUBMENU_KICK) / sizeof(SUBMENU_KICK[0])},
};

// Board 2 root descriptor table.
const MenuNodeDef DEFENSE_MENU[] = {
    {"Man-to-man", "This defensive strategy involves defenders guarding a specific opponent. It is considered highly effective against th run. Its primary strength is its efficacy against ground offenses, while its weakness can be a vulnerability to well-timed cross-routes or double-move plays.", NodeKind::VALUE, nullptr, 0},
    {"Zone", "Defenders are assigned to guard a specific area of the field rather than a single player. This method is highly effective at clogging passing lanes. The strength of zone coverage is that it provides a solid wall against many types of passes; its weakness is that it can leave gaps.", NodeKind::VALUE, nullptr, 0},
    {"Blitz", "A blitz involves sending an extra number of defenders to pressure the quarterback, and it is primarily used during pass defense. Its strength is the immediate pressure it puts on the offensive play, forcing a quicker decision or turnover; its weakness is that fewer defenders are left back in coverage.", NodeKind::VALUE, nullptr, 0},
    {"Prevent", "This strategy uses a deep defensive drop designed specifically to stop long passes. While its strength is stopping home run plays for the offense, its primary weakness is that it is often less effective against running offenses due to fewer bodies near the line of scrimmage.", NodeKind::VALUE, nullptr, 0},
    {"Goal line", "This play involves crowding the line of scrimmage with a heavy number of defenders to stop short gains in running plays. Its strength is the sheer physical mass used to block runners; its weakness is that it leaves very few players available to defend against deep pass attempts.", NodeKind::VALUE, nullptr, 0},
    {"Nickel", "The nickel defense adds a fifth defensive back specifically to counter passing-heavy offenses, though this often comes at the expense of run defense. The strength lies in superior coverage for passing lanes, while the primary weakness is a reduced ability to stop ground gains.", NodeKind::VALUE, nullptr, 0},
    {"Dime", "This scheme involves adding a sixth defensive back to provide extreme pass coverage. While it offers very high protection against long passes, its weakness is that it leaves the run game highly vulnerable.", NodeKind::VALUE, nullptr, 0},
    };

// Recursively convert descriptor rows into heap-allocated MenuItem trees.
// display and menuSys identify which board this subtree belongs to, so VALUE
// description callbacks draw on the correct screen and redraw the correct menu.

// Draw text wrapped on word boundaries within [x, x+maxWidth).
void drawWrappedText(lgfx::LGFX_Device* display, const String& text, int x, int y, int maxWidth) {
    int cursorX = x;
    int cursorY = y;
    int lineHeight = display->fontHeight() + 2;
    int len = text.length();
    int start = 0;

    while (start < len) {
        // Find end of next word.
        int end = start;
        while (end < len && text[end] != ' ') ++end;

        String word = text.substring(start, end);
        int wordW = display->textWidth(word.c_str());

        if (cursorX == x) {
            // First word on line — always place it even if it overflows.
            display->setCursor(cursorX, cursorY);
            display->print(word);
            cursorX += wordW;
        } else if (cursorX + display->textWidth(" ") + wordW <= x + maxWidth) {
            // Word fits on current line.
            display->print(' ');
            display->print(word);
            cursorX += display->textWidth(" ") + wordW;
        } else {
            // Start a new line.
            cursorX = x;
            cursorY += lineHeight;
            display->setCursor(cursorX, cursorY);
            display->print(word);
            cursorX += wordW;
        }

        // Skip trailing spaces.
        while (end < len && text[end] == ' ') ++end;
        start = end;
    }
}

std::vector<MenuItem*> buildMenuTree(const MenuNodeDef* defs, size_t count, const MenuBuildContext& ctx,
                                      lgfx::LGFX_Device* display, MenuSystem* menuSys) {
    std::vector<MenuItem*> items;
    items.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        const MenuNodeDef& def = defs[i];
        switch (def.kind) {
            case NodeKind::SUBMENU: {
                auto children = buildMenuTree(def.children, def.childCount, ctx, display, menuSys);
                items.push_back(new MenuItem(def.label, children));
                break;
            }
            case NodeKind::VALUE: {
                // First press: fill the screen with the description text.
                // Second press: dismiss and redraw the menu.
                auto shown = std::make_shared<bool>(false);
                String desc = def.description;
                items.push_back(new MenuItem(def.label, MenuItem::ACTION,
                    [display, menuSys, desc, shown]() mutable {
                        if (!*shown) {
                            *shown = true;
                            menuSys->setShowingDescription(true);
                            display->fillScreen(TFT_BLACK);
                            display->setCursor(10, 10);
                            display->setTextColor(TFT_WHITE, TFT_BLACK);
                            display->setTextWrap(false);
                            drawWrappedText(display, desc, 10, 10, display->width() - 20);
                        } else {
                            *shown = false;
                            menuSys->setShowingDescription(false);
                            menuSys->redraw();
                        }
                    }));
                break;
            }
        }
    }

    return items;
}

}  // namespace

// Build both board menus and attach them to their MenuSystem instances.
void configureMenus(const MenuBuildContext& ctx) {
    auto offenseRoot = buildMenuTree(OFFENSE_MENU, sizeof(OFFENSE_MENU) / sizeof(OFFENSE_MENU[0]), ctx, ctx.display1, ctx.offensiveMenu);
    auto defenseRoot = buildMenuTree(DEFENSE_MENU, sizeof(DEFENSE_MENU) / sizeof(DEFENSE_MENU[0]), ctx, ctx.display2, ctx.defensiveMenu);

    ctx.offensiveMenu->setRootMenu(offenseRoot);
    ctx.defensiveMenu->setRootMenu(defenseRoot);
}

// this will append all the action play names to the provided vector, which can be used for selection in the game logic
void appendActionPlayNames(std::vector<String>& outNames) {
    const MenuNodeDef* const actionMenus[] = {SUBMENU_PASS, SUBMENU_RUN, SUBMENU_KICK};
    const size_t actionMenuCounts[] = {
        sizeof(SUBMENU_PASS) / sizeof(SUBMENU_PASS[0]),
        sizeof(SUBMENU_RUN) / sizeof(SUBMENU_RUN[0]),
        sizeof(SUBMENU_KICK) / sizeof(SUBMENU_KICK[0])
    };

    for (size_t menuIndex = 0; menuIndex < (sizeof(actionMenus) / sizeof(actionMenus[0])); ++menuIndex) {
        const MenuNodeDef* defs = actionMenus[menuIndex];
        const size_t count = actionMenuCounts[menuIndex];
        for (size_t i = 0; i < count; ++i) {
            outNames.push_back(defs[i].label);
        }
    }
}