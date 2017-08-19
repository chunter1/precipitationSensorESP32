#include "DataPort.h"

DataPort::DataPort() : m_server(0) {
}

void DataPort::Begin(uint port) {
  m_enabled = true;
  m_port = port;

  m_server = WiFiServer(port);
  m_server.begin();
  m_server.setNoDelay(true);
}

uint DataPort::GetPort() {
  return m_port;
}

bool DataPort::IsEnabled() {
  return m_enabled;
}

void DataPort::AddPayload(String payload) {
  if (m_enabled && m_queue.Count() <= 10) {
    m_queue.Push(payload);
  }
}

void DataPort::Dispatch(String data) {
  if (m_enabled) {
    for (byte i = 0; i < m_clients.size(); i++) {
      m_clients[i].println(data);
    }
  }
}

bool DataPort::Handle(CommandCallbackType* commandCallback) {
  bool result = false;

  if (m_enabled) {
    if (m_server.hasClient()) {
      WiFiClient client = m_server.available();
      client.setNoDelay(true);
      m_clients.push_back(client);

      String result = commandCallback("version");
      if (result.length() > 0) {
        client.println(result);
      }
      Serial.println("Port " + String(m_port) + ": Client connected");
    }

    m_clients.erase(std::remove_if(m_clients.begin(), m_clients.end(), [=](WiFiClient c) {
      if (!c.connected()) {
        Serial.println("Port " + String(m_port) + ": Client disconnected");
        return true;
      }
      else {
        return false;
      }
    }), m_clients.end());

    for (unsigned idx = 0; idx < m_clients.size(); idx++) {
      if (m_clients[idx].available()) {
        char buffer[1024];
        int size = m_clients[idx].available();
        m_clients[idx].readBytes(buffer, size);
        buffer[size] = 0;
        String request = buffer;

        if (commandCallback) {
          String result = commandCallback(request);
          if (result.length() > 0) {
            m_clients[idx].println(result);
          }
        }
      }
    }

    while (!m_queue.IsEmpty()) {
      String pl = m_queue.Pop();
      Dispatch(pl);
    }

  }

  return result;
}
