# HassWeather Pebble
A clockface that shows a brief forecast and sunrise and sunset hours. What makes it unique, is that these are fetched from your own Home Assistant instance. This allows you easy acess to many weather providers with the API burden placed with Home Assistant.

The watchface also displays an indicator of your "progress" from the last sunrise/sunset to the upcoming sunrise/sunset.

## Attribution
* Weather icons are sourced from [here](https://github.com/D0-0K/Pebble-Icons).
* Some graphics are sourced from [Timestyle](https://github.com/freakified/TimeStylePebble).

## BDS

This project endorses the [Boycott, Divestment, Sanctions](https://bdsmovement.net/) movement against the apartheid state of Israel, in support of Palestinian human rights.

Support requests originating from Israel will not be addressed. Issues and pull requests from anyone, anywhere, that improve the software for everyone else, remain welcome.

## Screenshots

![Watchface 1](screenshots/emery_1.png)
![Watchface 2](screenshots/emery_2.png)

## TODO
- add "invert" color setting and pass it on in appMessage.
- optimise layout on gabbro

## Things I liked
- Piu is well thought-out and once you get it, is easy to follow
- Moddable is great and allows for all the syntax sugar and niceness. But we are limited in how we can use it.

## Things I did not like

- SVGImage doesn't allow me to dynamically change its `path`. So I can't change the image shown and would need many objects, giving me memory issues and crashes
- I really have to watch heap memory and number of allocations, and end up avoiding some template niceness to avoid crashes. All these nice tools, but we can't use them as much as we'd like. Any form of data processing on the watch is a no-go.
- Based on the above; whilst we now have `fetch` and stuff, it makes sense to do as much as possible on the phone side, as data transformations on the watch are resource heavy and practically impossible. But Pebblekit JS is not nice in 2026. It's completely seperate from the embeddedjs code, doesn't allow a lot of the syntax sugar and things like `fetch` and async. In an ideal world, this would be more like React or Sveltekit where what runs on the phone is SSR and "hydrates" the frontend on the watch. App messages are functional but not fantastic in 2026.
- The stack traces and error messages when things go bad are as useful as a chocolate teapot.
- Weird: when running PKJS on my machine with pypkjs the runtime uses GMT and not my local time. In the latest SDK Qemu too seems to run in GMT in spite of my local timezone.
