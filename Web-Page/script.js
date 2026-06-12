const versions = [
    {
        v:"V1",
        name:"Basic Button Control",
        desc:"Simple web buttons for Forward, Backward, Left, Right and Stop control."
    },
    {
        v:"V2",
        name:"Smooth Button Control",
        desc:"Motor acceleration and deceleration smoothing for premium driving feel."
    },
    {
        v:"V3",
        name:"Toggle / Hold Control",
        desc:"Added both hold-to-drive and toggle driving control modes."
    },
    {
        v:"V4",
        name:"Joystick Control",
        desc:"Virtual joystick based steering with smooth speed and direction control."
    },
    {
        v:"V5",
        name:"Gyroscope Control",
        desc:"Smartphone tilt based rover control using motion sensor input."
    },
    {
        v:"V6",
        name:"BLE Bluefy Web Controller",
        desc:"Bluetooth Low Energy control directly from iPhone using Bluefy browser."
    },
    {
        v:"V7",
        name:"Internet Cloud Control",
        desc:"Wi-Fi and MQTT based control from anywhere through internet dashboard."
    },
    {
        v:"V8",
        name:"Voice Command Control",
        desc:"Siri Shortcut based voice commands for hands-free rover control."
    },
    {
        v:"V9",
        name:"Draw Path Control",
        desc:"Draw a path on the phone screen and the rover follows the route."
    },
    {
        v:"V10",
        name:"AI Command Control",
        desc:"AI understands natural language commands and converts them into rover actions."
    },
    {
        v:"V11",
        name:"AI Vision Control",
        desc:"Camera based AI vision for tracking, recognition and autonomous behavior."
    }
];

const list = document.getElementById("versionList");
const focusLayer = document.getElementById("focusLayer");

versions.forEach(item => {
    const card = document.createElement("div");
    card.className = "version-card";

    card.innerHTML = `
        <div class="version-no">${item.v}</div>
        <div class="version-name">${item.name}</div>
        <div class="version-desc">${item.desc}</div>
    `;

    card.addEventListener("click", () => {
        list.classList.add("blurred");
        focusLayer.classList.add("show");
        card.classList.add("active");
    });

    list.appendChild(card);
});

focusLayer.addEventListener("click", () => {
    document.querySelectorAll(".version-card").forEach(card => {
        card.classList.remove("active");
    });

    list.classList.remove("blurred");
    focusLayer.classList.remove("show");
});

document.addEventListener("mousemove", e => {
    document.body.style.setProperty("--mouse-x", e.clientX + "px");
    document.body.style.setProperty("--mouse-y", e.clientY + "px");
});