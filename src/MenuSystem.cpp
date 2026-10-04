#include "MenuSystem.h"
#include <LovyanGFX.hpp>

/**
 * @file MenuSystem.cpp
 * @brief Implements menu item models and menu rendering/navigation logic.
 *
 * High-level behavior present in this file:
 * - Defines MenuItem construction and value/action operations.
 * - Defines MenuSystem state setup, rendering, and navigation operations.
 * - Performs drawing through DisplayBoard -> LGFX display APIs.
 * - Uses defensive null/empty checks in most public operations.
 *
 * Notes based on current code:
 * - Back items are auto-inserted for non-root submenus via ensureBackItems().
 * - update() currently does not apply encoder direction changes; redraw occurs when flagged.
 * - Ownership of dynamically created Back items is not deleted in this file.
 */

// MenuItem Implementation
/**
 * Function: MenuItem::MenuItem(const String& label, Type type, std::function<void()> action)
 * Purpose: Initialize a menu item configured as ACTION/SUBMENU-like base constructor with optional callback.
 * Parameters:
 * - const String& label: Display text for the item.
 * - Type type: Menu item type enum value.
 * - std::function<void()> action: Callback used when select() is called for ACTION type.
 * Returns: None (constructor).
 * Usage example: MenuItem* item = new MenuItem("Run", MenuItem::ACTION, []() { });
 * Warnings/constraints:
 * - valuePtr and boolPtr are initialized to nullptr.
 * - Callback is only executed by select() when type is ACTION.
 */
MenuItem::MenuItem(const String& label, Type type, std::function<void()> action)
    : label(label), type(type), action(action), valuePtr(nullptr), 
      minValue(0), maxValue(100), step(1), boolPtr(nullptr), 
      currentBool(false), selectedIndex(0) {}

/**
 * Function: MenuItem::MenuItem(const String& label, std::vector<MenuItem*> subItems)
 * Purpose: Initialize a SUBMENU menu item and copy provided child item pointers.
 * Parameters:
 * - const String& label: Display text for the submenu item.
 * - std::vector<MenuItem*> subItems: Child menu items for this submenu.
 * Returns: None (constructor).
 * Usage example: MenuItem* submenu = new MenuItem("Settings", children);
 * Warnings/constraints:
 * - This constructor forces type to SUBMENU.
 * - Pointer ownership/lifetime is not managed by this constructor.
 */
MenuItem::MenuItem(const String& label, std::vector<MenuItem*> subItems)
    : label(label), type(SUBMENU), action(nullptr), valuePtr(nullptr),
      minValue(0), maxValue(100), step(1), boolPtr(nullptr),
      currentBool(false), selectedIndex(0) {
    this->subItems = subItems;
}

/**
 * Function: MenuItem::MenuItem(const String& label, int32_t* valuePtr, int32_t min, int32_t max, int32_t step)
 * Purpose: Initialize a VALUE menu item bound to an external int32_t value with limits and step.
 * Parameters:
 * - const String& label: Display text for the value item.
 * - int32_t* valuePtr: Pointer to external numeric value storage.
 * - int32_t min: Lower clamp bound used by decrementValue().
 * - int32_t max: Upper clamp bound used by incrementValue().
 * - int32_t step: Increment/decrement amount.
 * Returns: None (constructor).
 * Usage example: MenuItem* vol = new MenuItem("Volume", &volume, 0, 100, 5);
 * Warnings/constraints:
 * - If valuePtr is nullptr, incrementValue()/decrementValue() do nothing.
 */
MenuItem::MenuItem(const String& label, int32_t* valuePtr, int32_t min, int32_t max, int32_t step)
    : label(label), type(VALUE), action(nullptr), valuePtr(valuePtr),
      minValue(min), maxValue(max), step(step), boolPtr(nullptr),
      currentBool(false), selectedIndex(0) {}

/**
 * Function: MenuItem::MenuItem(const String& label, bool* boolPtr)
 * Purpose: Initialize a TOGGLE menu item bound to an external boolean value.
 * Parameters:
 * - const String& label: Display text for the toggle item.
 * - bool* boolPtr: Pointer to external boolean storage.
 * Returns: None (constructor).
 * Usage example: MenuItem* power = new MenuItem("Power", &enabled);
 * Warnings/constraints:
 * - currentBool is initialized from boolPtr only when boolPtr is non-null.
 */
