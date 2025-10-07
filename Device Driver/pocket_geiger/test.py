import iio
import time

ctx = iio.Context() 
dev = ctx.find_device("pocket_geiger")

if dev != None:
    print("found pocket_geiger dev")
    
    while True:
        radiation = error = 0
        for ch in dev.channels:
            for attr in ch.attrs:
                if 'raw' in attr:
                    if 'radiation' in attr:
                        radiation = float(ch.attrs[attr].value)
                    if 'error' in attr:
                        error = float(ch.attrs[attr].value)
                    
        print("Radiation: ", round(radiation, 3), "uSv/h +/- ", round(error, 3))
        time.sleep(1)
