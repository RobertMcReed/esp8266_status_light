const char HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>ESP8266 Status Light</title>
    <link href="https://cdn.jsdelivr.net/gh/RobertMcReed/esp8266_status_light/var/www/index.min.css" rel="stylesheet" />
  </head>
  <body>
    <div id="default">
      <p>If you're seeing this message something has broken.</p>
      <p>Open the console to see if there are any useful errors.</p>
    </div>
    <script src="https://cdn.jsdelivr.net/gh/RobertMcReed/esp8266_status_light@789c187d6b7c4201966312cb33311f272a1ac6d2/var/www/index.min.js" type="text/javascript"></script>
  </body>
</html>)rawliteral";
