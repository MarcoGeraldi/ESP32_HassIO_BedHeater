#include "display.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool relayOn = true;

void updateDisplay(double _temperature, int _setpoint, bool _output) {
  // Clear the previous display contents
  display.clearDisplay();

  // Draw a rounded border around the screen
  display.drawRoundRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 10, SSD1306_WHITE);

  // Draw a horizontal line to separate temperature and setpoint sections (lowered position)
  display.drawLine(0, SCREEN_HEIGHT / 2 + 10, SCREEN_WIDTH, SCREEN_HEIGHT / 2 + 10, SSD1306_WHITE);

  // Display the temperature centered with padding above it and larger text
  display.setTextSize(TEMP_TEXT_SIZE);  // Larger text size for temperature
  display.setTextColor(SSD1306_WHITE);

  // Calculate the width of the text to properly center it
  String tempString = String(_temperature, 1);  // Convert temperature to string with 1 decimal place
  int textWidth = tempString.length() * 6 * TEMP_TEXT_SIZE;  // Each character width is approximately 6px in size
  int tempX = 8; //(SCREEN_WIDTH - textWidth) / 2;  // Center horizontally based on text width
  int tempY = (SCREEN_HEIGHT / 2) - 25;        // Adjust vertical position slightly higher
  display.setCursor(tempX, tempY);
  display.print(_temperature, 1);  // Print temperature with 1 decimal place
  
  // Add "°C" after the temperature 
  display.drawCircle(108, 8, 3, SSD1306_WHITE);  // Draw a small circle (3px radius)
  display.setTextSize(2);  // Medium text size for the unit
  display.setCursor(113, 8);  // Position after the circle
  display.print("C");  // Print the unit (Celsius)

  // Display "SET:" in the bottom left corner with smaller text size
  display.setTextSize(SET_LABEL_TEXT_SIZE);  // Text size for "SET:"
  display.setCursor(5, SCREEN_HEIGHT - 18);  // Position at the bottom-left
  display.print("SET:");

  // Display the setpoint in the bottom left corner, below the horizontal line with larger text
  display.setTextSize(SETPOINT_TEXT_SIZE);  // Larger text size for setpoint
  display.setCursor(35, SCREEN_HEIGHT / 2 + 15);  // Position just below the line
  display.print(_setpoint);  // Display setpoint as an integer

  // Add "°C" after the setpoint
  display.drawCircle(65, SCREEN_HEIGHT / 2 + 15, 3, SSD1306_WHITE);  // Draw a small circle (3px radius)

  // Add "C" after the circle to indicate Celsius
  display.setTextSize(2);  // Medium text size for the unit
  display.setCursor(70, SCREEN_HEIGHT / 2 + 15);  // Position after the circle
  display.print("C");  // Print the unit (Celsius)


  // Display relay state (white dot or circle in bottom right)
  if (_output) {
    display.fillCircle(SCREEN_WIDTH - 10, SCREEN_HEIGHT - 10, 5, SSD1306_WHITE);  // Glowing dot effect
  } else {
    display.drawCircle(SCREEN_WIDTH - 10, SCREEN_HEIGHT - 10, 5, SSD1306_WHITE);  // Hollow circle
  }

  // Update the display
  display.display();
}