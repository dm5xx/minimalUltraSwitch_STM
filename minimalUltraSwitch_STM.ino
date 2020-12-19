// cc by DM5XX @ GNU GPLv3
// LLAP! 

#include "Adafruit_MCP23017.h"
#include <Ethernet_STM32.h>


#define DHCP
#define DEBUG

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

//uncomment and change if you need to adress a static ip in your local environment
//IPAddress ip(192, 168, 97, 177);
int myPort = 59;
EthernetServer server(myPort); // (port 80 is default for HTTP)
const uint8_t ETHERNET_SPI_CS_PIN = PB12;

char requestString[100];
bool isLocked = false;

// all files are in their newest version on DM5XX webserver. You have to point this address to your websrver oder local computer, if you want to change something else than labels..
String URLToJS = "h.mmmedia-online.de/minimal63/";

// adjust this URL if you want to change Label.js, GroupDef.js, Lock.js, Disable.js (to customize them). So ALL 4 files must be stored on your own webserver/local computer than, even if you just want to leave one or more default!
//String URLToCustomize = "dl5nam.de/minimal63/";
String URLToCustomize = "h.mmmedia-online.de/minimal63XX/";

// this is the url, where the dashboard will call to reach the switch. can be an internal IP (same as IPADDRESS above!!!) or a dyndns-url, forwarded to IPADDRESS (above).
String SwitchURL = "255.255.255.255";
String MyLocalIP = "192.168.97.177"; 
String ClientIP = "255.255.255.255"; 

int pinsOrder[16] = { 0,8,1,9,2,10,3,11,4,12,5,13,6,14,7,15 };

#define  NUMBEROFBOARDS 8

struct RelayCard
{
  Adafruit_MCP23017 mcp;
  
  byte pins[16];
  byte pinStatus[16];
  unsigned int bankStatus = 0;
};


RelayCard relay0;
RelayCard relay1;
RelayCard relay2;
RelayCard relay3;
RelayCard relay4;
RelayCard relay5;
RelayCard relay6;
RelayCard relay7;

RelayCard relayArray[8];


////////////////////////////////////////////////////////////////////////////// FUNCTIONS //////////////////////////////////////////////////////
void GetOrderedArraybyValue(int value, byte * feld)
{
    int i;

    for (i = 0; i < 16; i++)
    {
        feld[i] = value % 2;
        value /= 2;
    }
}

int GetValueByOrderedArray(byte * arr)
{
  int result = 0;
      
  for(int a = 15; a >= 0; a--)
  {
        result = result + (arr[a] * 1<<a);
  }

  return result;
}

void updatePin(int pin, byte value, int bankNr)
{

       relayArray[bankNr].pinStatus[pin] = value;
       relayArray[bankNr].bankStatus = GetValueByOrderedArray(relayArray[bankNr].pinStatus); 
       updateSingleRelay(relayArray[bankNr], pin, value);
}

void updatePinStatus(unsigned int value, int bankNr)
{

  relayArray[0].bankStatus = value;
  GetOrderedArraybyValue(value, relayArray[bankNr].pinStatus);
  updateRelays(relayArray[bankNr], bankNr);

#ifdef DEBUG
      Serial.println(relayArray[bankNr].bankStatus);
      for(int a=0; a < 16; a++)
      {
        Serial.print(relayArray[bankNr].pinStatus[a]);
        Serial.print(',');
      }
#endif
}

void updateRelays(RelayCard &bank, int bankNr)
{
  for (int i = 0; i < 16; i++)
  {
    if(bank.pinStatus[i] == 1)
       bank.mcp.digitalWrite(bank.pins[i], LOW);
    else
       bank.mcp.digitalWrite(bank.pins[i], HIGH);    
  } 
}

void updateSingleRelay(RelayCard &bank, int pin, int value)
{
   if(value == 1)
    bank.mcp.digitalWrite(bank.pins[pin], LOW);
   else
    bank.mcp.digitalWrite(bank.pins[pin], HIGH);    
}

/*********************************************************************************************************************************/
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++ THE WEB PAGES ++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*********************************************************************************************************************************/

void sendEmptyPage(EthernetClient &client)
{
  Send200OK(client);
}

void SendFavicon(EthernetClient &client)
{
   Send200OK(client);
   client.println(F(""));
   client.println(F("<!DOCTYPE html>"));
   client.println(F("<HTML>"));
   client.println(F("<link rel=\"icon\" href=\"data:;base64,=\">"));
   client.println(F("</HTML>"));
}

