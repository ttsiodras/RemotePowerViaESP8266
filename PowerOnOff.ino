#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

const char* ssid = "Your SSID goes here";
const char* password = "Your WiFi pass goes here";

ESP8266WebServer server(80);

const int controlPin = 0;

bool poweredUp = false;

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
    server.send(200, "text/html", response);
}

void handleNotFound()
{
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET)?"GET":"POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i=0; i<server.args(); i++){
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
}

void setup(void){
    pinMode(controlPin, OUTPUT);
    digitalWrite(controlPin, 0);
    Serial.begin(9600);
    WiFi.begin(ssid, password);
    Serial.println("");
  
    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  
    if (MDNS.begin("esp8266")) {
        Serial.println("MDNS responder started");
    }
  
    server.on("/", handleRoot);
  
    auto redirect = []() {
        server.sendHeader("Location", String("http://") + WiFi.localIP() + String("/"), true);
        server.send(302, "text/plain", "");  
    };
    server.on("/poweron", [&]() {
        if (!poweredUp) {
            poweredUp = true;
            digitalWrite(controlPin, 1);
        }
        redirect();
    });
    server.on("/poweroff", [&]() {
        if (poweredUp) {
            poweredUp = false;
            digitalWrite(controlPin, 0);
        }
        redirect();
    });
  
    server.onNotFound(handleNotFound);
  
    server.begin();
    Serial.println("HTTP server started");
}

void loop(void)
{
    server.handleClient();
}
