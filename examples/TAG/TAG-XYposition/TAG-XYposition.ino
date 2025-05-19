#include <UWB-MaUWB-AT.h>

// Create TAG instance
UWBTAG myTag;

int tagID = 0; // make sure this is unique to each tag
int refreshRate = 100; //how fast the ESP polls the UWB chip (50ms is about as fast as possible)
int maxTags = 5; //setting this lower helps optimize the calculations

void setup() {
    Serial.begin(115200);
    Serial.println("UWB TAG Basic Example");
    
    // Configure the tag
    myTag.setTagNumber(tagID);           // Tag ID 0
    myTag.refreshRate(refreshRate);          // Update every 100ms
    myTag.totalTags(maxTags);             // Total tags in system
    
    // Set anchor positions (in cm)
    myTag.anchor0(0, 0);             // Anchor 0 at origin
    myTag.anchor1(0, 600);           // Anchor 1 
    myTag.anchor2(380, 600);         // Anchor 2
    myTag.anchor3(380, 0);           // Anchor 3
    
    Serial.println("TAG configured and ready");
}

void loop() {
    // Update tag (handles all UWB operations)
    myTag.update();
    
    /* Example: Using x,y position to control an LED on pin 5
     * -----------------------------------------------------
     * // Define the area of interest (a specific zone in your space)
     * const float zoneX = 200; // center X of zone in cm
     * const float zoneY = 300; // center Y of zone in cm
     * const float zoneRadius = 100; // radius of zone in cm
     * 
     * // Calculate distance from tag to zone center
     * float dx = myTag.positionX - zoneX;
     * float dy = myTag.positionY - zoneY;
     * float distanceToZone = sqrt(dx*dx + dy*dy);
     * 
     * // Set up LED pin if not already done in setup()
     * // pinMode(5, OUTPUT);
     * 
     * // Turn LED on when tag is inside the zone, off when outside
     * if(distanceToZone < zoneRadius) {
     *   digitalWrite(5, HIGH); // Turn LED on
     * } else {
     *   digitalWrite(5, LOW);  // Turn LED off
     * }
     * 
     * // Alternative: Brightness based on distance (requires analogWrite)
     * // int brightness = map(constrain(distanceToZone, 0, zoneRadius), 0, zoneRadius, 255, 0);
     * // analogWrite(5, brightness); // LED gets brighter as tag gets closer to zone center
     */

}