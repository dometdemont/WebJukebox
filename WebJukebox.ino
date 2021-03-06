#include <SoftwareSerial.h>
#include "WiFly.h"
#include <SdFat.h>
#include <MD_MIDIFile.h>
SdFat  SD;
MD_MIDIFile SMF;

#define SSID      "orgue"
#define KEY       "Ruche.1943"
// check your access point's security mode, mine was WPA20-PSK
// if yours is different you'll need to change the AUTH constant, see the file WiFly.h for avalable security codes
#define AUTH      WIFLY_AUTH_WPA2_PSK


// change this to match your SD shield or module;
// Arduino Ethernet shield: pin 4
// Adafruit SD shields and modules: pin 10
// Sparkfun SD shield: pin 8
// MKRZero SD: SDCARD_SS_PIN
const int chipSelect = 10;
 
#define USE_MIDI  1  // set to 1 for MIDI output, 0 for debug output
char userOutput[800];

#if USE_MIDI // set up for direct MIDI serial output
#define DEBUGS(s)   {if(strlen(userOutput)+strlen(s)<sizeof(userOutput))strcat(userOutput, s);}
#define DEBUG(s, x) {if(strlen(userOutput)+strlen(s)+10<sizeof(userOutput))sprintf(userOutput+strlen(userOutput), "\n%s %d", s, x);}
#define DEBUGX(s, x)
#define SERIAL_RATE 31250

#else // don't use MIDI to allow printing debug statements

#define DEBUGS(s)     Serial.print(s);
#define DEBUG(s, x)   { Serial.print(F(s)); Serial.print(x); }
#define DEBUGX(s, x)  { Serial.print(F(s)); Serial.print(x, HEX); }
#define SERIAL_RATE 57600

#endif // USE_MIDI

int flag = 0;
 
// Pins' connection
// Arduino       WiFly
//  2    <---->    TX
//  3    <---->    RX
// Mega          WiFly
//  11   <---->    TX
//  12   <---->    RX
 
SoftwareSerial wiflyUart(11, 12); // create a WiFi shield serial object
WiFly wifly(&wiflyUart); // pass the wifi siheld serial object to the WiFly class


void midiSilence(void)
// Turn everything off on every channel.
// Some midi files are badly behaved and leave notes hanging, so between songs turn
// off all the notes and sound
{
  midi_event ev;

  // All sound off
  // When All Sound Off is received all oscillators will turn off, and their volume
  // envelopes are set to zero as soon as possible.
  const char msg[]={
    120,  // All sounds off
    121,  // Reset all controllers
    123   // All notes off
  };
  
  ev.size = 0;
  ev.data[ev.size++] = 0xb0;
  ev.data[ev.size++] = 0;
  ev.data[ev.size++] = 0;

  for(int i=sizeof(msg)/sizeof(msg[0]); i--; ){
    ev.data[1]=msg[i];
    for (ev.channel = 0; ev.channel < 16; ev.channel++)
    midiCallback(&ev);
  }
}


void midiCallback(midi_event *pev)
// Called by the MIDIFile library when a file event needs to be processed
// thru the midi communications interface.
// This callback is set up in the setup() function.
{
  if ((pev->data[0] >= 0x80) && (pev->data[0] <= 0xe0))
  {
    Serial.write(pev->data[0] | pev->channel);
    Serial.write(&pev->data[1], pev->size-1);
  }
  else
    Serial.write(pev->data, pev->size);
}

void sysexCallback(sysex_event *pev)
// Called by the MIDIFile library when a system Exclusive (sysex) file event needs 
// to be processed through the midi communications interface. Most sysex events cannot 
// really be processed, so we just pass it through
// This callback is set up in the setup() function.
{
  Serial.write(pev->data, pev->size);
}


// Send the string s to wifly if emit is true; return the string length
uint16_t getLengthAndSend(const char* s, bool emit){
  if(emit)wiflyUart.print(s);
  return strlen(s);
}

class title {
  public:
    title(const char* aFile, const char* aDescription, const char* aTiming);
    void start();
    void cancel();
    void pause();
    void resume();
    uint16_t getHtmlEntry(bool emit);
    const char* file;
    const char* description;
    const char* timing;
};

title* currentTitle=NULL;
uint32_t currentTitleTimestamp;
uint32_t pauseTimestamp;

