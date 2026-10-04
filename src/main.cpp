#include <Arduino.h>
#include <LittleFS.h>
#include <esp_system.h>
#include <Wire.h>
#include <LovyanGFX.hpp>
#include <FastLED.h>
#include <LiquidCrystal_I2C.h>
#include <myFastLED.h>
#include <CountDownTimer.h>
#include "DisplayBoard.h"
#include "MenuSystem.h"
#include "MenuConfig.h"
#include "sequenceLoader.h"
#include "teamside.h"
#include "diceRoll.h"
#include "scoring.h"
#include "ballMovement.h"
#include "myCountDownTimer.h"
#include "StatusOutput.h"
#include "SoundBoard.h"

#define LED_DATA_PIN 16
CRGB* leds = nullptr;
int numLeds = 101;
std::vector<LEDMap> myLayout;
std::vector<int> bgLeds;
const CRGB bgCols[] = {CRGB::Black};
SequenceStep idleAnim[] = { {2000, nullptr, 0, bgCols, 1} };
// ball related variables
int ballLocation = -1; // Current ball location on the field (normally 0-100 yards)
int downMarkerLocation = 0; // this is the spot on the field where the down marker is
int downCounter=1; // this is the current down, initialized to 1 at the start of a new set of downs
// scoring variables
int points = 0; // non zero if there was a score on the play
int homeScore = 0;
int visitorScore = 0;
// game time variables
myCountDownTimer gameTimer; // game timer
myCountDownTimer playSelectionTimer; // 20s timer for second play selection
// button definition for the play action button
constexpr uint8_t Action_button_1_PIN = 19;
constexpr uint8_t Action_button_2_PIN = 27;
constexpr uint32_t ACTION_DEBOUNCE_MS = 50;
constexpr uint32_t LCD_UPDATE_INTERVAL_MS = 1000;
constexpr int DELAY_OF_GAME_YARDS = 5;

// I2C LCD pin definitions (Display3 in wiring.txt)
constexpr uint8_t LCD_I2C_SDA_PIN = 21;
constexpr uint8_t LCD_I2C_SCL_PIN = 22;
constexpr uint8_t LCD_I2C_ADDRESS = 0x27;
constexpr uint8_t LCD_COLUMNS = 20;
constexpr uint8_t LCD_ROWS = 4;
LiquidCrystal_I2C* lcdDisplay = nullptr;
uint8_t lcdDetectedAddress = 0;
//
String offense_play;
String defense_play;
bool offense_and_defense_selected = false;
bool actionButtonsEnabled = false;
std::vector<String> ActionPlayNames;
std::vector<String> playDataHeaders;
std::vector<std::vector<String>> playDataRows;
bool playSelectionCountdownActive = false;
bool waitingOnOffenseSelection = false;

constexpr unsigned long PLAY_SELECTION_TIMEOUT_MS = 20000;

extern TeamSide teamWithTheBall;

volatile bool actionButton1Pressed = false;
volatile bool actionButton2Pressed = false;
uint32_t lastAction1HandledMs = 0;
uint32_t lastAction2HandledMs = 0;
uint32_t lastLcdUpdateMs = 0;

static inline bool isMenu1Offense() {
  return (teamWithTheBall == HOME);
}

static inline void IRAM_ATTR handleActionButtonRelease(volatile bool* pressedFlag, bool isDefenseMenu) {
  if (!actionButtonsEnabled) {
    return;
  }
  *pressedFlag = true;
}

void IRAM_ATTR onActionButton1Release() {
  handleActionButtonRelease(&actionButton1Pressed, !isMenu1Offense());
}

void IRAM_ATTR onActionButton2Release() {
  handleActionButtonRelease(&actionButton2Pressed, isMenu1Offense());
}

void disableActionButtons() {
  actionButtonsEnabled = false;
  detachInterrupt(digitalPinToInterrupt(Action_button_1_PIN));
  detachInterrupt(digitalPinToInterrupt(Action_button_2_PIN));

  noInterrupts();
  actionButton1Pressed = false;
  actionButton2Pressed = false;
  interrupts();
}

void enableActionButtons() {
  actionButtonsEnabled = true;
  attachInterrupt(digitalPinToInterrupt(Action_button_1_PIN), onActionButton1Release, RISING);
  attachInterrupt(digitalPinToInterrupt(Action_button_2_PIN), onActionButton2Release, RISING);
}

// Build action play names from MenuConfig's canonical menu descriptors.
void buildActionPlayNames() {
  ActionPlayNames.clear();
  appendActionPlayNames(ActionPlayNames);
}

static String trimCopy(const String& input) {
  String out = input;
  out.trim();
  return out;
}

