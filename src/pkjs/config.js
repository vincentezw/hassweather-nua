module.exports = [
  {
    "type": "heading",
    "defaultValue": "Watchface Settings"
  },
  {
    "type": "text",
    "defaultValue": "Enter your configuration details"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Home Assistant Configuration"
      },
      {
        "type": "input",
        "messageKey": "HAUrl",
        "label": "Home Assistant URL",
        "defaultValue": "",
        "attributes": {
          "placeholder": "https://myhome.mydomain.ps"
        }
      },
      {
        "type": "input",
        "messageKey": "HAToken",
        "label": "Long-Lived Access Token",
        "defaultValue": "",
        "attributes": {
          "placeholder": "abcdef1234567890abcdef1234567890abcdef12"
        }
      },
      {
        "type": "input",
        "messageKey": "HAEntity",
        "label": "Weather Entity ID",
        "defaultValue": "weather.home",
        "attributes": {
          "placeholder": "weather.home"
        }
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];

