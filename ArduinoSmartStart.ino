#include <SoftwareSerial.h>

SoftwareSerial sim900(7, 8);
String message = "";
String number = "";
String command;
String allowedNums = "+1aaabbbcccc +1aaabbbcccc";
int understood;
long pongLoop = 0;
int ping = 0;
int DBAin;

void setup() {
  sim900.begin(19200);
  Serial.begin(1200); //sim900
  delay(100);
  sim900.print("AT+CMGF=1\r"); // set SMS mode to text
  delay(100);
  sim900.print("AT+CNMI=2,2,0,0,0\r");
  delay(100);
  //Serial.println("Ready...");
  pong();
  Serial.write(0x56); //tell the DBAll there's a "smart start" attached
}

String content = "" ;

void loop ()
{
  if (pongLoop == 200000) {  //ping the sim900 every few seconds
   pong(); 
  }
  check_sim900();
  pongLoop++;
}

void check_sim900() {  //check for incoming sim900 msgs
  if (sim900.available ()) {
    char ch = sim900.read () ;
    if (ch == '\r') {
      process_string (content) ;  //process them once fully received
      content = "" ;
    } else { 
      content.concat (ch) ;
    }
  }
}

void process_string (String s)
{
  understood = 0;
  //Serial.println(content);
  int sms = content.indexOf('+CMT:'); //+cmt means an sms was received
  if (sms != -1) {
    number = content.substring(8, 20);  //get the cell phone number
  }
  int allowed = allowedNums.indexOf(number); //allowed cell phone?
  if (allowed != -1) {
    int firstHashTag = content.indexOf('#');
    if (firstHashTag != -1) {
      if (content.substring(firstHashTag) == "#lock") {
        dba_cmd(0x91, "Doors locked");
        //dba_cmd(<hex bit to send DBAll>,<message if successful>);
        understood = 1;
      }
      if (content.substring(firstHashTag) == "#unlock") {
        dba_cmd(0x9A, "Doors unlocked");
        understood = 1;
      }
      if (content.substring(firstHashTag) == "#start") {
        dba_cmd(0x99, "Start/stop OK");
        understood = 1;
      }
      if (content.substring(firstHashTag) == "#ping") {  //just to see if everything's working
        dba_cmd(0x56, "Pong!");
        understood = 1;
      }
      if (understood == 0) {    //this will report an unauthorized command
        message = "Unrecognized command!";
        SendSMS(number, message);
      }
    }
  }
// else {                                         //uncomment these lines if you want to reply to an unauthorized number
//    message = "Unathorized number";
//    SendSMS(number, message);
//  }
}

void pong () { //check to see if the sim900 is on, turn it on if it isn't
  ping = 0;
  int count = 0;
  //Serial.println("Ping..");
  sim900.println("AT");
  while (ping == 0) {
    if(readCommand()) { //if something's available, read it
      int ok = command.indexOf('OK');
      if (ok != -1) { //was OK in the command? Yes.
        ping = 1;
        //Serial.println("Pong!!");
        pongLoop = 0;
      }
      command = "";
    }
    count++;
    if (count == 6000) {  //send a power on signal and go on about business
      //Serial.println("No pong :(");
      powerup();
      ping = 1;
      pongLoop = 0;
      delay(5000);
    }
  }
}

void powerup () {
  digitalWrite(9, HIGH);
  delay(2000);
  digitalWrite(9, LOW);
  delay(10000);
  sim900.print("AT+CMGF=1\r"); // set SMS mode to text
  delay(100);
  sim900.print("AT+CNMI=2,2,0,0,0\r");
}

int readCommand() {
    char c;
    if(sim900.available() > 0) {
        c = sim900.read();
        if(c != '\n') {       
            command += c;
            return false;
        }
        else
            return true;
    }
} 

void dba_cmd (char cmd, String message) {
  Serial.write(cmd);
  int ack = 0;
  int counter = 0;
  while (ack == 0) {                        //this is the part that's broke. I want it to hang out here and wait for the 
    if (Serial.available()) {               //command to be echoed back from the DBAll            
      int inByte = Serial.read();                       
      if (inByte == cmd) {
        ack = 1;
        SendSMS(number, message);
      } 
    } 
    else {
      counter++;
      //Serial.println(counter);
      if (counter == 10000) {
        message = "DBAll not responding";
        SendSMS(number, message);
        ack = 1;
      }
    }
  }
}

void SendSMS (String num, String mes) {
  //Serial.println("Sending sms");
  int sent = 0;
  sim900.println("AT + CMGS = \"" + num + "\"");
  delay(100);
  sim900.println(mes);
  delay(100);
  sim900.println((char)26);//ctrl+z
  while (sent == 0) {                       //hang out here until the SMS is finished sending
    if(readCommand()) {
      int ok = command.indexOf('Number:');
      if (ok != -1) {
        sent = 1;
        //Serial.println("SMS Sent");
      }
    }
  }  //end of while loop
  message = "";
  number = "";
  sim900.println("AT+CMGD=1,4"); // delete all SMS
}
