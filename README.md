# DualAxisLevel - read data from mpu6050 via esp12e and display remotely on phone / laptop browser
ESP8266 project using MPU6050 to measure vertical and horizontal axis and display live readings on a mobile phone browser while leveling remotely. Designed for the installation of a insulated glass production line at 6 degrees inclination. The enclosure is made on a
3d printer and the cad files are included.

The box hangs on top of a glass on the production line and has 3mm air float spacer built in.

After building, download and run MPU6050 calibration program, then add offsets to main program to improve accuracy.

Uses websocket communication of JSON string for update on demand which is very fast! Can be used with known wifi network or as its own Access Point.
