const char *index_html = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <style>
      html {
        font-family: Helvetica;
        display: inline-block;
        margin: 0px auto;
        text-align: center;
      }
      .joystick-container {
        display: inline-block;
        margin: 20px;
      }
      .joystick {
        width: 100px;
        height: 100px;
        background: #d3d3d3;
        border-radius: 50%;
        position: relative;
      }
      .knob {
        width: 30px;
        height: 30px;
        background: #04aa6d;
        border-radius: 50%;
        position: absolute;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
      }
      .button {
        margin: 5px;
        padding: 5px 5px;
        font-size: 10px;
      }
    </style>
    <script>
      let leftJoystick = document.getElementById("leftJoystick");
      let leftKnob = document.getElementById("leftKnob");
      let rightJoystick = document.getElementById("rightJoystick");
      let rightKnob = document.getElementById("rightKnob");

      function sendJoystickValues(leftX, leftY, rightX) {
        fetch("/control", {
          method: "POST",
          headers: {
            "Content-Type": "application/x-www-form-urlencoded",
          },
          body: `leftX=${leftX}&leftY=${leftY}&rightX=${rightX}`,
        })
          .then((response) => response.text())
          .then((data) => console.log(data));
      }

      // Changes the button to be green when on and red when off
      function fieldCentric() {
        let button = document.getElementsByClassName("button")[1];
        if (button.style.backgroundColor === "green") {
          button.style.backgroundColor = "red";
          sendCommand("fieldCentricOff");
        } else {
          button.style.backgroundColor = "green";
          sendCommand("fieldCentricOn");
        }
      }

      function sendCommand(command) {
        fetch("/control", {
          method: "POST",
          headers: {
            "Content-Type": "application/x-www-form-urlencoded",
          },
          body: `command=${command}`,
        })
          .then((response) => response.text())
          .then((data) => console.log(data));
      }

      function updateKnobPosition(knob, x, y) {
        knob.style.left = `${x}%`;
        knob.style.top = `${y}%`;
      }

      function handleJoystickMove(event, joystick, knob, isLeft) {
        let rect = joystick.getBoundingClientRect();
        let x, y;
        if (event.touches) {
          x = ((event.touches[0].clientX - rect.left) / rect.width) * 100;
          y = ((event.touches[0].clientY - rect.top) / rect.height) * 100;
        } else {
          x = ((event.clientX - rect.left) / rect.width) * 100;
          y = ((event.clientY - rect.top) / rect.height) * 100;
        }
        x = Math.max(0, Math.min(100, x));
        y = Math.max(0, Math.min(100, y));
        updateKnobPosition(knob, x, y);

        if (isLeft) {
          document.getElementById("leftX").innerText = x.toFixed(0);
          document.getElementById("leftY").innerText = y.toFixed(0);
          sendJoystickValues(
            x.toFixed(0),
            y.toFixed(0),
            document.getElementById("rightX").innerText
          );
        } else {
          document.getElementById("rightX").innerText = x.toFixed(0);
          sendJoystickValues(
            document.getElementById("leftX").innerText,
            document.getElementById("leftY").innerText,
            x.toFixed(0)
          );
        }
      }

      function startJoystickMove(event, joystick, knob, isLeft) {
        function moveHandler(e) {
          handleJoystickMove(e, joystick, knob, isLeft);
        }

        function endHandler() {
          document.removeEventListener("mousemove", moveHandler);
          document.removeEventListener("mouseup", endHandler);
          document.removeEventListener("touchmove", moveHandler);
          document.removeEventListener("touchend", endHandler);

          // Reset the knob position to the center
          if (isLeft) {
            document.getElementById("leftX").innerText = "50";
            document.getElementById("leftY").innerText = "50";
            sendJoystickValues(
              50,
              50,
              document.getElementById("rightX").innerText
            );
          } else {
            document.getElementById("rightX").innerText = "50";
            sendJoystickValues(
              document.getElementById("leftX").innerText,
              document.getElementById("leftY").innerText,
              50
            );
          }

          updateKnobPosition(knob, 50, 50);
        }

        document.addEventListener("mousemove", moveHandler);
        document.addEventListener("mouseup", endHandler);
        document.addEventListener("touchmove", moveHandler);
        document.addEventListener("touchend", endHandler);
      }

      leftJoystick.addEventListener("mousedown", (event) =>
        startJoystickMove(event, leftJoystick, leftKnob, true)
      );
      leftJoystick.addEventListener("touchstart", (event) =>
        startJoystickMove(event, leftJoystick, leftKnob, true)
      );
      rightJoystick.addEventListener("mousedown", (event) =>
        startJoystickMove(event, rightJoystick, rightKnob, false)
      );
      rightJoystick.addEventListener("touchstart", (event) =>
        startJoystickMove(event, rightJoystick, rightKnob, false)
      );

      function enterFullScreen() {
        if (document.documentElement.requestFullscreen) {
          document.documentElement.requestFullscreen();
        } else if (document.documentElement.mozRequestFullScreen) {
          // Firefox
          document.documentElement.mozRequestFullScreen();
        } else if (document.documentElement.webkitRequestFullscreen) {
          // Chrome, Safari and Opera
          document.documentElement.webkitRequestFullscreen();
        } else if (document.documentElement.msRequestFullscreen) {
          // IE/Edge
          document.documentElement.msRequestFullscreen();
        }
      }
    </script>
  </head>
  <body>
    <title>Buddy Controller</title>
    <h1>Buddy Controller</h1>
    <button class="button" onclick="enterFullScreen()">
      Enter Full Screen
    </button>
    <button class="button" onclick="fieldCentric()">Field Centric</button>
    <button class="button" onclick="sendCommand('resetHeading')">
      Reset Heading
    </button>
    <br />
    <br />
    <div class="joystick-container" style="margin-right: 32vw">
      <div class="joystick" id="leftJoystick">
        <div class="knob" id="leftKnob"></div>
      </div>
      <p><span id="leftX">0</span>, <span id="leftY">0</span></p>
    </div>
    <div class="joystick-container">
      <div class="joystick" id="rightJoystick">
        <div class="knob" id="rightKnob"></div>
      </div>
      <p><span id="rightX">0</span></p>
    </div>
  </body>
</html>
)rawliteral";