MenuItem::MenuItem(const String& label, bool* boolPtr)
    : label(label), type(TOGGLE), action(nullptr), valuePtr(nullptr),
      minValue(0), maxValue(1), step(1), boolPtr(boolPtr),
      currentBool(false), selectedIndex(0) {
    if (boolPtr) currentBool = *boolPtr;
}

/**
 * Function: MenuItem::~MenuItem()
 * Purpose: Destructor for MenuItem.
 * Parameters:
 * - None.
 * Returns: None (destructor).
 * Usage example: delete item;
 * Warnings/constraints:
 * - Does not delete subItems; code comment states they are managed by MenuSystem.
 */
MenuItem::~MenuItem() {
    // Don't delete subItems here - they're managed by MenuSystem
}

/**
 * Function: MenuItem::select()
 * Purpose: Execute the stored callback for ACTION items.
 * Parameters:
 * - None.
 * Returns: void.
 * Usage example: item->select();
 * Warnings/constraints:
 * - Callback runs only when both action is set and type == ACTION.
 */
void MenuItem::select() {
    if (action && type == ACTION) {
        action();
    }
}

/**
 * Function: MenuItem::getValueString() const
 * Purpose: Return text representation for VALUE/TOGGLE items.
 * Parameters:
 * - None.
 * Returns: String representing current value/state, or empty string for other types.
 * Usage example: String s = item->getValueString();
 * Warnings/constraints:
 * - TOGGLE returns "---" when boolPtr is null.
 */
String MenuItem::getValueString() const {
    if (type == VALUE && valuePtr) {
        return String(*valuePtr);
    } else if (type == TOGGLE) {
        return boolPtr ? (*boolPtr ? "ON" : "OFF") : "---";
    }
    return "";
}

/**
 * Function: MenuItem::incrementValue()
 * Purpose: Increase VALUE item by step and clamp at maxValue.
 * Parameters:
 * - None.
 * Returns: void.
 * Usage example: item->incrementValue();
 * Warnings/constraints:
 * - Operates only when type == VALUE and valuePtr is non-null.
 */
void MenuItem::incrementValue() {
    if (type == VALUE && valuePtr) {
        *valuePtr += step;
        if (*valuePtr > maxValue) *valuePtr = maxValue;
    }
}

/**
 * Function: MenuItem::decrementValue()
 * Purpose: Decrease VALUE item by step and clamp at minValue.
 * Parameters:
 * - None.
 * Returns: void.
 * Usage example: item->decrementValue();
 * Warnings/constraints:
 * - Operates only when type == VALUE and valuePtr is non-null.
 */
void MenuItem::decrementValue() {
    if (type == VALUE && valuePtr) {
        *valuePtr -= step;
        if (*valuePtr < minValue) *valuePtr = minValue;
    }
}

/**
 * Function: MenuItem::toggle()
 * Purpose: Toggle a bound boolean value for TOGGLE items.
 * Parameters:
 * - None.
 * Returns: void.
 * Usage example: item->toggle();
 * Warnings/constraints:
 * - Operates only when type == TOGGLE and boolPtr is non-null.
 */
void MenuItem::toggle() {
    if (type == TOGGLE && boolPtr) {
        *boolPtr = !*boolPtr;
        currentBool = *boolPtr;
    }
}

// MenuSystem Implementation
/**
 * Function: MenuSystem::MenuSystem(DisplayBoard* boardPtr, const String& menuTitle)
 * Purpose: Initialize menu system state, defaults, and landscape display rotation when display is available.
 * Parameters:
 * - DisplayBoard* boardPtr: Board wrapper that provides display and encoder.
 * - const String& menuTitle: Title rendered by renderMenu().
 * Returns: None (constructor).
 * Usage example: MenuSystem menu(&board, "Main Menu");
 * Warnings/constraints:
 * - If board/display is null, rotation is not set.
 * - Default orientation is LANDSCAPE and font size is 1.
 */