static std::vector<String> parseCSVLine(const String& line) {
  std::vector<String> fields;
  String current;
  bool inQuotes = false;

  for (int i = 0; i < line.length(); ++i) {
    char c = line[i];

    if (c == '"') {
      if (inQuotes && (i + 1) < line.length() && line[i + 1] == '"') {
        current += '"';
        ++i;
      } else {
        inQuotes = !inQuotes;
      }
    } else if (c == ',' && !inQuotes) {
      fields.push_back(trimCopy(current));
      current = "";
    } else {
      current += c;
    }
  }

  fields.push_back(trimCopy(current));
  return fields;
}

int getPlayDataColumnIndex(const String& columnName) {
  for (size_t i = 0; i < playDataHeaders.size(); ++i) {
    if (playDataHeaders[i] == columnName) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

String getPlayDataValue(size_t rowIndex, size_t columnIndex) {
  if (rowIndex >= playDataRows.size()) {
    return "";
  }
  if (columnIndex >= playDataRows[rowIndex].size()) {
    return "";
  }
  return playDataRows[rowIndex][columnIndex];
}

String getPlayDataValue(size_t rowIndex, const String& columnName) {
  int columnIndex = getPlayDataColumnIndex(columnName);
  if (columnIndex < 0) {
    return "";
  }
  return getPlayDataValue(rowIndex, static_cast<size_t>(columnIndex));
}

size_t getPlayDataRowCount() {
  return playDataRows.size();
}

bool loadPlayDataFromCSV(const char* filepath) {
  File file = LittleFS.open(filepath, "r");
  if (!file) {
    Serial.print("Play data file not found: ");
    Serial.println(filepath);
    return false;
  }

  playDataHeaders.clear();
  playDataRows.clear();

  bool headerLoaded = false;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) {
      continue;
    }

    std::vector<String> fields = parseCSVLine(line);
    if (!headerLoaded) {
      playDataHeaders = fields;
      headerLoaded = true;
      continue;
    }

    if (fields.size() < playDataHeaders.size()) {
      fields.resize(playDataHeaders.size());
    }
    playDataRows.push_back(fields);
  }

  file.close();

  if (playDataHeaders.empty()) {
    Serial.println("playdata.csv header row was not found.");
    return false;
  }

  Serial.printf("Loaded /playdata.csv with %u columns and %u rows.\n",
                static_cast<unsigned>(playDataHeaders.size()),
                static_cast<unsigned>(playDataRows.size()));
  return true;
}

static bool isI2CDevicePresent(uint8_t address) {
  Wire.beginTransmission(address);
  return (Wire.endTransmission() == 0);
}

void scanI2CBusForDebug() {
  Serial.println("Scanning I2C bus...");
  bool foundAny = false;
  for (uint8_t addr = 1; addr < 127; ++addr) {
    if (isI2CDevicePresent(addr)) {
      foundAny = true;
      Serial.printf("I2C device found at 0x%02X\n", addr);
    }
  }

  if (!foundAny) {
    Serial.println("No I2C devices detected.");
  }
}

bool LCDinit() {
  Wire.begin(LCD_I2C_SDA_PIN, LCD_I2C_SCL_PIN);
  Wire.setClock(100000);
  delay(50);

  scanI2CBusForDebug();

  // Common backpack ranges:
  // PCF8574:  0x20-0x27
  // PCF8574A: 0x38-0x3F
  static const uint8_t lcdCandidateAddresses[] = {
      0x27, 0x26, 0x25, 0x24, 0x23, 0x22, 0x21, 0x20,
      0x3F, 0x3E, 0x3D, 0x3C, 0x3B, 0x3A, 0x39, 0x38
  };

  lcdDetectedAddress = 0;
  for (uint8_t addr : lcdCandidateAddresses) {
    if (isI2CDevicePresent(addr)) {
      lcdDetectedAddress = addr;
      break;
    }
  }

  if (lcdDetectedAddress == 0) {
    lcdDisplay = nullptr;
    Serial.println("LCD not detected on common backpack addresses (0x20-0x27, 0x38-0x3F).");
    return false;
  }

  lcdDisplay = new LiquidCrystal_I2C(lcdDetectedAddress, LCD_COLUMNS, LCD_ROWS);
  if (!lcdDisplay) {
    Serial.println("Failed to allocate LCD object.");
    return false;
  }

  Serial.printf("Using LCD at 0x%02X\n", lcdDetectedAddress);
  lcdDisplay->init();
  lcdDisplay->backlight();
  lcdDisplay->clear();
  return true;
}

