import paho.mqtt.client as mqtt
import json
import time
import threading
import pymongo

from pprint import pprint

mqttserver = "127.0.0.1"

import signal
import sys

rawData = {}
rawDataMutex = threading.Lock()

def thread_avg(mongoc):
    global rawData
    global rawDataMutex
    while 1:
        time.sleep(60*2) #sleep 2 minutes
        avgresults = {}
        rawDataMutex.acquire()
        pprint(rawData)
        tlimit = time.time() - 60*2
        for topic in rawData:
            avg = 0.0
            cnt = 0
            try:
                droprest = -1
                for (tstamp, data) in rawData[topic]:
                    avg = avg + data
                    cnt = cnt + 1
                    if tlimit > tstamp:
                        if droprest < cnt:
                            droprest = cnt
                print("drop first " + str(droprest))

                if droprest > -1:
                    rawData[topic] =  rawData[topic][droprest:] #drop old values

                if (cnt>0):
                    avgresults[topic] = avg / cnt

            except Exception as e:
                print(str(e))

        avgresults["timestamp"] = time.time()
        rawDataMutex.release()
        pprint(avgresults)
 
        try:
            mongoc.ilto.data.insert_one(avgresults)
        except Exception as e:
            print(str(e))

def signal_handler(signal, frame):
        print('You pressed Ctrl+C!')
        global ctrlThreadStop
        sys.exit(0)

def on_connect(client, userdata, rc):
    print("Connected with result code " + str(rc))
    client.subscribe([("ilto/h/+"   ,0),
                      ("ilto/t/+"   ,0),
                      ("ilto/i/+"   ,0),
                      ("ilto/w/+"   ,0),
                      ("ilto/speed" ,0)])

def on_message(client, userdata, msg):
    rawDataMutex.acquire()
    
    try:
        if msg.topic not in rawData:
            rawData[msg.topic] = []
    
        rawData[msg.topic].append((time.time(), float(msg.payload)))
    except Exception as e:
        print(str(e))

    rawDataMutex.release()
    

def main():
    signal.signal(signal.SIGINT, signal_handler)
    mongoc = pymongo.MongoClient("10.8.0.1", serverSelectionTimeoutMS=2)
    mongoc.server_info()

    mqttc = mqtt.Client()
    mqttc.on_connect = on_connect
    mqttc.on_message = on_message
    print("MQTT...")
    mqttc.connect(mqttserver, 1883, 60)
    print("MQTTC starti")

    thread0=threading.Thread(target=thread_avg,args=(mongoc,))
    thread0.start()

    mqttc.loop_forever()

def runme():
    print("Mongo client starting...")
    main()

if __name__ == "__main__":
    main()
