#include "esphome.h"

#define CHECK_BIT(var, pos) (((var) >> (pos)) & 1)

class LD2412 : public PollingComponent, public UARTDevice
{
public:
  LD2412(UARTComponent *parent) : UARTDevice(parent) {}

  BinarySensor *hasTarget = new BinarySensor();
  BinarySensor *hasMovingTarget = new BinarySensor();
  BinarySensor *hasStillTarget = new BinarySensor();
  //BinarySensor *lastCommandSuccess = new BinarySensor();
  Sensor *movingTargetDistance = new Sensor();
  Sensor *movingTargetEnergy = new Sensor();
  Sensor *stillTargetDistance = new Sensor();
  Sensor *stillTargetEnergy = new Sensor();
  //Sensor *detectDistance = new Sensor();

  int movingSensitivities[14] = {0};
  int stillSensitivities[14] = {0};

  long lastPeriodicMillis = millis();

  /* verified with LD2412 */
  void sendCommand(char *commandStr, char *commandValue, int commandValueLen)
  {
    //lastCommandSuccess->publish_state(false);
    // frame start bytes
    write_byte(0xFD);
    write_byte(0xFC);
    write_byte(0xFB);
    write_byte(0xFA);
    // length bytes
    int len = 2;
    if (commandValue != nullptr)
      len += commandValueLen;
    write_byte(lowByte(len));
    write_byte(highByte(len));
    // command string bytes
    write_byte(commandStr[0]);
    write_byte(commandStr[1]);
    // command value bytes
    if (commandValue != nullptr)
    {
      for (int i = 0; i < commandValueLen; i++)
      {
        write_byte(commandValue[i]);
      }
    }
    // frame end bytes
    write_byte(0x04);
    write_byte(0x03);
    write_byte(0x02);
    write_byte(0x01);
    delay(50);
  }

  int twoByteToInt(char firstByte, char secondByte)
  {
    return (int16_t)(secondByte << 8) + firstByte;
  }

  void handlePeriodicData(char *buffer, int len)
  {
    if (len < 12)
      return; // 4 frame start bytes + 2 length bytes + 1 data end byte + 1 crc byte + 4 frame end bytes
    if (buffer[0] != 0xF4 || buffer[1] != 0xF3 || buffer[2] != 0xF2 || buffer[3] != 0xF1)
      return; // check 4 frame start bytes
    if (buffer[7] != 0xAA) 
      return; // data head=0xAA
    if (buffer[len - 6] != 0x55)
      return; // data end=0x55

    /* 
      that is somehow not just 0x00, but also other values. Should be 0x00 according to the docs, 
      so I'm ignoring this at the moment
      
    if (buffer[len - 5] != 0x00)
      return; // datacrc=0x00
    */

    /*
      Reduce data update rate to prevent home assistant database size glow fast
      only enter every 500ms
    */
    long currentMillis = millis();
    if (currentMillis - lastPeriodicMillis < 1000)
      return;
    lastPeriodicMillis = currentMillis;

    /*
      Data Type: 7th byte
      0x01: Engineering mode
      0x02: Normal mode
    */
    bool engineering_mode = buffer[6] == 0x01;
    /*
      Target states: 9th byte
      0x00 = No target
      0x01 = Moving targets
      0x02 = Still targets
      0x03 = Moving+Still targets
    */
    char stateByte = buffer[8];
    hasTarget->publish_state(stateByte != 0x00);

    if (stateByte & 0x01) {
      // moving
    }
    if (stateByte & 0x02) {
      // stationary
    }

    hasMovingTarget->publish_state(CHECK_BIT(stateByte, 0));
    hasStillTarget->publish_state(CHECK_BIT(stateByte, 1));
    
    /*
      Moving target distance: 10~11th bytes
      Moving target energy: 12th byte
      Still target distance: 13~14th bytes
      Still target energy: 15th byte
      Detect distance: 16~17th bytes (doesn't exist anymore... well never existed)
    */
    int newMovingTargetDistance = twoByteToInt(buffer[9], buffer[10]);
    if (movingTargetDistance->get_state() != newMovingTargetDistance)
      movingTargetDistance->publish_state(newMovingTargetDistance);

    int newMovingTargetEnergy = buffer[11];
    if (movingTargetEnergy->get_state() != newMovingTargetEnergy)
      movingTargetEnergy->publish_state(newMovingTargetEnergy);
    
    int newStillTargetDistance = twoByteToInt(buffer[12], buffer[13]);
    if (stillTargetDistance->get_state() != newStillTargetDistance)
      stillTargetDistance->publish_state(newStillTargetDistance);
    
    int newStillTargetEnergy = buffer[14];
    if (stillTargetEnergy->get_state() != newStillTargetEnergy)
      stillTargetEnergy->publish_state(newStillTargetEnergy);
    
    /* this isn't documented, but neither it is on the LD2410, so it shouldnt be there in the first place...
    int newDetectDistance = twoByteToInt(buffer[15], buffer[16]);
    if (detectDistance->get_state() != newDetectDistance)
      detectDistance->publish_state(newDetectDistance);
    */

    if (engineering_mode)
    { // engineering mode
      // TODO
      
    }
  }