void GetDataJSON(EthernetClient &client)
{
  client.println(F("HTTP/1.0 200 OK"));
  client.println(F("Access-Control-Allow-Origin: *"));
  client.println(F("Content-Type: application/json"));
  client.println(F("Connection: close"));
  client.println(F(""));
  client.print(F("{\"B0\": "));
  client.print(relayArray[0].bankStatus);
#if NUMBEROFBOARDS > 1
  client.print(F(", \"B1\": "));
  client.print(relayArray[1].bankStatus);
#endif
# if NUMBEROFBOARDS > 2
  client.print(F(", \"B2\": "));
  client.print(relayArray[2].bankStatus);
#endif
#if NUMBEROFBOARDS > 3
  client.print(F(", \"B3\": "));
  client.print(relayArray[3].bankStatus);
#endif
#if NUMBEROFBOARDS > 4
  client.print(F(", \"B4\": "));
  client.print(relayArray[4].bankStatus);
#endif
#if NUMBEROFBOARDS > 5
  client.print(F(", \"B5\": "));
  client.print(relayArray[5].bankStatus);
#endif
#if NUMBEROFBOARDS > 6
  client.print(F(", \"B6\": "));
  client.print(relayArray[6].bankStatus);
#endif
#if NUMBEROFBOARDS > 7
  client.print(F(", \"B7\": "));
  client.print(relayArray[7].bankStatus);
#endif
  client.print(F(", \"LockStatus\": "));
  client.print(isLocked);
  client.print(F(", \"Clients\": \""));
  client.print(ClientIP);
  client.print(F("\"}"));
}

void SendLocked(EthernetClient &client)
{
  client.println(F("HTTP/1.0 200 OK"));
  client.println(F("Access-Control-Allow-Origin: *"));
  client.println(F("Content-Type: application/json"));
  client.println(F("Connection: close"));
  client.println(F(""));
  client.print(F("{\"Lockingstatus\": "));
  client.print(isLocked);
  client.print(F("}"));
}
void Send200OK(EthernetClient &client)
{
  client.println(F("HTTP/1.1 200 OK")); //send new page
  client.println(F("Access-Control-Allow-Origin: *"));
  client.println(F(""));
  client.println(F("Content-Type: text/html"));
}

void MainPage(EthernetClient &client)
{
   client.println("HTTP/1.1 200 OK");
   client.println(F("Access-Control-Allow-Origin: *"));
   client.println(F("Content-Type: text/html"));
   client.println("Connection: close");  // the connection will be closed after completion of the response   client.println(F(""));
   client.println(F(""));
   client.println(F("<!DOCTYPE html>"));
   client.println(F("<HTML>"));
   client.println(F("<HEAD>"));
   client.println(F("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/>"));
   client.println(("<link rel=\"stylesheet\" type=\"text/css\" href=\"http://"+ URLToJS +"style.css\" media=\"screen\"/>"));
   client.println(("<script language=\"javascript\" type=\"text/javascript\" src=\"http://"+ URLToJS +"init.js\"></script>"));
   client.println(("<script language=\"javascript\" type=\"text/javascript\" src=\"http://"+ URLToJS +"ShortCut.js\"></script>"));
   client.println(("<script language=\"javascript\" type=\"text/javascript\" src=\"http://"+ URLToCustomize +"Custom_c.js\"></script>"));
   client.println(("<script language=\"javascript\" type=\"text/javascript\" src=\"http://"+ URLToCustomize +"Profile_c.js\"></script>"));
   client.println(("<script language=\"javascript\" type=\"text/javascript\" src=\"http://"+ URLToCustomize +"Disable_c.js\"></script>"));
   client.println(("<script language=\"javascript\" type=\"text/javascript\" src=\"http://"+ URLToCustomize +"Label_c.js\"></script>"));
   client.println(("<script language=\"javascript\" type=\"text/javascript\" src=\"http://"+ URLToCustomize +"BankDef_c.js\"></script>"));
   client.println(("<script language=\"javascript\" type=\"text/javascript\" src=\"http://"+ URLToJS +"Globals.js\"></script>"));
   client.println(("<script language=\"javascript\" type=\"text/javascript\" src=\"http://"+ URLToCustomize +"LockDef_c.js\"></script>"));
   client.println(("<script language=\"javascript\" type=\"text/javascript\" src=\"http://"+ URLToJS +"Lock.js\"></script>"));
   client.println(("<script language=\"javascript\" type=\"text/javascript\" src=\"http://"+ URLToCustomize +"GroupDef_c.js\"></script>"));
   client.println(("<script language=\"javascript\" type=\"text/javascript\" src=\"http://"+ URLToJS +"Group.js\"></script>"));
   client.println(("<script language=\"javascript\" type=\"text/javascript\" src=\"http://"+ URLToJS +"UiHandler.js\"></script>"));
   client.println(("<script language=\"javascript\" type=\"text/javascript\" src=\"http://"+ URLToJS +"GetData.js\"></script>"));
   client.println(("<script language=\"javascript\" type=\"text/javascript\" src=\"http://"+ URLToJS +"SetData.js\"></script>"));
   client.println(("<script language=\"javascript\" type=\"text/javascript\" src=\"http://"+ URLToJS +"Helper.js\"></script>"));
   client.print(F(""));
   client.println(F("<!-- Change SwitchURL to the url, where your webswitch is reachable from outside/inside. Dont forget the portforwarding...-->"));
   client.print(F("<script>var url='"));
   client.print((MyLocalIP));
   client.print(":");
   client.print(myPort);
   client.print(F("';\r"));
   client.print(F("var numberOfBoards="));
   client.print(NUMBEROFBOARDS);
   client.print(F(";\r"));
   client.println(F("</script>"));
   client.print(F("<TITLE>"));
   client.print("minimal63 - Remote Switch by DM5XX");
   client.println(F("</TITLE>"));
   client.println(F("</HEAD>"));
   client.println(F("<BODY>"));
   client.println(F("<div class=\"grid-container\" id=\"container\"></div>"));
   client.println(F("</BODY>"));
   client.println(F("<script>(() => { init(); })()</script>"));
   client.println(F("</HTML>"));
}


