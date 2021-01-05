(async () => {
  // what will get rendered as button groups
  const configGroups = {
    speed: [1, 2, 3, 4, 5],
    status: [
      ["Free", "Busy", "DND"],
      ["Unknown", "Party!", "Custom"],
    ],
    mode: [
      ["off", "solid", "breath", "marquee"],
      ["theater", "rainbow", "rainbow_marquee", "rainbow_theater"],
    ],
  };

  // all of our state, flags, and useful html nodes live here
  const app = {
    proxy: window.localStorage.getItem("__esp8266Proxy") || "",
    state: {},
    config: {
      keys: Object.keys(configGroups),
      groups: {},
    },
    modal: {},
    debounce: {
      color: Date.now(),
      viewport: 1,
    },
    localStorage: {
      hostname: "__esp8266Hostname",
      customStatus: "__esp8266CustomStatus",
    },
  };

  // remove the default text displayed in case of an error
  const clearDefault = () => {
    document.body.removeChild(document.querySelector("#default"));
  };

  // add bootstrap stylesheet
  const augmentHeader = () => {
    app.head = document.querySelector("head");
    app.head.append(
      ...buildComponents(
        `<link
          href="https://cdn.jsdelivr.net/npm/bootstrap@5.0.0-beta1/dist/css/bootstrap.min.css"
          rel="stylesheet"
          integrity="sha384-giJF6kkoqNQ00vy+HMDP7azOuL0xtbfIcaT9wjKHr8RbDVddVHyTfAAsrekwKmP1"
          crossorigin="anonymous"
        />`,
        `<link
          rel="stylesheet"
          href="https://cdn.jsdelivr.net/npm/@simonwep/pickr/dist/themes/nano.min.css"
        />`
      )
    );
  };

  // add body structure and bootstrap js
  const augmentBody = async () => {
    document.body.classList.add("container-fluid");
    await addScript(
      "https://cdn.jsdelivr.net/npm/bootstrap@5.0.0-beta1/dist/js/bootstrap.bundle.min.js",
      [
        [
          "integrity",
          "sha384-ygbV9kiqUc6oa4msXn9868pTtWMgiQaeYH7/t7LECLbyPA2x65Kgf80OJFdroafW",
        ],
        ["crossorigin", "anonymous"],
      ]
    );
    document.body.append(
      ...buildComponents(
        `<header class="container">
          <h1 id="title">ESP8266 Status Light</h1>
          <a
            class="btn btn-outline-dark github"
            href="https://github.com/RobertMcReed/esp8266_status_light"
          >
            <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" class="bi bi-github" viewBox="0 0 16 16">
            <path fill-rule="evenodd" d="M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17.55-.38 0-.19-.01-.82-.01-1.49-2.01.37-2.53-.49-2.69-.94-.09-.23-.48-.94-.82-1.13-.28-.15-.68-.52-.01-.53.63-.01 1.08.58 1.23.82.72 1.21 1.87.87 2.33.66.07-.52.28-.87.51-1.07-1.78-.2-3.64-.89-3.64-3.95 0-.87.31-1.59.82-2.15-.08-.2-.36-1.02.08-2.12 0 0 .67-.21 2.2.82.64-.18 1.32-.27 2-.27.68 0 1.36.09 2 .27 1.53-1.04 2.2-.82 2.2-.82.44 1.1.16 1.92.08 2.12.51.56.82 1.27.82 2.15 0 3.07-1.87 3.75-3.65 3.95.29.25.54.73.54 1.48 0 1.07-.01 1.93-.01 2.2 0 .21.15.46.55.38A8.012 8.012 0 0 0 16 8c0-4.42-3.58-8-8-8z"/>
          </svg>
          </a>
        </header>`,
        `<main>
          <div class="container">
            <h2>color</h2>
            <div class="picker"></div>
          </div>
          <div class="container">
            <h2>brightness</h2>
            <input type="range" class="form-range" id="brightness" min="0" max="150">
          </div>
        </main>`,
        `<div class="modal fade" id="staticBackdrop" data-bs-backdrop="static" data-bs-keyboard="false" tabindex="-1" aria-labelledby="staticBackdropLabel" aria-hidden="true">
          <div class="modal-dialog">
            <div class="modal-content">
              <div class="modal-header">
                <h5 class="modal-title" id="staticBackdropLabel">Communication Error</h5>
                <button type="button" id="modal-close-btn" style="display: none;" class="btn-close" data-bs-dismiss="modal">
              </div>
              <div class="modal-body">
                <p>An error occurred while attempting to communicate with the device.</p>
                <p>Make sure it is powered on and connected to the same network.</p>
              </div>
              <div class="modal-footer">
                <button type="button" id="refresh" class="btn btn-primary">Refresh</button>
              </div>
            </div>
          </div>
        </div>`,
        `<button
          type="button"
          id="modal-open-btn"
          style="display: none;"
          data-bs-toggle="modal"
          data-bs-target="#staticBackdrop"
        />`
      )
    );

    handleUpdateHost();
    const refreshBtn = document.querySelector("#refresh");
    const openBtn = document.querySelector("#modal-open-btn");
    const closeBtn = document.querySelector("#modal-close-btn");
    app.brightness = document.querySelector("#brightness");

    app.brightness.addEventListener("change", handleChangeBrightness);

    refreshBtn.addEventListener("click", handleRefresh);

    app.modal = {
      open: () => openBtn.click(),
      close: () => closeBtn.click(),
    };
  };

  // add all config groups and buttons, storing each in app.config
  const augmentMain = () => {
    app.main = document.querySelector("main");
    app.config.keys.map((configKey) => {
      const stacked = Array.isArray(configGroups[configKey][0]);
      const [container] = buildComponents(
        `<div class="container ${stacked ? "stacked" : ""} ${configKey}">
          <h2>${configKey}</h2>
        </div>`
      );
      app.main.appendChild(container);

      const groupsArr = stacked
        ? configGroups[configKey]
        : [configGroups[configKey]];

      app.config.groups[configKey] = {
        container,
        rows: [],
      };

      groupsArr.forEach((btnRow, rowNum) => {
        const [row] = buildComponents(
          `<div
            class="btn-group radio full-width-sm block"
            role="group"
            id="${configKey}"
          ></div>`
        );

        container.appendChild(row);

        app.config.groups[configKey].rows.push(row);

        const children = buildComponents(
          ...btnRow.flatMap((optionKey) => [
            `<input type="radio" class="btn-check" autocomplete="off" id="btn-${configKey}-${optionKey}" name="${configKey}" data-value="${optionKey}"></input>"`,
            `<label class="btn btn-outline-dark" for="btn-${configKey}-${optionKey}">${optionKey}</label>`,
          ])
        );

        row.append(...children);

        app.config.groups[configKey].labels = children
          .filter((i) => !(i % 2))
          .reduce(
            (acc, label) => ({ ...acc, [label.textContent]: label }),
            app.config.groups[configKey].labels || {}
          );

        // if we click, send the request to the api
        // unless the state's already match, which implies we
        // triggered the click ourselves from the update
        row.addEventListener("change", handleButtonClick(configKey));
      });

      if (configKey === "status") {
        const [customStatus] = buildComponents(
          `<input
            type="text"
            class="form-control"
            style="display: none;"
            id="custom-status-field"
            maxlength="60"
            placeholder="Enter a custom status..."
          />`
        );
        customStatus.addEventListener("keyup", handleCustomStatus);
        app.config.customStatus = customStatus;
        customStatus.value = window.localStorage.getItem(
          app.localStorage.customStatus
        );
        container.appendChild(customStatus);
      }
    });
  };

  const initPicker = async () => {
    await addScript(
      "https://cdn.jsdelivr.net/npm/@simonwep/pickr/dist/pickr.min.js"
    );
    app.picker = new Pickr({
      el: ".picker",
      theme: "nano",
      comparison: false,
      defaultRepresentation: "RGBA",
      position: "right-end",
      default: "rgba(255,255,255,0)",
      components: {
        opacity: true,
        hue: true,
        preview: true,
        useAsButton: true,
        interaction: {
          input: true,
        },
      },
    });

    app.picker.on("change", handleColorChange);
    app.picker.on("init", () => {
      app.pickerBtn = document.querySelector(".pcr-button");
    });
  };

  // build the ui, add the interactivity, and fetch the current state
  const render = async () => {
    clearDefault();
    augmentHeader();
    await augmentBody();
    augmentMain();
    handleResize();
    window.onresize = handleResize;
    await initPicker().then(handleRefresh);
  };

  // ------------------------------------------------------------------
  // helpers lie below //
  // ------------------------------------------------------------------

  // make a request to our api
  function F(path, body) {
    return window
      .fetch(
        `${app.proxy}${path}`,
        body ? { method: "post", body: JSON.stringify(body) } : {}
      )
      .then((r) => r.json())
      .then((j) => (app.proxy && console.log(j)) || j)
      .catch((e) => {
        handleError();

        throw e;
      });
  }

  // shortcut to make a request to the api with a single key-value pair
  // and then update our state with the return value
  function S(p, k, v) {
    return F(p, { [k]: v }).then(U);
  }

  // update state with the results from the api
  function U({ color, ...apiState }, setColorPicker = true) {
    const { status, mode } = apiState;
    if (color) {
      const [r, g, b] = color;
      const a = color[3] / 150; // 150 = MAX_ALPHA

      // need to update color picker if setting color via a status click
      if (setColorPicker)
        app.picker.setColor(`rgba(${[r, g, b, a].join(",")})`);

      document.body.classList[status === "Party!" ? "add" : "remove"](
        "rainbow"
      );

      if (mode === "off" || status === "Party!") {
        app.pickerBtn.style.color = "transparent";
      }

      app.state.color = [r, g, b, a];
    }

    app.state = { ...app.state, ...apiState };
    app.brightness.value = apiState.brightness;

    const y =
      window.pageYOffset ||
      (document.documentElement || document.body.parentNode || document.body)
        .scrollTop;

    if (apiState.status !== undefined) {
      const isCustom = !configGroups.status.flat().includes(apiState.status);
      handleEnsureCustomStatus(isCustom); // ensure correct visibility

      if (isCustom) {
        app.config.customStatus.value = apiState.status;
        apiState.status = "Custom"; // to ensure we toggle that radio button
      }
    }

    // click all of the appropriate buttons so the ui state matches the api state
    app.config.keys.forEach((configKey) => {
      app.config.groups[configKey].labels[apiState[configKey]].click();
    });

    window.scrollTo(0, y);
  }

  function handleCustomStatus({ target: { value } }) {
    clearTimeout(app.debounce.customStatus);

    if (value) {
      app.debounce.customStatus = setTimeout(() => {
        window.localStorage.setItem(app.localStorage.customStatus, value);
        S("/config", "status", value);
      }, 500);
    }
  }

  // refresh the state
  function handleRefresh() {
    return F("/config/state").then(U).then(app.modal.close);
  }

  // error modal
  function handleError() {
    app.modal.open();
  }

  // fetch hostname and update title and header
  function handleUpdateHost() {
    const hostname = window.localStorage.getItem(app.localStorage.hostname);
    if (hostname) updateSiteHost({ hostname });
    F("/hostname").then(updateSiteHost);
  }

  function updateSiteHost({ hostname }) {
    if (hostname === app.hostname) return;

    window.localStorage.setItem(app.localStorage.hostname, hostname);
    const title = `${hostname[0].toUpperCase()}${hostname.slice(
      1
    )}'s Status Light`;
    document.querySelector("#title").textContent = title;
    document.title = title;
  }

  function handleButtonClick(configKey) {
    return ({ target: { dataset } }) => {
      const isCustom = dataset.value === "Custom";
      const value = isCustom ? app.config.customStatus.value : dataset.value;

      // don't send duplicate requests
      if (value == app.state[configKey]) {
        // intentionally use soft equality for the sake of numbers
        return;
      }

      // this will be called on update if
      // the status is changed by the api
      if (configKey === "status") {
        handleEnsureCustomStatus(isCustom);
      }

      return (!isCustom || value) && S("/config", configKey, value);
    };
  }

  function handleEnsureCustomStatus(isCustom) {
    if (isCustom && !app.config.showCustom) {
      app.config.customStatus.style.display = "block";
      app.config.groups.status.container.classList.add("custom-visible");
    } else if (!isCustom && app.config.showCustom) {
      app.config.customStatus.style.display = "none";
      app.config.groups.status.container.classList.remove("custom-visible");
    }
    app.config.showCustom = isCustom;
  }

  // send color change request
  function handleColorChange(hsvColor) {
    // if the picker isn't open, then we are updating from a trigger we just sent, so we don't need to send another request
    if (!app.picker.isOpen()) {
      return;
    }

    const now = Date.now();

    if (now - app.debounce.color > 250) {
      app.debounce.color = now;
      const rgba = hsvColor.toRGBA(1);
      const [r, g, b] = rgba.map((n) => parseInt(n, 10));
      const a = rgba[3] * 150; // map from (0,1) to (0, 150) [MAX_ALPHA]
      F("/config", { color: [r, g, b, a] }).then((r) => U(r, 0)); // 0 indicates we shouldn't update the picker color
    }
  }

  // send color change request
  function handleChangeBrightness({ target: { value: brightness } }) {
    F("/config", { brightness }).then(U);
  }

  // stack buttons when viewport < 800
  function handleResize() {
    let trigger = 0;
    if (document.body.clientWidth < 800) {
      // 800 matches the full-width-sm breakpoint
      // we just switched to small
      if (app.debounce.viewport) {
        app.debounce.viewport = trigger++; // fancy 0
      }
      // we just switched to big
    } else if (!app.debounce.viewport) {
      app.debounce.viewport = ++trigger; // fancy 1
    }

    if (trigger) {
      // make buttons pretty
      app.config.keys.forEach((configKey) => {
        if (configKey === "speed") return;

        app.config.groups[configKey].rows.forEach((row, rowNum) => {
          if (configKey === "status" && !rowNum) {
            app.config.groups[configKey].container.classList[
              app.debounce.viewport ? "remove" : "add"
            ]("stacked-topper");
          } else {
            row.classList[app.debounce.viewport ? "remove" : "add"](
              "btn-group-vertical"
            );
          }
        });
      });
    }
  }

  // pass in some html strings and get some html nodes back (single => node or many multiple => [node1, node2])
  function buildComponents(...components) {
    const d = document.createElement("div");
    d.innerHTML = components.join("");

    return [...d.children];
  }

  async function addScript(src, kvs = []) {
    const script = document.createElement("script");
    script.src = src;
    kvs.forEach(([k, v]) => {
      script.setAttribute(k, v);
    });

    return new Promise((resolve) => {
      script.onload = resolve;
      app.head.appendChild(script);
    });
  }

  // ------------------------------------------------------------------
  // Here we go!
  // ------------------------------------------------------------------
  render();
})();
