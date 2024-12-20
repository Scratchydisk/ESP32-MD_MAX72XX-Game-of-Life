// Use the MD_MAX72XX library to scroll text on the display
// received through the ESP32 WiFi interface.
//
// Demonstrates the use of the callback function to control what
// is scrolled on the display text. User can enter text through
// a web browser and this will display as a scrolling message on
// the display.
//
// IP address for the ESP32 is displayed on the scrolling display
// after startup initialization and connected to the WiFi network.
//
// Connections for ESP32 hardware SPI are:
// Vcc       3.3V - A few matrices seem to work at 3.3V
// GND       GND
// DIN       VSPI_MOSI
// CS or LD  VSPI_CS
// CLK       VSPI_SCK
//

#include <WiFi.h>
#include <WiFiServer.h>
#include <MD_MAX72xx.h>
#include "LedPanel.h"
#include "life.h"

#define PRINT_CALLBACK 0
#define DEBUG 0
#define LED_HEARTBEAT 0

#if DEBUG
#define PRINT(s, v)     \
  {                     \
    Serial.print(F(s)); \
    Serial.print(v);    \
  }
#define PRINTS(s)       \
  {                     \
    Serial.print(F(s)); \
  }
#else
#define PRINT(s, v)
#define PRINTS(s)
#endif

#if LED_HEARTBEAT
#define HB_LED 2
#define HB_LED_TIME 500 // in milliseconds
#endif

// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4

#define SCREEN_DEVICE_WIDTH 4
#define SCREEN_DEVICE_HEIGHT 1

// GPIO pins
#define CLK_PIN 18  // VSPI_SCK
#define DATA_PIN 23 // VSPI_MOSI
#define CS_PIN 5    // VSPI_SS

// SPI hardware interface
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
// Arbitrary pins
// MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// Prototypes
void startNewMessage(const char *msg);


LedPanel lp(mx, SCREEN_DEVICE_WIDTH, SCREEN_DEVICE_HEIGHT);

GameOfLife life(lp.width(), lp.height());

// WiFi login parameters - network name and password
const char ssid[] = "Post_Office_85D1";
const char password[] = "vYT7tPVvr9";

// WiFi Server object and parameters
WiFiServer server(80);

// Global message buffers shared by Wifi and Scrolling functions
const uint8_t MESG_SIZE = 255;
const uint8_t CHAR_SPACING = 1;
const uint8_t SCROLL_DELAY = 75;

char curMessage[MESG_SIZE];
char newMessage[MESG_SIZE];
bool newMessageAvailable = false;

const char WebResponse[] = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n";

const char WebPage[] =
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<title>MajicDesigns Test Page</title>"

    "<script>"
    "strLine = \"\";"

    "function SendText()"
    "{"
    "  nocache = \"/&nocache=\" + Math.random() * 1000000;"
    "  var request = new XMLHttpRequest();"
    "  strLine = \"&MSG=\" + document.getElementById(\"txt_form\").Message.value;"
    "  request.open(\"GET\", strLine + nocache, false);"
    "  request.send(null);"
    "}"
    "</script>"
    "</head>"

    "<body>"
    "<p><b>MD_MAX72xx set message</b></p>"

    "<form id=\"txt_form\" name=\"frmText\">"
    "<label>Msg:<input type=\"text\" name=\"Message\" maxlength=\"255\"></label><br><br>"
    "</form>"
    "<br>"
    "<input type=\"submit\" value=\"Send Text\" onclick=\"SendText()\">"
    "</body>"
    "</html>";

const char *err2Str(wl_status_t code)
{
  switch (code)
  {
  case WL_IDLE_STATUS:
    return ("IDLE");
    break; // WiFi is in process of changing between statuses
  case WL_NO_SSID_AVAIL:
    return ("NO_SSID_AVAIL");
    break; // case configured SSID cannot be reached
  case WL_CONNECTED:
    return ("CONNECTED");
    break; // successful connection is established
  case WL_CONNECT_FAILED:
    return ("CONNECT_FAILED");
    break; // password is incorrect
  case WL_DISCONNECTED:
    return ("CONNECT_FAILED");
    break; // module is not configured in station mode
  default:
    return ("??");
  }
}

uint8_t htoi(char c)
{
  c = toupper(c);
  if ((c >= '0') && (c <= '9'))
    return (c - '0');
  if ((c >= 'A') && (c <= 'F'))
    return (c - 'A' + 0xa);
  return (0);
}

