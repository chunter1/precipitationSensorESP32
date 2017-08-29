#ifndef __CONNECTIONKEEPER__h
#define __CONNECTIONKEEPER__h

#include "Arduino.h"

class ConnectionKeeper {
public:
  typedef void CriticalActionCallbackType(bool);
  void Begin(CriticalActionCallbackType *criticalActionCallback);
  void Handle();
  bool IsConnected();

private:
  CriticalActionCallbackType *m_criticalActionCallback;
  bool m_isConnected;

};

#endif