void lcdPrintCenteredLine(uint8_t row, const String& text) {

  int startCol = (LCD_COLUMNS - static_cast<int>(text.length())) / 2;
  std::string myText = "";

  // pad the start with spaces if necessary
  for (int i = 0; i < startCol; ++i) {
    myText += ' ';
  }
  myText.append(text.c_str());

  lcdPrintLine(row, myText.c_str());
}

void statusPrintLine(const String& text) {
  lcdPrintLine(2, text);
}

void eventPrintLine(const String& text) {
  lcdPrintLine(3, text);
}

void lcdPrintLine(uint8_t row, const String& text) {
  if (!lcdDisplay) {
    return;
  }
  lcdDisplay->setCursor(0, row);
  std::string paddedText = text.c_str();
  if (paddedText.length() < LCD_COLUMNS) {
    paddedText.append(LCD_COLUMNS - paddedText.length(), ' ');
  }
  lcdDisplay->print(paddedText.c_str());
} 

void updateLCDDisplay(const String& line1, const String& line2) {

  lcdPrintLine(0, line1);
  lcdPrintLine(1, line2);
}

void updateCenteredLCDDisplay(const String& line1, const String& line2) {

  lcdPrintCenteredLine(0, line1);
  lcdPrintCenteredLine(1, line2);
}

bool loadLayoutFromCSV(const char* filepath, std::vector<LEDMap>& layoutOut, int& numLedsOut) {
    File file = LittleFS.open(filepath, "r");
    if (!file) {
        Serial.print("Layout file not found: ");
        Serial.println(filepath);
        return false;
    }

    layoutOut.clear();
    numLedsOut = 0;

    char line[128];
    int lineNum = 0;

    while (file.available()) {
        size_t n = file.readBytesUntil('\n', line, sizeof(line) - 1);
        line[n] = '\0';

        if (n > 0 && line[n - 1] == '\r') {
            line[n - 1] = '\0';
        }

        lineNum++;
        if (line[0] == '\0') {
            continue;
        }

        char* first = strtok(line, ",");
        char* second = strtok(nullptr, ",");

        // Skip header or invalid lines
        if (!first || !second) {
            continue;
        }
        if (!isdigit((unsigned char)first[0]) && first[0] != '-') {
            continue;
        }
        if (!isdigit((unsigned char)second[0]) && second[0] != '-') {
            continue;
        }

        int logical = atoi(first);
        int physical = atoi(second);

        if (logical < 0 || physical < 0) {
            Serial.print("Skipping invalid layout row ");
            Serial.println(lineNum);
            continue;
        }

        layoutOut.push_back({logical, physical});
        if (physical + 1 > numLedsOut) {
            numLedsOut = physical + 1;
        }
    }

    file.close();

    if (layoutOut.empty() || numLedsOut <= 0) {
        Serial.println("No valid layout rows found in myLEDLayout.csv");
        return false;
    }

    return true;
}

// Parameterized ST7789 display configuration class
class LGFX_Display : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel;
  lgfx::Bus_SPI _bus;

public:
  LGFX_Display(int pinDc, int pinCs, int pinRst) {
    {
      auto cfg = _bus.config();

      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read  = 16000000;
      cfg.spi_3wire  = false;
      cfg.use_lock   = true;
      cfg.dma_channel = 1;

      cfg.pin_sclk = 18;
      cfg.pin_mosi = 23;
      cfg.pin_miso = -1;
      cfg.pin_dc = pinDc;

      _bus.config(cfg);
    }

    {
      auto cfg = _panel.config();

      cfg.pin_cs  = pinCs;
      cfg.pin_rst = pinRst;
      cfg.pin_busy = -1;

      cfg.memory_width  = 240;
      cfg.memory_height = 320;
      cfg.panel_width   = 240;
      cfg.panel_height  = 320;
      cfg.offset_x = 0;
      cfg.offset_y = 0;

      _panel.config(cfg);
    }

    _panel.setBus(&_bus);
    setPanel(&_panel);
  }
};

// Global objects
LGFX_Display display1(2, 5, 4);
LGFX_Display display2(15, 14, 12);

// Encoder objects
constexpr uint8_t Display1_Encoder_A = 13;
constexpr uint8_t Display1_Encoder_B = 17;
constexpr uint8_t Display1_Encoder_Push = 26;
constexpr uint8_t Display2_Encoder_A = 32;
constexpr uint8_t Display2_Encoder_B = 33;
constexpr uint8_t Display2_Encoder_Push = 25;

RotaryEncoder encoder1(Display1_Encoder_A, Display1_Encoder_B, Display1_Encoder_Push);  // A, B, PUSH for display 1
RotaryEncoder encoder2(Display2_Encoder_A, Display2_Encoder_B, Display2_Encoder_Push);  // A, B, PUSH for display 2