const title  liszt("LISZT.MID",       "Franz Liszt : Pr??lude et Fugue sur B.A.C.H. (10')", "15, 617, 20");
const title  messiaen("MESSIAEN.MID", "Olivier Messiaen : Banquet c??leste (6')", "5, 375, 19");
const title  wagner("WAGNER.MID",     "Richard Wagner : Mort d'Isolde (8'40)", "8, 520, 20");
const title  dupre("DUPRE.MID",       "Marcel Dupr?? : Pr??lude et Fugue en sol mineur", "12, 403, 10");
const title  taille("COUPERIN.MID",   "Fran??ois Couperin : Tierce en taille (4')", "6, 240, 20");
const title  franck("FRANCK.MID",     "C??sar Franck : Troisi??me Choral (10'20)", "13, 622, 20");
const title  chromorne("CHROMORN.MID","Fran??ois Couperin : Chromorne en taille (4')", "7, 233, 20");
const title  toccata("TOCAREM.MID",   "J.S. Bach: Toccata en R?? mineur (2'30)", "2, 145, 5");
const title  langlais("LANGLAIS.MID", "Jean Langlais : Chant de Paix (2'30)", "6, 150, 4");
const title  guilmant("GUILMANT.MID", "Alexandre Guilmant : No??l 'Or dites-nous Marie' (2'20)", "8, 136, 4");
const title *playList[] = {&liszt, &franck, &dupre, &wagner, &chromorne, &taille, &messiaen, &langlais, &guilmant, &toccata};


title::title(const char* aFile, const char* aDescription,  const char* aTiming){
  file = aFile;
  description = aDescription;
  timing = aTiming;
}
void title::start(){
  if(this == currentTitle)return;
  int err = SMF.load(this->file);
  if (err != MD_MIDIFile::E_OK){
    DEBUG("SD load error:  ", err); 
  }else{
    currentTitle=this;
  }  
}
void title::cancel(){
  if(currentTitle==NULL)return;
  DEBUG("Now stopping: ", description);
  SMF.close();
  pauseTimestamp=0;  
}
void title::pause(){
  if(currentTitle==NULL)return;
  DEBUG("Now pausing: ", description);
  SMF.pause(true);
  midiSilence();
  pauseTimestamp=millis();
}
void title::resume(){
  if(pauseTimestamp==0 || currentTitle==NULL)return;
  DEBUG("Now resuming: ", description);
  SMF.pause(false);
  uint32_t elapsedPause=millis()-pauseTimestamp;
  currentTitleTimestamp+=elapsedPause;
  pauseTimestamp=0;  
}

uint16_t title::getHtmlEntry(bool emit){
  return 
    getLengthAndSend("<li><a href=\"", emit)+
    getLengthAndSend(file, emit)+
    getLengthAndSend("\">", emit)+
    getLengthAndSend(description, emit)+
    getLengthAndSend("</a></li>", emit);
}

const char * pageHeader = R"(
<html><head><title>Pipe organ as a juke box</title>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
body {  background-color: black;  font-family: verdana;  color: white;}
h1,h2 {  text-align: center;}
h3 { color: grey }
a {  color: yellow;  font-size: 20px;}
table {width: 100%;}
</style>
</head>
<link rel="icon" href="data:;base64,iVBORw0KGgo=">
<body><table><tr>
<td><svg id='logo' version="1.1"
     baseProfile="full"
     width="180" height="150"
     xmlns="http://www.w3.org/2000/svg">
  <rect width="100%" height="100%" fill="black" />
