#ifndef SIGNALS_H
#define SIGNALS_H

#include <M5Stack.h>
#include <set>
#include <map>

#include "debug.h"

typedef unsigned long milliseconds;
typedef unsigned int signal;

using funcp = void(*)();
using signalClearFuncp = void(*)(signal);
using progressFuncp = void(*)(milliseconds,milliseconds);

typedef struct {
  milliseconds timeout;
  progressFuncp onProgress;  
  funcp onTimeout;
} SignalSpec;

class Signals {
  private:
    std::map<signal, std::pair<milliseconds,SignalSpec>> triggeredSignals;
    signalClearFuncp onSignalClearHandler;
    
  public:
    Signals() : onSignalClearHandler(NULL){};
    Signals( signalClearFuncp onSignalClear );
    
    void fire(signal s, milliseconds timeout = 1000);
    void fire(signal s, milliseconds timeout, funcp timeoutHandler);
    void fire(signal s, milliseconds timeout, progressFuncp progressHandler);
    void fire(signal s, milliseconds timeout, progressFuncp progressHandler, funcp timeoutHandler);
    void clear(signal s);
    bool isTriggered(signal s);
    bool noneTriggered();
    bool anyTriggered();
    bool anyTriggeredExcept(signal s);
    void processSignals();
    
};
#endif
