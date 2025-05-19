# UWB-MaUWB-AT Library v1.1.0

A simplified Arduino library for the Makerfabs UWB Module with ESP32S3 and STM32 AT Command interface. This library provides an easy-to-use interface for UWB positioning applications with automatic display management and multi-tag tracking.



## Dependencies

This library requires the following libraries (installed automatically via Library Manager):
- **Adafruit GFX Library** - For graphics support
- **Adafruit SSD1306** - For OLED display support

## Quick Start

### Basic TAG Usage
```cpp
#include <UWB-MaUWB-AT.h>

UWBTAG trackTag;

void setup() {
    Serial.begin(115200);
    trackTag.setTagNumber(0);
    trackTag.refreshRate(50);
    trackTag.totalTags(10);
    trackTag.anchor0(0,0);
    trackTag.anchor1(0,600);
    trackTag.anchor2(380,600);
    trackTag.anchor3(380,0);
}

void loop() {
    trackTag.update();
    
    // Access own position
    float x = trackTag.positionX;
    float y = trackTag.positionY;
    
    // Get distance to another tag (requires Position Server anchor)
    if (trackTag.isTagActive(1)) {
        float distance = trackTag.getTagDistance(1);
        Serial.printf("Distance to Tag 1: %.1f cm\n", distance);
    }
}
```

### ANCHOR Usage
```cpp
#include <UWB-MaUWB-AT.h>

// Choose anchor type:
UWBAnchor generalAnchor(GENERAL);           // Basic anchor
UWBAnchor dataLogger(DATA_LOGGER);          // Data forwarding
UWBAnchor positionServer(POSITION_SERVER);  // Multi-tag tracking

void setup() {
    // For Position Server (enables multi-tag features)
    positionServer.setAnchorNumber(0);
    positionServer.setAnchorPosition(0, 0);
    positionServer.setOtherAnchor(1, 0, 600);
    positionServer.setOtherAnchor(2, 380, 600);
    positionServer.setOtherAnchor(3, 380, 0);
}

void loop() {
    positionServer.update();
    
    // Access tracked tags (Position Server only)
    int count = positionServer.getTrackedTagCount();
    if (positionServer.isTagActive(0)) {
        float x = positionServer.getTagX(0);
        float y = positionServer.getTagY(0);
    }
}
```

## Hardware Support

- **Makerfabs UWB Module** with ESP32S3
- **SSD1306 OLED Display** (128x64)
- **At least 3 UWB anchors** for position calculation

## API Reference

### UWBTAG Class

#### Configuration
- `setTagNumber(int)` - Set tag ID (0-63)
- `refreshRate(unsigned long)` - Set update rate in ms
- `totalTags(int)` - Set total tags in system
- `anchor0/1/2/3(float x, float y)` - Set anchor positions

#### Position Access
- `positionX`, `positionY` - Own calculated position
- `a0Distance`, `a1Distance`, etc. - Distances to anchors

#### Multi-Tag Features *(New in v1.1.0)*
- `getTagDistance(int tagID)` - Distance to another tag
- `getTagX(int tagID)`, `getTagY(int tagID)` - Other tag positions  
- `isTagActive(int tagID)` - Check if tag is visible
- `getActiveTagCount()` - Total active tags

### UWBAnchor Class

#### Configuration
- `setAnchorNumber(int)` - Set anchor ID (0-7)
- `setAnchorPosition(float x, float y)` - Set anchor position
- `setOtherAnchor(int id, float x, float y)` - Configure other anchors

#### Position Server Features
- `getTrackedTagCount()` - Number of tracked tags
- `getTagX/Y(int tagID)` - Tag positions
- `isTagActive(int tagID)` - Tag status

## Examples

### Basic Examples
- `UWBTAG_Basic` - Simple tag usage
- `UWBTAG_Position` - Advanced position processing
- `UWBAnchor_General` - Basic anchor
- `UWBAnchor_DataLogger` - Data forwarding

### Advanced Examples *(New in v1.1.0)*
- `UWBTAG_MultiTagDistance` - Inter-tag distance tracking
- `UWBAnchor_PositionServer` - Multi-tag system control

## System Configurations

### Single Tag Tracking (Original)
```
[Anchor 0] ←--→ [Tag] ←--→ [Anchor 1]
     ↑                        ↑
     └────────[Anchor 2]──────┘
```
*Tag calculates own position using anchor distances*

### Multi-Tag Tracking *(New)*
```
[Position Server] ←--→ [Tag 0] [Tag 1] [Tag 2]
      ↑              ↖     ↑     ↑
      └── Broadcasts all tag positions ──┘
```
*Position Server calculates and broadcasts all positions*

## Migration from v1.0.x

Existing code requires **no changes** - all original functionality is preserved:

```cpp
// This continues to work exactly as before
UWBTAG tag;
tag.setTagNumber(0);
tag.update();
float x = tag.positionX;  // Own position calculation unchanged
```

New multi-tag features are additive and optional.

## Applications

- **Asset Tracking** - Monitor equipment in real-time
- **Social Distancing** - Proximity monitoring systems  
- **Robotics** - Coordinate multiple autonomous vehicles
- **Interactive Systems** - Location-based experiences
- **Safety Monitoring** - Track personnel in hazardous areas
- **Research** - Study movement patterns and behaviors

## License

This library is provided as-is for educational and development purposes.

## Version History

- **v1.1.0**: Added multi-tag tracking, Position Server anchor, enhanced TAG APIs
- **v1.0.0**: Initial release with basic positioning functionality

## Support

- Check the [examples](examples/) for common use cases
- Open an issue on GitHub for bugs or feature requests
- See the [Makerfabs documentation](https://www.makerfabs.com/) for hardware details