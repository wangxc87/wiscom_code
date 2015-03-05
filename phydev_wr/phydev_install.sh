#!/bin/sh 
module="phydev" 
device="phydev" 
mode="664" 
 
# invoke insmod with all arguments we got 
# and use a pathname, as newer modutils don't look in . by default

rmmod $module
/sbin/insmod  /home/root/$module.ko  || exit 1 
 
# remove stale nodes 
rm -f /dev/${device}[0-3] 


mknod /dev/${module} c `awk "\\$2==\"$module\" {print \\$1}" /proc/devices` 0
#mknod /dev/${module}2 c `awk "\\$2==\"$module\" {print \\$1}" /proc/devices` 1

#major='awk "\\$2==\"$module\" {print \\$1}" /proc/devices'
#major=$'awk "\\$2==\"tsl2771\" {print \\$1}" /proc/devices'
#echo ${major}

#mknod /dev/${device} c $major 0 
#mknod /dev/${device}1 c $major 1 
#mknod /dev/${device}2 c $major 2 
#mknod /dev/${device}3 c $major 3 
 
# give appropriate group/permissions, and change the group. 
# Not all distributions have staff, some have "wheel" instead. 
group="staff" 
grep -q '^staff:' /etc/group || group="wheel" 
 
#chgrp $group /dev/${device}[0-3] 
#chmod $mode /dev/${device}[0-3]

chgrp $group /dev/${device}
chmod $mode /dev/${device}
