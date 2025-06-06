//developed with Claude 3.7

#include "UWB-MaUWB-AT.h"
#include <cmath> // Include for sqrt() and abs() functions

UWBTAG::UWBTAG() {
    // Initialize variables
    _tagNumber = 0;
    _refreshRate = 50;
    _totalTags = 10;
    _displayInitialized = false;
    _positionHistoryIndex = 0;
    _positionHistoryFilled = false;
    _lastRangeRequest = 0;
    _lastDisplayUpdate = 0;
    _newData = false;
    _response = "";
    _activeOtherTagCount = 0;
    
    // Initialize positions
    positionX = 0.0;
    positionY = 0.0;
    a0Distance = 0.0;
    a1Distance = 0.0;
    a2Distance = 0.0;
    a3Distance = 0.0;
    
    // Initialize anchor positions (default)
    for (int i = 0; i < 4; i++) {
        _anchorPositions[i][0] = 0.0;
        _anchorPositions[i][1] = 0.0;
    }
    
    // Initialize position history
    for (int i = 0; i < POSITION_HISTORY_LENGTH; i++) {
        _positionXHistory[i] = 0.0;
        _positionYHistory[i] = 0.0;
    }
    
    // Initialize other tags tracking
    for (int i = 0; i < MAX_OTHER_TAGS; i++) {
        _otherTags[i].tagID = -1;
        _otherTags[i].x = 0.0;
        _otherTags[i].y = 0.0;
        _otherTags[i].lastSeen = 0;
        _otherTags[i].active = false;
    }
    
    // Initialize hardware immediately
    initializeHardware();
}

UWBTAG::~UWBTAG() {
    // Free allocated memory for display
    if (_display != nullptr) {
        delete _display;
        _display = nullptr;
    }
}

void UWBTAG::initializeHardware() {
    // Initialize reset pin
    pinMode(RESET_PIN, OUTPUT);
    digitalWrite(RESET_PIN, HIGH);
    
    // Initialize Serial2 for UWB communication
    Serial2.begin(115200, SERIAL_8N1, IO_RXD2, IO_TXD2);
    
    // Initialize I2C
    Wire.begin(I2C_SDA, I2C_SCL);
    
    delay(500);
    
    // Initialize display
    _display = new Adafruit_SSD1306(128, 64, &Wire, -1);
    if (_display->begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        _displayInitialized = true;
        _display->clearDisplay();
        _display->setTextColor(SSD1306_WHITE);
        
        // Show initialization message
        _display->setTextSize(1);
        _display->setCursor(0, 0);
        _display->println(F("UWB TAG Initializing..."));
        _display->display();
    }
    
    delay(1000);
    
    // Configure UWB module
    configureUWBModule();
}

void UWBTAG::configureUWBModule() {
    Serial2.println("AT");
    
    sendCommand("AT?", 500, false);
    sendCommand("AT+RESTORE", 1000, false);
    
    // Configure as tag (role=0)
    String cfg = "AT+SETCFG=" + String(_tagNumber) + ",0,1,1";
    sendCommand(cfg, 500, false);
    
    // Set capacity
    String cap = "AT+SETCAP=" + String(_totalTags) + ",10,1";
    sendCommand(cap, 500, false);
    
    sendCommand("AT+SETRPT=1", 500, false);
    sendCommand("AT+SAVE", 500, false);
    sendCommand("AT+RESTART", 1000, false);
    
    if (_displayInitialized) {
        _display->clearDisplay();
        _display->setCursor(0, 0);
        _display->print(F("TAG "));
        _display->println(_tagNumber);
        _display->setCursor(0, 15);
        _display->println(F("Ready for data"));
        _display->display();
    }
    
    delay(1000);
}

void UWBTAG::setTagNumber(int tagNum) {
    _tagNumber = tagNum;
}

void UWBTAG::refreshRate(unsigned long rate) {
    _refreshRate = rate;
}

void UWBTAG::totalTags(int count) {
    _totalTags = count;
}

void UWBTAG::anchor0(float x, float y) {
    _anchorPositions[0][0] = x;
    _anchorPositions[0][1] = y;
}

void UWBTAG::anchor1(float x, float y) {
    _anchorPositions[1][0] = x;
    _anchorPositions[1][1] = y;
}

void UWBTAG::anchor2(float x, float y) {
    _anchorPositions[2][0] = x;
    _anchorPositions[2][1] = y;
}

void UWBTAG::anchor3(float x, float y) {
    _anchorPositions[3][0] = x;
    _anchorPositions[3][1] = y;
}

