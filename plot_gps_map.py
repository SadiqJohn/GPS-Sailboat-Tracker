import pandas as pd
import folium

# 1) Read your CSV (assuming it’s in the same directory and named gpslog.csv)
#    Combine Local_Date and Local_Time into one datetime column.
df = pd.read_csv(
    'GPSLOG.CSV',
    parse_dates=[['Local_Date', 'Local_Time']],
    dayfirst=False  # Month/Day/Year order
)

# 2) Compute map center (mean of all lat/lon)
center_lat = df['Latitude'].mean()
center_lon = df['Longitude'].mean()

# 3) Create a Folium map centered at that location, with zoom level 15
m = folium.Map(location=[center_lat, center_lon], zoom_start=15)

# 4) Add Esri World Imagery (satellite) as the base tile layer
folium.TileLayer('Esri.WorldImagery', name='Satellite').add_to(m)

# 5) Add circle markers for each point:
for _, row in df.iterrows():
    timestamp = row['Local_Date_Local_Time'].strftime('%Y-%m-%d %H:%M:%S')
    speed = f"{row['Speed_mph']:.2f} mph"
    popup_text = f"{timestamp}<br>Speed: {speed}"
    folium.CircleMarker(
        location=(row['Latitude'], row['Longitude']),
        radius=4,
        popup=popup_text,
        color='red',
        fill=True,
        fill_color='red'
    ).add_to(m)

# 6) Draw a blue polyline connecting all points in order
locations = list(zip(df['Latitude'], df['Longitude']))
folium.PolyLine(locations, color='blue', weight=2, opacity=0.8).add_to(m)

# 7) Save to HTML file
m.save('gps_path_map.html')

print("Map saved to gps_path_map.html. Open this file in your browser to see the satellite map.")
