// Network.h
#ifndef NETWORK_H
#define NETWORK_H

#include "Config.h"
#include "Types.h"

class Network {
private:
  static bool initialized;
  static unsigned long lastWiFiCheck;
  static int apiRetryCount;
  
public:
  static void init();
  static void update();
  static bool isConnected();
  
  // API调用
  static CardInfo getCardInfo(const String& cardUID);
  static bool updateCardBalance(const String& cardUID, float newBalance);
  static bool recordTransaction(const String& cardUID, float amount, 
                               float balanceBefore, const String& packageName);
  
private:
  static void connectWiFi();
  static void checkConnection();
  static String makeHTTPRequest(const String& method, const String& endpoint, const String& data = "");
};

#endif