void UWBTAG::update() {
    // Read data from UWB module
    readUWBData();
    
    // Request range data periodically
    if (millis() - _lastRangeRequest > _refreshRate) {
        sendCommand("AT+RANGE", 100, false);
        _lastRangeRequest = millis();
    }
    
    // Update display at same rate as data refresh
    if ((millis() - _lastDisplayUpdate > _refreshRate) || _newData) {
        updateDisplay();
        _newData = false;
        _lastDisplayUpdate = millis();
    }
}

void UWBTAG::readUWBData() {
    while (Serial2.available() > 0) {
        char c = Serial2.read();
        
        if (c == '\r') {
            continue;
        } else if (c == '\n') {
            if (_response.length() > 0) {
                // Check if it's range data or position data
                if (_response.startsWith("AT+RANGE=")) {
                    parseRangeData(_response);
                } else if (_response.startsWith("AT+RDATA=")) {
                    parsePositionData(_response);
                }
                _response = "";
            }
        } else {
            _response += c;
        }
    }
}

void UWBTAG::parseRangeData(String data) {
    // Look for "range:(" in the response
    int rangeStart = data.indexOf("range:(");
    if (rangeStart >= 0) {
        // Find the end of the range array
        int rangeEnd = data.indexOf(")", rangeStart);
        if (rangeEnd > rangeStart) {
            // Extract just the range values
            String rangeValues = data.substring(rangeStart + 7, rangeEnd);
            
            // Split the range values by commas
            int commaIndex = 0;
            int startIndex = 0;
            int valueIndex = 0;
            
            // We're only interested in the first 4 values (0-3)
            float distances[4] = {0, 0, 0, 0};
            
            // Parse each value
            while (valueIndex < 4 && startIndex < rangeValues.length()) {
                commaIndex = rangeValues.indexOf(',', startIndex);
                
                if (commaIndex == -1) {
                    commaIndex = rangeValues.length();
                }
                
                String valueStr = rangeValues.substring(startIndex, commaIndex);
                distances[valueIndex] = valueStr.toFloat();
                
                valueIndex++;
                startIndex = commaIndex + 1;
            }
            
            // Update distance variables
            a0Distance = distances[0];
            a1Distance = distances[1];
            a2Distance = distances[2];
            a3Distance = distances[3];
            
            // Calculate 2D position
            calculatePosition();
            
            _newData = true;
        }
    }
}

void UWBTAG::parsePositionData(String data) {
    // Parse position data from Position Server anchor
    // Format: AT+RDATA=1,0,timestamp,length,ALLPOS:tag1:x1:y1:tag2:x2:y2:...
    
    // Look for "ALLPOS:" in the response
    int posStart = data.indexOf("ALLPOS:");
    if (posStart >= 0) {
        String posData = data.substring(posStart + 7); // Skip "ALLPOS:"
        
        // Reset all other tags to inactive
        for (int i = 0; i < MAX_OTHER_TAGS; i++) {
            _otherTags[i].active = false;
        }
        _activeOtherTagCount = 0;
        
        // Parse tag positions
        int index = 0;
        while (index < posData.length()) {
            // Find tag ID
            int colonIndex1 = posData.indexOf(':', index);
            if (colonIndex1 == -1) break;
            
            int tagID = posData.substring(index, colonIndex1).toInt();
            
            // Find X coordinate
            int colonIndex2 = posData.indexOf(':', colonIndex1 + 1);
            if (colonIndex2 == -1) break;
            
            float x = posData.substring(colonIndex1 + 1, colonIndex2).toFloat();
            
            // Find Y coordinate
            int colonIndex3 = posData.indexOf(':', colonIndex2 + 1);
            if (colonIndex3 == -1) {
                // Last tag in the list
                float y = posData.substring(colonIndex2 + 1).toFloat();
                
                // Store this tag (if it's not us)
                if (tagID != _tagNumber) {
                    OtherTag* tag = getOtherTag(tagID);
                    if (tag != nullptr) {
                        tag->x = x;
                        tag->y = y;
                        tag->active = true;
                        tag->lastSeen = millis();
                    }
                }
                break;
            } else {
                float y = posData.substring(colonIndex2 + 1, colonIndex3).toFloat();
                
                // Store this tag (if it's not us)
                if (tagID != _tagNumber) {
                    OtherTag* tag = getOtherTag(tagID);
                    if (tag != nullptr) {
                        tag->x = x;
                        tag->y = y;
                        tag->active = true;
                        tag->lastSeen = millis();
                    }
                }
                
                index = colonIndex3 + 1;
            }
        }
        
        // Count active other tags
        _activeOtherTagCount = 0;
        for (int i = 0; i < MAX_OTHER_TAGS; i++) {
            if (_otherTags[i].active) {
                _activeOtherTagCount++;
            }
        }
        
        _newData = true;
    }
}

