#!/usr/bin/python3
from gpsdclient import GPSDClient
from datetime import datetime, timedelta
import os
import time

# Путь к FIFO (именованному каналу)
FIFO_PATH = "/tmp/subtitles.fifo"

if __name__ == '__main__':
    try:
        with GPSDClient() as client:
            # Создаем FIFO, если его нет
            if not os.path.exists(FIFO_PATH):
                os.mkfifo(FIFO_PATH)
            with open(FIFO_PATH, "w") as fifo:
            #print("create fifo file")
                gpsd_start_time = 0
                subtitle_index = 1
                for result in client.dict_stream(convert_datetime=True, filter=["TPV"]):
                    if gpsd_start_time == 0:
                        zero_time = datetime.fromisoformat(str(result.get("time", "n/a")))
                        gpsd_start_time = datetime.fromisoformat(str(result.get("time", "n/a"))) - zero_time + timedelta(microseconds=1)
                        gpsd_end_time = gpsd_start_time + timedelta(seconds=10)
                    else :
                        gpsd_end_time = datetime.fromisoformat(str(result.get("time", "n/a"))) - zero_time + timedelta(microseconds=1)
                    # print(subtitle_index)
                    # #00:00:10,000 --> 00:00:15,000
                    # print(str(gpsd_start_time).replace(".", ",")[:-3], "-->", str(gpsd_end_time).replace(".", ",")[:-3])
                    # print("jettime: %s" % result.get("time", "n/a"))
                    # print("Latitude: %s" % result.get("lat", "n/a"))
                    # print("Longitude: %s" % result.get("lon", "n/a"))
                    # print()

                    duration = str(gpsd_start_time).replace(".", ",")[:-3] + " --> " + str(gpsd_end_time).replace(".", ",")[:-3]
                    subtitle_block = (
                        f"{subtitle_index}\n"
                        f"{duration}\n"
                        "jettime: %s" % result.get("time", "n/a") + "\n"
                        "Latitude: %s" % result.get("lat", "n/a") + "\n"
                        "Longitude: %s" % result.get("lon", "n/a") + "\n\n"
                    )
                    #print(subtitle_block)
                    fifo.write(subtitle_block)
                    fifo.flush()  # Важно: принудительно записываем в FIFO
                    #print(f"Отправлен субтитр #{subtitle_index}")
                    subtitle_index += 1
                    gpsd_start_time = gpsd_end_time

    except (KeyboardInterrupt, SystemExit): #when you press ctrl+c
        print("\nKilling gpsd_client...")
    finally:
        if os.path.exists(FIFO_PATH):
            os.remove(FIFO_PATH)  # Удаляем FIFO при завершении
