
# GPS Sailboat Tracker

## Description

This project tracks GPS coordinates during a sailboat trip using an Arduino MKR ZERO microcontroller and a GT-U7 GPS module. GPS data is logged every five minutes and stored in a CSV file (`GPSLOG.CSV`). A Python script (`plot_gps_map.py`) plots these coordinates onto an interactive map (`gps_path_map.html`), allowing users to zoom, pan, and view details like timestamp and speed at each logged location.

## Components Used

- Arduino MKR ZERO
- GT-U7 GPS Module
- micro‑SD card (≥ 128 MB is plenty)
- 10 400 mAh USB power bank (≈ 24 h run‑time margin)

## Files

- **GPS_to_SD.ino**: Arduino sketch that collects GPS data and logs it onto an SD card (`GPSLOG.CSV`).
- **plot_gps_map.py**: Python script to visualize the recorded GPS data.
- **gps_path_map.html**: Generated interactive HTML map showing the GPS path.

## How to Use

### At Home

1. Upload the `GPS_to_SD.ino` sketch to your Arduino MKR ZERO.

### On the Boat

1. Ensure the GT-U7 GPS module is connected correctly.
2. Let the device log GPS data automatically every 5 minutes.

### Back At Home

1. Transfer the generated `GPSLOG.CSV` from the SD card to your computer.
2. Run the Python script:
   ```bash
   python plot_gps_map.py
   ```
3. Open the resulting `gps_path_map.html` in your web browser to interactively explore your sailing route.

## Interactive Map Features

- Zoom in/out
- Clickable markers showing timestamp and boat speed
- Satellite imagery for realistic mapping

## Requirements

- Python libraries: `pandas`, `folium`

Install required libraries with:

```bash
pip install pandas folium
```

## License

This project is open source under the MIT License.