void UWBTAG::calculatePosition() {
    // Check if we have valid distances from all 4 anchors
    if (a0Distance <= 0 || a1Distance <= 0 || a2Distance <= 0 || a3Distance <= 0) {
        return;
    }
    
    // First calculate position using anchors 0,1,2 (original method)
    // Convert distances from cm to m for numerical stability
    float r1 = a0Distance / 100.0;
    float r2 = a1Distance / 100.0;
    float r3 = a2Distance / 100.0;
    float r4 = a3Distance / 100.0;
    
    // Convert anchor positions to meters
    float x1 = _anchorPositions[0][0] / 100.0;
    float y1_pos = _anchorPositions[0][1] / 100.0;
    float x2 = _anchorPositions[1][0] / 100.0;
    float y2 = _anchorPositions[1][1] / 100.0;
    float x3 = _anchorPositions[2][0] / 100.0;
    float y3 = _anchorPositions[2][1] / 100.0;
    float x4 = _anchorPositions[3][0] / 100.0;
    float y4 = _anchorPositions[3][1] / 100.0;
    
    // Calculate positions using three different sets of three anchors
    // Position 1: Using anchors 0,1,2 (original method)
    float A1 = 2 * (x2 - x1);
    float B1 = 2 * (y2 - y1_pos);
    float C1 = r1 * r1 - r2 * r2 - x1 * x1 + x2 * x2 - y1_pos * y1_pos + y2 * y2;
    float D1 = 2 * (x3 - x2);
    float E1 = 2 * (y3 - y2);
    float F1 = r2 * r2 - r3 * r3 - x2 * x2 + x3 * x3 - y2 * y2 + y3 * y3;
    
    float den1 = (A1 * E1 - B1 * D1);
    float x_1 = 0.0, y_1 = 0.0;
    bool valid1 = false;
    
    if (std::abs(den1) > 0.000001) {
        x_1 = (C1 * E1 - F1 * B1) / den1;
        y_1 = (A1 * F1 - C1 * D1) / den1;
        valid1 = true;
    }
    
    // Position 2: Using anchors 0,1,3
    float A2 = 2 * (x2 - x1);
    float B2 = 2 * (y2 - y1_pos);
    float C2 = r1 * r1 - r2 * r2 - x1 * x1 + x2 * x2 - y1_pos * y1_pos + y2 * y2;
    float D2 = 2 * (x4 - x2);
    float E2 = 2 * (y4 - y2);
    float F2 = r2 * r2 - r4 * r4 - x2 * x2 + x4 * x4 - y2 * y2 + y4 * y4;
    
    float den2 = (A2 * E2 - B2 * D2);
    float x_2 = 0.0, y_2 = 0.0;
    bool valid2 = false;
    
    if (std::abs(den2) > 0.000001) {
        x_2 = (C2 * E2 - F2 * B2) / den2;
        y_2 = (A2 * F2 - C2 * D2) / den2;
        valid2 = true;
    }
    
    // Position 3: Using anchors 0,2,3
    float A3 = 2 * (x3 - x1);
    float B3 = 2 * (y3 - y1_pos);
    float C3 = r1 * r1 - r3 * r3 - x1 * x1 + x3 * x3 - y1_pos * y1_pos + y3 * y3;
    float D3 = 2 * (x4 - x3);
    float E3 = 2 * (y4 - y3);
    float F3 = r3 * r3 - r4 * r4 - x3 * x3 + x4 * x4 - y3 * y3 + y4 * y4;
    
    float den3 = (A3 * E3 - B3 * D3);
    float x_3 = 0.0, y_3 = 0.0;
    bool valid3 = false;
    
    if (std::abs(den3) > 0.000001) {
        x_3 = (C3 * E3 - F3 * B3) / den3;
        y_3 = (A3 * F3 - C3 * D3) / den3;
        valid3 = true;
    }
    
    // Position 4: Using anchors 1,2,3
    float A4 = 2 * (x3 - x2);
    float B4 = 2 * (y3 - y2);
    float C4 = r2 * r2 - r3 * r3 - x2 * x2 + x3 * x3 - y2 * y2 + y3 * y3;
    float D4 = 2 * (x4 - x3);
    float E4 = 2 * (y4 - y3);
    float F4 = r3 * r3 - r4 * r4 - x3 * x3 + x4 * x4 - y3 * y3 + y4 * y4;
    
    float den4 = (A4 * E4 - B4 * D4);
    float x_4 = 0.0, y_4 = 0.0;
    bool valid4 = false;
    
    if (std::abs(den4) > 0.000001) {
        x_4 = (C4 * E4 - F4 * B4) / den4;
        y_4 = (A4 * F4 - C4 * D4) / den4;
        valid4 = true;
    }
    
    // Calculate weighted average position
    float x_sum = 0.0;
    float y_sum = 0.0;
    float weight_sum = 0.0;
    
    if (valid1) {
        x_sum += x_1;
        y_sum += y_1;
        weight_sum += 1.0;
    }
    
    if (valid2) {
        x_sum += x_2;
        y_sum += y_2;
        weight_sum += 1.0;
    }
    
    if (valid3) {
        x_sum += x_3;
        y_sum += y_3;
        weight_sum += 1.0;
    }
    
    if (valid4) {
        x_sum += x_4;
        y_sum += y_4;
        weight_sum += 1.0;
    }
    
    // Check if we have at least one valid position
    if (weight_sum < 1.0) {
        return;
    }
    
    // Calculate final position (in meters)
    float x = x_sum / weight_sum;
    float y = y_sum / weight_sum;
    
    // Convert back to cm
    float rawX = x * 100.0;
    float rawY = y * 100.0;
    
    // Filter out values outside boundaries
    if (rawX < 0 || rawY < 0 || rawX > _anchorPositions[2][0] || rawY > _anchorPositions[1][1]) {
        return;
    }
    
    // Add to position history (length 1, so just direct assignment)
    _positionXHistory[0] = rawX;
    _positionYHistory[0] = rawY;
    
    // Update final position (with length 1, no averaging needed)
    positionX = _positionXHistory[0];
    positionY = _positionYHistory[0];
}