// Display board objects
DisplayBoard board1(&display1, &encoder1, -1, "Home");
DisplayBoard board2(&display2, &encoder2, -1, "Visitor");

// Menu systems
MenuSystem menu1(&board1, "HOME");
MenuSystem menu2(&board2, "VISITOR");

void applyPossessionMenus(TeamSide offenseTeam);

namespace
{
  DisplayBoard *boardForTeam(TeamSide side)
  {
    return (side == HOME) ? &board1 : &board2;
  }

  MenuSystem *menuForTeam(TeamSide side)
  {
    return (side == HOME) ? &menu1 : &menu2;
  }

  bool areActionButtonsEnabled()
  {
    return actionButtonsEnabled;
  }

  bool consumeActionButton1ForPointAfter()
  {
    bool consumed = false;

    noInterrupts();
    if (actionButtonsEnabled && actionButton1Pressed)
    {
      actionButton1Pressed = false;
      consumed = true;
    }
    interrupts();

    return consumed;
  }

  bool consumeActionButton2ForPointAfter()
  {
    bool consumed = false;

    noInterrupts();
    if (actionButtonsEnabled && actionButton2Pressed)
    {
      actionButton2Pressed = false;
      consumed = true;
    }
    interrupts();

    return consumed;
  }

  void resetActionButtonFlags()
  {
    noInterrupts();
    actionButton1Pressed = false;
    actionButton2Pressed = false;
    interrupts();
  }

  void pumpUiWhileWaitingForPointAfterSelection()
  {
    // Keep both boards and menus responsive while waiting for PAT selection.
    board1.update();
    board2.update();
    board1.processEncoderInput();
    board2.processEncoderInput();
    menu1.update();
    menu2.update();
  }

  void restorePossessionMenusAfterPointAfterSelection()
  {
    applyPossessionMenus(teamWithTheBall);
  }
}
// Create instances
MyFastLED* myStrip = nullptr;
SequenceLoader seqLoader;
TeamSide teamWithTheBall = HOME;

void applyPossessionMenus(TeamSide offenseTeam) {
  MenuBuildContext menuContext;

  if (offenseTeam == HOME) {
    menuContext = {&display1, &display2, &menu1, &menu2};
  } else {
    menuContext = {&display2, &display1, &menu2, &menu1};
  }

  configureMenus(menuContext);

  // Force redraw in case a description page was open when roles changed.
  menu1.setShowingDescription(false);
  menu2.setShowingDescription(false);
  menu1.redraw();
  menu2.redraw();

  Serial.printf("Possession set: %s offense on %s display\n",
                teamSideToString(offenseTeam).c_str(),
                (offenseTeam == HOME) ? "Display1" : "Display2"); // which display is the offense on

}

void setPossession(TeamSide offenseTeam) {
  teamWithTheBall = offenseTeam;
  applyPossessionMenus(teamWithTheBall);
}

void flipPossession() {
  setPossession(switchTeamSide(teamWithTheBall));
}

static bool isActionPlayName(const String& selectedText) {
  for (const String& actionName : ActionPlayNames) {
    if (actionName == selectedText) {
      return true;
    }
  }
  return false;
}

static bool isSelectionValidForMenu(MenuSystem* pressedMenu) {
  if (!pressedMenu) {
    return false;
  }

  String selectedText = pressedMenu->getSelectedItemLabel();
  if (selectedText.length() == 0 || selectedText == "Back") {
    return false;
  }

  MenuSystem* offenseMenu = (teamWithTheBall == HOME) ? &menu1 : &menu2;
  if (pressedMenu != offenseMenu) {
    return true;
  }

  return isActionPlayName(selectedText);
}

void captureSelectedPlayForMenu(MenuSystem* pressedMenu) {
  if (!pressedMenu) {
    return;
  }

  String selectedText = pressedMenu->getSelectedItemLabel();
  if (selectedText.length() == 0) {
    return;
  }

  if (!isSelectionValidForMenu(pressedMenu)) {
    // Mirror encoder push behavior for offense menu navigation entries
    // (e.g., Run/Pass/Kick and Back), while still preventing invalid captures.
    MenuSystem* offenseMenu = (teamWithTheBall == HOME) ? &menu1 : &menu2;
    if (pressedMenu == offenseMenu) {
      pressedMenu->selectCurrentItem();
      return;
    }

    Serial.println("Selection ignored for countdown: invalid play for current role.");
    return;
  }

  MenuSystem* offenseMenu = (teamWithTheBall == HOME) ? &menu1 : &menu2;

  if (pressedMenu == offenseMenu) {
    offense_play = selectedText;
  } else {
    defense_play = selectedText;
  }

  bool offenseChosen = (offense_play.length() > 0);
  bool defenseChosen = (defense_play.length() > 0);

  offense_and_defense_selected = (offenseChosen && defenseChosen);

  if (offense_and_defense_selected) {
    playSelectionCountdownActive = false;
    disableActionButtons();
  }
}