</svg>
</td>
<td><h1>Les grandes orgues</h1><h2>de la Basilique St Joseph de Grenoble</h2></tr></td>
</table>
<script>
var xmlns="http://www.w3.org/2000/svg";pipe=function(t,e,n,r,o){var i=document.createElementNS(xmlns,"polygon"),a=n/60;i.setAttribute("style","fill:black;stroke:orange;");var g=(t+r/2).toString()+","+(e-o).toString();return g+=" "+(t-r/2).toString()+","+(e-o).toString(),g+=" "+(t-r/2).toString()+","+(e-o-n).toString(),g+=" "+(t+r/2).toString()+","+(e-o-n).toString(),g+=" "+(t+r/2).toString()+","+(e-o).toString(),g+=" "+t.toString()+","+e.toString(),g+=" "+(t-r/2).toString()+","+(e-o).toString(),g+=" "+(t-r/2).toString()+","+(e-o-a).toString(),g+=" "+(t+r/2).toString()+","+(e-o-a).toString(),i.setAttribute("points",g),i},plateface=function(e,n,r,t,o,i,a,g,l){var S=(o-t)/(e-1),p=(g-a)/(e-1);l&&(S*=2,p*=2);var c=t,f=a;for(let t=0;t<e;t++)l&&t==~~(e/2)&&(S*=-1,p*=-1),document.getElementById("logo").appendChild(pipe(n+t*(i+0),r,c,i,f)),c+=S,f+=p;return n+i/2+(e-1)*(i+0)},logo=function(t,e,n){var r=n,o=7*r,i=4.5*r,a=5*r,g=3*r,l=3*r,n=2*r,t=plateface(3,t,e-o,77*r,88*r,a,17*r,17*r,!0)+i,t=plateface(5,t,e-o,68*r,42*r,g,13*r,17*r,!1)+i,t=plateface(10,t,e,42*r,30*r,l,11*r,11*r,!1)+i,n=plateface(9,t,e,32*r,41*r,n,11*r,6*r,!0)+i,l=plateface(10,n,e,30*r,42*r,l,11*r,11*r,!1)+i,g=plateface(5,l,e-o,42*r,68*r,g,17*r,13*r,!1)+i;plateface(3,g,e-o,77*r,88*r,a,17*r,17*r,!0)};
logo(10, 120, 1)
</script>
)";
const char * pageFooter = "</body></html>";
const char *refreshPauseCancel = R"(
</p><table><tr>
<td align=left><a href='/cancel'>Arr??ter</a></td>
<td align=center><a href='/pause'>Pause</a></td>
<td align=right><a id='refresh' href='/'>Actualiser</a></td>
</tr></table>
)";
const char *refreshResumeCancel = R"(
</p><table><tr>
<td align=left><a href='/cancel'>Arr??ter</a></td>
<td align=center><a href='/resume'>Reprendre</a></td>
<td align=right><a id='refresh' href='/'>Actualiser</a></td>
</tr></table>
)";

const char *countDownBegin = R"(
<p id="countDown"></p>
<script>
var distances=[)";
const char *countDownEnd=R"(]
var phases=["Chargement : ", "Ex??cution : ", "D??chargement : "]
// Update the count down every 1 second
var rate=1000;
var phase=0;
var elapsed = distances[phases.length]
phases.forEach(function(p, i){
  if(elapsed <= distances[i]){
    distances[i]-=elapsed
    elapsed= distances[phases.length]=0
    }else{
  phase=i
    elapsed-=distances[i]
    distances[i]=0
    }
})
var distance = distances[phase]
var refreshLink = document.getElementById("refresh")
function doRefresh(){
  refreshLink.click();
}
function countDown() {
  var minutes = Math.floor((distance % (60 * 60)) / 60);
  var seconds = Math.floor(distance % 60);
  document.getElementById("countDown").innerHTML = phases[phase] + (minutes?minutes+"mn ":"")+seconds+"s"
  if(distance == 0 && phase < phases.length-1){
   phase++
   distance = distances[phase]
  }
  if(distance > 0)setTimeout(countDown, rate);
  distance--
}

if(rate)setTimeout(countDown, rate);
setTimeout(doRefresh, 1000*distances.reduce(function(a, b) { return a + b; }, 4))</script>

)";

uint16_t getPageBody(char* request, bool emit){
  uint16_t pageLength=0;
  // number of elements in array
  int const n = sizeof( playList ) / sizeof( playList[ 0 ] );
  int i;
  // Check for a pause/resume request
  if(currentTitle != NULL && strstr(request, "pause") != NULL){
    currentTitle->pause();
  }
  if(currentTitle != NULL && strstr(request, "resume") != NULL){
    currentTitle->resume();
  }
  // Check for a cancel request
  if(currentTitle != NULL && strstr(request, "cancel") != NULL){
    currentTitle->cancel();
    i=-1;
  }else{ 
    for (i = n; i--;){ // search for a known file in the request
      title *e = (title*)playList[i];
      if(strstr(request, e->file) != NULL)break;
    }
    // if a play is in progress, display its page
    if(currentTitle != NULL){
       for (i = n; currentTitle != playList[--i];);
    }
  }  
  
  if(i<0){
    // not found: push the full list of titles
    pageLength+=getLengthAndSend("<h3>??uvres disponibles :</h3><ul>", emit);
    for (int i = 0; i != n; ++i){
      title *e = (title*)playList[i];
      pageLength+=e->getHtmlEntry(emit);
    }
    pageLength+=getLengthAndSend(pauseTimestamp==0 ? refreshPauseCancel:refreshResumeCancel, emit);    
  }
  
  if(i >= 0){
    // Found:
    // If this title is not playing yet, start it now
    if(currentTitle != playList[i]){
      currentTitleTimestamp=millis();
      title* c=(title*)playList[i];
      if(!emit)c->start();
    }
    // push the page for this title
    pageLength+=getLengthAndSend("<h3>??uvre en cours d'audition :</h3><p>", emit);
    pageLength+=getLengthAndSend(playList[i]->description, emit);
    if(pauseTimestamp==0){
      pageLength+=getLengthAndSend(refreshPauseCancel, emit);  
      pageLength+=getLengthAndSend(countDownBegin, emit);
      pageLength+=getLengthAndSend(playList[i]->timing, emit);
      {
        char remain[10]; 
        sprintf(remain, ",%d", (millis()-currentTitleTimestamp)/1000);
        pageLength+=getLengthAndSend(remain, emit);
      }
      pageLength+=getLengthAndSend(countDownEnd, emit);
    }
    else{
      pageLength+=getLengthAndSend(refreshResumeCancel, emit);
    }  
  }
  return pageLength;
}

