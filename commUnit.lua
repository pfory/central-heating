--Init  Centra and solar heating
DeviceID="esp06"  
RoomID="Corridor"  

tINSolar                   = 0
tOUTSolar                  = 0
tPumpSolar                 = 0    
tBojler                    = 0
tINCentralHeating          = 0
tOUTCentralHeating         = 0
tPumpCentralHeating        = 0
tRad1IN                    = 0
tRad1OUT                   = 0
tRad2IN                    = 0
tRad2OUT                   = 0
tRad3IN                    = 0
tRad3OUT                   = 0
tRad4IN                    = 0
tRad4OUT                   = 0
tRad5IN                    = 0
tRad5OUT                   = 0
tRad6IN                    = 0
tRad6OUT                   = 0
pumpSolarStatus            = "ON"
pumpCentralHeatingStatus   = "ON"
tPumpCentralHeating        = 0

versionSWSolar             = "0.82"
versionSWCentral           = "0.4"
heartBeat                  = 0


wifi.setmode(wifi.STATION)
wifi.sta.config("Datlovo","Nu6kMABmseYwbCoJ7LyG")

Broker="213.192.58.66"  

pinLed = 4
gpio.mode(pinLed,gpio.OUTPUT)  
gpio.write(pinLed,gpio.HIGH)  

versionSWHeating         = 0.1
versionSWString   = "Heating /Solar and central/ v" 
print(versionSWString .. versionSWHeating)

function sendData()
  print("I am sending data from Solar unit to OpenHab")
  t = math.random (0, 350)
  t = t/10.0
  print(t)
  tINSolar = t
  tOUTSolar = t
  tPumpSolar = t
  tBojler = t
  tINCentralHeating = t
  tOUTCentralHeating = t
  tPumpCentralHeating = t
  tRad1IN = t
  tRad1OUT = t
  tRad2IN = t
  tRad2OUT = t
  tRad3IN = t
  tRad3OUT = t
  tRad4IN = t
  tRad4OUT = t
  tRad5IN = t
  tRad5OUT = t
  tRad6IN = t
  tRad6OUT = t

  --m:publish("/home/".. RoomID .."/" .. DeviceID .. "/tIN",                string.format("%.1f",tINSolar),0,0)  
  --m:publish("/home/".. RoomID .."/" .. DeviceID .. "/tOUT",               string.format("%.1f",tOUTSolar),0,0)  
  --m:publish("/home/".. RoomID .."/" .. DeviceID .. "/tPump",              string.format("%.1f",tPumpSolar),0,0)  
  --m:publish("/home/".. RoomID .."/" .. DeviceID .. "/tBojler",            string.format("%.1f",tBojler),0,0)  
  --m:publish("/home/".. RoomID .."/" .. DeviceID .. "/com",                pumpSolarStatus,0,0)  
  --m:publish("/home/".. RoomID .."/" .. DeviceID .. "/HeartBeat",          heartBeat,0,0)
  --m:publish("/home/".. RoomID .."/" .. DeviceID .. "/VersionSWSolar",     versionSWSolar,0,0)
  m:publish("/home/".. RoomID .."/" .. DeviceID .. "/VersionSWCentral",     versionSWCentral,0,0)
  m:publish("/home/".. RoomID .."/" .. DeviceID .. "/VersionSWHeating",     versionSWHeating,0,0)
  print("I am sending data from Central Heating unit to OpenHab")
  m:publish("/home/".. RoomID .."/" .. DeviceID .. "/tINKamna",             string.format("%.1f",tINCentralHeating),0,0)  
  m:publish("/home/".. RoomID .."/" .. DeviceID .. "/tOUTKamna",            string.format("%.1f",tOUTCentralHeating),0,0)  
  m:publish("/home/".. RoomID .."/" .. DeviceID .. "/tPumpKamna",           string.format("%.1f",tPumpCentralHeating),0,0)  
  m:publish("/home/".. RoomID .."/" .. DeviceID .. "/sPumpKamna",           pumpCentralHeatingStatus,0,0)  
  m:publish("/home/".. RoomID .."/" .. DeviceID .. "/tRad1IN",              string.format("%.1f",tRad1IN),0,0)  
  m:publish("/home/".. RoomID .."/" .. DeviceID .. "/tRad1OUT",             string.format("%.1f",tRad1OUT),0,0)  
  m:publish("/home/".. RoomID .."/" .. DeviceID .. "/tRad2IN",              string.format("%.1f",tRad2IN),0,0)  
  m:publish("/home/".. RoomID .."/" .. DeviceID .. "/tRad2OUT",             string.format("%.1f",tRad2OUT),0,0)  
  m:publish("/home/".. RoomID .."/" .. DeviceID .. "/tRad3IN",              string.format("%.1f",tRad3IN),0,0)  
  m:publish("/home/".. RoomID .."/" .. DeviceID .. "/tRad3OUT",             string.format("%.1f",tRad3OUT),0,0)  
  m:publish("/home/".. RoomID .."/" .. DeviceID .. "/tRad4IN",              string.format("%.1f",tRad4IN),0,0)  
  m:publish("/home/".. RoomID .."/" .. DeviceID .. "/tRad4OUT",             string.format("%.1f",tRad4OUT),0,0)  
  m:publish("/home/".. RoomID .."/" .. DeviceID .. "/tRad5IN",              string.format("%.1f",tRad5IN),0,0)  
  m:publish("/home/".. RoomID .."/" .. DeviceID .. "/tRad5OUT",             string.format("%.1f",tRad5OUT),0,0)  
  m:publish("/home/".. RoomID .."/" .. DeviceID .. "/tRad6IN",              string.format("%.1f",tRad6IN),0,0)  
  m:publish("/home/".. RoomID .."/" .. DeviceID .. "/tRad6OUT",             string.format("%.1f",tRad6OUT),0,0)  
  
  if heartBeat==0 then heartBeat=1
  else heartBeat=0
  end