void UWBTAG::updateDisplay() {
    if (!_displayInitialized) {
        return;
    }
    
    _display->clearDisplay();
    
    // Display header
    _display->setTextSize(1);
    _display->setCursor(0, 0);
    _display->print(F("TAG "));
    _display->println(_tagNumber);
    _display->drawLine(0, 9, 128, 9, SSD1306_WHITE);
    
    // Display distances to A0 and A1
    _display->setCursor(0, 12);
    _display->print(F("A0: "));
    if (a0Distance > 0) {
        _display->print(a0Distance, 1);
    } else {
        _display->print(F("---"));
    }
    
    _display->setCursor(64, 12);
    _display->print(F("A1: "));
    if (a1Distance > 0) {
        _display->print(a1Distance, 1);
    } else {
        _display->print(F("---"));
    }
    
    // Display distances to A2 and A3
    _display->setCursor(0, 24);
    _display->print(F("A2: "));
    if (a2Distance > 0) {
        _display->print(a2Distance, 1);
    } else {
        _display->print(F("---"));
    }
    
    _display->setCursor(64, 24);
    _display->print(F("A3: "));
    if (a3Distance > 0) {
        _display->print(a3Distance, 1);
    } else {
        _display->print(F("---"));
    }
    
    // Display divider
    _display->drawLine(0, 35, 128, 35, SSD1306_WHITE);
    
    // Display position
    _display->setCursor(0, 40);
    _display->println(F("POSITION:"));
    
    _display->setCursor(0, 50);
    _display->print(F("X: "));
    _display->print(positionX, 1);
    
    _display->setCursor(64, 50);
    _display->print(F("Y: "));
    _display->print(positionY, 1);
    
    _display->display();
}

float UWBTAG::getTagDistance(int tagID) {
    if (tagID == _tagNumber) {
        return 0.0; // Distance to self is 0
    }
    
    // Find the tag
    for (int i = 0; i < MAX_OTHER_TAGS; i++) {
        if (_otherTags[i].tagID == tagID && _otherTags[i].active) {
            // Calculate distance using Euclidean formula
            float dx = _otherTags[i].x - positionX;
            float dy = _otherTags[i].y - positionY;
            return std::sqrt(dx * dx + dy * dy);
        }
    }
    
    return -1.0; // Tag not found or not active
}

float UWBTAG::getTagX(int tagID) {
    if (tagID == _tagNumber) {
        return positionX; // Our own position
    }
    
    for (int i = 0; i < MAX_OTHER_TAGS; i++) {
        if (_otherTags[i].tagID == tagID && _otherTags[i].active) {
            return _otherTags[i].x;
        }
    }
    
    return 0.0; // Tag not found
}

float UWBTAG::getTagY(int tagID) {
    if (tagID == _tagNumber) {
        return positionY; // Our own position
    }
    
    for (int i = 0; i < MAX_OTHER_TAGS; i++) {
        if (_otherTags[i].tagID == tagID && _otherTags[i].active) {
            return _otherTags[i].y;
        }
    }
    
    return 0.0; // Tag not found
}

bool UWBTAG::isTagActive(int tagID) {
    if (tagID == _tagNumber) {
        return true; // We are always active (for ourselves)
    }
    
    for (int i = 0; i < MAX_OTHER_TAGS; i++) {
        if (_otherTags[i].tagID == tagID && _otherTags[i].active) {
            return true;
        }
    }
    
    return false;
}

