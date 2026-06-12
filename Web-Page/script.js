const versions = [
  ["V1","Basic Button Control","Simple forward, backward, left, right and stop web buttons."],
  ["V2","Smooth Button Control","Smooth acceleration and deceleration for better driving."],
  ["V3","Toggle / Hold Control","Hold and toggle driving modes added."],
  ["V4","Joystick Control","Virtual joystick based smooth rover control."],
  ["V5","Gyroscope Control","Phone tilt based motion control."],
  ["V6","BLE Bluefy Controller","Bluetooth Low Energy control using Bluefy browser."],
  ["V7","Internet Cloud Control","Wi-Fi and MQTT based worldwide control."],
  ["V8","Voice Command Control","Siri Shortcut based voice control."],
  ["V9","Draw Path Control","Draw a path and rover follows it."],
  ["V10","AI Command Control","AI converts natural language into rover actions."],
  ["V11","AI Vision Control","Camera based AI tracking and recognition."]
];

const list = document.getElementById("list");

let activeItem = null;
let timer = null;

versions.forEach(v=>{
  const slot = document.createElement("div");
  slot.className = "slot";

  const item = document.createElement("div");
  item.className = "item";

  item.innerHTML = `
    <div class="check"></div>
    <div class="v">${v[0]}</div>

    <div class="text">
      <div class="name">${v[1]}</div>
      <div class="desc"></div>
    </div>

    <button class="expand">+</button>
  `;

  const check = item.querySelector(".check");
  const expand = item.querySelector(".expand");
  const name = item.querySelector(".name");
  const desc = item.querySelector(".desc");

  check.onclick = e=>{
    e.stopPropagation();
    check.classList.toggle("checked");
  };

  expand.onclick = e=>{
    e.stopPropagation();

    if(item.classList.contains("open")){
      closeItem(item);
      return;
    }

    if(activeItem && activeItem !== item){
      forceClose(activeItem);
    }

    check.classList.add("checked");

    setTimeout(()=>{
      item.classList.add("open");
      expand.classList.add("open");
      expand.textContent = "−";

      activeItem = item;

      setTimeout(()=>{
        showHeadingThenTypeDesc(name, desc, v[1], v[2]);
      },180);
    },90);
  };

  slot.appendChild(item);
  list.appendChild(slot);
});

function showHeadingThenTypeDesc(nameEl, descEl, headingText, descText){
  clearInterval(timer);

  nameEl.dataset.full = headingText;
  nameEl.textContent = headingText;
  descEl.textContent = "";

  setTimeout(()=>{
    type(descEl, descText);
  },150);
}

function closeItem(item){
  const expand = item.querySelector(".expand");
  const desc = item.querySelector(".desc");
  const name = item.querySelector(".name");

  reverseType(desc,()=>{
    name.textContent = name.dataset.full || name.textContent;

    item.classList.remove("open");
    expand.classList.remove("open");
    expand.textContent = "+";

    if(activeItem === item){
      activeItem = null;
    }
  });
}

function forceClose(item){
  clearInterval(timer);

  item.classList.remove("open");

  item.querySelector(".expand").classList.remove("open");
  item.querySelector(".expand").textContent = "+";

  const name = item.querySelector(".name");
  const desc = item.querySelector(".desc");

  if(name.dataset.full){
    name.textContent = name.dataset.full;
  }

  desc.textContent = "";

  if(activeItem === item){
    activeItem = null;
  }
}

function type(el,text){
  clearInterval(timer);

  el.textContent = "";

  let i = 0;

  timer = setInterval(()=>{
    el.textContent += text[i++];

    if(i >= text.length){
      clearInterval(timer);
    }
  },9);
}

function reverseType(el,done){
  clearInterval(timer);

  timer = setInterval(()=>{
    el.textContent = el.textContent.slice(0,-1);

    if(el.textContent.length === 0){
      clearInterval(timer);
      done();
    }
  },5);
}

document.addEventListener("click",()=>{
  if(activeItem){
    closeItem(activeItem);
  }
});