MenuSystem::MenuSystem(DisplayBoard* boardPtr, const String& menuTitle)
    : board(boardPtr), currentMenu(nullptr), selectedIndex(0), 
      topVisible(0), visibleItems(8), _needsRedraw(true), editingValue(false), showingDescription(false),
      title(menuTitle), fontSize(1), orientation(MenuSystem::LANDSCAPE) {
    // Set display rotation for landscape by default
    if (board && board->getDisplay()) {
        board->getDisplay()->setRotation(1);  // 1 = landscape
    }
}

/**
 * Function: MenuSystem::ensureBackItems(std::vector<MenuItem*>& menu, bool isRoot)
 * Purpose: Recursively ensure non-root menus start with a "Back" ACTION item.
 * Parameters:
 * - std::vector<MenuItem*>& menu: Menu list to inspect and mutate.
 * - bool isRoot: True for root menu; false for submenu.
 * Returns: void.
 * Usage example: ensureBackItems(rootMenu, true);
 * Warnings/constraints:
 * - Inserts a new dynamically allocated MenuItem("Back", ACTION) when missing.
 * - Recurses into SUBMENU items only when their subItems are non-empty.
 */
void MenuSystem::ensureBackItems(std::vector<MenuItem*>& menu, bool isRoot) {
    if (!isRoot) {
        if (menu.empty() || menu[0]->getLabel() != "Back") {
            menu.insert(menu.begin(), new MenuItem("Back", MenuItem::ACTION));
        }
    }

    for (MenuItem* item : menu) {
        if (!item || item->getType() != MenuItem::SUBMENU) {
            continue;
        }
        auto& subItems = item->getSubItems();
        if (!subItems.empty()) {
            ensureBackItems(subItems, false);
        }
    }
}

/**
 * Function: MenuSystem::clearMenuItems(std::vector<MenuItem*>& menu)
 * Purpose: Recursively delete a menu tree and clear the vector.
 * Parameters:
 * - std::vector<MenuItem*>& menu: Root vector to delete.
 * Returns: void.
 * Usage example: clearMenuItems(rootMenu);
 * Warnings/constraints:
 * - Assumes items are uniquely owned by this MenuSystem tree.
 */
void MenuSystem::clearMenuItems(std::vector<MenuItem*>& menu) {
    for (MenuItem* item : menu) {
        if (!item) {
            continue;
        }

        if (item->getType() == MenuItem::SUBMENU) {
            auto& children = item->getSubItems();
            if (!children.empty()) {
                clearMenuItems(children);
            }
        }

        delete item;
    }

    menu.clear();
}

/**
 * Function: MenuSystem::setRootMenu(std::vector<MenuItem*> menu)
 * Purpose: Replace root menu, normalize Back entries, and reset navigation stacks/state.
 * Parameters:
 * - std::vector<MenuItem*> menu: New root menu list (copied into rootMenu).
 * Returns: void.
 * Usage example: menuSystem.setRootMenu(items);
 * Warnings/constraints:
 * - Marks redraw needed.
 * - Does not free previously stored menu item pointers.
 */
void MenuSystem::setRootMenu(std::vector<MenuItem*> menu) {
    if (!rootMenu.empty()) {
        clearMenuItems(rootMenu);
    }

    rootMenu = menu;
    ensureBackItems(rootMenu, true);
    resetMenu();
}

/**
 * Function: MenuSystem::begin()
 * Purpose: Initialize display drawing state and render the menu immediately.
 * Parameters:
 * - None.
 * Returns: void.
 * Usage example: menuSystem.begin();
 * Warnings/constraints:
 * - Does nothing when board or display is null.
 */
void MenuSystem::begin() {
    if (board && board->getDisplay()) {
        auto display = board->getDisplay();
        display->fillScreen(TFT_BLACK);
        display->setTextColor(TFT_WHITE, TFT_BLACK);
        display->setTextSize(fontSize);
        renderMenu();
    }
}

/**
 * Function: MenuSystem::update()
 * Purpose: Poll board/encoder pointers and render when redraw flag is set.
 * Parameters:
 * - None.
 * Returns: void.
 * Usage example: menuSystem.update();
 * Warnings/constraints:
 * - Returns early if board or encoder is null.
 * - Current direction calculation uses encoder->getPosition() - encoder->getPosition(), which always evaluates to 0.
 */
