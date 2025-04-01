from gpsdclient import GPSDClient
from datetime import datetime, timedelta

if __name__ == '__main__':
    try:
        with GPSDClient() as client:
            gpsd_start_time = 0
            subtitle_index = 1
            for result in client.dict_stream(convert_datetime=True, filter=["TPV"]):
                if gpsd_start_time == 0:
                    zero_time = datetime.fromisoformat(str(result.get("time", "n/a")))
                    gpsd_start_time = datetime.fromisoformat(str(result.get("time", "n/a"))) - zero_time + timedelta(microseconds=1)
                    gpsd_end_time = gpsd_start_time + timedelta(seconds=1)
                else :
                    gpsd_end_time = datetime.fromisoformat(str(result.get("time", "n/a"))) - zero_time + timedelta(microseconds=1)
                print(subtitle_index)
                #00:00:10,000 --> 00:00:15,000
                print(str(gpsd_start_time).replace(".", ",")[:-3], "-->", str(gpsd_end_time).replace(".", ",")[:-3])
                print("jettime: %s" % result.get("time", "n/a"))
                print("Latitude: %s" % result.get("lat", "n/a"))
                print("Longitude: %s" % result.get("lon", "n/a"))
                print()

                subtitle_index += 1
                gpsd_start_time = gpsd_end_time

    except (KeyboardInterrupt, SystemExit): #when you press ctrl+c
        print("\nKilling gpsd_client...")
    print("Done.\nExiting.")