/*--------------------------------------------------------------
  Program:      eth_websrv_SD_image

  Description:  Arduino web server that serves up a basic web
                page that displays an image.

  Hardware:     Arduino Uno and official Arduino Ethernet
                shield. Should work with other Arduinos and
                compatible Ethernet shields.
                2Gb micro SD card formatted FAT16

  Software:     Developed using Arduino 1.0.5 software
                Should be compatible with Arduino 1.0 +

                Requires index.htm, page2.htm and pic.jpg to be
                on the micro SD card in the Ethernet shield
                micro SD card socket.

  References:   - WebServer example by David A. Mellis and
                  modified by Tom Igoe
                - SD card examples by David A. Mellis and
                  Tom Igoe
                - Ethernet library documentation:
                  http://arduino.cc/en/Reference/Ethernet
                - SD Card library documentation:
                  http://arduino.cc/en/Reference/SD

  Date:         7 March 2013
  Modified:     17 June 2013

  Author:       W.A. Smith, http://startingelectronics.com
--------------------------------------------------------------*/

#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <DHT.h>

#define DHTPIN 2     // what pin we're connected to
#define DHTTYPE DHT11   // DHT 11 
DHT dht(DHTPIN, DHTTYPE);

// size of buffer used to capture HTTP requests
#define REQ_BUF_SZ   20

// MAC address from Ethernet shield sticker under board
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 0, 20); // IP address, may need to change depending on network
EthernetServer server(80);  // create a server at port 80
File webFile;
char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
char req_index = 0;              // index into HTTP_req buffer
EthernetClient client;

void setup()
{
    // disable Ethernet chip
    pinMode(10, OUTPUT);
    digitalWrite(10, HIGH);

    Serial.begin(9600);       // for debugging

    // initialize SD card
    Serial.println("Initializing SD card...");
    if (!SD.begin(4)) {
        Serial.println("ERROR - SD card initialization failed!");
        return;    // init failed
    }
    Serial.println("SUCCESS - SD card initialized.");
    // check for index.htm file
    if (!SD.exists("index.htm")) {
        Serial.println("ERROR - Can't find index.htm file!");
        return;  // can't find index file
    }
    Serial.println("SUCCESS - Found index.htm file.");

    Ethernet.begin(mac, ip);  // initialize Ethernet device
    server.begin();           // start to listen for clients
    
    dht.begin();
}

void loop()
{
    client = server.available();  // try to get client

    if (client) {  // got client?
        boolean currentLineIsBlank = true;
        while (client.connected()) {
            if (client.available()) {   // client data available to read
                char c = client.read(); // read 1 byte (character) from client
                // buffer first part of HTTP request in HTTP_req array (string)
                // leave last element in array as 0 to null terminate string (REQ_BUF_SZ - 1)
                if (req_index < (REQ_BUF_SZ - 1)) {
                    HTTP_req[req_index] = c;          // save HTTP request character
                    req_index++;
                }
                // print HTTP request character to serial monitor
                Serial.print(c);
                // last line of client request is blank and ends with \n
                // respond to client only after last line received
                if (c == '\n' && currentLineIsBlank) {
                  if (StrContains(HTTP_req, "/ws/")) {
                    Serial.print("WebService");
                        //Web Services
                        WS(client,HTTP_req);
                  }
                  else if(IsWebPage(HTTP_req))
                  {
                    Serial.print("WebPage");
                    ShowWebPageInSD(HTTP_req);
                  }
                  else
                  {
                    Serial.print("Argument : ");
                    char* arg1 = GetFirstArg(HTTP_req);
                    Serial.println(arg1);
                    Serial.println(HTTP_req);
                    char* arg2 = GetFirstArg(HTTP_req);
                    Serial.println(arg2);
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-Type: text/html");
                    client.print("<h2>");
                    client.print(arg1);
                    client.print("</h2>");
                  }
                    /*if (webFile) {
                        while(webFile.available()) {
                            client.write(webFile.read()); // send web page to client
                        }
                        webFile.close();
                    }*/
                    // reset buffer index and all buffer elements to 0
                    req_index = 0;
                    StrClear(HTTP_req, REQ_BUF_SZ);
                    break;
                }
                // every line of text received from the client ends with \r\n
                if (c == '\n') {
                    // last character on line of received text
                    // starting new line with next character read
                    currentLineIsBlank = true;
                }
                else if (c != '\r') {
                    // a text character was received from client
                    currentLineIsBlank = false;
                }
            } // end if (client.available())
        } // end while (client.connected())
        delay(1);      // give the web browser time to receive the data
        client.stop(); // close the connection
        Serial.println("Stop");
    } // end if (client)
}

