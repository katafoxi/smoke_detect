from gpsdclient import GPSDClient
from datetime import datetime, timedelta


# or as python dicts (optionally convert time information to `datetime` objects)
with GPSDClient() as client:
    gpsd_start_time = 0
    subtitle_index = 1
    for result in client.dict_stream(convert_datetime=True, filter=["TPV"]):
        if gpsd_start_time == 0:
            sub_conveer_start_time = datetime.fromisoformat(str(result.get("time", "n/a")))
            gpsd_start_time = datetime.fromisoformat(str(result.get("time", "n/a"))) - sub_conveer_start_time
            gpsd_end_time = gpsd_start_time + timedelta(microseconds=10)
        else :
            gpsd_end_time = datetime.fromisoformat(str(result.get("time", "n/a"))) - sub_conveer_start_time
        print(subtitle_index)
        #00:00:10,000 --> 00:00:15,000
        print(gpsd_start_time, "-->", gpsd_end_time)
        print("jettime: %s" % result.get("time", "n/a"))
        print("Latitude: %s" % result.get("lat", "n/a"))
        print("Longitude: %s" % result.get("lon", "n/a"))

        subtitle_index += 1
        gpsd_start_time = gpsd_end_time
        print()