int UWBTAG::getActiveTagCount() {
    return _activeOtherTagCount + 1; // +1 for ourselves
}

String UWBTAG::sendCommand(String command, int timeout, bool debug) {
    String response = "";
    
    Serial2.println(command);
    
    long int time = millis();
    
    while ((time + timeout) > millis()) {
        while (Serial2.available()) {
            char c = Serial2.read();
            response += c;
        }
    }
    
    return response;
}

UWBTAG::OtherTag* UWBTAG::getOtherTag(int tagID) {
    // First, look for existing tag
    for (int i = 0; i < MAX_OTHER_TAGS; i++) {
        if (_otherTags[i].tagID == tagID && _otherTags[i].active) {
            return &_otherTags[i];
        }
    }
    
    // If not found, find an empty slot
    for (int i = 0; i < MAX_OTHER_TAGS; i++) {
        if (!_otherTags[i].active || _otherTags[i].tagID == -1) {
            _otherTags[i].tagID = tagID;
            _otherTags[i].active = true;
            _otherTags[i].lastSeen = millis();
            _activeOtherTagCount++;
            return &_otherTags[i];
        }
    }
    
    return nullptr; // No available slots
}

void UWBTAG::updateOtherTagLastSeen(int tagID) {
    for (int i = 0; i < MAX_OTHER_TAGS; i++) {
        if (_otherTags[i].tagID == tagID && _otherTags[i].active) {
            _otherTags[i].lastSeen = millis();
            break;
        }
    }
}

// ==============================
// UWBAnchor Implementation
// ==============================

UWBAnchor::UWBAnchor(AnchorType type) {
    // Set anchor type
    anchorType = type;
    
    // Initialize variables
    _anchorNumber = 0;
    _anchorPosition[0] = 0.0;
    _anchorPosition[1] = 0.0;
    _refreshRate = 100;
    _totalTags = 10;
    _displayInitialized = false;
    _lastDisplayUpdate = 0;
    _lastPositionBroadcast = 0;
    _lastRangeProcess = 0;
    _newData = false;
    _response = "";
    trackedTagCount = 0;
    
    // Initialize anchor configurations
    for (int i = 0; i < MAX_ANCHORS; i++) {
        _otherAnchorPositions[i][0] = 0.0;
        _otherAnchorPositions[i][1] = 0.0;
        _anchorConfigured[i] = false;
    }
    
    // Initialize tracked tags
    for (int i = 0; i < MAX_TRACKED_TAGS; i++) {
        _trackedTags[i].tagID = -1;
        _trackedTags[i].x = 0.0;
        _trackedTags[i].y = 0.0;
        _trackedTags[i].lastSeen = 0;
        _trackedTags[i].active = false;
        _trackedTags[i].positionValid = false;
        for (int j = 0; j < 8; j++) {
            _trackedTags[i].distanceToAnchor[j] = 0.0;
        }
    }
    
    // Initialize hardware immediately
    initializeHardware();
}

UWBAnchor::~UWBAnchor() {
    // Free allocated memory for display
    if (_display != nullptr) {
        delete _display;
        _display = nullptr;
    }
}

void UWBAnchor::initializeHardware() {
    // Initialize reset pin
    pinMode(RESET_PIN, OUTPUT);
    digitalWrite(RESET_PIN, HIGH);
    
    // Initialize Serial2 for UWB communication
    Serial2.begin(115200, SERIAL_8N1, IO_RXD2, IO_TXD2);
    
    // Initialize I2C
    Wire.begin(I2C_SDA, I2C_SCL);
    
    delay(500);
    
    // Initialize display
    _display = new Adafruit_SSD1306(128, 64, &Wire, -1);
    if (_display->begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        _displayInitialized = true;
        _display->clearDisplay();
        _display->setTextColor(SSD1306_WHITE);
        
        // Show initialization message
        _display->setTextSize(1);
        _display->setCursor(0, 0);
        _display->println(F("UWB ANCHOR"));
        _display->setCursor(0, 15);
        _display->print(F("Type: "));
        switch(anchorType) {
            case GENERAL:
                _display->println(F("GENERAL"));
                break;
            case DATA_LOGGER:
                _display->println(F("DATA_LOGGER"));
                break;
            case POSITION_SERVER:
                _display->println(F("POS_SERVER"));
                break;
        }
        _display->setCursor(0, 30);
        _display->println(F("Initializing..."));
        _display->display();
    }
    
    delay(1000);
    
    // Configure UWB module
    configureUWBModule();
}