void MenuSystem::update() {
    if (!board) return;
    
    if (_needsRedraw && !showingDescription) {
        renderMenu();
        _needsRedraw = false;
    }
}

/**
 * Function: MenuSystem::renderMenu()
 * Purpose: Draw menu title, visible items, and scroll indicators onto the display.
 * Parameters:
 * - None.
 * Returns: void.
 * Usage example: renderMenu();
 * Warnings/constraints:
 * - Requires non-null display and currentMenu.
 * - Mutates selectedIndex/topVisible/visibleItems to maintain valid viewport state.
 */
void MenuSystem::renderMenu() {
    auto display = board->getDisplay();
    if (!display || !currentMenu) return;
    
    display->fillScreen(TFT_BLACK);
    display->setTextSize(fontSize);
    
    // Draw title
    display->setTextColor(TFT_CYAN, TFT_BLACK);
    display->setCursor(10, 5);
    display->print(title);
    
    // Draw separator line
    int separatorY = 8 * fontSize + 10;
    display->drawLine(0, separatorY, display->width(), separatorY, TFT_DARKGREY);
    
    // Calculate item height and spacing based on font size
    int itemHeight = 8 * fontSize + 4;  // Approximate height per item
    int titleHeight = separatorY;
    int maxVisible = (display->height() - titleHeight - 10) / itemHeight;
    if (maxVisible < 1) {
        maxVisible = 1;
    }
    visibleItems = maxVisible;
    int menuSize = static_cast<int>(currentMenu->size());
    if (menuSize <= 0) {
        return;
    }

    if (selectedIndex < 0) {
        selectedIndex = 0;
    } else if (selectedIndex >= menuSize) {
        selectedIndex = menuSize - 1;
    }
    
    // Adjust topVisible if needed
    if (selectedIndex < topVisible) {
        topVisible = selectedIndex;
    } else if (selectedIndex >= topVisible + maxVisible) {
        topVisible = selectedIndex - maxVisible + 1;
    }
    
    // Draw menu items
    int startY = titleHeight + 5;
    for (int i = topVisible; i < min(topVisible + maxVisible, menuSize); i++) {
        MenuItem* item = (*currentMenu)[i];
        bool selected = (i == selectedIndex);
        int yPos = startY + (i - topVisible) * itemHeight;
        drawMenuItem(item, i, yPos, selected, itemHeight);
    }
    
    // Draw scroll indicators
    if (topVisible > 0) {
        display->setCursor(display->width() - 15, startY);
        display->setTextColor(TFT_DARKGREY);
        display->print("^");
    }
    if (topVisible + maxVisible < menuSize) {
        display->setCursor(display->width() - 15, display->height() - 15);
        display->setTextColor(TFT_DARKGREY);
        display->print("v");
    }
}

/**
 * Function: MenuSystem::drawMenuItem(MenuItem* item, int index, int yPos, bool selected, int height)
 * Purpose: Draw one row of menu text, highlight state, submenu marker, and value text.
 * Parameters:
 * - MenuItem* item: Item to draw.
 * - int index: Row index parameter (currently unused in function body).
 * - int yPos: Vertical text position for this row.
 * - bool selected: Whether to draw selected background/highlight colors.
 * - int height: Row height used for selection background rectangle.
 * Returns: void.
 * Usage example: drawMenuItem(item, i, y, i == selectedIndex, itemHeight);
 * Warnings/constraints:
 * - Requires non-null display.
 * - Assumes item is non-null; function does not null-check item before dereference.
 * - Value text color is orange when the item is selected and editingValue is true,
 *   yellow when selected but not editing, green when unselected.
 */
