#ifndef PRINT_DEBUG_H
#define PRINT_DEBUG_H

class MyDebug {
  public:
  static void printDebug(const char *str) {
#ifdef DEBUG    
    Serial.println(str);
#endif
  }
};

#endif