bool ballPastMarkerCheck(TeamSide offenseTeam, int ballLocation, int downMarkerLocation) {
  if (offenseTeam == HOME) {
    return (ballLocation >= downMarkerLocation);
  } else {
    return (ballLocation <= downMarkerLocation);
  }
}

void delayOfGame(bool offenseCausedPenalty) {
  Serial.println("Pre penalty To Go: " + String(abs(downMarkerLocation - ballLocation)));
  Serial.println("current ball location: " + String(ballLocation));
  Serial.println("current down: " + String(downCounter));
  Serial.println("team with the ball: " + teamSideToString(teamWithTheBall));
  if (offenseCausedPenalty) {
    if (downCounter < 4) {
      statusPrintLine("Delay of game -5 yd");
      ballLocation = ballMovement(ballLocation, teamWithTheBall, -DELAY_OF_GAME_YARDS);
      downCounter += 1;
    } else {
      statusPrintLine("Delay of game turnover");
      downCounter = 6; // Set downCounter to 6 to indicate turnover on downs because of penalty. This will trigger the turnover handling logic in the main loop.
    }
  } else {
    statusPrintLine("D delay of game +5 yd");
    int penaltyYards = DELAY_OF_GAME_YARDS;
    // check to make sure we don't move the ball past the end zone
    if (teamWithTheBall == HOME && ballLocation + penaltyYards > 100) {
      penaltyYards = 100 - ballLocation/2; // Move half the distance to the end zone
    } else if (teamWithTheBall == VISITOR && ballLocation - penaltyYards < 0) {
      penaltyYards = ballLocation/2; // Adjust for half the distance to the end zone for visitor team
    }
    ballLocation = ballMovement(ballLocation, teamWithTheBall, penaltyYards);

    if (ballPastMarkerCheck(teamWithTheBall, ballLocation, downMarkerLocation)) {
      downCounter = 1;
      downMarkerLocation = firstAndTen(ballLocation, teamWithTheBall);
      eventPrintLine("1st down for " + teamSideToString(teamWithTheBall) + ".");
      delay(2000);
    }
  }
  Serial.println(" To Go: " + String(abs(downMarkerLocation - ballLocation)));
  Serial.println("current down: " + String(downCounter));
  Serial.println("current down marker location: " + String(downMarkerLocation));
  Serial.println("current ball location: " + String(ballLocation));
}

void handleTurnoverOnDowns() {

  TeamSide previousOffense = teamWithTheBall;
  setPossession(switchTeamSide(teamWithTheBall));
  downCounter = 1;
  downMarkerLocation = firstAndTen(ballLocation, teamWithTheBall);

  // Reset pending play-selection state after possession flips.
  offense_play = "";
  defense_play = "";
  offense_and_defense_selected = false;
  playSelectionCountdownActive = false;
  enableActionButtons();
  eventPrintLine("Turnover on downs");
  
  Serial.printf("Turnover on downs: %s -> %s, ball=%d marker=%d\n",
                teamSideToString(previousOffense).c_str(),
                teamSideToString(teamWithTheBall).c_str(),
                ballLocation,
                downMarkerLocation);
}

int randomBellShaped(int size) {
    int random1 = rand() % size;
    int random2 = rand() % size;
    return (random1 + random2) / 2;
}

void ballMovement(TeamSide side, int movement) {
    // This function will determine the new ball location based on the current location and which team has the ball. It will use the teamBallLocation function to determine the new location based on the current location and the team with the ball.
    if (TeamSide::HOME == side) {
        ballLocation += movement; // Move the ball forward for the home team

    } else {
        ballLocation -= movement; // Move the ball forward for the visitor team (which is in the opposite direction)
    }
    if (ballLocation > 100) {
        ballLocation = 100; // Cap the ball location at the opponent's end zone if it exceeds 100 yards
    }
    if (ballLocation < 0) {
            ballLocation = 0; // Cap the ball location at the opponent's end zone if it goes below 0 yards
    }
}

