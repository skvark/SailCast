# SailCast
Screencasting app for Sailfish OS. Streams MJPEG over socket.

# Usage

Just hit start button and the app will start listening on address provided in below buttons.

## Notes

Tested only on Firefox. Chrome requires that the mjpeg stream is embedded into html element so the stream won't show up when the plain address is accessed.

*I made this just for the fun and for demonstrating SFOS capabilities. Polling Lipstick's saveScreenshot method via dbus is suboptimal and pretty stupid. It will cause the UI to lag because the calls to the method block.*