  void handleACKData(char *buffer, int len)
  {
    if (len < 10)
      return;
    if (buffer[0] != 0xFD || buffer[1] != 0xFC || buffer[2] != 0xFB || buffer[3] != 0xFA)
      return; // check 4 frame start bytes
    if (buffer[7] != 0x01)
      return;
    if (twoByteToInt(buffer[8], buffer[9]) != 0x00)
    {
      //lastCommandSuccess->publish_state(false);
      return;
    }
    //lastCommandSuccess->publish_state(true);
    /* disabled for LD2412 at the moment... TODO
    switch (buffer[6])
    {
    case 0x61: // Query parameters response
    {
      if (buffer[10] != 0xAA)
        return; // value head=0xAA
      //  Moving distance range: 13th byte
      //  Still distance range: 14th byte
      maxMovingDistanceRange->publish_state(buffer[12]);
      maxStillDistanceRange->publish_state(buffer[13]);
      
      //  Moving Sensitivities: 15~23th bytes
      //  Still Sensitivities: 24~32th bytes
      for (int i = 0; i < 9; i++)
      {
        movingSensitivities[i] = buffer[14 + i];
      }
      for (int i = 0; i < 9; i++)
      {
        stillSensitivities[i] = buffer[23 + i];
      }
      //  None Duration: 33~34th bytes
      noneDuration->publish_state(twoByteToInt(buffer[32], buffer[33]));
    }
    break;
    default:
      break;
    }
    */
  }

  void readline(int readch, char *buffer, int len)
  {
    static int pos = 0;

    if (readch >= 0)
    {
      if (pos < len - 1)
      {
        buffer[pos++] = readch;
        buffer[pos] = 0;
      }
      else
      {
        pos = 0;
      }
      if (pos >= 4)
      {
        if (buffer[pos - 4] == 0xF8 && buffer[pos - 3] == 0xF7 && buffer[pos - 2] == 0xF6 && buffer[pos - 1] == 0xF5)
        {
          handlePeriodicData(buffer, pos);
          pos = 0; // Reset position index ready for next time
        }
        else if (buffer[pos - 4] == 0x04 && buffer[pos - 3] == 0x03 && buffer[pos - 2] == 0x02 && buffer[pos - 1] == 0x01)
        {
          handleACKData(buffer, pos);
          pos = 0; // Reset position index ready for next time
        }
      }
    }
    return;
  }

  /* verified with LD2412 */
  void setConfigMode(bool enable)
  {
    char cmd[2] = {enable ? 0xFF : 0xFE, 0x00};
    char value[2] = {0x01, 0x00};
    sendCommand(cmd, enable ? value : nullptr, 2);
  }

  /* this is different! */
  void queryParameters()
  {
    char cmd_query[2] = {0x61, 0x00};
    sendCommand(cmd_query, nullptr, 0);
  }

  void setup() override
  {
    set_update_interval(15000);
  }

  void loop() override
  {
    const int max_line_length = 80;
    static char buffer[max_line_length];
    while (available())
    {
      readline(read(), buffer, max_line_length);
    }
  }

  /* verified with LD2412 */
  void setEngineeringMode(bool enable)
  {
    char cmd[2] = {enable ? 0x62 : 0x63, 0x00};
    sendCommand(cmd, nullptr, 0);
  }

  /* verified with LD2412 */
  void factoryReset()
  {
    char cmd[2] = {0xA2, 0x00};
    sendCommand(cmd, nullptr, 0);
  }

  /* verified with LD2412 */
  void reboot()
  {
    char cmd[2] = {0xA3, 0x00};
    sendCommand(cmd, nullptr, 0);
    // not need to exit config mode because the ld2410 will reboot automatically
  }

  /* verified with LD2412*/
  void setBaudrate(int index)
  {
    char cmd[2] = {0xA1, 0x00};
    char value[2] = {index, 0x00};
    sendCommand(cmd, value, 2);
  }

  void update()
  {
  }
};
