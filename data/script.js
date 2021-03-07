var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
var BatVoltage=4.3;
var Temperature = 20;
var firstupdate = true;

setInterval(updateBattery, 10000);

window.addEventListener('load', onload);
function onload(event) {
    initWebSocket();
}
function initWebSocket() {
    console.log('Trying to open a WebSocket connection');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}
function onOpen(event) {
    console.log('Connection opened');   
}
function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}
function onMessage(event) {
    var jsonReceived = JSON.parse(event.data);
    //set all objects on webpage after receiving message from server
    document.getElementById("axis1").innerHTML = jsonReceived["meanP"] + "&deg";
    document.getElementById("axis2").innerHTML= jsonReceived["meanR"] + "&deg";
    BatVoltage = jsonReceived["batVoltage"];
    Temperature = jsonReceived["Temperature"];
    if(firstupdate==true)updateBattery();
    console.log('Browser received message');
}
function updateBattery()
{
    firstupdate=false;
    document.getElementById("batImg").src="\\images\\Bat_100.png";
    if(BatVoltage <4.1) document.getElementById("batImg").src="\\images\\Bat_80.png";
    if(BatVoltage <3.9) document.getElementById("batImg").src="\\images\\Bat_60.png";
    if(BatVoltage <3.7) document.getElementById("batImg").src="\\images\\Bat_40.png";
    if(BatVoltage <3.5) document.getElementById("batImg").src="\\images\\Bat_20.png";
    //Display the battery voltage as % 3.3v to 4.3v = 0-100% 
    document.getElementById("batteryVoltage").innerHTML= ((BatVoltage - 3.3) * 100).toFixed(0) + "%"; 
    document.getElementById("LevelTemperature").innerHTML= Temperature + "&degC";
}
function sendMessage(event) 
{
    websocket.send(sendJSONstring());
    console.log('Browser sent message');
}
function sendJSONstring(){
    //set up JSON message before sending mesage to server
    var jsonSend =
    {"Calibrate": calibrate.checked}; //calibrate level
        return(JSON.stringify(jsonSend));   
}

function waitSwitchOn()
{
    setTimeout(confirmRecal, 0);
}
function confirmRecal()
{ 
    if (window.confirm("Recalibrate to 6 degrees?"))
    { 
        sendMessage();
    }
    setTimeout(resetSwitch, 0);   
}
function resetSwitch()
{
    calibrate.checked=false;
}