void UWBAnchor::configureUWBModule() {
    Serial2.println("AT");
    
    sendCommand("AT?", 500, false);
    sendCommand("AT+RESTORE", 1000, false);
    
    // Configure as anchor (role=1)
    String cfg = "AT+SETCFG=" + String(_anchorNumber) + ",1,1,1";
    sendCommand(cfg, 500, false);
    
    // Set capacity
    String cap = "AT+SETCAP=" + String(_totalTags) + ",10,1";
    sendCommand(cap, 500, false);
    
    sendCommand("AT+SETRPT=1", 500, false);
    sendCommand("AT+SAVE", 500, false);
    sendCommand("AT+RESTART", 1000, false);
    
    if (_displayInitialized) {
        _display->clearDisplay();
        _display->setTextSize(1);
        _display->setCursor(0, 0);
        _display->print(F("ANCHOR "));
        _display->println(_anchorNumber);
        _display->setCursor(0, 15);
        _display->print(F("Type: "));
        switch(anchorType) {
            case GENERAL:
                _display->println(F("GENERAL"));
                break;
            case DATA_LOGGER:
                _display->println(F("DATA_LOGGER"));
                break;
            case POSITION_SERVER:
                _display->println(F("POS_SERVER"));
                break;
        }
        _display->setCursor(0, 30);
        _display->println(F("Ready"));
        _display->display();
    }
    
    delay(1000);
}

void UWBAnchor::setAnchorNumber(int anchorNum) {
    _anchorNumber = anchorNum;
}

void UWBAnchor::setAnchorPosition(float x, float y) {
    _anchorPosition[0] = x;
    _anchorPosition[1] = y;
}

void UWBAnchor::refreshRate(unsigned long rate) {
    _refreshRate = rate;
}

void UWBAnchor::totalTags(int count) {
    _totalTags = count;
}

void UWBAnchor::setOtherAnchor(int anchorID, float x, float y) {
    if (anchorID >= 0 && anchorID < MAX_ANCHORS) {
        _otherAnchorPositions[anchorID][0] = x;
        _otherAnchorPositions[anchorID][1] = y;
        _anchorConfigured[anchorID] = true;
    }
}

void UWBAnchor::update() {
    // Read data from UWB module
    readUWBData();
    
    // Process based on anchor type
    switch(anchorType) {
        case GENERAL:
            processGeneralAnchor();
            break;
        case DATA_LOGGER:
            processDataLogger();
            break;
        case POSITION_SERVER:
            processPositionServer();
            break;
    }
    
    // Update display
    if (millis() - _lastDisplayUpdate > _refreshRate) {
        updateDisplay();
        _lastDisplayUpdate = millis();
    }
}

void UWBAnchor::readUWBData() {
    while (Serial2.available() > 0) {
        char c = Serial2.read();
        
        if (c == '\r') {
            continue;
        } else if (c == '\n') {
            if (_response.length() > 0) {
                parseRangeData(_response);
                _response = "";
            }
        } else {
            _response += c;
        }
    }
}

void UWBAnchor::parseRangeData(String data) {
    // Look for "AT+RANGE=" in the response
    if (data.startsWith("AT+RANGE=")) {
        _newData = true;
        
        // Extract tag ID
        int tidStart = data.indexOf("tid:");
        if (tidStart >= 0) {
            int tidEnd = data.indexOf(",", tidStart);
            if (tidEnd > tidStart) {
                String tidStr = data.substring(tidStart + 4, tidEnd);
                int tagID = tidStr.toInt();
                
                // Update tag last seen
                updateTagLastSeen(tagID);
                
                // For Position Server, extract and store range data
                if (anchorType == POSITION_SERVER) {
                    // Extract range data for position calculation
                    int rangeStart = data.indexOf("range:(");
                    if (rangeStart >= 0) {
                        int rangeEnd = data.indexOf(")", rangeStart);
                        if (rangeEnd > rangeStart) {
                            String rangeValues = data.substring(rangeStart + 7, rangeEnd);
                            
                            // Parse range values and store in tracked tag
                            TrackedTag* tag = getTrackedTag(tagID);
                            if (tag != nullptr) {
                                // Parse and store distances to each anchor
                                int valueIndex = 0;
                                int startIndex = 0;
                                int commaIndex = 0;
                                
                                while (valueIndex < 8 && startIndex < rangeValues.length()) {
                                    commaIndex = rangeValues.indexOf(',', startIndex);
                                    if (commaIndex == -1) {
                                        commaIndex = rangeValues.length();
                                    }
                                    
                                    String valueStr = rangeValues.substring(startIndex, commaIndex);
                                    tag->distanceToAnchor[valueIndex] = valueStr.toFloat();
                                    
                                    valueIndex++;
                                    startIndex = commaIndex + 1;
                                }
                                
                                // Mark as active and calculate position
                                tag->active = true;
                                calculateTagPosition(tag - _trackedTags);
                            }
                        }
                    }
                }
                
                // For Data Logger, forward to Serial
                if (anchorType == DATA_LOGGER) {
                    Serial.println(data);
                }
            }
        }
    }
}

