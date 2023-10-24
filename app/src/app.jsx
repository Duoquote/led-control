import { useEffect, useMemo, useState } from "preact/hooks";
import { css, cx } from "@emotion/css";
import mngen from "mngen";

export function App() {

  const [ssid, setSSID] = useState("");
  const [password, setPassword] = useState("");
  const [token, setToken] = useState(mngen.word(4));
  const [loading, setLoading] = useState(false);

  useEffect(() => navigator.clipboard.writeText(token), [token]);

  const applyConfig = useMemo(() => () => {
    const params = new URLSearchParams();
    params.append("ssid", ssid);
    params.append("password", password);
    params.append("token", token);
    fetch("/save" + "?" + params.toString());
  }, [ssid, password, token]);

  return (
    <div class={css`
      width: 100%;
      transform: translateY(-50%);
      position: absolute;
      top: 50%;
    `}>
      <h1 class={css`
        text-align: center;
      `}>Config</h1>
      <div class={cx("content", css`
        width: 300px;
        margin: 0 auto;
      `)}>
        <div class={cx("pure-u-1", css`
          padding: 2em;
          box-sizing: border-box;
        `)}>
          <form class="pure-form pure-form-stacked">
            <fieldset>
              <label for="ssid">SSID</label>
              <input
                class="pure-input-1"
                id="ssid"
                type="text"
                value={ssid}
                onInput={e => setSSID(e.target.value)}
              />
              <label for="password">Password</label>
              <input
                class="pure-input-1"
                id="password"
                type="password"
                value={password}
                onInput={e => setPassword(e.target.value)}
              />
              <label for="token">Token</label>
              <input
                class="pure-input-1"
                id="token"
                type="text"
                value={token}
                onInput={e => setToken(e.target.value)}
              />
              <button
                class={cx("pure-button", css`
                  width: 100%;
                `)}
                type="button"
                disabled={loading}
                onClick={() => setToken(mngen.word(4))}
              >Generate</button>
              <br />
              <br />
              {/* <button
                class={cx("pure-button", css`
                  width: 100%;
                `)}
                type="button"
                disabled={!ssid || !password || !token || loading}
                onClick={testWifi}
              >Test</button>
              <br />
              <br /> */}
              <button
                class={cx("pure-button pure-button-primary", css`
                  width: 100%;
                `)}
                type="button"
                disabled={!ssid || !password || !token || loading}
                onClick={applyConfig}
              >Apply</button>
            </fieldset>
          </form>
        </div>
      </div>
    </div>
  )
}