boolean getText(char *szMesg, char *psz, uint8_t len)
{
  boolean isValid = false; // text received flag
  char *pStart, *pEnd;     // pointer to start and end of text

  // get pointer to the beginning of the text
  pStart = strstr(szMesg, "/&MSG=");

  if (pStart != NULL)
  {
    pStart += 6; // skip to start of data
    pEnd = strstr(pStart, "/&");

    if (pEnd != NULL)
    {
      while (pStart != pEnd)
      {
        if ((*pStart == '%') && isxdigit(*(pStart + 1)))
        {
          // replace %xx hex code with the ASCII character
          char c = 0;
          pStart++;
          c += (htoi(*pStart++) << 4);
          c += htoi(*pStart++);
          *psz++ = c;
        }
        else
          *psz++ = *pStart++;
      }

      *psz = '\0'; // terminate the string
      isValid = true;
    }
  }

  return (isValid);
}

void handleWiFi(void)
{
  static enum { S_IDLE,
                S_WAIT_CONN,
                S_READ,
                S_EXTRACT,
                S_RESPONSE,
                S_DISCONN } state = S_IDLE;
  static char szBuf[1024];
  static uint16_t idxBuf = 0;
  static WiFiClient client;
  static uint32_t timeStart;

  switch (state)
  {
  case S_IDLE: // initialize
    PRINTS("\nS_IDLE");
    idxBuf = 0;
    state = S_WAIT_CONN;
    break;

  case S_WAIT_CONN: // waiting for connection
  {
    client = server.accept();
    if (!client)
      break;
    if (!client.connected())
      break;

#if DEBUG
    char szTxt[20];
    sprintf(szTxt, "%d:%d:%d:%d", client.remoteIP()[0], client.remoteIP()[1], client.remoteIP()[2], client.remoteIP()[3]);
    PRINT("\nNew client @ ", szTxt);
#endif

    timeStart = millis();
    state = S_READ;
  }
  break;

  case S_READ: // get the first line of data
    PRINTS("\nS_READ");
    while (client.available())
    {
      char c = client.read();
      if ((c == '\r') || (c == '\n'))
      {
        szBuf[idxBuf] = '\0';
        client.flush();
        PRINT("\nRecv: ", szBuf);
        state = S_EXTRACT;
      }
      else
        szBuf[idxBuf++] = (char)c;
    }
    if (millis() - timeStart > 1000)
    {
      PRINTS("\nWait timeout");
      state = S_DISCONN;
    }
    break;

  case S_EXTRACT: // extract data
    PRINTS("\nS_EXTRACT");
    // Extract the string from the message if there is one
    newMessageAvailable = getText(szBuf, newMessage, MESG_SIZE);
    if (newMessageAvailable)
    {
      startNewMessage(newMessage);
    }
    PRINT("\nNew Msg: ", newMessage);
    state = S_RESPONSE;
    break;

  case S_RESPONSE: // send the response to the client
    PRINTS("\nS_RESPONSE");
    // Return the response to the client (web page)
    client.print(WebResponse);
    client.print(WebPage);
    state = S_DISCONN;
    break;

  case S_DISCONN: // disconnect client
    PRINTS("\nS_DISCONN");
    client.flush();
    client.stop();
    state = S_IDLE;
    break;

  default:
    state = S_IDLE;
  }
}

class ScrollState
{
public:
  enum State
  {
    S_IDLE,
    S_NEXT_CHAR,
    S_SHOW_CHAR,
    S_SHOW_SPACE
  };

  State state = S_IDLE;
  char *messagePtr = nullptr;
  uint16_t curLen = 0;
  uint16_t showLen = 0;
};

ScrollState scrollState;

void scrollDataSink(uint8_t dev, MD_MAX72XX::transformType_t t, uint8_t col)
// Callback function for data that is being scrolled off the display
{
#if PRINT_CALLBACK
  Serial.print("\n cb ");
  Serial.print(dev);
  Serial.print(' ');
  Serial.print(t);
  Serial.print(' ');
  Serial.println(col);
#endif
}

bool messageComplete = false;  // The end of the message is on the display
bool messageDone = false;  // The message has scrolled off the display
int extraScrollColumns = MAX_DEVICES * COL_SIZE; // Width of display

