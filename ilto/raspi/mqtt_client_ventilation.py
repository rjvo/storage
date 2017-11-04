import paho.mqtt.client as mqtt
import json
import time
import threading

mqttserver = "127.0.0.1"

ctrlThreadStop = threading.Event()
previousSpeed   = 0
previousHeating = 1
previousMode    = 9

timedAction = False
timedActionRunTime = 0
restored = False

import signal
import sys
def signal_handler(signal, frame):
        print('You pressed Ctrl+C!')
        global ctrlThreadStop
        ctrlThreadStop.set()
        sys.exit(0)

def printstate(msg):
    print(msg)

def thread_ventilation(client, duration, mode):
    duration = duration * 10 #count 0.1 seconds
    printstate("thread_ventilation " + str(duration/10))
    if (mode == "V"):
        client.publish("ilto/ventilation", duration/10)
    else:
        client.publish("ilto/boost", duration/10)
    global ctrlThreadStop
    while(False == ctrlThreadStop.isSet() and duration > 0):
        time.sleep(0.1)
        duration = duration - 1
        if (duration%100 == 0): #every 10 seconds publish
            print("Remaining time " + str(duration/10))
            if (mode == "V"):
                client.publish("ilto/ventilation", duration/10)
            else:
                client.publish("ilto/boost", duration/10)
        global timedActionRunTime
        timedActionRunTime = timedActionRunTime + 1
    print("Duration " + str(duration/10))
    client.publish("ilto", "V0")
    printstate("thread_ventilation stopped")

def on_connect(client, userdata, rc):
    print("Connected with result code " + str(rc))
    client.subscribe([("ilto"            ,0),
                      ("ilto/i/lampo"    ,0),
                      ("ilto/i/moodi"    ,0),
                      ("ilto/speed"      ,0)])
    
    client.publish("ilto", "GG")

def on_message(client, userdata, msg):

    global ctrlThreadStop
    global timedActionRunTime
    global timedAction
    global previousHeating
    global previousMode
    global previousSpeed
    global restored

    if (msg.topic == "ilto" and (msg.payload[0] == "V" or msg.payload[0] == "B")):
        duration = int(msg.payload[1:])

        if (duration > 0):
            timedAction = True
            timedActionRunTime = 0
            printstate("Ventilation/Boost " + str(duration/10))
            # Turn off heating
            
            # Turn switch to summer mode and turn off heating in case of ventilation
            if (msg.payload[0] == "V"):
                printstate("Ventilation")
                client.publish("ilto", "L0")
                time.sleep(1)
                client.publish("ilto", "T0") 
                time.sleep(1)

            # boost mode will just turn speed to highest one
            client.publish("ilto", "S2")
            ctrlThreadStop.clear()
            ctrlThread=threading.Thread(target=thread_ventilation,args=(client, duration, msg.payload[0]))
            ctrlThread.start()
            time.sleep(1)
            client.publish("ilto", "GG")
            restored = False
    
        else:
            # Stop thread and restore previous values
            if restored == False:
                if False == ctrlThreadStop.isSet():
                    ctrlThreadStop.set()
                timedAction = False
                timedActionRunTime = 0
                printstate("restore T" + str(previousMode))
                client.publish("ilto", "T" + str(previousMode))
                time.sleep(1)
                printstate("restore S" + str(previousSpeed))
                client.publish("ilto", "S" + str(previousSpeed))
                time.sleep(1)
                printstate("restore L" + str(previousHeating))
                client.publish("ilto", "L" + str(previousHeating))
                time.sleep(1)
                client.publish("ilto", "GG")
                time.sleep(1)
                restored = True

    else:
        if (msg.topic == "ilto/i/lampo"):
            if (False == timedAction):
                previousHeating = int(msg.payload[0])
                printstate("previousHeating " + str(previousHeating))

        elif (msg.topic == "ilto/i/moodi"):
            if (False == timedAction):
                previousMode    = int(msg.payload[0])
                printstate("previousMode " + str(previousMode))
     
        elif (msg.topic == "ilto/speed"):
            if (False == timedAction):
                previousSpeed   = int(msg.payload[0])
                printstate("previousSpeed " + str(previousSpeed))
     
        elif (msg.topic == "ilto"):
            if (msg.payload[0] == "L" or msg.payload[0] == "T" or msg.payload[0] == "S"):
                if(timedActionRunTime > 25):
                    printstate("Cancell " + msg.topic + " "+ msg.payload)
                    client.publish("ilto", "V0")
            else:
                pass 
        else:
            pass 

def main():
    signal.signal(signal.SIGINT, signal_handler)

    mqttc = mqtt.Client()
    mqttc.on_connect = on_connect
    mqttc.on_message = on_message
    print("MQTT...")
    mqttc.connect(mqttserver, 1883, 60)
    print("MQTTC starti")

    mqttc.loop_forever()

def runme():
    main()

if __name__ == "__main__":
    main()