void setup()
{
    memset(userOutput, 0, sizeof(userOutput));
    DEBUGS("Starting...");
    
    // No current title, no pause
    currentTitle=NULL;
    pauseTimestamp=0;
    
    wiflyUart.begin(9600); // start wifi shield uart port
    Serial.begin(SERIAL_RATE);
    DEBUGS("\n--------- WIFLY Webserver --------");
 
    // wait for initilization of wifly
    delay(1000);
 
    wifly.reset(); // reset the shield
    delay(1000);
    //set WiFly params
 
    wifly.sendCommand("set ip local 80\r"); // set the local comm port to 80
    delay(100);
 
    wifly.sendCommand("set comm remote 0\r"); // do not send a default string when a connection opens
    delay(100);
 
    wifly.sendCommand("set comm open *OPEN*\r"); // set the string that the wifi shield will output when a connection is opened
    delay(100);
 
    DEBUGS("\nJoin " SSID );
    if (wifly.join(SSID, KEY, AUTH)) {
        DEBUGS("\nOK");
    } else {
        DEBUGS("Join " SSID " Failed");
    }
     delay(5000);
    DEBUGS("\nget ip\n");
    wifly.sendCommand("get ip\r");
    char response[20];
    memset(response, 0, sizeof(response));
    while (wifly.receive((uint8_t *)&response, sizeof(response)-1, 300) > 0) { // print the response from the get ip command
        DEBUGS(response); memset(response, 0, sizeof(response));
    }
 
    DEBUGS("Web server ready: ");

    if(!SD.begin(chipSelect, SPI_FULL_SPEED) )DEBUGS("Error SD")
    else 
      DEBUGS("\nSD init successful");

    // Initialize MIDIFile
    SMF.begin(&SD);
    SMF.setMidiHandler(midiCallback);
    SMF.setSysexHandler(sysexCallback);
    DEBUGS("\nMIDIFile init successful");
}

void loop()
{
  if(wifly.available())
  { // the wifi shield has data available
    if(wiflyUart.find((char *)"*OPEN*")) // see if the data available is from an open connection by looking for the *OPEN* string
    {
      DEBUGS("\nNew Browser Request: ");
      // delay(1000); // delay enough time for the browser to complete sending its HTTP request string
      char request[20];
      memset(request, 0, sizeof(request));  
      wifly.receive((uint8_t *)&request, sizeof(request), 300);
      DEBUGS(request);
        
      // Build the returned page
      uint16_t htmlLength=strlen(pageHeader)+getPageBody(request, false)+strlen(pageFooter)+strlen(userOutput)+strlen("<pre></pre>")-strlen("<html></html>")-1;
      
      // send HTTP header
      wiflyUart.println("HTTP/1.1 200 OK");
      wiflyUart.println("Content-Type: text/html; charset=UTF-8");
      wiflyUart.print("Content-Length: "); 
      wiflyUart.println(htmlLength); // length of HTML code between <html> and </html>
      wiflyUart.println("Connection: close");
      wiflyUart.println();
      
      // send webpage's HTML code
      wiflyUart.print(pageHeader);
      getPageBody(request, true);
      
      // send user message
      wiflyUart.print("<pre>");
      wiflyUart.print(userOutput);memset(userOutput, 0, sizeof(userOutput));
      wiflyUart.print("</pre>");

      wiflyUart.print(pageFooter);
    }
  }

  if(currentTitle != NULL){
    if (SMF.isEOF()){
      SMF.close();
      midiSilence();
      currentTitle=NULL;
    }else{
      SMF.getNextEvent();
    }
  }
}
