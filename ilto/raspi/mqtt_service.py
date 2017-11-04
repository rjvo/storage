import paho.mqtt.client as mqtt 
import json 
import time 
import threading 
from multiprocessing import Process 
import mqtt_client_test
import mqtt_client_ventilation
import mqtt_client_mongo
from datetime import datetime

watchdog = 0

mqttserver = "127.0.0.1"

def on_connect(client, userdata, rc):
    print("WD "+str(rc))
    client.subscribe("ilto/i/ping", 2)

def on_message(client, userdata, msg):
    print("WD Clear")
    global watchdog
    watchdog = 12

def main():
    mqttc = mqtt.Client()
    mqttc.on_connect = on_connect
    mqttc.on_message = on_message
    mqttc.connect(mqttserver, 1883, 60)
    print("MQTTC starti")

    f = open('status.log','w')
    f.write("Begin " + str(datetime.now()) + "\n")
    
    #mqttc.loop_forever()
    mqttc.loop_start()

    while (True):
        print("Start process")
        p = Process(target=mqtt_client_test.main)
        p.start()
        f.write("Start " + str(datetime.now()) + " " + str(p.pid) + "\n")
        f.close()
        pv = Process(target=mqtt_client_ventilation.main)
        pv.start()
        pm = Process(target=mqtt_client_mongo.runme)
        pm.start()
        global watchdog
        watchdog = 12
        while (watchdog > 0):
            time.sleep(10)
            watchdog = watchdog - 1
            print(watchdog)
        print("Kill process")
        f.write("Killed " + str(datetime.now()) + "\n")
        f.close()
        p.terminate()
        pv.terminate()
        pm.terminate()
    
    loop_stop(force=False)


    f.write("Exit " + str(datetime.now()) + "\n")
    f.close()


if __name__ == "__main__":
    main()
