#include "Signals.h"

Signals::Signals( signalClearFuncp onSignalClear ) : onSignalClearHandler( onSignalClear ) {}

void Signals::fire(signal s, milliseconds timeout /*=1000*/) {   
  this->fire(s, timeout, NULL, NULL);
}

void Signals::fire(signal s, milliseconds timeout, progressFuncp progressHandler) {   
  this->fire(s,timeout, progressHandler, NULL);
}

void Signals::fire(signal s, milliseconds timeout, funcp timeoutHandler) {
  this->fire(s,timeout,NULL, timeoutHandler);
}

void Signals::fire(signal s, milliseconds timeout, progressFuncp progressHandler, funcp timeoutHandler) {
  clear(s);
  SignalSpec spec = {.timeout = timeout, .onProgress=progressHandler, .onTimeout=timeoutHandler};
  triggeredSignals.insert(std::pair<signal,std::pair<milliseconds,SignalSpec>>(s, std::pair<milliseconds,SignalSpec>(millis(), spec)));  
  char strStatus[100]; 
  sprintf(strStatus, "Signal %d fired", s);
  MyDebug::printDebug(strStatus);    
}
    
void Signals::clear(signal s) {
  if ( triggeredSignals.find(s) != triggeredSignals.end() ) {
    triggeredSignals.erase(s);
    char strStatus[100]; 
    sprintf(strStatus, "Signal %d cleared", s);
    MyDebug::printDebug(strStatus);
    if ( onSignalClearHandler != NULL ) {
      onSignalClearHandler(s);
    }
  }
}
    
bool Signals::isTriggered(signal s) {
  return !triggeredSignals.empty() && 
         triggeredSignals.find(s) != triggeredSignals.end();   
}
    
bool Signals::noneTriggered() {
  std::map<signal, std::pair<milliseconds, SignalSpec>>::iterator it;
  for ( it=this->triggeredSignals.begin(); it != this->triggeredSignals.end(); it++ ) {
    std::pair<milliseconds,SignalSpec> props = it->second;
    milliseconds triggerStart = props.first;
    SignalSpec spec = props.second; 
    if ( spec.timeout != 0 ) return false; // endless triggers are ignored by this method
  }
  
  return true;
}
    
bool Signals::anyTriggered() {
  std::map<signal, std::pair<milliseconds, SignalSpec>>::iterator it;
  for ( it=this->triggeredSignals.begin(); it != this->triggeredSignals.end(); it++ ) {
    std::pair<milliseconds,SignalSpec> props = it->second;
    milliseconds triggerStart = props.first;
    SignalSpec spec = props.second; 
    if ( spec.timeout != 0 ) return true; // endless triggers are ignored by this method
  }  
  
  return false;
}

bool Signals::anyTriggeredExcept(signal s) {
  std::map<signal, std::pair<milliseconds, SignalSpec>>::iterator it;
  for ( it=this->triggeredSignals.begin(); it != this->triggeredSignals.end(); it++ ) {
    std::pair<milliseconds,SignalSpec> props = it->second;
    milliseconds triggerStart = props.first;
    SignalSpec spec = props.second; 
    if ( spec.timeout != 0 && it->first != s ) return true; // endless triggers are ignored by this method
  }  
  
  return false;  
}
    
void Signals::processSignals() {
  milliseconds currentTime = millis();
  std::map<signal, std::pair<milliseconds, SignalSpec>>::iterator it;
  
  for ( it=this->triggeredSignals.begin(); it != this->triggeredSignals.end(); it++ ) {
    std::pair<milliseconds,SignalSpec> props = it->second;
    milliseconds triggerStart = props.first;
    SignalSpec spec = props.second;

    // signals with 0 timeout will are going to be triggered forever
    if ( spec.timeout != 0 && spec.timeout <= currentTime - triggerStart ) {       
      char strStatus[100]; 
      sprintf(strStatus, "Signal %d timed out, after: %d ms", it->first, spec.timeout);
      MyDebug::printDebug(strStatus);
      
      if ( spec.onTimeout != NULL ) {
        MyDebug::printDebug("Call signal timeout handler");
        spec.onTimeout();
      }
      this->triggeredSignals.erase(it->first);
    } else {
      if ( spec.onProgress != NULL ) {
        spec.onProgress(spec.timeout, currentTime - triggerStart);  
      }
    }
  }
}
