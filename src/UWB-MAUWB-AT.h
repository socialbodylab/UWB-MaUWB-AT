#ifndef UWB_MAUWB_AT_H
#define UWB_MAUWB_AT_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Forward declarations
class UWBTAG;
class UWBAnchor;

// Anchor types enumeration
enum AnchorType {
    GENERAL,        // Basic anchor functionality
    DATA_LOGGER,    // Forwards data to computer via serial
    POSITION_SERVER // Calculates and broadcasts all tag positions
};

// Structure for tracked tag data (used by Position Server anchor)
struct TrackedTag {
    int tagID;
    float x, y;
    float distanceToAnchor[8];  // Distance to each anchor (0-7)
    unsigned long lastSeen;
    bool active;
    bool positionValid;
};

class UWBTAG {
public:
    // Constructor
    UWBTAG();
    
    // Destructor
    ~UWBTAG();
    
    // Configuration methods
    void setTagNumber(int tagNum);
    void refreshRate(unsigned long rate);
    void totalTags(int count);
    void anchor0(float x, float y);
    void anchor1(float x, float y);
    void anchor2(float x, float y);
    void anchor3(float x, float y);
    
    // Main update method
    void update();
    
    // Multi-tag methods
    float getTagDistance(int tagID);
    float getTagX(int tagID);
    float getTagY(int tagID);
    bool isTagActive(int tagID);
    int getActiveTagCount();
    
    // Public variables for accessing data
    float positionX;
    float positionY;
    float a0Distance;
    float a1Distance;
    float a2Distance;
    float a3Distance;
    
private:
    // Hardware configuration (hardcoded for ESP32S3)
    static const int RESET_PIN = 16;
    static const int IO_RXD2 = 18;
    static const int IO_TXD2 = 17;
    static const int I2C_SDA = 39;
    static const int I2C_SCL = 38;
    
    // Configuration variables
    int _tagNumber;
    unsigned long _refreshRate;
    int _totalTags;
    float _anchorPositions[4][2]; // [anchor][x,y]
    
    // Display
    Adafruit_SSD1306* _display;
    bool _displayInitialized;
    
    // Position filtering (hardcoded to length 1)
    static const int POSITION_HISTORY_LENGTH = 1;
    float _positionXHistory[POSITION_HISTORY_LENGTH];
    float _positionYHistory[POSITION_HISTORY_LENGTH];
    int _positionHistoryIndex;
    bool _positionHistoryFilled;
    
    // Multi-tag tracking
    static const int MAX_OTHER_TAGS = 64;
    struct OtherTag {
        int tagID;
        float x, y;
        unsigned long lastSeen;
        bool active;
    };
    OtherTag _otherTags[MAX_OTHER_TAGS];
    int _activeOtherTagCount;
    
    // Timing
    unsigned long _lastRangeRequest;
    unsigned long _lastDisplayUpdate;
    
    // Communication
    String _response;
    bool _newData;
    
    // Private methods
    void initializeHardware();
    void configureUWBModule();
    void parseRangeData(String data);
    void parsePositionData(String data);
    void calculatePosition();
    void updateDisplay();
    void readUWBData();
    OtherTag* getOtherTag(int tagID);
    void updateOtherTagLastSeen(int tagID);
    String sendCommand(String command, int timeout = 500, bool debug = false);
};

class UWBAnchor {
public:
    // Constructor
    UWBAnchor(AnchorType type);
    
    // Destructor
    ~UWBAnchor();
    
    // Configuration methods
    void setAnchorNumber(int anchorNum);
    void setAnchorPosition(float x, float y);
    void refreshRate(unsigned long rate);
    void totalTags(int count);
    
    // Anchor network configuration (for Position Server)
    void setOtherAnchor(int anchorID, float x, float y);
    
    // Main update method
    void update();
    
    // Data access methods (for Position Server type)
    int getTrackedTagCount();
    float getTagX(int tagID);
    float getTagY(int tagID);
    bool isTagActive(int tagID);
    unsigned long getTagLastSeen(int tagID);
    
    // Public variables
    AnchorType anchorType;
    int trackedTagCount;
    
private:
    // Hardware configuration (hardcoded for ESP32S3)
    static const int RESET_PIN = 16;
    static const int IO_RXD2 = 18;
    static const int IO_TXD2 = 17;
    static const int I2C_SDA = 39;
    static const int I2C_SCL = 38;
    
    // Configuration variables
    int _anchorNumber;
    float _anchorPosition[2]; // [x,y]
    unsigned long _refreshRate;
    int _totalTags;
    
    // Other anchor positions (for Position Server)
    static const int MAX_ANCHORS = 8;
    float _otherAnchorPositions[MAX_ANCHORS][2];
    bool _anchorConfigured[MAX_ANCHORS];
    
    // Display
    Adafruit_SSD1306* _display;
    bool _displayInitialized;
    
    // Tag tracking (for Position Server)
    static const int MAX_TRACKED_TAGS = 64;
    TrackedTag _trackedTags[MAX_TRACKED_TAGS];
    
    // Timing
    unsigned long _lastDisplayUpdate;
    unsigned long _lastPositionBroadcast;
    unsigned long _lastRangeProcess;
    
    // Communication
    String _response;
    bool _newData;
    
    // Private methods
    void initializeHardware();
    void configureUWBModule();
    void parseRangeData(String data);
    void updateDisplay();
    void readUWBData();
    
    // Type-specific methods
    void processGeneralAnchor();
    void processDataLogger();
    void processPositionServer();
    
    // Position calculation (for Position Server)
    void calculateTagPosition(int tagIndex);
    void broadcastAllPositions();
    TrackedTag* getTrackedTag(int tagID);
    void updateTagLastSeen(int tagID);
    
    String sendCommand(String command, int timeout = 500, bool debug = false);
};

#endif