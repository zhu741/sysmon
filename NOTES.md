cat /proc/meminfo | head : 
MemTotal:        8252608 kB
MemFree:         7146528 kB
MemAvailable:    7701600 kB
Buffers:           34288 kB
Cached:           617264 kB
SwapCached:            0 kB
Active:           523776 kB
Inactive:         354592 kB
Active(anon):     274608 kB
Inactive(anon):        0 kB

cat /proc/uptime:
5851.40 23374.64

head -n 1 /proc/stat:
cpu  1294 2 559 2345122 758 0 18 0 0 0

cat /sys/class/thermal/thermal_zone0/temp:
52350
