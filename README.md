# Linux Rootkit

## Description

i straight up jacked this rootkit and modified it because i didnt want
to do it from scratch. this copies sys_call_table[], and compares the running
copy of sys_call_table[] with our copy after modules are loaded. if these
differ, someone probably hooked a syscall. If this happens, replace it with our
"known good" pointers.


## Presentation

This is _based off of _the code associated with the presentation from the *Toorcamp 2018* and *Thotcon 2019*.

 * [Thotcon 2019 Slides](https://github.com/rootfoo/pub/blob/master/Developing%20a%20Linux%20Rootkit%20-%20Thotcon%20-%202019-05-03.pdf)


## Compiling, Loading

Use `dmesg -w` to see the diagnostic output. After loading, experiment running various
shell commands to see execve being hijacked in real time.


```
make
sudo insmod syscallslol.ko
lsmod
sudo rmmod syscallslol.ko
```

## Status

This project was last developed and tested on Ubuntu 18.04 (Linux kernel 4.15.0-48-generic).


## Installing binary modules

Generally you should always compile kernel modules on the same host they will be installed
on. However, it is possible to compile it offline and install it on a target system. Note 
that it must be compiled with the same kernel version and Linux distribution for this to 
work. The script below outlines the process.

```
#!/bin/bash
NAME="syscallslol"
DIR="/lib/modules/`uname -r`/kernel/drivers/$NAME/"
sudo mkdir -p $DIR
sudo cp $NAME.ko $DIR
sudo depmod
sudo bash -c 'cat << EOF > /etc/modules-load.d/rootkit.conf
syscallslol
EOF'
```

