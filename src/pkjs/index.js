const Clay = require('@rebble/clay');
const clayConfig = require('./config');
const clay = new Clay(clayConfig, null, {autoHandleEvents: false});

let haUrl, haToken, haEntity;
const APP_MESSAGE_RETRY_COUNT = 3;
const APP_MESSAGE_RETRY_DELAY_MS = 10000;
const FETCH_RETRY_COUNT = 2;
const FETCH_RETRY_DELAY_MS = 30000;
let fetchedData = {
  forecast: null,
  sun: null
};
let sendingMessage = false;

function loadSettings() {
  haUrl = localStorage.getItem("HA_URL") || "";
  haToken = localStorage.getItem("HA_TOKEN") || "";
  haEntity = localStorage.getItem("HA_ENTITY") || "";
}

Pebble.addEventListener('showConfiguration', function(e) {
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (!e || !e.response) return;

  const decoded = decodeURIComponent(e.response);
  const dict = JSON.parse(decoded);

  haUrl = (dict.HAUrl && dict.HAUrl.value) || "";
  haToken = (dict.HAToken && dict.HAToken.value) || "";
  haEntity = (dict.HAEntity && dict.HAEntity.value) || "";

  localStorage.setItem("HA_URL", haUrl);
  localStorage.setItem("HA_TOKEN", haToken);
  localStorage.setItem("HA_ENTITY", haEntity);

  loadSettings();
});

Pebble.addEventListener('ready', function (e) {
  loadSettings();
  startUpdate();
});

function normalizeCondition(c) {
  const map = {
    // Clear / sun
    "clear-night": 0,       // sunny
    "sunny": 0,             // sunny
    // Clouds
    "partlycloudy-night": 1, // partly-cloudy
    "partlycloudy": 1,       // partly-cloudy
    "cloudy": 2,             // cloudy
    // Rain
    "rainy": 3,              // light-rain
    "pouring": 4,            // heavy-rain
    // Snow
    "snowy": 5,              // heavy-snow
    "snowy-rainy": 6,        // rain-snow
    // Other precipitation
    "hail": 4,               // heavy-rain (closest)
    // Storms
    "lightning-rainy": 4,    // heavy-rain
    "lightning": 4,          // heavy-rain
    // Wind/fog don't have dedicated icons
    "windy": 7,              // generic
    "windy-variant": 7,      // generic
    "fog": 7                 // generic
  };

  if (map[c] === undefined) {
    return 8; // unknown
  }

  return map[c];
}

function retryFetch(type, url, token, entity, retriesRemaining) {
  if (retriesRemaining <= 0) {
    return;
  }

  setTimeout(function() {
    if (type === "weather") {
      getWeather(url, token, entity, retriesRemaining - 1);
    } else if (type === "sun") {
      getSunData(url, token, retriesRemaining - 1);
    }
  }, FETCH_RETRY_DELAY_MS);
}

function startUpdate() {
  if (!haUrl || !haToken || !haEntity) {
    return;
  }

  fetchedData = {
    forecast: null,
    sun: null
  };

  getWeather(haUrl, haToken, haEntity, null);
  getSunData(haUrl, haToken, null);
}

function getWeather(url, token, entity, retriesRemaining) {
  if (retriesRemaining === undefined) {
    retriesRemaining = FETCH_RETRY_COUNT;
  }

  const RETURN_SIZE = 8;
  const baseUrl = url.endsWith("/") ? url.slice(0, -1) : url;
  const fullUrl = baseUrl + "/api/services/weather/get_forecasts?return_response=true";

  const xhr = new XMLHttpRequest();
  xhr.open("POST", fullUrl, true);
  xhr.setRequestHeader("Authorization", "Bearer " + token);
  xhr.setRequestHeader("Content-Type", "application/json");

  xhr.onload = function() {
    if (xhr.status !== 200) {
      retryFetch("weather", url, token, entity, retriesRemaining);
      return;
    }

    try {
      const data = JSON.parse(xhr.responseText);
      const raw = data.service_response[entity] ? data.service_response[entity].forecast : [];
      if (!raw.length) {
        retryFetch("weather", url, token, entity, retriesRemaining);
        return;
      }

      const now = Math.floor(Date.now() / 1000);
      const forecast = [now];
      for (let i = 0; i < RETURN_SIZE; i++) {
        const item = raw[i];
        if (!item) break;
        forecast.push(normalizeCondition(item.condition));
        forecast.push(Math.round(item.temperature));
      }

      fetchedData.forecast = forecast.join(",");
      sendAppmessage();
    } catch (e) {
      console.log("Weather Error: " + e);
      retryFetch("weather", url, token, entity, retriesRemaining);
    }
  };

  xhr.onerror = function() {
    console.error("Weather XHR Network Error occurred");
    retryFetch("weather", url, token, entity, retriesRemaining);
  };

  xhr.send(JSON.stringify({ entity_id: entity, type: "hourly" }));
}

function getSunData(url, token, retriesRemaining) {
  if (retriesRemaining === undefined) {
    retriesRemaining = FETCH_RETRY_COUNT;
  }

  const baseUrl = url.endsWith("/")
    ? url.slice(0, -1)
    : url;
  const requestUrl = baseUrl + "/api/states/sun.sun";
  const xhr = new XMLHttpRequest();

  xhr.onload = function() {
    if (xhr.status >= 200 && xhr.status < 300) {
      try {
        const data = JSON.parse(xhr.responseText);
        const nextRise = new Date(data.attributes.next_rising);
        const nextSet = new Date(data.attributes.next_setting);

        fetchedData.sun = [
          Math.floor(nextRise.getTime() / 1000),
          Math.floor(nextSet.getTime() / 1000),
        ].join(",");
        sendAppmessage()
      } catch (e) {
        console.error("JSON Parse error: " + e);
        retryFetch("sun", url, token, undefined, retriesRemaining);
      }
    } else {
      console.error("Fetch failed with status: " + xhr.status);
      retryFetch("sun", url, token, undefined, retriesRemaining);
    }
  };

  xhr.onerror = function() {
    console.error("XHR Network Error occurred");
    retryFetch("sun", url, token, undefined, retriesRemaining);
  };

  xhr.open("GET", requestUrl, true);
  xhr.setRequestHeader("Authorization", "Bearer " + token);
  xhr.setRequestHeader("Content-Type", "application/json");

  xhr.send();
}

Pebble.addEventListener('appmessage', function (e) {
  const command = e.payload.COMMAND;

  switch (command) {
    case 0:
      startUpdate();
      break;
    default:
      console.log("Unknown command, you muppet!");
  }
});

function sendAppmessage(retryCount) {
  if (sendingMessage || !fetchedData.forecast || !fetchedData.sun) {
    return;
  }

  if (retryCount === undefined) {
    retryCount = APP_MESSAGE_RETRY_COUNT;
  }

  sendingMessage = true;
  const message = {
    COMMAND: 0,
    FORECAST: fetchedData.forecast,
    SUN: fetchedData.sun,
  };

  Pebble.sendAppMessage(message, function() {
    console.log("Message sent successfully: " + JSON.stringify(message))
    sendingMessage = false;
  }, function(e) {
    console.error("Message failed to send: " + JSON.stringify(e))
    sendingMessage = false;
    if (retryCount > 0) {
      setTimeout(function() {
        sendAppmessage(retryCount - 1);
      }, APP_MESSAGE_RETRY_DELAY_MS);
    }
  });
}