void UWBAnchor::processGeneralAnchor() {
    // Basic anchor - just receive and display data
    // No additional processing needed
}

void UWBAnchor::processDataLogger() {
    // Data Logger - forward all range data to Serial
    // Processing is done in parseRangeData
}

void UWBAnchor::processPositionServer() {
    // Position Server - calculate positions and broadcast
    if (millis() - _lastPositionBroadcast > 500) { // Broadcast every 500ms
        broadcastAllPositions();
        _lastPositionBroadcast = millis();
    }
    
    // Clean up inactive tags
    for (int i = 0; i < MAX_TRACKED_TAGS; i++) {
        if (_trackedTags[i].active && 
            (millis() - _trackedTags[i].lastSeen > 5000)) { // 5 second timeout
            _trackedTags[i].active = false;
            _trackedTags[i].positionValid = false;
            trackedTagCount--;
        }
    }
}

TrackedTag* UWBAnchor::getTrackedTag(int tagID) {
    // First, look for existing tag
    for (int i = 0; i < MAX_TRACKED_TAGS; i++) {
        if (_trackedTags[i].tagID == tagID && _trackedTags[i].active) {
            return &_trackedTags[i];
        }
    }
    
    // If not found, find an empty slot
    for (int i = 0; i < MAX_TRACKED_TAGS; i++) {
        if (!_trackedTags[i].active) {
            _trackedTags[i].tagID = tagID;
            _trackedTags[i].active = true;
            _trackedTags[i].positionValid = false;
            trackedTagCount++;
            return &_trackedTags[i];
        }
    }
    
    return nullptr; // No available slots
}

void UWBAnchor::updateTagLastSeen(int tagID) {
    TrackedTag* tag = getTrackedTag(tagID);
    if (tag != nullptr) {
        tag->lastSeen = millis();
    }
}

void UWBAnchor::calculateTagPosition(int tagIndex) {
    if (tagIndex < 0 || tagIndex >= MAX_TRACKED_TAGS) return;
    
    TrackedTag* tag = &_trackedTags[tagIndex];
    
    // Find all anchors with valid distances
    int anchorCount = 0;
    int validAnchors[MAX_ANCHORS];
    float distances[MAX_ANCHORS];
    float anchorX[MAX_ANCHORS], anchorY[MAX_ANCHORS];
    
    for (int i = 0; i < MAX_ANCHORS; i++) {
        if (_anchorConfigured[i] && tag->distanceToAnchor[i] > 0) {
            validAnchors[anchorCount] = i;
            distances[anchorCount] = tag->distanceToAnchor[i];
            anchorX[anchorCount] = _otherAnchorPositions[i][0];
            anchorY[anchorCount] = _otherAnchorPositions[i][1];
            anchorCount++;
        }
    }
    
    // Need at least 3 anchors for triangulation
    if (anchorCount >= 3) {
        // Calculate all possible triangle combinations for triangulation
        int combinationCount = 0;
        float posX[10] = {0}; // Max 10 possible triangles
        float posY[10] = {0};
        bool valid[10] = {false};
        
        // Try all triangle combinations
        for (int i = 0; i < anchorCount - 2; i++) {
            for (int j = i + 1; j < anchorCount - 1; j++) {
                for (int k = j + 1; k < anchorCount; k++) {
                    if (combinationCount >= 10) break; // Safety limit
                    
                    // Use triangulation algorithm
                    float r1 = distances[i] / 100.0;
                    float r2 = distances[j] / 100.0;
                    float r3 = distances[k] / 100.0;
                    
                    float x1 = anchorX[i] / 100.0;
                    float y1_pos = anchorY[i] / 100.0;
                    float x2 = anchorX[j] / 100.0;
                    float y2 = anchorY[j] / 100.0;
                    float x3 = anchorX[k] / 100.0;
                    float y3 = anchorY[k] / 100.0;
                    
                    float A = 2 * (x2 - x1);
                    float B = 2 * (y2 - y1_pos);
                    float C = r1 * r1 - r2 * r2 - x1 * x1 + x2 * x2 - y1_pos * y1_pos + y2 * y2;
                    float D = 2 * (x3 - x2);
                    float E = 2 * (y3 - y2);
                    float F = r2 * r2 - r3 * r3 - x2 * x2 + x3 * x3 - y2 * y2 + y3 * y3;
                    
                    float denominator = (A * E - B * D);
                    if (std::abs(denominator) > 0.000001) {
                        posX[combinationCount] = (C * E - F * B) / denominator;
                        posY[combinationCount] = (A * F - C * D) / denominator;
                        valid[combinationCount] = true;
                        combinationCount++;
                    }
                }
            }
        }
        
        // Average all valid positions
        float x_sum = 0.0;
        float y_sum = 0.0;
        int validCount = 0;
        
        for (int i = 0; i < combinationCount; i++) {
            if (valid[i]) {
                x_sum += posX[i];
                y_sum += posY[i];
                validCount++;
            }
        }
        
        if (validCount > 0) {
            // Convert back to cm
            tag->x = (x_sum / validCount) * 100.0;
            tag->y = (y_sum / validCount) * 100.0;
            tag->positionValid = true;
        }
    }
}

