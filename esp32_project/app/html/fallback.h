#pragma once

static const char* fallback_html = R"rawhtml(
<!DOCTYPE html>
<html>
<head>
    <title>RC Car 123</title>
    <style>
        body { font-family: Arial; text-align: center; background: #1a1a1a; color: white; }
    </style>
</head>
<body>
    <h1>Web Interface Unavailable</h1>
    <p>Please check the SD card and make sure <b>index.html</b> is present.</p>
    <p>Insert the SD card and reboot the device.</p>
</body>
</html>
)rawhtml";