// kickoff function simulates the kickoff and determines the starting location of the ball
// side is the team that is kicking off
int kickoff(TeamSide side)
{
  lcdPrintLine(0, " ");
  lcdPrintLine(1, " ");
  statusPrintLine("Kickoff for " + teamSideToString(side));
  eventPrintLine(" ");
  // place the ball at the kicking team's 20-yard line
  ballLocation = teamBallLocation(side, 20);
  // Simulate kickoff logic here
  int randomTemp = diceRoll(3); // Roll three 6-sided dies to determine kickoff location
  TeamSide newBallOwner = switchTeamSide(side);
  int newBallLocation = teamBallLocation(newBallOwner, 30 - randomTemp);
  animateBallMovement(ballLocation, newBallLocation, newBallOwner);
  if (randomTemp == 3)
  {
    statusPrintLine("Turnover on kickoff!");
    // they get to keep the ball
    setPossession(side);                                             // the kicking team gets to keep the ball but at the other end of the field
    ballLocation = newBallLocation;                                  // Ball starts at the 20-yard line of the receiving team because of the turnover but the kicking team gets to keep the ball
    downMarkerLocation = firstAndTen(ballLocation, teamWithTheBall); // Set the down marker location after the turnover
    return ballLocation;                                             // Return ball location
  }
  if (randomTemp == 18)
  {
    setPossession(newBallOwner); // the receiving team
    updateCenteredLCDDisplay(teamSideToString(newBallOwner), "Touchdown!");
    Serial.println("Returned for Touchdown!");
    eventPrintLine("Point after option?");
    statusPrintLine("Ball run in for touchdown");
    myStrip->triggerEventByName("touchdown");
    points = 6 + pointAfter(newBallOwner);
    return -1; // Return -1 to indicate a touchdown
  }
  else
  {
    statusPrintLine(teamSideToString(newBallOwner) + " has ball");
    setPossession(newBallOwner);                                    // The receiving team gets the ball after the kickoff
    ballLocation = teamBallLocation(newBallOwner, 30 - randomTemp); // Ball starts at the 30-yard line minus the randomTemp to simulate different kickoff distances
    downMarkerLocation = firstAndTen(ballLocation, newBallOwner);   // Set the down marker location after the turnover
    return ballLocation;                                            // Return ball location
  }
}

int lastBallLocation=0;
int lastDownMarkerLocation=0;
// displayField function updates the LED strip to show the current state of the field, including the ball location and down marker location
int displayField(int ballLocation, TeamSide sideWithTheBall, int downMarkerLocation) {

    // This function will handle displaying the field on the LED strip based on the current ball location, which team has the ball, and where the down marker is. It will use different colors to represent the ball, the down marker, and the background of the field.
    if (!myStrip) // check to see if the LED strip is initialized before trying to display the field
    {
        Serial.println("LED strip not initialized");
        return -1;
    }
    if (ballLocation != lastBallLocation || downMarkerLocation != lastDownMarkerLocation) {
      //looks like we need to redraw the field
        lastBallLocation = ballLocation;
        lastDownMarkerLocation = downMarkerLocation;
        Serial.printf("Displaying field: ballLocation=%d, downMarkerLocation=%d, teamWithTheBall=%s, color=%s\n",
                  ballLocation,
                  downMarkerLocation,
                  teamSideToString(sideWithTheBall).c_str(),
                  ballOwnerColor(teamWithTheBall).toString().c_str());
        myStrip->setLogicalLED(ballLocation, ballOwnerColor(teamWithTheBall), true);   // lights up the ball location
        myStrip->setLogicalLED(downMarkerLocation, CRGB::White, false); // lights up the down marker location
    }
    return 0;
}