uint8_t scrollDataSource(uint8_t dev, MD_MAX72XX::transformType_t t)
{
  static uint8_t cBuf[8];
  static int remainingScrolls = 0;
  uint8_t colData = 0;

  switch (scrollState.state)
  {
  case ScrollState::S_IDLE:
    if (messageDone)
    {
      return 0;
    }
    if (messageComplete)
    {
      // Keep scrolling empty columns until message is fully off screen
      if (remainingScrolls > 0)
      {
        remainingScrolls--;
        return 0;
      }
      messageDone = true;
      return 0;
    }
    scrollState.messagePtr = curMessage;
    if (newMessageAvailable)
    {
      strcpy(curMessage, newMessage);
      newMessageAvailable = false;
      messageComplete = false;
      messageDone = false;
    }
    scrollState.state = ScrollState::S_NEXT_CHAR;
    break;

  case ScrollState::S_NEXT_CHAR:
    if (*scrollState.messagePtr == '\0')
    {
      scrollState.state = ScrollState::S_IDLE;
      messageComplete = true;
      remainingScrolls = extraScrollColumns;
      return 0;
    }
    else
    {
      scrollState.showLen = mx.getChar(*scrollState.messagePtr++, sizeof(cBuf) / sizeof(cBuf[0]), cBuf);
      scrollState.curLen = 0;
      scrollState.state = ScrollState::S_SHOW_CHAR;
    }
    break;

  case ScrollState::S_SHOW_CHAR: // display the next part of the character
    PRINTS("\nS_SHOW_CHAR");
    colData = cBuf[scrollState.curLen++];
    if (scrollState.curLen < scrollState.showLen)
      break;

    // set up the inter character spacing
    scrollState.showLen = (*scrollState.messagePtr != '\0' ? CHAR_SPACING : (MAX_DEVICES * COL_SIZE) / 2);
    scrollState.curLen = 0;
    scrollState.state = ScrollState::S_SHOW_SPACE;
    // fall through

  case ScrollState::S_SHOW_SPACE: // display inter-character spacing (blank column)
    PRINT("\nS_SHOW_SPACE: ", curLen);
    PRINT("/", showLen);
    scrollState.curLen++;
    if (scrollState.curLen == scrollState.showLen)
      scrollState.state = ScrollState::S_NEXT_CHAR;
    break;

  default:
    scrollState.state = ScrollState::S_IDLE;
  }

  return (colData);
}

bool isScrollingComplete()
{
  return (curMessage[0] == '\0' ||
          (scrollState.messagePtr == curMessage &&
           scrollState.state == ScrollState::S_IDLE));
}

void scrollText(void)
{
  static uint32_t prevTime = 0;

  // Is it time to scroll the text?
  if (millis() - prevTime >= SCROLL_DELAY)
  {
    mx.transform(MD_MAX72XX::TSL); // scroll along - the callback will load all the data
    prevTime = millis();           // starting point for next time
  }
}

void startNewMessage(const char *msg)
{
  strcpy(newMessage, msg);
  newMessageAvailable = true;
  messageComplete = false;
  messageDone = false;
}

void spotRun()
{
  mx.clear();
  for (int y = 0; y < lp.height(); y++)
  {
    for (int x = 0; x < lp.width(); x++)
    {
      lp.drawPoint(x, y, HIGH);
      delay(10);
      lp.drawPoint(x, y, LOW);
    }
  }
}

void drawBorder()
{
  mx.clear();

  lp.drawLine(0, 0, lp.width() - 1, 0);
  lp.drawLine(0, lp.height() - 1, lp.width() - 1, lp.height() - 1);
  lp.drawLine(0, 0, 0, lp.height() - 1);
  lp.drawLine(lp.width() - 1, 0, lp.width() - 1, lp.height() - 1);
}

void identifyPanel()
{
  mx.clear();
  // A dot at 0,0
  lp.drawPoint(0, 0, true);
  // A diagronal at 7,7
  lp.drawPoint(7, 7, true);
  lp.drawPoint(6, 6, true);
  // A corner at 7,0
  lp.drawPoint(7, 0, true);
  lp.drawPoint(6, 0, true);
  lp.drawPoint(7, 1, true);
  // A filled corner at 0,7
  lp.drawPoint(0, 7, true);
  lp.drawPoint(1, 7, true);
  lp.drawPoint(0, 6, true);
  lp.drawPoint(1, 6, true);
}

