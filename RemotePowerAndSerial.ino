/****************************************************************************
  This code merges functionality from two ESP8266 examples.
  The remote-serial-over-netcat one is by far the most complex,
  and is originally called:

      WiFiTelnetToSerial
      Example Transparent UART to Telnet Server for esp8266

      Copyright (c) 2015 Hristo Gochkov. All rights reserved.
      This file is part of the ESP8266WiFi library for Arduino environment.

  The other example is the simple Web server.  Both of these are originally
  licensed as LGPL - so this simple amalgamation of the two also follows the
  LGPL.

  The crude surgery to build this Frankenstein-y thing, was performed by
  Thanassis Tsiodras, in late July 2019.  The details of the HW that this was
  used for are detailed in his blog post here:

     https://www.thanassis.space/remoteserial.html

  containing with pictures and videos of the HW and SW in action, remotely
  commanding an AtomicPI Single Board Computer.
***************************************************************************/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

const char* ssid = "Your SSID goes here";
const char* password = "Your WiFi pass goes here";

// how many clients should be able to telnet to this ESP8266
#define MAX_SRV_CLIENTS 1

// Remote-serial related globals

WiFiServer serverSerial(2345);
WiFiClient serverClients[MAX_SRV_CLIENTS];

// Web-server related globals

ESP8266WebServer serverWeb(80);
const int controlPin = 0;
bool poweredUp = false;

// Web-server callbacks

void handleRoot()
{
    String response ="<html>";
    response += "<head><title>Power up the AtomicPI!</title></head>\n";
    response += "<body><h1>Power up the AtomicPI!</h1><div style=\"font-size:48px\"><p>Current status: ";
    response += poweredUp?"<b><font color=\"red\">ON</font></b>":"OFF";
    if (!poweredUp) {
        response += "<p>Click <a href=\"/poweron\">here</a> to power up";
    } else {
        response += "<p>Click <a href=\"/poweroff\">here</a> to power off";
    } 
    response += "</div>";
    serverWeb.send(200, "text/html", response);
}

void handleNotFound()
{
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += serverWeb.uri();
    message += "\nMethod: ";
    message += (serverWeb.method() == HTTP_GET)?"GET":"POST";
    message += "\nArguments: ";
    message += serverWeb.args();
    message += "\n";
    for (uint8_t i=0; i<serverWeb.args(); i++){
        message += " " + serverWeb.argName(i) + ": " + serverWeb.arg(i) + "\n";
    }
    serverWeb.send(404, "text/plain", message);
}

void setupWifi()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial1.print("\nConnecting to "); Serial1.println(ssid);
    uint8_t i = 0;
    while (WiFi.status() != WL_CONNECTED && i++ < 20) delay(500);
    if(i == 21) {
        Serial1.print("Could not connect to"); Serial1.println(ssid);
        while(1)
            delay(500);
    }
    Serial1.begin(115200);
    Serial1.print("Connected to ");
    Serial1.println(ssid);
    Serial1.print("IP address: ");
    Serial1.println(WiFi.localIP());
}

void setupRemoteSerial()
{
    // Remote serial
    Serial.begin(115200);
    serverSerial.begin();
    serverSerial.setNoDelay(true);

    Serial1.print("Ready! Use 'telnet ");
    Serial1.print(WiFi.localIP());
    Serial1.println(" 2345' to connect");
}

void setupWebServer(void)
{
    pinMode(controlPin, OUTPUT);
    digitalWrite(controlPin, 0);
  
    if (MDNS.begin("esp8266")) {
        Serial.println("MDNS responder started");
    }
  
    serverWeb.on("/", handleRoot);
  
    auto redirect = []() {
        serverWeb.sendHeader("Location", String("http://") + WiFi.localIP() + String("/"), true);
        serverWeb.send(302, "text/plain", "");  
    };
    serverWeb.on("/poweron", [&]() {
        if (!poweredUp) {
            poweredUp = true;
            digitalWrite(controlPin, 1);
        }
        redirect();
    });
    serverWeb.on("/poweroff", [&]() {
        if (poweredUp) {
            poweredUp = false;
            digitalWrite(controlPin, 0);
        }
        redirect();
    });
  
    serverWeb.onNotFound(handleNotFound);
  
    serverWeb.begin();
    Serial.println("HTTP server started");
}

void setup()
{
    // As soon as possible, close the relay!
    pinMode(controlPin, OUTPUT);
    digitalWrite(controlPin, 0);
  
    setupWifi();
    setupWebServer();
    setupRemoteSerial();
}

void loop()
{
    uint8_t i;

    // check if there are any new clients
    if (serverSerial.hasClient()) {
        for(i = 0; i < MAX_SRV_CLIENTS; i++) {
            // find free/disconnected spot
            if (!serverClients[i] || !serverClients[i].connected()) {
                if(serverClients[i]) serverClients[i].stop();
                serverClients[i] = serverSerial.available();
                Serial1.print("New client: "); Serial1.print(i);
                break;
            }
        }
        // no free/disconnected spot so reject
        if ( i == MAX_SRV_CLIENTS) {
            WiFiClient serverClient = serverSerial.available();
            serverClient.stop();
            Serial1.println("Connection rejected ");
        }
    }
    // check clients for data
    for(i = 0; i < MAX_SRV_CLIENTS; i++) {
        if (serverClients[i] && serverClients[i].connected()) {
            if(serverClients[i].available()) {
                // get data from the telnet client and push it to the UART
                while(serverClients[i].available()) Serial.write(serverClients[i].read());
            }
        }
    }
    // check UART for data
    if(Serial.available()) {
        size_t len = Serial.available();
        uint8_t sbuf[len];
        Serial.readBytes(sbuf, len);
        // push UART data to all connected telnet clients
        for(i = 0; i < MAX_SRV_CLIENTS; i++) {
            if (serverClients[i] && serverClients[i].connected()) {
                serverClients[i].write(sbuf, len);
                delay(1);
            }
        }
    }

    serverWeb.handleClient();
}