void setup() {
  Serial.begin(115200);
  delay(1000); // let the system stabilize

  buildActionPlayNames();

  pinMode(Action_button_1_PIN, INPUT_PULLUP);
  pinMode(Action_button_2_PIN, INPUT_PULLUP);
  enableActionButtons();

  Serial.println("Initialize the filesystem");
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
    return;
  }

  if (!loadPlayDataFromCSV("/playdata.csv")) {
    Serial.println("Failed to load /playdata.csv");
  }

  // Quick sanity check: List all files in the root directory
  File root = LittleFS.open("/");
  Serial.println("Root directory opened.");
  File file = root.openNextFile();
  Serial.println("Files in LittleFS root directory:");
  while(file){
    Serial.print("Found File: ");
    Serial.println(file.name());
    file = root.openNextFile();
  }

  if (!loadLayoutFromCSV("/myLEDLayout.csv", myLayout, numLeds)) {
    Serial.println("Failed to load LED layout");
    return;
  }
  Serial.println("LED Strip Starting...");
  //setup the LED strip
  leds= new CRGB[numLeds];
  FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(leds, numLeds);
  myStrip = new MyFastLED(leds, numLeds, myLayout.data(), myLayout.size());
  Serial.println("LED strip initialized.");

  // Quick startup chase to confirm strip power and data pin wiring.
  myStrip->logicalLEDsequence(0, numLeds - 1, 10, CRGB::Green, false);
  Serial.println("LED strip chase complete.");
  bgLeds.clear();
 
  Serial.println("Starting dual display menu system...");
  // Initialize boards
  board1.begin();
  board2.begin();

  // Randomly choose who starts on offense each boot.
  setPossession((esp_random() & 0x1) == 0 ? HOME : VISITOR);

  // Bind each menu to its board's encoder input handler.
  board1.bindMenu(&menu1, 1);
  board2.bindMenu(&menu2, 1);
  
  // Set to landscape with font size 2 (larger text)
  menu1.setFontSize(2);
  menu1.setOrientation(MenuSystem::LANDSCAPE);
  // Set to portrait with font size 2 (larger text)
  menu2.setFontSize(2);
  menu2.setOrientation(MenuSystem::LANDSCAPE);

  // Initialize menu systems
  menu1.begin();
  menu2.begin();

  PointAfterMenuContext pointAfterMenuContext;
  pointAfterMenuContext.menuForTeam = menuForTeam;
  pointAfterMenuContext.boardForTeam = boardForTeam;
  pointAfterMenuContext.areActionButtonsEnabled = areActionButtonsEnabled;
  pointAfterMenuContext.consumeActionButton1 = consumeActionButton1ForPointAfter;
  pointAfterMenuContext.consumeActionButton2 = consumeActionButton2ForPointAfter;
  pointAfterMenuContext.disableActionButtons = disableActionButtons;
  pointAfterMenuContext.enableActionButtons = enableActionButtons;
  pointAfterMenuContext.resetActionButtonFlags = resetActionButtonFlags;
  pointAfterMenuContext.pumpUiWhileWaiting = pumpUiWhileWaitingForPointAfterSelection;
  pointAfterMenuContext.restoreMenusAfterSelection = restorePossessionMenusAfterPointAfterSelection;
  pointAfterMenuContext.timeoutMs = 30000;
  configurePointAfterMenuContext(pointAfterMenuContext);

  registerPointAfterDecisionProvider(promptPointAfterViaMenu);

  menu1.redraw();
  menu2.redraw();
  // Load sequences from CSV file in LittleFS
  if (seqLoader.loadFromCSV("/sequences.csv"))
    {
        Serial.println("Sequences loaded successfully!");
        myStrip->setSequences(seqLoader.getSequences(), seqLoader.getSequenceCount());
    }
  else
    {
        Serial.println("Failed to load sequences. Using fallback.");
        // Fallback to hardcoded idle animation if CSV load fails
        myStrip->setSequences(nullptr, 0);
    }
  Serial.println("Sequences found were: ");
   for (int i = 0; i < seqLoader.getSequenceCount(); i++)
    {
        Serial.println(seqLoader.getSequences()[i].name);
    }
  if (LCDinit()) {
    lcdPrintCenteredLine(0, "Football");
    lcdPrintCenteredLine(1, "Game");
  }
  // Initialize sound board
  if (!soundBoard.begin()) {
    Serial.println("SoundBoard initialization failed.");
  }
  soundBoard.playSound("crowd_cheer"); // Play crowd cheer sound at the start of the game
  kickoff(teamWithTheBall);
  // start the timer
  gameTimer.start(4*5*60 * 1000); //5 minutes a quarter, 4 quarters in a game, converted to milliseconds
}

