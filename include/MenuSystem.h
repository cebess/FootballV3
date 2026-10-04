#pragma once

#include "DisplayBoard.h"
#include <vector>
#include <functional>

/**
 * @brief Represents a menu item with callback functionality
 * 
 * Each menu item can be:
 * - A selectable option with action
 * - A submenu parent
 * - A value changer (increase/decrease)
 */
class MenuItem {
public:
    enum Type {
        ACTION,     // Execute a function when selected
        SUBMENU,    // Navigate to submenu
        VALUE,      // Change a numeric value
        TOGGLE      // Toggle boolean value
    };
    
private:
    String label;
    Type type;
    std::function<void()> action;
    std::vector<MenuItem*> subItems;
    int32_t* valuePtr;
    int32_t minValue;
    int32_t maxValue;
    int32_t step;
    bool* boolPtr;
    bool currentBool;
    int selectedIndex;
    
public:
    /**
     * @brief Constructor for ACTION or SUBMENU type
     * @param label Display text
     * @param type Menu item type
     * @param action Callback function (for ACTION type)
     */
    MenuItem(const String& label, Type type = ACTION, std::function<void()> action = nullptr);
    
    /**
     * @brief Constructor for SUBMENU with subitems
     * @param label Display text
     * @param subItems Vector of child menu items
     */
    MenuItem(const String& label, std::vector<MenuItem*> subItems);
    
    /**
     * @brief Constructor for VALUE type
     * @param label Display text
     * @param valuePtr Pointer to int32_t value
     * @param min Minimum value
     * @param max Maximum value
     * @param step Increment step
     */
    MenuItem(const String& label, int32_t* valuePtr, int32_t min, int32_t max, int32_t step = 1);
    
    /**
     * @brief Constructor for TOGGLE type
     * @param label Display text
     * @param boolPtr Pointer to boolean value
     */
    MenuItem(const String& label, bool* boolPtr);
    
    ~MenuItem();
    
    /**
     * @brief Get display label
     * @return Label string
     */
    String getLabel() const { return label; }
    
    /**
     * @brief Get item type
     * @return Type enum
     */
    Type getType() const { return type; }
    
    /**
     * @brief Select this menu item
     */
    void select();
    
    /**
     * @brief Get submenu items (if SUBMENU type)
     * @return Vector of child MenuItem pointers
     */
    std::vector<MenuItem*>& getSubItems() { return subItems; }
    
    /**
     * @brief Get current value (for VALUE type)
     * @return Current value as string
     */
    String getValueString() const;
    
    /**
     * @brief Increment value (for VALUE type)
     */
    void incrementValue();
    
    /**
     * @brief Decrement value (for VALUE type)
     */
    void decrementValue();
    
    /**
     * @brief Toggle boolean (for TOGGLE type)
     */
    void toggle();
    
    /**
     * @brief Get selected submenu index
     * @return Selected index
     */
    int getSelectedIndex() const { return selectedIndex; }
    
    /**
     * @brief Set selected submenu index
     * @param index New selected index
     */
    void setSelectedIndex(int index) { selectedIndex = index; }
};

/**
 * @brief Hierarchical menu system with display and encoder control
 * 
 * This class manages a menu tree, handles navigation via rotary encoder,
 * and renders the menu on the associated display.
 */
class MenuSystem {
public:
    /**
     * @brief Orientation options for display
     */
    enum Orientation {
        PORTRAIT,   // Vertical orientation (0 degrees)
        LANDSCAPE   // Horizontal orientation (90 degrees, default)
    };
    
private:
    DisplayBoard* board;
    std::vector<MenuItem*> rootMenu;
    std::vector<MenuItem*>* currentMenu;
    std::vector<std::vector<MenuItem*>*> menuStack;
    std::vector<int> selectedStack;
    std::vector<int> topVisibleStack;
    int selectedIndex;
    int topVisible;
    int visibleItems;
    bool _needsRedraw;
    bool editingValue;  // True when in value-edit mode (press-and-rotate)
    bool showingDescription;  // True while a VALUE description is displayed; suppresses redraws
    String title;
    uint8_t fontSize;  // Font size (1-7)
    Orientation orientation;  // Display orientation
    
    /**
     * @brief Render menu on display
     */
    void renderMenu();

    /**
     * @brief Ensure every submenu contains a Back entry as its first item
     * @param menu Menu vector to process recursively
     * @param isRoot True for root menu; false for submenu
     */
    void ensureBackItems(std::vector<MenuItem*>& menu, bool isRoot);
    
    /**
     * @brief Draw a single menu item with custom height
     * @param item MenuItem to draw
     * @param index Item index
     * @param yPos Y position on screen
     * @param selected Whether this item is selected
     * @param height Height of item row
     */
    void drawMenuItem(MenuItem* item, int index, int yPos, bool selected, int height = 20);

    /**
     * @brief Recursively delete a menu tree.
     * @param menu Root vector of items to delete
     */
    void clearMenuItems(std::vector<MenuItem*>& menu);
    
public:
    /**
     * @brief Constructor for MenuSystem
     * @param boardPtr Pointer to DisplayBoard
     * @param menuTitle Title for menu header
     */
    MenuSystem(DisplayBoard* boardPtr, const String& menuTitle = "Menu");
    
    /**
     * @brief Set root menu items
     * @param menu Vector of MenuItem pointers
     */
    void setRootMenu(std::vector<MenuItem*> menu);
    
    /**
     * @brief Initialize the menu system
     */
    void begin();
    
    /**
     * @brief Update menu state (call in loop)
     */
    void update();
    
    /**
     * @brief Navigate to parent menu
     */
    void goToParent();

    /**
     * @brief Reset menu navigation to root-level state
     */
    void resetMenu();
    
    /**
     * @brief Select current menu item
     */
    void selectCurrentItem();
    
    /**
     * @brief Change value of current item (for VALUE type)
     * @param direction +1 for increment, -1 for decrement
     */
    void changeValue(int direction);
    
    /**
     * @brief Navigate menu selection up
     */
    void selectUp();
    
    /**
     * @brief Navigate menu selection down
     */
    void selectDown();
    
    /**
     * @brief Set display orientation (PORTRAIT or LANDSCAPE)
     * @param orient Orientation to set
     */
    void setOrientation(Orientation orient);
    
    /**
     * @brief Set font size for menu text
     * @param size Font size (1-7, where 1 is smallest, default 1)
     */
    void setFontSize(uint8_t size);
    
    /**
     * @brief Get current font size
     * @return Font size (1-7)
     */
    uint8_t getFontSize() const { return fontSize; }
    
    /**
     * @brief Get current orientation
     * @return Current orientation (PORTRAIT or LANDSCAPE)
     */
    Orientation getOrientation() const { return orientation; }
    
    /**
     * @brief Check if currently editing a value item
     * @return true when press-and-rotate value-edit mode is active
     */
    bool isEditingValue() const { return editingValue; }

    /**
     * @brief Get the label of the currently selected item in the active menu view.
     * @return Selected item label or empty string when unavailable
     */
    String getSelectedItemLabel() const;

    /**
     * @brief Suppress or restore automatic redraws (used by VALUE description display)
     */
    void setShowingDescription(bool value) { showingDescription = value; }

    /**
     * @brief Force menu redraw
     */
    void redraw() { _needsRedraw = true; }
    
    /**
     * @brief Check if menu needs redraw
     * @return true if redraw needed
     */
    bool needsRedraw() const { return _needsRedraw; }
};