void MenuSystem::drawMenuItem(MenuItem* item, int index, int yPos, bool selected, int height) {
    auto display = board->getDisplay();
    if (!display) return;
    
    // Background
    if (selected) {
        display->fillRect(0, yPos - 2, display->width(), height, TFT_NAVY);
    }
    
    // Item label
    uint16_t color = selected ? TFT_WHITE : TFT_LIGHTGREY;
    display->setTextColor(color);
    display->setCursor(10, yPos);
    
    String text = item->getLabel();
    
    // Add submenu indicator
    if (item->getType() == MenuItem::SUBMENU) {
        text += " >";
    }
    
    // Add value display
    if (item->getType() == MenuItem::VALUE || item->getType() == MenuItem::TOGGLE) {
        String valueStr = item->getValueString();
        display->setCursor(display->width() - 60, yPos);
        // Highlight value in orange when this item is being edited
        uint16_t valueColor = (selected && editingValue) ? TFT_ORANGE
                            : (selected ? TFT_YELLOW : TFT_GREEN);
        display->setTextColor(valueColor);
        display->print(valueStr);
        display->setCursor(10, yPos);
        display->setTextColor(color);
    }
    
    display->print(text);
}

/**
 * Function: MenuSystem::goToParent()
 * Purpose: Navigate to previous menu context from stacks, or reset to root when no parent exists.
 * Parameters:
 * - None.
 * Returns: void.
 * Usage example: menuSystem.goToParent();
 * Warnings/constraints:
 * - Assumes selectedStack/topVisibleStack contain entries when menuStack is non-empty.
 * - Clears editingValue before navigating so value-edit mode does not persist across levels.
 * - Marks redraw needed.
 */
void MenuSystem::goToParent() {
    editingValue = false;
    if (!menuStack.empty()) {
        currentMenu = menuStack.back();
        menuStack.pop_back();

        selectedIndex = selectedStack.back();
        selectedStack.pop_back();

        topVisible = topVisibleStack.back();
        topVisibleStack.pop_back();
    } else {
        currentMenu = &rootMenu;
        selectedIndex = 0;
        topVisible = 0;
    }
    _needsRedraw = true;
}

/**
 * Function: MenuSystem::resetMenu()
 * Purpose: Return to root menu and clear navigation/editing state.
 * Parameters:
 * - None.
 * Returns: void.
 * Usage example: menuSystem.resetMenu();
 * Warnings/constraints:
 * - Does not modify rootMenu contents.
 * - Marks redraw needed.
 */
void MenuSystem::resetMenu() {
    currentMenu = &rootMenu;
    menuStack.clear();
    selectedStack.clear();
    topVisibleStack.clear();
    selectedIndex = 0;
    topVisible = 0;
    editingValue = false;
    showingDescription = false;
    _needsRedraw = true;
}

/**
 * Function: MenuSystem::selectCurrentItem()
 * Purpose: Execute action, enter submenu, toggle value, or handle Back for the currently selected item.
 * Parameters:
 * - None.
 * Returns: void.
 * Usage example: menuSystem.selectCurrentItem();
 * Warnings/constraints:
 * - Returns early on null/empty menu or out-of-range selectedIndex.
 * - "Back" is recognized by item label text equality to "Back".
 * - VALUE case toggles editingValue; while true, encoder rotation calls changeValue() instead of navigation.
 * - SUBMENU case clears editingValue before descending.
 */
void MenuSystem::selectCurrentItem() {
    if (!currentMenu || currentMenu->empty()) return;
    if (selectedIndex >= static_cast<int>(currentMenu->size())) return;
    
    MenuItem* item = (*currentMenu)[selectedIndex];

    if (item && item->getLabel() == "Back") {
        goToParent();
        return;
    }
    
    switch (item->getType()) {
        case MenuItem::ACTION:
            item->select();
            if (!showingDescription) {
                _needsRedraw = true;
            }
            break;
            
        case MenuItem::SUBMENU: {
            auto& subItems = item->getSubItems();
            if (!subItems.empty()) {
                editingValue = false;
                menuStack.push_back(currentMenu);
                selectedStack.push_back(selectedIndex);
                topVisibleStack.push_back(topVisible);
                currentMenu = &subItems;
                selectedIndex = 0;
                topVisible = 0;
                _needsRedraw = true;
            }
            break;
        }
        
        case MenuItem::TOGGLE:
            item->toggle();
            _needsRedraw = true;
            break;
            
        case MenuItem::VALUE:
            // Toggle value-edit mode on each press
            editingValue = !editingValue;
            _needsRedraw = true;
            break;
    }
}