void startNextGame()
{
  PRINTS("\nstartNextGame");
  int choice = random(100);
  life.resetGenerations();

  if (choice < 40)
  {
    PRINTS(" 40% chance to randomize");
    // 40% chance to randomize
    life.randomize();
  }
  else if (choice < 50)
  {
    PRINTS(" 10% chance to create pulsar");
    // 10% chance to create pulsar
    life.createPulsar(2, 0);
  }
  else if (choice < 60)
  {
    PRINTS(" 10% chance to create glider gun");
    // 10% chance to create glider gun
    life.createGliderGun(0, 0);
  }
  else if (choice < 70)
  {
    // Print Life!
    startNewMessage("Life!");
  }
  else
  {
    PRINTS(" 50% chance to add random gliders and blinkers");
    // 50% chance to add random gliders and blinkers
    life.clear();

    // Add 1-3 gliders
    int numGliders = random(1, 4);
    for (int i = 0; i < numGliders; i++)
    {
      int x = random(0, life.getWidth() - 3);
      int y = random(0, life.getHeight() - 3);
      life.createGlider(x, y);
    }

    // Add 1-4 blinkers
    int numBlinkers = random(1, 5);
    for (int i = 0; i < numBlinkers; i++)
    {
      int x = random(0, life.getWidth() - 3);
      int y = random(0, life.getHeight() - 1);
      life.createBlinker(x, y);
    }
  }
}

void showEndGameEffect()
{
  int effect = random(4);
  switch (effect)
  {
  case 0:
    PRINTS("\nSpiral(true)");
    lp.spiral(true);
    break;
  case 1:
    PRINTS("\nSpiral(false)");
    lp.spiral(false);
    break;
  case 2:
    PRINTS("\nWave");
    lp.wave();
    break;
  case 3:
    PRINTS("\nFlash");
    lp.flash();
    break;
  }
  delay(1000);
}

void drawLifeBoard()
{
  mx.clear();
  // Update LED matrix
  for (int y = 0; y < life.getHeight(); y++)
  {
    for (int x = 0; x < life.getWidth(); x++)
    {
      // Set LED state based on cell state
      lp.drawPoint(x, y, life.getCell(x, y));
    }
  }
}

void setup(void)
{
#if DEBUG
  Serial.begin(115200);
  PRINTS("\n[MD_MAX72XX WiFi Message Display]\nType a message for the scrolling display from your internet browser");
#endif

#if LED_HEARTBEAT
  pinMode(HB_LED, OUTPUT);
  digitalWrite(HB_LED, LOW);
#endif

  // Display initialization
  PRINTS("\nInitializing Display");
  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, 0);
  // Flip the characters horizontally so they display correctly
  // Rotate the entire display 180 degrees to fix the panel order from 1->4 instead of 4->1
  // mx.transform(MD_MAX72XX::TFLR);     // Flip characters horizontally
  // mx.set.setRotation(MD_MAX72XX::MD_ROTATION_180); // Reverse panel order

  mx.setShiftDataInCallback(scrollDataSource);
  mx.setShiftDataOutCallback(scrollDataSink);

  curMessage[0] = newMessage[0] = '\0';

  // Connect to and initialize WiFi network
  PRINT("\nConnecting to ", ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    PRINT("\n", err2Str(WiFi.status()));
    // busy wait delay
    uint32_t t = millis();
    while (millis() - t <= 1000)
      yield();
  }
  PRINTS("\nWiFi connected");

  // Start the server
  PRINTS("\nStarting Server");
  server.begin();

  // Set up first message as the IP address
  sprintf(curMessage, "%d:%d:%d:%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  PRINT("\nAssigned IP ", curMessage);

  startNextGame();
}

void loop(void)
{
#if LED_HEARTBEAT
  static uint32_t timeLast = 0;

  if (millis() - timeLast >= HB_LED_TIME)
  {
    digitalWrite(HB_LED, digitalRead(HB_LED) == LOW ? HIGH : LOW);
    timeLast = millis();
  }
#endif
  handleWiFi();

  static uint32_t lastUpdate = 0;
  if (!messageDone)
  {
    scrollText();
  }
  else
  {
    if (millis() - lastUpdate >= 333)
    {

      drawLifeBoard();

      if (!life.isGameFinished())
      {
        life.computeNextGeneration();
      }
      else
      {
        // Pattern is stable or oscillating
        showEndGameEffect();
        startNextGame();
      }
      lastUpdate = millis();
    }
  }
}