void setup()
{
   // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while ( !Serial ); delay(100);


  for (int a = 0; a < 16; a++)
  {
    relay0.pins[a] = pinsOrder[a];
    relay1.pins[a] = pinsOrder[a];              
    relay2.pins[a] = pinsOrder[a];              
    relay3.pins[a] = pinsOrder[a];              
    relay4.pins[a] = pinsOrder[a];              
    relay5.pins[a] = pinsOrder[a];              
    relay6.pins[a] = pinsOrder[a];              
    relay7.pins[a] = pinsOrder[a];              
  }  
  
  relayArray[0] = relay0;
  relayArray[1] = relay1;
  relayArray[2] = relay2;
  relayArray[3] = relay3;
  relayArray[4] = relay4;
  relayArray[5] = relay5;
  relayArray[6] = relay6;
  relayArray[7] = relay7;

  for(int a = 0; a < NUMBEROFBOARDS; a++)
  {
    relayArray[a].mcp.begin(a);      // use default address 0
    
    for (int p = 0; p < 16; p++)
    {
      relayArray[a].mcp.pinMode(p, OUTPUT);
      relayArray[a].mcp.digitalWrite(p, HIGH);
      delay(20);
    }
  }    

  Serial.println(("*************************************************************"));
  Serial.println(("***** Web server xxx *****"));
  Serial.println(("*************************************************************"));

  SPI.setModule(2);
  // init Ethernet interface
  Ethernet.init(SPI, ETHERNET_SPI_CS_PIN);

  Serial.print("\nGetting IP address using DHCP...");
  Ethernet.begin(mac);
  server.begin();
  Serial.print("server is at ");

  IPAddress address = Ethernet.localIP();
  MyLocalIP = String() + address[0] + "." + address[1] + "." + address[2] + "." + address[3];
  
  Serial.println(MyLocalIP);

}


#define BUFSIZE 20

