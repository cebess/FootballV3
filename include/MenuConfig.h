#ifndef MENU_CONFIG_H
#define MENU_CONFIG_H

#include <LovyanGFX.hpp>
#include <vector>
#include "MenuSystem.h"

/**
 * @brief Runtime handles used to materialize compile-time menu descriptors.
 *
 * The descriptor tables in MenuConfig.cpp are intentionally static and contain
 * only IDs and constants. This context provides the concrete display/menu
 * instances plus writable value/toggle bindings that the builder resolves.
 */
struct MenuBuildContext {
    // Display targets used by action callbacks.
    lgfx::LGFX_Device* display1;
    lgfx::LGFX_Device* display2;

    // Menu instances that receive redraw/select/value operations.
    MenuSystem* offensiveMenu;
    MenuSystem* defensiveMenu;
};

/**
 * @brief Build and attach both menu trees from compile-time descriptor tables.
 *
 * This function keeps main.cpp focused on high-level initialization by moving
 * menu structure definitions and callback wiring into MenuConfig.cpp.
 */
void configureMenus(const MenuBuildContext& ctx);

/**
 * @brief Append all offensive action play names (Run/Pass/Kick submenu items).
 *
 * This exposes action labels defined in MenuConfig.cpp so other modules can
 * validate selections without duplicating the play name lists.
 */
void appendActionPlayNames(std::vector<String>& outNames);

#endif