void loop() {
  // Update board states
  board1.update();
  board2.update();

  // Process encoder input through board-owned handlers.
  board1.processEncoderInput();
  board2.processEncoderInput();
  
  // Update menu systems (they'll handle redraws)
  menu1.update();
  menu2.update();

  // Consume ISR latches in the main loop and snapshot selected menu labels.
  bool consumeAction1 = false;
  bool consumeAction2 = false;

  noInterrupts();
  if (actionButtonsEnabled && actionButton1Pressed) {
    consumeAction1 = true;
    actionButton1Pressed = false;
    Serial.println("Action button 1 pressed");
    String menu1Value = menu1.getSelectedItemLabel();
    Serial.println("Menu 1 selected value: " + menu1Value);
  }
  if (actionButtonsEnabled && actionButton2Pressed) {
    consumeAction2 = true;
    actionButton2Pressed = false;
    Serial.println("Action button 2 pressed");
    String menu2Value = menu2.getSelectedItemLabel();
    Serial.println("Menu 2 selected value: " + menu2Value);
  }
  interrupts();

  uint32_t nowMs = millis();

  if (consumeAction1 && (nowMs - lastAction1HandledMs >= ACTION_DEBOUNCE_MS) && digitalRead(Action_button_1_PIN) == HIGH) {
    lastAction1HandledMs = nowMs;
    captureSelectedPlayForMenu(&menu1);
  }
  if (consumeAction2 && (nowMs - lastAction2HandledMs >= ACTION_DEBOUNCE_MS) && digitalRead(Action_button_2_PIN) == HIGH) {
    lastAction2HandledMs = nowMs;
    captureSelectedPlayForMenu(&menu2);
  }

  bool offenseChosen = (offense_play.length() > 0);
  bool defenseChosen = (defense_play.length() > 0);
  bool oneSideChosen = (offenseChosen ^ defenseChosen);

  if (oneSideChosen) {
    waitingOnOffenseSelection = !offenseChosen;
    if (!playSelectionCountdownActive) {
      playSelectionTimer.start(PLAY_SELECTION_TIMEOUT_MS);
      playSelectionCountdownActive = true;
      Serial.println("Play selection timeout started (20s).");
    }
  } else if (!offense_and_defense_selected) {
    playSelectionCountdownActive = false;
  }

  if (playSelectionCountdownActive) {
    playSelectionTimer.update();
    if (playSelectionTimer.isFinished()) {
      delayOfGame(waitingOnOffenseSelection);
      playSelectionCountdownActive = false;
      offense_play = "";
      defense_play = "";
      offense_and_defense_selected = false;
    }
  }

  if (gameTimer.getRemainingSeconds() <= 0) {
    // Game timer has expired, handle end of game logic here
    statusPrintLine("Game Over");
    // You can add additional logic to handle the end of the game, such as displaying final scores or resetting the game state. 
    String line1 = "Hom: " + String(homeScore) + " Vis: " + String(visitorScore);
    String line2 = "" ;
    if (homeScore > visitorScore) {
      line2 += " Winner: Home";
    } else if (visitorScore > homeScore) {
      line2 += " Winner: Visitor";
    } else {
      line2 += " Tie Game";
    }
    updateCenteredLCDDisplay(line1, line2);
    delay(50000);// Display the final score for 50 seconds
    // Reset the game state or perform any other necessary actions here
    // restart the ESP32 to reset the game
    ESP.restart();
  }

  if (lastLcdUpdateMs == 0 || (nowMs - lastLcdUpdateMs >= LCD_UPDATE_INTERVAL_MS))
  {
    // Update the display with current scores and down information
    String line1 = "Hom:" + String(homeScore) + " Vis:" + String(visitorScore) + " T:" + String(gameTimer.getRemainingTime());
    String line2 = "Down: " + String(downCounter) + " To Go: " + String(abs(downMarkerLocation - ballLocation));
    if (playSelectionCountdownActive)
    {
      String waitingLabel = waitingOnOffenseSelection ? "Offense" : "Defense";
      String line3 = waitingLabel + ": " + String(playSelectionTimer.getRemainingTime());
      statusPrintLine(line3);
      eventPrintLine(" ");
    }
    updateLCDDisplay(line1, line2);
    lastLcdUpdateMs = nowMs;
  }

  if (offense_and_defense_selected) {
     lcdDisplay->clear();
    TeamSide nextPossession = playExecution(teamWithTheBall, offense_play, defense_play);
    if (nextPossession != teamWithTheBall) {
      // Possession changed as a result of the play execution.
      setPossession(nextPossession);
      Serial.printf("Possession changed by playExecution: %s -> %s\n",
                    teamSideToString(teamWithTheBall).c_str(),
                    teamSideToString(nextPossession).c_str());
      teamWithTheBall = nextPossession;
    }

    // Require both buttons to select fresh plays before the next execution.
    offense_play = "";
    defense_play = "";
    // reset the menus
    menu1.resetMenu();
    menu2.resetMenu();
    // reset the action button pressed flags to avoid accidental double execution
    noInterrupts();
    actionButton1Pressed = false;
    actionButton2Pressed = false;
    interrupts();

    offense_and_defense_selected = false;
    playSelectionCountdownActive = false;
    enableActionButtons();
  }

  if (downCounter > 4) {
    handleTurnoverOnDowns();
  } else if (points > 0) {
    if (teamWithTheBall == HOME) {
      homeScore += points;
    } else {
      visitorScore += points;
    }
    points = 0;
    kickoff(teamWithTheBall);
  }

  // place ball on the field
  displayField(ballLocation, teamWithTheBall, downMarkerLocation);

  delay(20);
}