void loop() 
{
  // Create a client connection
  EthernetClient client = server.available();
  char clientline[BUFSIZE];
  int index = 0;
  
  if (client) 
  {
    //IPAddress localClientIP = client.remoteIP();   ?????????????????????????????????????????????????????????
    //ClientIP = String() + localClientIP[0] + "." + localClientIP[1] + "." + localClientIP[2] + "." + localClientIP[3];
  
    byte charIndex = 0;

    // connectLoop controls the hardware fail timeout
    int connectLoop = 0;
    
    while (client.connected()) 
    { 
      connectLoop++;

      if(connectLoop > 10000)
      {
        client.stop();
      };
      
      if(client.available()) 
      {
        char c = client.read();
      
        if (c != '\n' && c != '\r') {
          clientline[index] = c;
          index++;

          if (index >= BUFSIZE) 
            index = BUFSIZE -1;
          
          continue;
        }
        
        // got a \n or \r new line, which means the string is done
        clientline[index] = 0;
        
        // Print it out for debugging
#ifdef DEBUG
        Serial.println(clientline);
#endif
        
        //if HTTP request has ended
        if (strstr(clientline, "GET /Get/") != 0) 
        {    
#ifdef DEBUG
            Serial.println("Get detected");
#endif
            GetDataJSON(client);
            connectLoop = 0;
            break;
        }

        byte end = 0;            
        for(int r = 11; r < 16; r++)
        {
          if(clientline[r] == ' ')
            end = r;
          else
            continue;
        }

        String sss = clientline;
        String subSss = sss.substring(10, end);
        
        if(strstr(clientline, "GET /Set") != 0)
        {
          if(isLocked)
          {
            SendLocked(client);
            connectLoop = 0;
            break;
          }
          
          int value = subSss.toInt(); 
  
#ifdef DEBUG
          Serial.println(clientline[8]);
          Serial.println(value);
#endif

          switch (clientline[8])
          {
            case '0':
#ifdef DEBUG
              Serial.println("SetBank0 detected");
#endif
              updatePinStatus(value, 0);            
              break;
            case '1':
#ifdef DEBUG
              Serial.println("SetBank1 detected");
#endif
              updatePinStatus(value, 1);            
              break;
            case '2':
#ifdef DEBUG
              Serial.println("SetBank2 detected");
#endif
              updatePinStatus(value, 2);            
              break;
            case '3':
#ifdef DEBUG
              Serial.println("SetBank3 detected");
#endif
              updatePinStatus(value, 3);            
              break;
          }
          Send200OK(client);
          connectLoop = 0;
          break;
        }
        else if(strstr(clientline, "GET /set") != 0)
        {
          int delim = subSss.indexOf('/');
          int pinNr = subSss.substring(0, delim).toInt();
          int cmd = (subSss[subSss.length()-1])-48;

#ifdef DEBUG
          Serial.println(pinNr);
          Serial.println(cmd);
#endif

          switch (clientline[8])
          {
            case '0':
#ifdef DEBUG
              Serial.println("Manual Set at bank 0");          
#endif
              updatePin(pinNr, cmd, 0);
              break;
            case '1':
#ifdef DEBUG
              Serial.println("Manual Set at bank 1");          
#endif
              updatePin(pinNr, cmd, 1);
              break;
            case '2':
#ifdef DEBUG
              Serial.println("Manual Set at bank 2");          
#endif
               updatePin(pinNr, cmd, 2);
               break;
            case '3':
#ifdef DEBUG
                Serial.println("Manual Set at bank 3");          
#endif
               updatePin(pinNr, cmd, 3);
               break;
          }
          Send200OK(client);
          connectLoop = 0;
          break;
        }
        else if (strstr(clientline, "GET /favicon") != 0)
        {
#ifdef DEBUG
          Serial.println("Favicon requested");
#endif
          SendFavicon(client);
          connectLoop = 0;
          break;
        }
        else if (strstr(clientline, "GET /Reset") != 0)
        {
          client.println(F(""));
          client.println(F("<!DOCTYPE html>"));
          client.println(F("<HTML><BODY>R-E-S-E-T!!!</BODY></HTML>"));
          delay(50); // 1?  
          client.stop();
          nvic_sys_reset();
          //soft_restart();
        }
        else if (strstr(clientline, "GET /Lock") != 0)
        {
          isLocked = true;
          SendLocked(client);
          connectLoop = 0;
          break;
        }
        else if (strstr(clientline, "GET /UnLock") != 0)
        {
          isLocked = false;
          SendLocked(client);
          connectLoop = 0;
          break;
        }
                  
#ifdef DEBUG
        Serial.println("Call main");
#endif
        MainPage(client);
        connectLoop = 0;
        break;
      }
    }
    delay(10); // 1?
  
    client.stop();
#ifdef DEBUG
    Serial.println("disconnected.");
#endif
  }
}
