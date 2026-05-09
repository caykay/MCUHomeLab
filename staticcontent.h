static const char *APIndexContent = R"(
  <!DOCTYPE html>
  <html>
    <body>
      <p>Welcome to my ESP32 Board</p>
      <form action="/connect-wifi">
        <input type="submit" value="Configure Wifi"/>
      </form>
    </body>
  </html>
)";

static const char *APWifiSetupContent = R"(
  <!DOCTYPE html>
  <html>
    <body>
     <p>Setup the STA wifi:</p>
     <form method='post' action='/connect-wifi'>
       <input name="ssid" type='text' placeholder='ssid' required/>
       <input name='password' type='password' placeholder='password' required/>
       <input type="submit" value="Connect"/>
     </form>
    <!-- error message placeholder -->
    <p>%s</p>
    </body>
  </html>
)";

static const char *APEndedContent = R"(
  <!DOCTYPE html>
  <html>
    <body>
      <p>Connected to Wifi. Closing ESP32 Wifi Access Point.</p>
    </body>
  </html>
)";

static const char *STAIndexContent = R"(
<!doctype html>
<html lang="en">
  <head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width,initial-scale=1" />
    <title>Remote LED Toggle</title>
    <style>
      :root {
        --bg: #0f1724;
        --card: #111827;
        --muted: #9ca3af;
        --accent: #10b981;
        --danger: #ef4444;
      }
      html,
      body {
        height: 100%;
      }
      body {
        margin: 0;
        font-family:
          system-ui,
          -apple-system,
          Segoe UI,
          Roboto,
          "Helvetica Neue",
          Arial;
        background: linear-gradient(180deg, var(--bg), #071021);
        color: #e6eef8;
        display: grid;
        place-items: center;
        padding: 24px;
      }
      .card {
        background: linear-gradient(
          180deg,
          rgba(255, 255, 255, 0.02),
          rgba(255, 255, 255, 0.01)
        );
        border: 1px solid rgba(255, 255, 255, 0.04);
        padding: 20px;
        border-radius: 12px;
        width: 320px;
        text-align: center;
        box-shadow: 0 6px 30px rgba(2, 6, 23, 0.6);
      }
      .led {
        width: 92px;
        height: 92px;
        border-radius: 50%;
        margin: 10px auto 18px;
        background: #11161b;
        box-shadow: inset 0 -6px 18px rgba(0, 0, 0, 0.6);
        transition:
          box-shadow 240ms linear,
          background-color 240ms linear,
          transform 120ms ease;
        display: flex;
        align-items: center;
        justify-content: center;
        color: #0b1220;
        font-weight: 700;
        font-size: 12px;
        letter-spacing: 0.08em;
      }
      .led.on {
        background: linear-gradient(180deg, #5ef6b1, #06a56a);
        box-shadow:
          0 0 28px rgba(16, 185, 129, 0.34),
          inset 0 -6px 18px rgba(0, 0, 0, 0.2);
        transform: translateY(-2px);
        color: #042014;
      }
      button {
        background: linear-gradient(180deg, #0ea5a8, #08857e);
        color: white;
        border: 0;
        padding: 12px 18px;
        border-radius: 8px;
        font-size: 15px;
        cursor: pointer;
        width: 100%;
        transition:
          transform 120ms ease,
          opacity 120ms ease;
      }
      button:disabled {
        opacity: 0.6;
        cursor: wait;
        transform: none;
      }
      .muted {
        color: var(--muted);
        font-size: 13px;
        margin-top: 10px;
      }
      .error {
        color: var(--danger);
        font-size: 13px;
        margin-top: 10px;
      }
    </style>
  </head>
  <body>
    <main class="card" role="main">
      <h1 style="margin: 0 0 6px; font-size: 18px">LED Remote Control</h1>
      <div id="led" class="led" aria-hidden="true">OFF</div>
      <button id="toggleBtn" type="button" aria-pressed="false">
        Toggle LED
      </button>
      <div id="hint" class="muted">
        Click to toggle the LED at endpoint <code>/toggleLED</code>
      </div>
      <div id="error" class="error" style="display: none"></div>
    </main>

    <script>
      (function () {
        const ledEl = document.getElementById("led");
        const btn = document.getElementById("toggleBtn");
        const hint = document.getElementById("hint");
        const errEl = document.getElementById("error");

        // Visual state: true = on, false = off
        let state = false;

        function render() {
          if (state) {
            ledEl.classList.add("on");
            ledEl.textContent = "ON";
            btn.setAttribute("aria-pressed", "true");
            hint.textContent = "LED is ON";
          } else {
            ledEl.classList.remove("on");
            ledEl.textContent = "OFF";
            btn.setAttribute("aria-pressed", "false");
            hint.textContent = "LED is OFF";
          }
        }

        async function toggleRemote() {
          errEl.style.display = "none";
          btn.disabled = true;
          // Optimistic UI: flip locally first for snappy feel
          const previous = state;
          state = !state;
          render();

          try {
            const res = await fetch("/toggleLED", {
              method: "POST",
              headers: { Accept: "application/json" },
              // Add body or credentials here if your API requires them
            });

            if (!res.ok) {
              throw new Error(
                'Network response was not ok (' + res.status + ')',
              );
            }

            // Try to parse JSON state if provided: { state: "on" } or {state: true}
            let data;
            try {
              data = await res.json();
            } catch (e) {
              data = null;
            }

            if (data && typeof data.state !== "undefined") {
              // normalize a couple of possible formats
              if (typeof data.state === "string") {
                state =
                  data.state.toLowerCase() === "on" ||
                  data.state.toLowerCase() === "true";
              } else {
                state = Boolean(data.state);
              }
              render();
            } else {
              // Server didn't return explicit state: assume request succeeded and keep optimistic state
            }
          } catch (err) {
            // Revert optimistic change on error
            state = previous;
            render();
            errEl.textContent =
              "Failed to toggle LED: " + (err.message || "Unknown error");
            errEl.style.display = "block";
          } finally {
            btn.disabled = false;
          }
        }

        // Initial render
        render();

        btn.addEventListener("click", toggleRemote);

        // poll initial state on load if your endpoint supports GET /toggleLED or /ledState
        (async function fetchInitial(){
            try {
                const r = await fetch('/LEDstate');
                if (!r.ok) return;
                const d = await r.json();
                if (d && typeof d.state !== 'undefined') {
                    state = (typeof d.state === 'string') ? d.state.toLowerCase() === 'on' : Boolean(d.state);
                    render();
                }
            } catch(e){}
        })();
      })();
    </script>
  </body>
</html>
)";

static const char *NotFoundContent = R"(
  <!DOCTYPE html>
  <html>
    <body>
      <p>Page not found </p>
    </body>
  </html>
)";