end

function reconnect()
  print ("Waiting for Wifi")
  if wifi.sta.status() == 5 and wifi.sta.getip() ~= nil then 
    print ("Wifi Up!")
    tmr.stop(1) 
    m:connect(Broker, 31883, 0, function(conn) 
      print("Mqtt Connected to:" .. Broker) 
      mqtt_sub() --run the subscription function 
    end)
  end
end

m = mqtt.Client("ESP8266".. DeviceID, 180, "datel", "hanka")  
m:lwt("/lwt", "ESP8266", 0, 0)  
m:on("offline", function(con)   
  print ("Mqtt Reconnecting...")   
  tmr.alarm(1, 10000, 1, function()  
    reconnect()
  end)
end)  

function mqtt_sub()  
  m:subscribe("/home/".. RoomID .."/" .. DeviceID .. "/p1/Energy",0, function(conn)   
    print("Mqtt Subscribed to OpenHAB feed for device " .. DeviceID .. "(Soalr and Cenral unit)")  
  end)  
end  

 -- on publish message receive event  
m:on("message", function(conn, topic, data)   
  print("Received:" .. topic .. ":" .. data) 
end)  

tmr.alarm(0, 1000, 1, function() 
  print ("Connecting to Wifi... ")
  if wifi.sta.status() == 5 and wifi.sta.getip() ~= nil then 
    print ("Wifi connected")
    tmr.stop(0) 
    --print ("Read config file... ")
    --file.open("config.ini", "r")
    --s = file.readline()
    --pulseTotal = string.sub(s, 12, 20)
    --print(pulseTotal)
    --file.close()  
    m:connect(Broker, 31883, 0, function(conn) 
      print("Mqtt Connected to:" .. Broker) 
      tmr.alarm(2, 30000, 1, function() sendData() end )
    end) 
  end
end)