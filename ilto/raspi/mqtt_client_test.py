import paho.mqtt.client as mqtt
import json
import time
import threading
import serial
import requests
import rele

mqttserver = "127.0.0.1"

esp01_pingcntr = 0
port = None
cache = {}
ilto_speed = 0
ilto_speed_to_text = {0:"2/M", 3:"1", 2:"4"}

_openweathermapapikey = "ddddddddddddddddddddddddddddd"
_location = u"Pirkkala"
_polling_intervall_in_sec = 90


def thread_publish_rr(client):
    message="on"
    print("publish data")
    while(1):
        global esp01_pingcntr
        if(esp01_pingcntr > 60*5):
            print("Send reset command")
            #client.publish("ilto", "RR",0, False)
            esp01_pingcntr = 0
        esp01_pingcntr = esp01_pingcntr + 1
        time.sleep(1)

def thread_publish_pp(client):
    message="on"
    while(1):
        print("ping")
        client.publish("ilto", "PP",0, False)
        time.sleep(60)

def thread_read_com(client, port):
    comdata = ""
    while True:
        #port.write("PP")
        if(port.inWaiting() > 0):
            exit = False
            while(port.inWaiting() and exit == False):
                chr = port.read(1)
                if(chr != '}'):
                    comdata += chr
                else:
                    comdata = comdata.strip() + "}"
                    print("COM> " + comdata)
                    #client.publish("ilto/data", comdata ,0, False)
                    try:
                        client.publish("ilto/data", comdata ,0, False)
                        b = json.loads(comdata)
                        if("TRACE" in b):
                            client.publish("ilto/trace", b["TRACE"], 0, False)
                            if ("Setup" in comdata):
                                global cache
                                for c in cache:
                                    # Ignore reset command(s)
                                    if ("R" != c[0]):
                                      print("Restore command: ", c)
                                      port.write(c)
                                      time.delay(1);
                        if("DATA" in b):
                            client.publish("ilto/t/poisto", b["DATA"][4])
                            client.publish("ilto/h/poisto", b["DATA"][5])
                            client.publish("ilto/t/tulo",   b["DATA"][0])
                            client.publish("ilto/h/tulo",   b["DATA"][1])
                            client.publish("ilto/t/jate",   b["DATA"][2]) 
                            client.publish("ilto/h/jate",   b["DATA"][3])
                            client.publish("ilto/t/raikas", b["DATA"][6])
                            client.publish("ilto/h/raikas", b["DATA"][7])
                            
                            client.publish("ilto/i/lampo",  b["DATA"][8])
                            client.publish("ilto/i/moodi",  b["DATA"][9])
                            client.publish("ilto/i/ping",   b["DATA"][10])

                        # Paivitetaan ilton nopeus tieto
                        global ilto_speed
                        client.publish("ilto/speed", ilto_speed)
                        client.publish("ilto/speedtxt", ilto_speed_to_text[ilto_speed])
                    except Exception, e:
                        print(str(e))
                    comdata = ""
                    exit = True

def thread_openweahtermap(client):

    while True:
        r = {}
        r["pvm"]           = None
        r["aika"]          = None
        r["Ulko C"]        = None
        r["Ulko RH%"]      = None
        r["Ulko paine"]    = None
        r["Saa"]           = None
        r["Tuuli m/s"]     = None
        r["Tuulen suunta"] = None
        r["Saa ikoni"]     = None
        
        try:
            print("Reading openwather")
            query = 'http://api.openweathermap.org/data/2.5/weather?q=' + _location + '&APPID=' + _openweathermapapikey
            owm = requests.get( query )
            #req = urllib2.Request('http://www.python.org/fish.html')
            #owm = urllib2.urlopen(query).read()

            j = owm.json()
            print("open weather done")
            r["pvm"]           = time.strftime("%Y-%m-%d")
            r["aika"]          = time.strftime("%H:%M.%S")
            r["Ulko C"]        = float('{0:0.1f}'.format(float(j["main"]["temp"]) - 273.15 )) #kelvin to C
            r["Ulko RH%"]      = float('{0:0.1f}'.format(float(j["main"]["humidity"])))
            r["Ulko paine"]    = int(j["main"]["pressure"])
            r["Saa"]           = str(j["weather"][0]["description"])
            r["Tuuli m/s"]     = float('{0:0.1f}'.format(float(j["wind"]["speed"])))
            r["Tuulen suunta"] = int(j["wind"]["deg"])
            r["Saa ikoni"]     = str(j["weather"][0]["icon"])
        except Exception, e:
            print("Exception..."),
            print(str(e))
        
        client.publish("ilto/t/ulko",  r["Ulko C"])
        client.publish("ilto/h/ulko",  r["Ulko RH%"])
        client.publish("ilto/p/ulko",  r["Ulko paine"])
        client.publish("ilto/ws/ulko", r["Tuuli m/s"])
        client.publish("ilto/wd/ulko", r["Tuulen suunta"])
        client.publish("ilto/wi/ulko", r["Saa ikoni"])

        time.sleep(90)

def on_connect(client, userdata, rc):
    print("Connected with result code "+str(rc))
    client.subscribe([("ilto/data",0), ("ilto",0)])

def on_message(client, userdata, msg):
    print msg.payload;
    if(msg.topic == "ilto/data"):
        if("ESP-01_Pong" in str(msg.payload)):
            print("Ping response received")
            esp01_pingcntr = 0
            client.publish("ilto/state", "online")
            global ilto_speed
            client.publish("ilto/speed", ilto_speed)
            client.publish("ilto/speedtxt", ilto_speed_to_text[ilto_speed])
    else:
        if("RR" in msg.payload):
            print(">COM --- Reset ignored")
        else:
            if (msg.payload[0].upper() == 'S'):
                print("Change speed")
                try:
                    global ilto_speed
                    ilto_speed = int(msg.payload[1])
                    rele.setstate(ilto_speed)
                    client.publish("ilto/speed", ilto_speed)
                    client.publish("ilto/speedtxt", ilto_speed_to_text[ilto_speed])
                except Exception as e:
                    print(str(e))
            else:
                print(">COM " + msg.payload)
                global port
                port.write(msg.payload)
                global cache
                cache[msg.payload[0]] = msg.payload


def main():
    mqttc = mqtt.Client()
    mqttc.on_connect = on_connect
    mqttc.on_message = on_message
    print("MQTT...")
    mqttc.connect(mqttserver, 1883, 60)
    print("MQTTC starti")
    rele.initialize()

    global port
    port = serial.Serial("/dev/ttyAMA0", baudrate=115200, timeout=3.0)
    thread0=threading.Thread(target=thread_read_com,args=(mqttc,port))
    thread0.start()


    #thread1=threading.Thread(target=thread_publish_rr,args=(mqttc,))
    #thread1.start()
    thread2=threading.Thread(target=thread_publish_pp,args=(mqttc,))
    thread2.start()

    #thread2=threading.Thread(target=thread_openweahtermap,args=(mqttc,))
    #thread2.start()

    mqttc.loop_forever()

def runme():
    main()

if __name__ == "__main__":
    main()