// sets every element of str to 0 (clears array)
void StrClear(char *str, char length)
{
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
}

// searches for the string sfind in the string str
// returns 1 if string found
// returns 0 if string not found
char StrContains(char *str, char *sfind)
{
    char found = 0;
    char index = 0;
    char len;

    len = strlen(str);

    if (strlen(sfind) > len) {
        return 0;
    }
    while (index < len) {
        if (str[index] == sfind[found]) {
            found++;
            if (strlen(sfind) == found) {
                return 1;
            }
        }
        else {
            found = 0;
        }
        index++;
    }

    return 0;
}

#define DATE_BUFFER_SIZE 30
#define CMDBUF 50

char* ParseUrl(char *str)
{
  int8_t r=-1;
  int8_t i = 0;
  int index = 4;
  int index1 = 0;
  int plen = 0;
  char ch = str[index];
  char clientline[CMDBUF];
  while( ch != ' ' && index < CMDBUF)
  {
    clientline[index1++] = ch;
    ch = str[++index];
  }
  clientline[index1] = '\0';

  // convert clientline into a proper
  // string for further processing
  String urlString = String(clientline);
  // extract the operation
  String op = urlString.substring(0,urlString.indexOf(' '));
  // we're only interested in the first part...
  urlString = urlString.substring(urlString.indexOf('/'), urlString.indexOf(' ', urlString.indexOf('/')));

  // put what's left of the URL back in client line
  urlString.toCharArray(clientline, CMDBUF);

  return clientline;
}

char* GetFirstArg(char *str)
{
  //static char clientline[CMDBUF];

  if(str != NULL)
  {
          sprintf(str,"%s",ParseUrl(str));
          return strtok(str,"/");
  }
  else
  {
          return strtok(NULL,"/");
  }
}

// returns 1 if Web Page
// returns 0 if not web Page
char IsWebPage(char *str)
{
  if (StrContains(str, "GET / HTTP/") || StrContains(str, ".htm") || StrContains(str, ".jpg") || StrContains(str, ".ico"))
  {
    return 1;
  }
  return 0;
}

void ShowWebPageInSD(char *str)
{
    char v_Fichier[CMDBUF];
    String urlString = String(str);
    urlString = urlString.substring(urlString.indexOf('/')+1, urlString.indexOf(' ', urlString.indexOf('/')));
    urlString.toCharArray(v_Fichier, CMDBUF);

    if(StrContains(str, "GET / HTTP/"))
    {
      String("index.htm").toCharArray(v_Fichier, CMDBUF);
    }
    Serial.print("ShowWebPageInSD ");
    Serial.print("(");
    Serial.print(v_Fichier);
    Serial.print(")");
    Serial.println(str);
    if(SD.exists(v_Fichier))
    {
      if (StrContains(v_Fichier, ".htm"))
      {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connnection: close");
          client.println();
          webFile = SD.open(v_Fichier);        // open web page file
      }
      else if (StrContains(v_Fichier, ".jpg") || StrContains(v_Fichier, ".ico"))
      {
          webFile = SD.open(v_Fichier);
          if (webFile) {
              client.println("HTTP/1.1 200 OK");
              client.println();
          }
      }
          if (webFile) {
            Serial.println("On lit le fichier");
                        while(webFile.available()) {
                            client.write(webFile.read()); // send web page to client
                        }
                        webFile.close();
                    }
    }
    else
    {
      Serial.println("Erreur 404");
      client.println("HTTP/1.1 404 Not Found");
      client.println("Content-Type: text/html");
      client.println();
      client.println("<h2>Fichier non trouve!</h2>");
    }

}

// send the state of the switch to the web browser
void WS(EthernetClient cl, char* p_req)
{
  Serial.println("Web Services");
  
  if (StrContains(p_req, "temp")){
    
     float t = dht.readTemperature();
     if (isnan(t))
     {
       cl.println("Failed to read temp from DHT");
     }else{
        cl.print("Température (°C): ");
        cl.println(t);
     }
  }
 else if (StrContains(p_req, "humidite")){
     float h = dht.readHumidity();
     if (isnan(h))
     {
       cl.println("Failed to read humidity from DHT");
     }else{
        cl.print("Humidité: ");
        cl.println(h);
     }
 }
 else{
    cl.println("inconnu");
 }
}