/**
 * Function: MenuSystem::changeValue(int direction)
 * Purpose: Increment or decrement currently selected VALUE item based on direction sign.
 * Parameters:
 * - int direction: Positive for increment, otherwise decrement.
 * Returns: void.
 * Usage example: menuSystem.changeValue(1);
 * Warnings/constraints:
 * - Returns early on null/empty menu or out-of-range selectedIndex.
 * - Non-VALUE items are ignored.
 */
void MenuSystem::changeValue(int direction) {
    if (!currentMenu || currentMenu->empty()) return;
    if (selectedIndex >= static_cast<int>(currentMenu->size())) return;
    
    MenuItem* item = (*currentMenu)[selectedIndex];
    
    if (item->getType() == MenuItem::VALUE) {
        if (direction > 0) {
            item->incrementValue();
        } else {
            item->decrementValue();
        }
        _needsRedraw = true;
    }
}

/**
 * Function: MenuSystem::selectUp()
 * Purpose: Move selection one item up and adjust topVisible when needed.
 * Parameters:
 * - None.
 * Returns: void.
 * Usage example: menuSystem.selectUp();
 * Warnings/constraints:
 * - Does nothing if menu is null/empty or selectedIndex is already at first item.
 */
void MenuSystem::selectUp() {
    if (!currentMenu || currentMenu->empty()) return;
    
    if (selectedIndex > 0) {
        selectedIndex--;
        _needsRedraw = true;
        
        // Scroll up if needed
        if (selectedIndex < topVisible) {
            topVisible = selectedIndex;
        }
    }
}

/**
 * Function: MenuSystem::selectDown()
 * Purpose: Move selection one item down and adjust topVisible for scrolling.
 * Parameters:
 * - None.
 * Returns: void.
 * Usage example: menuSystem.selectDown();
 * Warnings/constraints:
 * - Does nothing if menu is null/empty or selectedIndex is already at last item.
 * - Uses visibleItems; falls back to 1 when visibleItems <= 0.
 */
void MenuSystem::selectDown() {
    if (!currentMenu || currentMenu->empty()) return;
    
    if (selectedIndex < static_cast<int>(currentMenu->size()) - 1) {
        selectedIndex++;
        _needsRedraw = true;
        
        // Scroll down if needed
        int maxVisible = visibleItems > 0 ? visibleItems : 1;
        if (selectedIndex >= topVisible + maxVisible) {
            topVisible = selectedIndex - maxVisible + 1;
        }
    }
}

/**
 * Function: MenuSystem::setOrientation(Orientation orient)
 * Purpose: Store orientation and apply corresponding display rotation.
 * Parameters:
 * - Orientation orient: Desired orientation enum (PORTRAIT or LANDSCAPE).
 * Returns: void.
 * Usage example: menuSystem.setOrientation(MenuSystem::LANDSCAPE);
 * Warnings/constraints:
 * - Applies rotation only if board and display pointers are valid.
 * - Marks redraw needed.
 */
void MenuSystem::setOrientation(Orientation orient) {
    orientation = orient;
    if (board && board->getDisplay()) {
        // 0 = portrait, 1 = landscape
        board->getDisplay()->setRotation(orient == LANDSCAPE ? 1 : 0);
    }
    _needsRedraw = true;
}

/**
 * Function: MenuSystem::setFontSize(uint8_t size)
 * Purpose: Clamp and store font size in range [1, 7].
 * Parameters:
 * - uint8_t size: Requested text size.
 * Returns: void.
 * Usage example: menuSystem.setFontSize(2);
 * Warnings/constraints:
 * - Values below 1 are set to 1; values above 7 are set to 7.
 * - Marks redraw needed.
 */
void MenuSystem::setFontSize(uint8_t size) {
    // Clamp size between 1 and 7
    fontSize = (size < 1 ? 1 : (size > 7 ? 7 : size));
    _needsRedraw = true;
}

String MenuSystem::getSelectedItemLabel() const {
    if (!currentMenu || currentMenu->empty()) {
        return "";
    }

    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(currentMenu->size())) {
        return "";
    }

    MenuItem* item = (*currentMenu)[selectedIndex];
    if (!item) {
        return "";
    }

    return item->getLabel();
}
