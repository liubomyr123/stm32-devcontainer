#pragma once

static const char* html = R"rawhtml(
<!DOCTYPE html>
<html>
<head>
    <title>RC Car 123</title>
    <style>
        body { font-family: Arial; text-align: center; background: #1a1a1a; color: white; }
        button { 
            width: 120px; height: 60px; 
            margin: 10px; font-size: 20px;
            border-radius: 10px; border: none;
            cursor: pointer; background: #444;
            color: white;
        }
        button:active { background: #666; }
        #stream { display: none; width: 100%; }
    </style>
</head>
<body>
    <h1>RC Car 11111</h1>
    <div><button onclick="send('F:80')">Forward</button></div>
    <div><button onclick="send('S')">Stop</button></div>
    <div><button onclick="send('B:80')">Backward</button></div>
    <img id="stream" />
    <button onclick="startStream()">Start</button>
    <button onclick="stopStream()">Stop</button>
    <script>
        function send(cmd) {
            fetch('/cmd?v=' + cmd);
        }
        function startStream() {
            const img = document.getElementById('stream');
            img.style.display = 'block';
            img.src = '/stream?' + Date.now();
        }

        function stopStream() {
            fetch('/stream/stop');
            const img = document.getElementById('stream');
            img.style.display = 'none';
            img.src = '';
        }
    </script>
</body>
</html>
)rawhtml";
