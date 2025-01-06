/*
 * Fernotron 2 MQTT
 *
 * File: index_html.h
 *
 * HTML code for displaying the command history in a browser.
 *
 */

/**********************************************************************************
 *
 * Includes
 *
 **********************************************************************************/
#include <Arduino.h>

/**********************************************************************************
 *
 * HTML Code index.html
 *
 **********************************************************************************/
const char index_html[] PROGMEM = R"stringliteral(
<!DOCTYPE HTML>
<html>
<head>
<style>
  * {
  padding: 0;
  margin: 0;
  box-sizing: border-box;
}
body {
  font-family: sans-serif;
  line-height: 1;
  font-weight: 400;
  color: #555;
  margin: 1.6rem !important;
  overflow-x: hidden;
}
h1 {
  text-align: center;
  line-height: 1.8;
  font-weight: 700;
  color: #333;
}
h2 {
  text-align: center;
  line-height: 1.4;
  font-weight: 600;
  color: #333;
  margin-top: 2rem;
}
h3 {
  text-align: center;
  font-weight: 600;
  line-height: 0.6;
  color: teal;
  margin-top: 0.8rem;
}
table {
  text-align: center;
  margin-top: 1rem;
}
th {
  color: royalblue; 
}
td {
  padding: 0.3rem;
}
.centered {
  display: flex;
  align-items: center;
  justify-content: center;
}
</style>
</head>
<body>
  <h1>Fernotron 2 MQTT Gateway</h1>
  <h3>Wifi connection: %WIFIRSSI% dBm</h2>
  <h3>433Mhz connection: %C1101RSSI% dBm</h2>
  <h2>Command History</h2>
  <p>%TABLE%</p>
</body>
</html>
)stringliteral";