void UWBAnchor::broadcastAllPositions() {
    String positionData = "ALLPOS:";
    int activeCount = 0;
    
    for (int i = 0; i < MAX_TRACKED_TAGS; i++) {
        if (_trackedTags[i].active && _trackedTags[i].positionValid) {
            if (activeCount > 0) positionData += ":";
            positionData += String(_trackedTags[i].tagID) + ":" + 
                           String(_trackedTags[i].x, 1) + ":" + 
                           String(_trackedTags[i].y, 1);
            activeCount++;
        }
    }
    
    if (activeCount > 0) {
        String command = "AT+DATA=" + String(positionData.length()) + "," + positionData;
        sendCommand(command, 100, false);
    }
}

void UWBAnchor::updateDisplay() {
    if (!_displayInitialized) return;
    
    _display->clearDisplay();
    
    // Display header
    _display->setTextSize(1);
    _display->setCursor(0, 0);
    _display->print(F("ANCHOR "));
    _display->println(_anchorNumber);
    _display->drawLine(0, 9, 128, 9, SSD1306_WHITE);
    
    // Display type
    _display->setCursor(0, 12);
    _display->print(F("Type: "));
    switch(anchorType) {
        case GENERAL:
            _display->println(F("GENERAL"));
            break;
        case DATA_LOGGER:
            _display->println(F("DATA_LOG"));
            break;
        case POSITION_SERVER:
            _display->println(F("POS_SVR"));
            break;
    }
    
    // Display position
    _display->setCursor(0, 24);
    _display->print(F("Pos: ("));
    _display->print(_anchorPosition[0], 0);
    _display->print(F(","));
    _display->print(_anchorPosition[1], 0);
    _display->println(F(")"));
    
    // Type-specific display
    if (anchorType == POSITION_SERVER) {
        _display->setCursor(0, 36);
        _display->print(F("Tags: "));
        _display->println(trackedTagCount);
        
        // Show some active tags
        _display->setCursor(0, 48);
        int shown = 0;
        for (int i = 0; i < MAX_TRACKED_TAGS && shown < 3; i++) {
            if (_trackedTags[i].active) {
                if (shown > 0) _display->print(F(" "));
                _display->print(F("T"));
                _display->print(_trackedTags[i].tagID);
                shown++;
            }
        }
    } else {
        _display->setCursor(0, 36);
        _display->println(F("Listening..."));
        
        if (_newData) {
            _display->setCursor(0, 48);
            _display->println(F("Data received"));
            _newData = false;
        }
    }
    
    _display->display();
}

// Data access methods
int UWBAnchor::getTrackedTagCount() {
    return trackedTagCount;
}

float UWBAnchor::getTagX(int tagID) {
    for (int i = 0; i < MAX_TRACKED_TAGS; i++) {
        if (_trackedTags[i].tagID == tagID && _trackedTags[i].active && _trackedTags[i].positionValid) {
            return _trackedTags[i].x;
        }
    }
    return 0.0;
}

float UWBAnchor::getTagY(int tagID) {
    for (int i = 0; i < MAX_TRACKED_TAGS; i++) {
        if (_trackedTags[i].tagID == tagID && _trackedTags[i].active && _trackedTags[i].positionValid) {
            return _trackedTags[i].y;
        }
    }
    return 0.0;
}

bool UWBAnchor::isTagActive(int tagID) {
    for (int i = 0; i < MAX_TRACKED_TAGS; i++) {
        if (_trackedTags[i].tagID == tagID && _trackedTags[i].active) {
            return true;
        }
    }
    return false;
}

unsigned long UWBAnchor::getTagLastSeen(int tagID) {
    for (int i = 0; i < MAX_TRACKED_TAGS; i++) {
        if (_trackedTags[i].tagID == tagID && _trackedTags[i].active) {
            return _trackedTags[i].lastSeen;
        }
    }
    return 0;
}

String UWBAnchor::sendCommand(String command, int timeout, bool debug) {
    String response = "";
    
    Serial2.println(command);
    
    long int time = millis();
    
    while ((time + timeout) > millis()) {
        while (Serial2.available()) {
            char c = Serial2.read();
            response += c;
        }
    }
    
    return response;
}